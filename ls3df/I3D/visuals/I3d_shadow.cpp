// The shadows (PS2 I3d_shadow.cpp, whose functions are stubs), at 0x10050900-0x10054240 in the original:
// globalShadowList's $E, I3D_shadow in PS2's order (SetTo's statics' $E before SetTo, its search right after it) with
// the PC's UpdateVertices and UpdateLight after SetTextureSize, the header's zero vector's $E (0x100529a0), then
// RenderTexture, Prepare, ShadowDef's destructor, the shadows' visual, its queue's $E, its m_AddFGroups, ProcessShadows
// with the visual's ProcessShadow and m_RenderFGroup. One object, though the visual's code has no zero vector or
// identity matrix of its own: the visuals' queue sits among the shadow's globals in .bss (0x101b4f3c, between the zero
// vector and globalShadowList, as VC6's hash orders them), where a second object's globals couldn't be, and the
// visual's constructor copies the file's zero vector. The receivers' code is the projectors' (I3d_projector.cpp), two
// of their messages included. Names are PS2's unless noted.
//
// Not matching yet:
// - SetTo keeps the vertex base in memory (add [esp + 0x10], eax), where the original loads it into edi after the
//   vertex copy and stores it with the loop's counters.
// - UpdateLight: its frame (0xd0 bytes, so sorted) has several locals elsewhere, and the spot light's squares, its
//   dot product and SetNormalized's reciprocal are scheduled differently on the FPU.
// - The visual's constructor finishes copying the zero vector later, so its stores of 1 take eax, not ecx.
#include "I3D/I3D.h"
#include "I3D/render/IBManager.h"
#include "I3D/render/clipper.h"
#include "I3D/render/render.h"
#include "I3D/render/VBManager.h"
#include "IGraph/IGraph.h"

#include <string.h>

// The driver's bias for the shadows (RS_ZBIAS_SHADOW)
extern int RsZBiasShadow;

// Every shadow (PS2 name)
// GLOBAL: LS3DF 0x101b4f4c
C_list<I3D_shadow *> globalShadowList;

// FUNCTION: LS3DF 0x10050950
I3D_shadow::I3D_shadow()
{
    type = FRAME_SHADOW;
    orthogonal = true;
    def = NULL;
    scene = NULL;
    sector = NULL;
    verts = NULL;
    verts_w = NULL;
    points = NULL;
    num_verts = 0;
    faces = NULL;
    num_faces = 0;
    frames = NULL;
    num_frames = 0;
    texture = NULL;
    size_y = 64;
    size_x = 64;
    range = 8.0f;
    reverse_factor = 0.5f;
    light_matrix.Identity();
    shadow_lock = I3DLOCK_NONE;
    opacity = 1.0f;
    max_texture_distance = 1.0f;
    visible_range = 100.0f;
    color.Zero();
    verts_count = -1;
    lights_count = -1;
    shd_flags = 0;
    light_dir = S_vector(0.0f, 0.0f, 1.0f);
    visual = new I3D_shadow_visual;
    if (!visual) {
        dbgPrintf("!! Out of memory  I3D_shadow constructor");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return;
    }
    visual->shadow = this;
    globalShadowList.push_back(this);
}

// FUNCTION: LS3DF 0x10050b90
I3D_shadow::~I3D_shadow()
{
    Close();
    if (scene)
        scene->DeleteShadow(this);
    if (visual)
        visual->Release();
    globalShadowList.remove(this);
}

// A copy shares the original's geometry; SetTo gives it its frames
// FUNCTION: LS3DF 0x10050c60
LS3D_RESULT I3D_shadow::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    Close();
    if (frame->GetType() != FRAME_SHADOW)
        return I3D_OK;
    I3D_shadow *shd = (I3D_shadow *)frame;
    if (shd->def) {
        def = shd->def;
        def->ref++;
    }
    max_texture_distance = shd->max_texture_distance;
    range = shd->range;
    size_x = shd->size_x;
    size_y = shd->size_y;
    SetVisibleRange(shd->visible_range);
    SetReverseFactor(shd->reverse_factor);
    SetColor(shd->color);
    SetShadowLock(shd->shadow_lock);
    SetOrthogonal(shd->orthogonal);
    return I3D_OK;
}

// A frame of the hierarchy SetTo searches, and the part of the geometry it moves (PC only, the names are guesses)
struct S_shadow_match {
    I3D_frame *frame;
    int part;
};

// SetTo's search (PC only, the names are guesses): the parts to find, the frames found, and what they add up to. The
// names are chosen for the original's .bss order (see tools/bsshash.py); the counts are zero-initialized, which puts
// them last, in their order.
// GLOBAL: LS3DF 0x101b4f5c
static S_shadow_part *SetToParts;

// GLOBAL: LS3DF 0x101b4f60
static C_list<S_shadow_match> FoundFrames;

// GLOBAL: LS3DF 0x101b4f70
static int SetToNumFaces = 0;

// GLOBAL: LS3DF 0x101b4f74
static int SetToNumParts = 0;

// GLOBAL: LS3DF 0x101b4f78
static int SetToNumVerts = 0;

static inline void FindParts(I3D_frame *frame);

// Takes the geometry's parts whose frames are found under frame: their vertices, and their faces with the indices
// moved by the vertices before them. NULL leaves the scene. The vertices' 2D points aren't freed (the original's too).
// FUNCTION: LS3DF 0x10050d80
void I3D_shadow::SetTo(I3D_frame *frame)
{
    if (!frame && scene)
        scene->DeleteShadow(this);
    if (visual && visual->parent)
        visual->LinkTo(NULL, 0);
    S_shadow_frame *sf = frames;
    for (int i = num_frames; i--; sf++) {
        if (sf->frame)
            sf->frame->frm_flags &= ~FRMFLAGS_SHADOW;
    }
    delete[] frames;
    frames = NULL;
    num_frames = 0;
    delete[] verts;
    verts = NULL;
    verts_w = NULL;
    num_verts = 0;
    delete[] faces;
    verts_count = -1;
    lights_count = -1;
    faces = NULL;
    num_faces = 0;
    shd_flags = 0;
    if (!def || !frame)
        return;
    FoundFrames.reset();
    SetToNumVerts = 0;
    SetToNumFaces = 0;
    SetToParts = &def->parts[0];
    SetToNumParts = def->parts.size();
    FindParts(frame);
    if (!SetToNumFaces)
        return;
    faces = new ushort[SetToNumFaces * 3];
    verts = new S_vector[SetToNumVerts * 2];
    points = new float[SetToNumVerts * 2];
    frames = new S_shadow_frame[FoundFrames.size()];
    if (!faces || !verts || !points || !frames) {
        dbgPrintf("!! Out of memory  I3D_shadow::SetTo()");
        delete[] verts;
        verts = NULL;
        delete[] points;
        verts = NULL;
        delete[] faces;
        faces = NULL;
        delete[] frames;
        frames = NULL;
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return;
    }
    verts_w = verts + SetToNumVerts;
    num_verts = SetToNumVerts;
    num_faces = SetToNumFaces;
    num_frames = FoundFrames.size();
    S_shadow_match *m = &FoundFrames[0];
    S_vector *v = verts;
    ushort *f = faces;
    sf = frames;
    int base = 0;
    for (i = FoundFrames.size(); i--; m++, sf++) {
        S_shadow_part *p = SetToParts;
        int first_vert = 0, first_face = 0;
        for (int j = m->part; j--; p++) {
            first_vert += p->num_verts;
            first_face += p->num_faces;
        }
        const ushort *src = &def->faces[first_face * 3];
        for (j = p->num_faces * 3; j--; )
            *f++ = *src++ + base;
        memcpy(v, &def->verts[first_vert], p->num_verts * sizeof(S_vector));
        int n = p->num_verts;
        v += n;
        base += n;
        sf->num_verts = n;
        sf->frame = m->frame;
        m->frame->frm_flags |= FRMFLAGS_SHADOW;
    }
}

// The frames under frame that are on and whose names (without the model's) are the parts', each for its first part of
// the name. Recursive: SetTo has its first level inlined, the copy follows SetTo. Defined after SetTo, as its marker's
// address is; VC6 inlines a static inline function defined after its caller too. The loop steps p before i.
// FUNCTION: LS3DF 0x10051260
static inline void FindParts(I3D_frame *frame)
{
    for (I3D_frame *frm = frame->child; frm; frm = frm->next) {
        if (!(frm->frm_flags & FRMFLAGS_ON))
            continue;
        S_shadow_part *p = SetToParts;
        for (int i = 0; i < SetToNumParts; p++, i++) {
            if (!strcmp(frm->GetOrigName(), p->name)) {
                S_shadow_match &m = FoundFrames.push_back();
                m.part = i;
                m.frame = frm;
                SetToNumFaces += p->num_faces;
                SetToNumVerts += p->num_verts;
                break;
            }
        }
        FindParts(frm);
    }
}

// FUNCTION: LS3DF 0x100513d0
LS3D_RESULT I3D_shadow::LinkTo(I3D_frame *frame, uint flags)
{
    SetTo(frame);
    return I3D_frame::LinkTo(frame, flags);
}

// FUNCTION: LS3DF 0x10051400
void I3D_shadow::SetShadowLock(I3D_LOCK lock)
{
    shadow_lock = lock;
    shd_flags = 0;
    lights_count = -1;
}

// A shadow under a sector joins the sector's scene. The original's search isn't rotated: a while (true) whose sector
// case returns inside the loop gives that (a for, while or goto loop with a break is rotated).
// FUNCTION: LS3DF 0x10051430
void I3D_shadow::UpdateV()
{
    if (scene)
        return;
    I3D_frame *frm = parent;
    while (true) {
        if (!frm)
            return;
        if (frm->type == FRAME_SECTOR) {
            ((I3D_sector *)frm)->scene->AddShadow(this);
            shd_flags = 0;
            lights_count = -1;
            return;
        }
        frm = frm->parent;
    }
}

// FUNCTION: LS3DF 0x10051480
void I3D_shadow::DeleteFromAnyShadow(I3D_frame *frame)
{
    I3D_shadow **sp = &globalShadowList[0];
    for (int i = globalShadowList.size(); i--; sp++) {
        S_shadow_frame *sf = (*sp)->frames;
        for (int j = (*sp)->num_frames; j--; sf++) {
            if (sf->frame && sf->frame == frame) {
                sf->frame->frm_flags &= ~FRMFLAGS_SHADOW;
                sf->frame = NULL;
            }
        }
    }
}

// Releases the geometry and the texture. The world space vertices' pointer stays (the original's too). The definition
// is released through its inline Release, which decrements the count in a register and tests that.
// FUNCTION: LS3DF 0x100514e0
void I3D_shadow::Close()
{
    S_shadow_frame *sf = frames;
    for (int i = num_frames; i--; sf++) {
        if (sf->frame)
            sf->frame->frm_flags &= ~FRMFLAGS_SHADOW;
    }
    delete[] frames;
    frames = NULL;
    num_frames = 0;
    if (def)
        def->Release();
    def = NULL;
    delete[] verts;
    verts = NULL;
    num_verts = 0;
    delete[] points;
    points = NULL;
    delete[] faces;
    faces = NULL;
    num_faces = 0;
    if (texture) {
        texture->Release();
        texture = NULL;
    }
    shd_flags = 0;
    verts_count = -1;
    lights_count = -1;
}

// FUNCTION: LS3DF 0x100515e0
LS3D_RESULT I3D_shadow::Open(const char *name, uint flags, I3D_LOAD_CB_PROC *cb, void *context)
{
    Close();
    def = new ShadowDef;
    if (!def)
        return I3DERR_OUTOFMEM;
    LS3D_RESULT r = LoadShadow(name, flags, cb, context, def);
    if (r < 0)
        return r;
    if (parent)
        SetTo(parent);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10051800
void I3D_shadow::SetRange(float r)
{
    if (r > 0.0f)
        range = r;
    light_dir.Zero();
    shd_flags = 0;
}

// FUNCTION: LS3DF 0x10051840
LS3D_RESULT I3D_shadow::SetMaxTextureDistance(float d)
{
    if (d < 0.0f || d > 32.0f)
        return I3DERR_INVALIDPARAMS;
    max_texture_distance = d;
    light_dir.Zero();
    lights_count = -1;
    shd_flags = 0;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100518b0
void I3D_shadow::SetColor(const S_vector &c)
{
    color = c;
    if (color.x < 0.0f)
        color.x = 0.0f;
    else if (color.x > 1.0f)
        color.x = 1.0f;
    if (color.y < 0.0f)
        color.y = 0.0f;
    else if (color.y > 1.0f)
        color.y = 1.0f;
    if (color.z < 0.0f)
        color.z = 0.0f;
    else if (color.z > 1.0f)
        color.z = 1.0f;
    shd_flags = 0;
    lights_count = -1;
}

// FUNCTION: LS3DF 0x10051980
void I3D_shadow::SetReverseFactor(float f)
{
    reverse_factor = f;
    if (f < 0.0f)
        reverse_factor = 0.0f;
    else if (f > 1.0f)
        reverse_factor = 1.0f;
    shd_flags = 0;
}

// FUNCTION: LS3DF 0x100519e0
void I3D_shadow::SetOrthogonal(bool b)
{
    orthogonal = b;
    lights_count = -1;
    light_dir.Zero();
    shd_flags = 0;
}

// The powers of two from 4 up to 128 that hold the sizes. Both are computed from x (the original's too).
// FUNCTION: LS3DF 0x10051a20
LS3D_RESULT I3D_shadow::SetTextureSize(int x, int y)
{
    if (x > 128 || y > 128)
        return I3DERR_INVALIDPARAMS;
    size_x = 4;
    while (size_x < x)
        size_x <<= 1;
    size_y = 4;
    while (size_y < x)
        size_y <<= 1;
    if (texture) {
        texture->Release();
        texture = NULL;
    }
    lights_count = -1;
    shd_flags = 0;
    return I3D_OK;
}

// The parts' vertices into world space by their frames' matrices, when one of the frames moved. A destroyed frame's
// part is skipped without moving on in the vertices (the original's too). The pointers step in the loop's increment,
// after the copy.
// FUNCTION: LS3DF 0x10051ab0
bool I3D_shadow::UpdateVertices()
{
    if (!def)
        return false;
    if (!verts)
        return false;
    if (!num_frames)
        return false;
    if (!frames)
        return false;
    if (shadow_lock && shd_flags & SHDF_VERTS_VALID)
        return true;
    int count = 0;
    S_shadow_frame *sf = frames;
    for (int i = num_frames; i--; sf++) {
        if (sf->frame)
            count += sf->frame->update_count;
    }
    if (verts_count == count)
        return true;
    verts_count = count;
    const S_vector *src = verts;
    S_vector *dst = verts_w;
    sf = frames;
    for (i = num_frames; i--; sf++) {
        if (!sf->frame)
            continue;
        const S_matrix &m = sf->frame->GetMatrix();
        for (int j = sf->num_verts; j--; src++, dst++)
            *dst = *src * m;
    }
    shd_flags = (shd_flags & ~SHDF_NONE) | SHDF_VERTS_VALID | SHDF_VERTS_CHANGED;
    return true;
}

// The light of the shadow's sector that casts it: of the lights with I3DLIGHTMODE_DYNAMIC_SHADOW that are on, the
// strongest at the shadow's position. The opacity is its share of all of their light, less as the second strongest
// comes close to it. Then the vertices into the light's space, their box, and the visual placed and scaled to it.
// Shapes the original needs: the sector's search as UpdateV's, everything else inside its sector case; the spot
// light's direction written out (GetMatrix()(2), then Normalize: through GetWorldDir the nested Normalize stays out of
// line); the range tests with range_f_2 on the left; the floats initialized before the light pointer (so VC6 stores
// immediates, not its zero register); the points' first values stored through the member, before the walking pointers
// are set.
// FUNCTION: LS3DF 0x10051c00
bool I3D_shadow::UpdateLight()
{
    if (shadow_lock >= I3DLOCK_LIGHT && shd_flags & SHDF_LIGHT_VALID)
        return true;
    I3D_frame *frm = parent;
    while (true) {
        if (!frm)
            break;
        if (frm->type == FRAME_SECTOR) {
            if (sector != frm) {
                sector = (I3D_sector *)frm;
                lights_count = -1;
            }
            int count = sector->lights_changed + update_count;
            if (count != lights_count) {
                lights_count = count;
                shd_flags &= ~SHDF_LIGHT_VALID;
            }
            UpdateWMatrix();
            if (!(shd_flags & SHDF_LIGHT_VALID)) {
                I3D_light **lp = &sector->lights[0];
                float best = 0.0f, second = 0.0f, sum = 0.0f;
                I3D_light *light = NULL;
                for (int i = sector->lights.size(); i--; lp++) {
                    if (!((*lp)->mode & I3DLIGHTMODE_DYNAMIC_SHADOW) || !((*lp)->frm_flags & FRMFLAGS_ON))
                        continue;
                    float f;
                    switch ((*lp)->ltype) {
                    case I3DLIGHT_AMBIENT:
                        sum += (*lp)->color.x + (*lp)->color.y + (*lp)->color.z;
                        continue;
                    case I3DLIGHT_SPOT: {
                        S_vector d = matrix(3) - (*lp)->GetWorldPos();
                        float dist_2 = d.Square();
                        if ((*lp)->range_f_2 <= dist_2)
                            continue;
                        f = (*lp)->color.x + (*lp)->color.y + (*lp)->color.z;
                        if (IsMrgZeroLess(dist_2))
                            break;
                        float dist = (float)sqrt(dist_2);
                        if (dist > (*lp)->range_n)
                            f *= ((*lp)->range_f - dist) / (*lp)->range_delta;
                        S_vector ldir = (*lp)->GetMatrix()(2);
                        ldir.Normalize();
                        float cos = ldir.Dot(d) / dist;
                        if (cos <= (*lp)->outer_cos)
                            continue;
                        if (cos <= (*lp)->inner_cos)
                            f *= (cos - (*lp)->outer_cos) * (*lp)->cone_scale;
                    } break;
                    case I3DLIGHT_POINT: {
                        S_vector d = (*lp)->GetWorldPos() - matrix(3);
                        float dist_2 = d.x * d.x + d.y * d.y + d.z * d.z;
                        if ((*lp)->range_f_2 <= dist_2)
                            continue;
                        f = (*lp)->color.x + (*lp)->color.y + (*lp)->color.z;
                        float dist = (float)sqrt(dist_2);
                        if (dist > (*lp)->range_n)
                            f *= ((*lp)->range_f - dist) / (*lp)->range_delta;
                    } break;
                    case I3DLIGHT_DIRECTIONAL:
                        f = (*lp)->color.x + (*lp)->color.y + (*lp)->color.z;
                        break;
                    case I3DLIGHT_UNKNOWN_8: {
                        S_vector d = (*lp)->GetWorldPos() - matrix(3);
                        float dist_2 = d.x * d.x + d.y * d.y + d.z * d.z;
                        if ((*lp)->range_f_2 <= dist_2)
                            continue;
                        f = (*lp)->color.x + (*lp)->color.y + (*lp)->color.z;
                        float dist = (float)sqrt(dist_2);
                        if (dist > (*lp)->range_n)
                            f *= ((*lp)->range_f - dist) / (*lp)->range_delta;
                        sum += f;
                        f = 0.0f;
                    } break;
                    default:
                        continue;
                    }
                    sum += f;
                    if (second < f) {
                        if (best < f) {
                            second = best;
                            best = f;
                            light = *lp;
                        } else
                            second = f;
                    }
                }
                if (sum < 1.0f / 256)
                    break;
                if (!light) {
                    opacity = 0.0f;
                    shd_flags |= SHDF_NONE | SHDF_LIGHT_VALID | SHDF_OPACITY_CHANGED | SHDF_DIR_CHANGED;
                    return false;
                }
                float o = best / sum * best * (1.0f / 3);
                // The second strongest light fades the shadow out from 95% of the strongest one's light on. The
                // constant is a local float: VC6 folds 1 - edge_k from the float 0.95 (0x3d4cccd0), where
                // 1.0f - .95f gives 0.05 itself, and a file's static const float stays a variable.
                const float edge_k = .95f;
                float edge = best * edge_k;
                if (second > edge)
                    o *= 1.0f - (second - edge) / (best * (1.0f - edge_k));
                if (o < 1.0f / 256) {
                    opacity = 0.0f;
                    shd_flags |= SHDF_NONE | SHDF_LIGHT_VALID | SHDF_OPACITY_CHANGED | SHDF_DIR_CHANGED;
                    return false;
                }
                shd_flags &= ~SHDF_NONE;
                if (o > 1.0f)
                    o = 1.0f;
                if (!IsAbsMrgZero(opacity - o)) {
                    opacity = o;
                    shd_flags |= SHDF_OPACITY_CHANGED;
                }
                S_vector dir;
                if (light->ltype == I3DLIGHT_DIRECTIONAL)
                    dir = light->GetWorldDir();
                else
                    dir.SetNormalized(matrix(3) - light->GetWorldPos());
                if (!IsMrgZeroLess(fabs(dir.x - light_dir.x) + fabs(dir.y - light_dir.y) +
                    fabs(dir.z - light_dir.z))) {
                    light_dir = dir;
                    S_matrix m;
                    m.SetDir(light_dir, S_vector(0.0f, 1.0f, 0.0f));
                    light_matrix.Inverse(m);
                    shd_flags |= SHDF_LIGHT_VALID | SHDF_DIR_CHANGED;
                } else
                    shd_flags |= SHDF_LIGHT_VALID;
            }
            if (shd_flags & SHDF_LIGHT_VALID && !(shd_flags & (SHDF_DIR_CHANGED | SHDF_VERTS_CHANGED)))
                return shd_flags & SHDF_NONE ? false : true;
            S_vector pt = verts_w->RotateByMatrix(light_matrix);
            points[0] = pt.x;
            bbox.min.x = bbox.max.x = pt.x;
            points[1] = pt.y;
            bbox.min.y = bbox.max.y = pt.y;
            bbox.min.z = pt.z;
            const S_vector *v = verts_w;
            float *p = points;
            for (int i = num_verts - 1; i--; ) {
                v++;
                p += 2;
                pt = v->RotateByMatrix(light_matrix);
                p[0] = pt.x;
                p[1] = pt.y;
                if (pt.x < bbox.min.x)
                    bbox.min.x = pt.x;
                else if (pt.x > bbox.max.x)
                    bbox.max.x = pt.x;
                if (pt.y < bbox.min.y)
                    bbox.min.y = pt.y;
                else if (pt.y > bbox.max.y)
                    bbox.max.y = pt.y;
                if (pt.z < bbox.min.z)
                    bbox.min.z = pt.z;
            }
            if (IsAbsMrgZero(bbox.max.x - bbox.min.x) || IsAbsMrgZero(bbox.max.y - bbox.min.y))
                break;
            // The box reaches towards the light at most max_texture_distance from the shadow's position
            float z = matrix.m[3][0] * light_matrix.m[0][2] + matrix.m[3][1] * light_matrix.m[1][2] +
                matrix.m[3][2] * light_matrix.m[2][2];
            float dz = bbox.min.z - z;
            if (fabs(dz) > max_texture_distance)
                bbox.min.z = (dz > 0.0f ? max_texture_distance : -max_texture_distance) + z;
            visual->SetDir(light_dir, 0.0f);
            visual->SetScale(S_vector((bbox.max.x - bbox.min.x) * .5f, (bbox.max.y - bbox.min.y) * .5f, range));
            S_vector pos((bbox.max.x + bbox.min.x) * .5f, (bbox.min.y + bbox.max.y) * .5f, bbox.min.z);
            S_matrix m;
            m.SetDir3(light_dir, S_vector(0.0f, 1.0f, 0.0f));
            pos = pos.RotateByMatrix(m);
            visual->SetPos(pos);
            visual->UpdateV();
            return shd_flags & SHDF_NONE ? false : true;
        }
        frm = frm->parent;
    }
    opacity = 0.0f;
    shd_flags |= SHDF_NONE | SHDF_LIGHT_VALID | SHDF_OPACITY_CHANGED | SHDF_DIR_CHANGED;
    return false;
}

// Renders the points into the texture, created first, unless it is up to date or locked
// FUNCTION: LS3DF 0x100529c0
bool I3D_shadow::RenderTexture()
{
    if (shd_flags & SHDF_TEXTURE_VALID) {
        if (!(shd_flags & (SHDF_DIR_CHANGED | SHDF_VERTS_CHANGED)))
            return true;
        if (shadow_lock > I3DLOCK_VERTICES)
            return true;
    }
    if (shd_flags & SHDF_NONE)
        return false;
    if (!texture) {
        texture = pGlobalIGraph->CreateITexture();
        if (texture->Create(NULL, NULL, ITC_USE_ALPHA | ITC_FILL | ITC_EMPTY, (ushort)size_x, (ushort)size_y, 0) < 0) {
            shd_flags |= SHDF_NONE;
            return false;
        }
        pI3DDriver->m_RecomputeUsedVRAM();
    }
    pI3DDriver->RenderShadowTexture(points, num_verts, faces, num_faces, texture, &bbox.min.x);
    shd_flags |= SHDF_TEXTURE_VALID | SHDF_TEXTURE_CHANGED;
    return true;
}

// A shadow that is on, set to a frame that is on and near enough to the camera has its visual in the scene's omni
// sector, once its vertices, light and texture are ready; otherwise the visual leaves it. The original has the failure
// inline after the last step's test, which the earlier tests jump into.
// FUNCTION: LS3DF 0x10052a90
void I3D_shadow::Prepare()
{
    if (!parent) {
        if (scene)
            scene->DeleteShadow(this);
        if (visual && visual->parent)
            visual->LinkTo(NULL, 0);
        shd_flags = 0;
        return;
    }
    if (!(frm_flags & FRMFLAGS_ON) || !(parent->frm_flags & FRMFLAGS_ON)) {
        if (visual && visual->parent)
            visual->LinkTo(NULL, 0);
        shd_flags = 0;
        return;
    }
    if (!def)
        return;
    float r = FOVDistCorrection * visible_range;
    S_vector d = GetWorldPos() - pCurrentCamera->GetWorldPos();
    if (d.Square() > r * r)
        goto hide;
    if (shadow_lock == I3DLOCK_RECEIVERS && shd_flags & SHDF_RECEIVERS_VALID)
        return;
    if (!scene) {
        UpdateV();
        if (!scene)
            return;
    }
    shd_flags &= ~(SHDF_TEXTURE_CHANGED | SHDF_DIR_CHANGED | SHDF_VERTS_CHANGED | SHDF_OPACITY_CHANGED |
        SHDF_RECEIVERS_CHANGED);
    if (!UpdateVertices())
        goto hide;
    if (!UpdateLight())
        goto hide;
    if (!RenderTexture()) {
hide:
        if (visual && visual->parent)
            visual->LinkTo(NULL, 0);
        shd_flags &= ~SHDF_RECEIVERS_VALID;
        return;
    }
    if (!visual->parent)
        visual->LinkTo(scene->omni_sector, 0);
}

// FUNCTION: LS3DF 0x10052c70
ShadowDef::~ShadowDef()
{
    S_shadow_part *p = &parts[0];
    for (int i = parts.size(); i--; p++)
        delete[] p->name;
}

// FUNCTION: LS3DF 0x10052ce0
I3D_shadow_visual::I3D_shadow_visual()
{
    verts = NULL;
    verts_alloc = 0;
    num_verts = 0;
    faces = NULL;
    faces_alloc = 0;
    num_faces = 0;
    num_faces_t = 0;
    visual_type = I3D_VISUAL_SHADOW;
    center_t = ZeroSVector;
    rfg.fg = NULL;
    rfg.sector = NULL;
    rfg.texture = NULL;
    rfg_t.fg = NULL;
    rfg_t.sector = NULL;
    rfg_t.texture = NULL;
    shadow = NULL;
    num_receivers = 0;
    receivers_count = 0;
    rfg.clip = 1;
    rfg_t.clip = 1;
    rfg.vis = this;
    rfg_t.vis = this;
    memset(&vb, 0, sizeof(vb) + sizeof(ib));
    vis_bound.bbox = I3D_bbox(S_vector(-1.0f, -1.0f, 0.0f), S_vector(1.0f, 1.0f, 1.0f));
    vis_bound.bsphere.radius = (float)sqrt(2.25);
    vis_bound.bsphere.pos = S_vector(0.0f, 0.0f, .5f);
}

// FUNCTION: LS3DF 0x10052e90
I3D_shadow_visual::~I3D_shadow_visual()
{
    delete[] verts;
    delete[] faces;
    cVBMan.m_Free(vb);
    cIBMan.m_Free(ib);
}

// The shadows' visuals whose receivers ProcessShadows clips this frame (PC only; the name is a guess, chosen for the
// original's .bss order)
// GLOBAL: LS3DF 0x101b4f3c
C_list<I3D_shadow_visual *> ShadowVisualQueue;

// The receivers kept in the buffers (I3DLOCK_RECEIVERS) are queued at once; otherwise ProcessShadows clips them first
// FUNCTION: LS3DF 0x10052f80
void I3D_shadow_visual::m_AddFGroups()
{
    if (shadow->shadow_lock == I3DLOCK_RECEIVERS && shadow->shd_flags & SHDF_RECEIVERS_VALID) {
        if (!num_verts)
            return;
        if (vb.vb) {
            AddFGroups();
            return;
        }
        shadow->shd_flags &= ~SHDF_RECEIVERS_VALID;
    }
    if (shadow->shd_flags & SHDF_TEXTURE_VALID)
        ShadowVisualQueue.push_back(this);
}

// Queues the opaque and the transparent receivers' faces for m_RenderFGroup. Both take the texture in rfg, so the
// transparent ones' has none, which tells m_RenderFGroup which faces it renders.
// FUNCTION: LS3DF 0x10053060
void I3D_shadow_visual::AddFGroups()
{
    if (!num_verts)
        return;
    if (num_faces) {
        rfg.sector = shadow->sector;
        rfg.texture = shadow->texture;
        rfg.sort_value = 0xbfffffff;
        AddFGHlpStruct.fgroups.push_back(rfg);
        AddFGHlpStruct.num_transparent++;
    }
    if (num_faces_t) {
        rfg_t.sector = shadow->sector;
        rfg.texture = shadow->texture;
        rfg_t.sort_value = (FloatToInt(1.0f) & 0x3fffffff) | 0x80000000;
        AddFGHlpStruct.fgroups.push_back(rfg_t);
        AddFGHlpStruct.num_transparent++;
    }
}

// What the rendering calls once the visuals are queued (render.cpp; PC only, the name is a guess). A do-while under
// the count's test: the original saves edi before the test, where a for or while loop over the count tests first.
// FUNCTION: LS3DF 0x10053250
void ProcessShadows()
{
    I3D_shadow_visual **vp = &ShadowVisualQueue[0];
    int i = ShadowVisualQueue.size();
    if (i) {
        do {
            (*vp)->ProcessShadow();
            vp++;
        } while (--i);
    }
    ShadowVisualQueue.reset();
}

// The receivers: the visuals with a mesh among the queued face groups that take shadows (SHADOW_MODE 1 or 2), in the
// shadow's sector or one of its group, whose boxes meet the visual's and that aren't of the shadow's own model. Their
// meshes' faces are clipped to the visual's volume (clipper.h), and the result becomes the vertices, colored by the
// shadow and its opacity, and the faces m_RenderFGroup renders: in system memory, or with I3DLOCK_RECEIVERS in the
// buffer managers' buffers, which keep them from frame to frame.
// Shapes the original needs: the receivers read through (*rp) at each use, so GetRenderMatrix tests the flag again
// after the branch on it, with the single meshes' matrix a local before the call; their lod index through
// I3D_object::GetMeshLOD; the lift through I3DIntAsFloat; the buffer's failure jumping into the system memory one; the
// two empty results as copies, joined for the flags.
// FUNCTION: LS3DF 0x10053280
void I3D_shadow_visual::ProcessShadow()
{
    Update();
    I3D_visual *last = NULL;
    int num = 0;
    I3D_visual **rp = ShadowReceivers;
    const I3D_bvolume &bvol = GetWBoundVolume();
    uint bits = shadow->sector->group_bits;
    RENDER_FG_HELP_STRUCT *fg = &AddFGHlpStruct.fgroups[0];
    for (int i = AddFGHlpStruct.fgroups.size(); i--; fg++) {
        if (last == fg->vis)
            continue;
        last = fg->vis;
        if (!last)
            continue;
        switch (last->visual_type) {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_SINGLEMESH:
        case I3D_VISUAL_MORPH:
        case I3D_VISUAL_SINGLEMORPH:
            break;
        default:
            continue;
        }
        if (!(last->vis_flags & VISF_SHADOW))
            continue;
        if (fg->sector != shadow->sector && !(fg->sector->group_bits & bits))
            continue;
        const I3D_bbox &bb = last->GetWBoundVolume().bbox;
        if (bb.min.x > bvol.bbox.max.x || bb.min.y > bvol.bbox.max.y || bb.min.z > bvol.bbox.max.z ||
            bb.max.x < bvol.bbox.min.x || bb.max.y < bvol.bbox.min.y || bb.max.z < bvol.bbox.min.z)
            continue;
        for (I3D_frame *frm = last; frm; frm = frm->parent) {
            if (frm == shadow->parent)
                goto next;
            if (frm->type == FRAME_MODEL || frm->type == FRAME_SECTOR)
                break;
        }
        *rp++ = last;
        if (++num >= 512) {
            dbgPrintf("!! Maximum shadow receivers reached:%d", 512);
            break;
        }
    next:;
    }
    if (!num) {
        num_verts = 0;
        num_faces = 0;
        num_faces_t = 0;
    } else {
        int count = 0;
        rp = ShadowReceivers;
        for (i = num; i--; rp++)
            count += (*rp)->update_count + ((I3D_object *)*rp)->GetMeshLOD();
        if (num != num_receivers || count != receivers_count) {
            num_receivers = num;
            receivers_count = count;
            shadow->shd_flags |= SHDF_RECEIVERS_CHANGED;
        }
        if (shadow->shd_flags & SHDF_RECEIVERS_VALID && !(shadow->shd_flags & (SHDF_DIR_CHANGED | SHDF_VERTS_CHANGED |
            SHDF_OPACITY_CHANGED | SHDF_RECEIVERS_CHANGED))) {
            if (num_verts)
                AddFGroups();
            return;
        }
        const S_matrix &inv = GetInvMatrix();
        ResetClipArrays();
        float lift = I3DIntAsFloat(renderState[61]);
        SetClipParams(inv, GetRenderMatrix(), shadow->orthogonal, lift, lift, shadow->reverse_factor);
        rp = ShadowReceivers;
        for (i = num; i--; rp++) {
            const S_vector *corners = BBoxVertices((*rp)->GetBoundVolume().bbox);
            S_matrix m = (*rp)->GetMatrix() * inv;
            uint clip = 0;
            for (int j = 8; j--; corners++) {
                S_vector v = *corners * m;
                if (v.x < 1.0f)
                    clip |= 1;
                if (v.x > -1.0f)
                    clip |= 2;
                if (v.y < 1.0f)
                    clip |= 4;
                if (v.y > -1.0f)
                    clip |= 8;
                if (v.z > 0.0f)
                    clip |= 0x10;
                if (v.z < 1.0f)
                    clip |= 0x20;
            }
            if (clip != 0x3f)
                continue;
            I3D_object *obj = (I3D_object *)*rp;
            if (!obj->mesh)
                continue;
            I3D_mesh_level *level = obj->mesh->GetLOD(obj->GetMeshLOD());
            if (!level)
                continue;
            I3D_visual *vis = *rp;
            if (vis->visual_type == I3D_VISUAL_SINGLEMESH || vis->visual_type == I3D_VISUAL_SINGLEMORPH) {
                if ((*rp)->vis_flags & VISF_WORLD_SPACE)
                    ClipSingleMeshW(((I3D_singlemesh *)*rp)->GetVertexPos(((I3D_object *)*rp)->GetMeshLOD()),
                        level->num_vertices, level->fgroups, level->NumFaces(), (*rp)->vis_flags & VISF_SHADOW_2);
                else {
                    const S_matrix &m = (*rp)->GetRenderMatrix();
                    ClipSingleMesh(((I3D_singlemesh *)*rp)->GetVertexPos(((I3D_object *)*rp)->GetMeshLOD()),
                        level->num_vertices, level->fgroups, level->NumFaces(), (*rp)->vis_flags & VISF_SHADOW_2, m);
                }
            } else {
                if ((*rp)->vis_flags & VISF_WORLD_SPACE)
                    ClipMeshW(level->LockVertices(0), level->num_vertices, level->fgroups, level->NumFaces(),
                        (*rp)->vis_flags & VISF_SHADOW_2);
                else {
                    const S_matrix &m = (*rp)->GetRenderMatrix();
                    ClipMesh(level->LockVertices(0), level->num_vertices, level->fgroups, level->NumFaces(),
                        (*rp)->vis_flags & VISF_SHADOW_2, m);
                }
            }
        }
        if (!num_clip_results) {
            num_verts = 0;
            num_faces = 0;
            num_faces_t = 0;
        } else {
            S_shadow_vertex *v;
            if (shadow->shadow_lock == I3DLOCK_RECEIVERS) {
                if (verts) {
                    delete[] verts;
                    verts = NULL;
                    verts_alloc = 0;
                    delete[] faces;
                    faces = NULL;
                    faces_alloc = 0;
                }
                if (verts_alloc < num_clip_results || !vb.vb) {
                    cVBMan.m_Free(vb);
                    verts_alloc = num_clip_results + 8;
                    if (!cVBMan.m_Alloc(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DUSAGE_SOFTWAREPROCESSING,
                            sizeof(S_shadow_vertex), verts_alloc, vb, VF_LVERTEX, false)) {
                        vb.buffer = NULL;
                        goto oom;
                    }
                }
                v = (S_shadow_vertex *)vb.buffer->Lock(vb.base, 0);
            } else {
                if (vb.buffer) {
                    cVBMan.m_Free(vb);
                    vb.buffer = NULL;
                    verts_alloc = 0;
                    cIBMan.m_Free(ib);
                    ib.buffer = NULL;
                    faces_alloc = 0;
                }
                if (verts_alloc < num_clip_results) {
                    delete[] verts;
                    verts_alloc = num_clip_results + 16;
                    if (verts_alloc < 100)
                        verts_alloc = 100;
                    verts = new S_shadow_vertex[verts_alloc];
                    if (!verts) {
oom:
                        verts_alloc = 0;
                        dbgPrintf("!! Out of memory  I3D_projector::ProcessShadow()");
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return;
                    }
                }
                v = verts;
            }
            num_verts = num_clip_results;
            const S_vector *r = clip_results;
            for (i = num_clip_results; i--; v++, r++) {
                v->pos = *r;
                v->u = (r->x + 1.0f) * .5f;
                v->v = (r->y + 1.0f) * .5f;
                v->color = (((FloatToInt((1.0f - r->z) * shadow->opacity * 255.0f) << 8 |
                    FloatToInt(shadow->color.x * 255.0f)) << 8 | FloatToInt(shadow->color.y * 255.0f)) << 8) |
                    FloatToInt(shadow->color.z * 255.0f);
            }
            ushort *f;
            if (shadow->shadow_lock == I3DLOCK_RECEIVERS) {
                if (faces_alloc < num_clip_faces + num_clip_faces_t) {
                    faces_alloc = num_clip_faces + num_clip_faces_t + 4;
                    if (!cIBMan.m_Alloc(D3DUSAGE_SOFTWAREPROCESSING, faces_alloc * 3, ib)) {
                        faces_alloc = 0;
                        ib.buffer = NULL;
                        dbgPrintf("!! Out of memory  I3D_projector::ProcessShadow()");
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return;
                    }
                }
                f = (ushort *)ib.buffer->Lock(ib.base, 0);
            } else {
                if (faces_alloc < num_clip_faces + num_clip_faces_t) {
                    delete[] faces;
                    faces_alloc = num_clip_faces + num_clip_faces_t + 16;
                    if (faces_alloc < 100)
                        faces_alloc = 100;
                    faces = new ushort[faces_alloc * 3];
                    if (!faces) {
                        faces_alloc = 0;
                        dbgPrintf("!! Out of memory  I3D_shadow::ProcessShadow()");
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return;
                    }
                }
                f = faces;
            }
            memcpy(f, clip_faces, num_clip_faces * 3 * sizeof(ushort));
            num_faces = num_clip_faces;
            memcpy(f + num_clip_faces * 3, clip_faces_t, num_clip_faces_t * 3 * sizeof(ushort));
            num_faces_t = num_clip_faces_t;
            if (num_clip_results_t > 0)
                center_t = clip_center_t / (float)num_clip_results_t * GetRenderMatrix();
            AddFGroups();
            shadow->shd_flags = (shadow->shd_flags & ~(SHDF_TEXTURE_CHANGED | SHDF_DIR_CHANGED | SHDF_VERTS_CHANGED |
                SHDF_OPACITY_CHANGED | SHDF_RECEIVERS_CHANGED)) | SHDF_RECEIVERS_VALID;
            return;
        }
    }
    shadow->shd_flags = (shadow->shd_flags & ~(SHDF_TEXTURE_CHANGED | SHDF_DIR_CHANGED | SHDF_VERTS_CHANGED |
        SHDF_OPACITY_CHANGED | SHDF_RECEIVERS_CHANGED)) | SHDF_RECEIVERS_VALID;
}

// The receivers' faces with the shadow's texture, alpha blended, clamped, biased towards the camera and without z
// writes: the opaque ones (help's texture set, see AddFGroups) or the transparent ones after them. From the buffers the
// transparent faces' base vertex is moved by the opaque faces' indices (the original's too). SetStreamSource is in
// both branches: VC6 pushes its arguments before the test.
// FUNCTION: LS3DF 0x10053ee0
void I3D_shadow_visual::m_RenderFGroup(RENDER_FG_HELP_STRUCT *help)
{
    SetColorOp(D3DTOP_MODULATE);
    SetColorOp1(D3DTOP_DISABLE);
    SetAlphaOp(D3DTOP_MODULATE);
    SetAlphaState(1);
    SetBlendState(0);
    SetCullState(D3DCULL_CCW);
    SetClipping(1);
    m_pD3DDevice8->SetTexture(0, shadow->texture->texture);
    pGlobalIGraph->SetWorldMatrix(GetMatrix());
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    SetZBias(RsZBiasShadow);
    m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    if (shadow->shadow_lock == I3DLOCK_RECEIVERS) {
        SetVertexFormat(0);
        if (help->texture) {
            m_pD3DDevice8->SetStreamSource(0, vb.vb, sizeof(S_shadow_vertex));
            m_pD3DDevice8->SetIndices(ib.ib, vb.base);
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, ib.base, num_faces);
        } else {
            m_pD3DDevice8->SetStreamSource(0, vb.vb, sizeof(S_shadow_vertex));
            m_pD3DDevice8->SetIndices(ib.ib, vb.base + num_faces * 3);
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, ib.base, num_faces_t);
        }
    } else {
        if (help->texture)
            pGlobalIGraph->DrawIndexedPrimitiveList(PT_TRIANGLELIST, num_verts, faces, num_faces, verts,
                LS3D_STREAM_LVERTEX);
        else
            pGlobalIGraph->DrawIndexedPrimitiveList(PT_TRIANGLELIST, num_verts, faces + num_faces * 3, num_faces_t,
                verts, LS3D_STREAM_LVERTEX);
    }
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}
