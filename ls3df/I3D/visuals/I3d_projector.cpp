// The projectors (PS2 I3d_projector.cpp, which has only the settings and the loader), at 0x10045e30-0x10048a00 in the
// original: I3D_projector from its constructor in PS2's order, then the PC's receivers: AddFGroups, the queue,
// m_AddFGroups, ProcessProjectors and ProcessRecList, the header's zero vector's $E (0x10048040) after it, then
// ProcessShadow, MakeProjection with CollectReceivers, m_RenderFGroup and LoadImage (PS2's is after the setters). The
// shadows' visual (I3d_shadow.cpp) is a copy of the receivers' code, which shares their array with it. Names are PS2's
// unless noted.
//
// Not matching yet: ProcessRecList inlines C_list::push_back into the vertex buffer's Lock, as the original does, only
// with about 11 more statements' worth of source anywhere in the function (dead code measures it; the alpha in more
// statements, int literals, casts, a helper for the vertex loops or __forceinline float helpers don't count). Ours
// calls it, and the registers differ from there to the end. The original's push_back, which stores the size in the
// copy's statement (ls3df.h), gives the function exactly.
#include "I3D/I3D.h"
#include "I3D/render/IBManager.h"
#include "I3D/render/clipper.h"
#include "I3D/render/render.h"
#include "I3D/render/VBManager.h"
#include "IGraph/IGraph.h"
#include "dta.h"

#include <string.h>

// The driver's bias for the projectors (RS_ZBIAS_PROJECTOR)
extern int RsZBiasProjector;

// MakeProjection's search for CollectReceivers (PC only; the names are guesses, chosen for the original's .bss order,
// see tools/bsshash.py): its flags, the projector's box in world space, its sector and that sector's group bits, and
// where the next receiver goes. receiver_count counts ShadowReceivers for ProcessShadow too.
// GLOBAL: LS3DF 0x101b45f8
static uint projection_mode;

// GLOBAL: LS3DF 0x101b4608
static int receiver_count;

// GLOBAL: LS3DF 0x101b460c
static const I3D_bbox *projection_bbox;

// GLOBAL: LS3DF 0x101b4610
static I3D_sector *projection_sector;

// GLOBAL: LS3DF 0x101b4624
static I3D_visual **projection_rp;

// GLOBAL: LS3DF 0x101b4628
static uint projection_bits;

// The visuals a projector or shadow falls on (PC only; the name is a guess)
// GLOBAL: LS3DF 0x101b4630
I3D_visual *ShadowReceivers[512];

// PS2's order, the PC's members after the bound volume
// FUNCTION: LS3DF 0x10045e30
I3D_projector::I3D_projector()
{
    visual_type = I3D_VISUAL_PROJECTOR;
    orthogonal = true;
    texture = NULL;
    material = NULL;
    visible_range = 100.0f;
    alpha = 1.0f;
    reverse_factor = .5f;
    vis_bound.bbox = I3D_bbox(S_vector(-1.0f, -1.0f, 0.0f), S_vector(1.0f, 1.0f, 1.0f));
    vis_bound.bsphere.radius = (float)sqrt(2.25);
    vis_bound.bsphere.pos = S_vector(0.0f, 0.0f, .5f);
    verts = NULL;
    verts_alloc = 0;
    num_verts = 0;
    faces = NULL;
    faces_alloc = 0;
    num_faces = 0;
    num_faces_t = 0;
    rfg = NULL;
    projector_lock = I3DLOCK_NONE;
    center_t = ZeroSVector;
    color = UnitVector;
    mode = PROJECTOR_MODE_ADD;
    receivers_update = -1;
    num_receivers = 0;
    receivers_count = 0;
    sector = NULL;
    proj_flags = 0;
    memset(&vb, 0, sizeof(vb) + sizeof(ib));
}

// FUNCTION: LS3DF 0x10046000
I3D_projector::~I3D_projector()
{
    if (texture)
        texture->Release();
    if (material)
        material->Release();
    delete[] verts;
    delete[] faces;
    delete[] rfg;
    cVBMan.m_Free(vb);
    cIBMan.m_Free(ib);
}

// A projector takes another's settings; a mesh visual's first face group gives it its material
// FUNCTION: LS3DF 0x100460c0
LS3D_RESULT I3D_projector::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    if (frame->type != FRAME_VISUAL)
        return I3D_OK;
    switch (((I3D_visual *)frame)->visual_type) {
    case I3D_VISUAL_PROJECTOR: {
        I3D_projector *proj = (I3D_projector *)frame;
        SetTexture(proj->texture);
        SetMaterial(proj->material);
        SetOrthogonal(proj->orthogonal);
        SetVisibleRange(proj->visible_range);
        SetColor(proj->color);
        SetAlpha(proj->alpha);
        SetReverseFactor(proj->reverse_factor);
        SetMode(proj->mode);
        SetProjectorLock(proj->projector_lock);
        delete[] rfg;
        rfg = NULL;
        sector = NULL;
    } break;
    case I3D_VISUAL_OBJECT:
    case I3D_VISUAL_LIT_OBJECT:
    case I3D_VISUAL_BILLBOARD:
    case I3D_VISUAL_MORPH:
    case I3D_VISUAL_SINGLEMESH:
    case I3D_VISUAL_SINGLEMORPH:
    case I3D_VISUAL_LENSF: {
        I3D_mesh_object *mesh = ((I3D_object *)frame)->mesh;
        if (mesh) {
            I3D_mesh_level *level = mesh->GetLOD(0);
            if (level) {
                I3D_face_group *fg = level->GetFGroup(0);
                if (fg)
                    SetMaterial(fg->material);
            }
        }
    } break;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10046240
void I3D_projector::SetTexture(ITexture *tex)
{
    if (texture != tex) {
        if (texture)
            texture->Release();
        texture = tex;
        if (tex)
            tex->m_RefCount++;
    }
}

// FUNCTION: LS3DF 0x10046270
void I3D_projector::SetMaterial(I3D_material *mat)
{
    if (material != mat) {
        if (material)
            material->Release();
        material = mat;
        if (mat)
            mat->AddRef();
    }
}

// FUNCTION: LS3DF 0x100462a0
void I3D_projector::SetOrthogonal(bool b)
{
    orthogonal = b;
    proj_flags = 0;
    receivers_count = 0;
}

// FUNCTION: LS3DF 0x100462c0
void I3D_projector::SetColor(const S_vector &c)
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
    proj_flags = 0;
    receivers_count = 0;
}

// Only a negative alpha is clamped (the original's too)
// FUNCTION: LS3DF 0x100463a0
void I3D_projector::SetAlpha(float a)
{
    alpha = a;
    if (a < 0.0f)
        alpha = 0.0f;
    proj_flags = 0;
    receivers_count = 0;
}

// FUNCTION: LS3DF 0x100463f0
void I3D_projector::SetReverseFactor(float f)
{
    reverse_factor = f;
    if (f < 0.0f)
        reverse_factor = 0.0f;
    else if (f > 1.0f)
        reverse_factor = 1.0f;
    proj_flags = 0;
    receivers_count = 0;
}

// FUNCTION: LS3DF 0x10046460
void I3D_projector::SetMode(PROJECTOR_MODE m)
{
    mode = m;
    proj_flags = 0;
    receivers_count = 0;
}

// FUNCTION: LS3DF 0x10046480
void I3D_projector::SetProjectorLock(I3D_LOCK lock)
{
    projector_lock = lock;
    proj_flags = 0;
    receivers_count = 0;
}

// I3D_visual's, and a move looks for the sector again
// FUNCTION: LS3DF 0x100464a0
void I3D_projector::UpdateV()
{
    if (!(vis_flags & VISF_WORLD_SPACE) && frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED)) {
        vis_flags &= ~(VISF_WBOUND_VALID | VISF_UNKNOWN_80000 | VISF_INV_MAT_VALID);
        sector = NULL;
    }
}

// Queues the opaque and the transparent receivers' faces for m_RenderFGroup, the transparent ones by their center's
// distance from the camera's near plane. Both take the texture in rfg[0], so the transparent ones' has none, which
// tells m_RenderFGroup which faces it renders.
// FUNCTION: LS3DF 0x100464d0
void I3D_projector::AddFGroups()
{
    if (num_faces) {
        rfg[0].sector = AddFGHlpStruct.sector;
        rfg[0].texture = texture ? texture : material->GetTexture();
        rfg[0].sort_value = 0xbfffffff;
        AddFGHlpStruct.fgroups.push_back(rfg[0]);
        AddFGHlpStruct.num_transparent++;
    }
    if (num_faces_t) {
        rfg[1].sector = AddFGHlpStruct.sector;
        rfg[0].texture = texture ? texture : material->GetTexture();
        const S_frustum &f = pCurrentCamera->frustums[0];
        rfg[1].sort_value = (FloatToInt((center_t.x * f.normal[0].x + center_t.y * f.normal[0].y + center_t.z *
            f.normal[0].z + f.d[0]) * 100.0f) & 0x3fffffff) | 0x80000000;
        AddFGHlpStruct.fgroups.push_back(rfg[1]);
        AddFGHlpStruct.num_transparent++;
    }
}

// The projectors whose receivers the rendering clips this frame (PC only; the name is a guess, chosen for the
// original's .bss order)
// GLOBAL: LS3DF 0x101b4614
C_list<I3D_projector *> ProjectorsToRender;

// A projector that is on, has a texture, is in a sector and near enough to the camera is queued; with its receivers'
// faces kept in the buffers it queues them at once
// FUNCTION: LS3DF 0x10046780
void I3D_projector::m_AddFGroups()
{
    if (!(frm_flags & FRMFLAGS_ON))
        return;
    if (!texture && (!material || !material->GetTexture()))
        return;
    if (!sector) {
        I3D_frame *frm = parent;
        while (true) {
            if (!frm)
                return;
            if (frm->type == FRAME_SECTOR)
                break;
            frm = frm->parent;
        }
        sector = (I3D_sector *)frm;
    }
    float r = FOVDistCorrection * visible_range;
    S_vector d = GetWorldPos() - pCurrentCamera->GetWorldPos();
    if (d.Square() > r * r)
        return;
    if (proj_flags & PROJF_RECEIVERS_VALID) {
        if (!num_verts)
            return;
        if (vb.vb) {
            AddFGroups();
            return;
        }
        proj_flags &= ~PROJF_RECEIVERS_VALID;
    }
    ProjectorsToRender.push_back(this);
}

// What the rendering calls once the visuals are queued (render.cpp; PC only, the name is a guess), as ProcessShadows
// FUNCTION: LS3DF 0x10046930
void ProcessProjectors()
{
    I3D_projector **pp = &ProjectorsToRender[0];
    int i = ProjectorsToRender.size();
    if (i) {
        do {
            (*pp)->ProcessShadow();
            pp++;
        } while (--i);
    }
    ProjectorsToRender.reset();
}

// Clips the faces of the receivers (ShadowReceivers, receiver_count of them) whose boxes reach into the projector's
// volume (clipper.h), and makes the result the vertices, with the texture coordinates of the projection and the
// projector's color by its mode, and the faces m_RenderFGroup renders: in system memory, or with a lock in the buffer
// managers' buffers. Close to the projector a perspective projection's coordinates are those of the side the vertex
// is on, both stored in u (the original's too). The added projections aren't lifted from the faces.
// FUNCTION: LS3DF 0x10046960
void I3D_projector::ProcessRecList()
{
    ResetClipArrays();
    float lift = I3DIntAsFloat(renderState[60]);
    if (mode != PROJECTOR_MODE_ADD && mode != PROJECTOR_MODE_ADD_FADE && mode != PROJECTOR_MODE_ADD_FADE_MID)
        SetClipParams(GetInvMatrix(), GetRenderMatrix(), orthogonal, lift, lift, reverse_factor);
    else
        SetClipParams(GetInvMatrix(), GetRenderMatrix(), orthogonal, lift, 0.0f, reverse_factor);
    I3D_visual **rp = ShadowReceivers;
    int i;
    for (i = receiver_count; i--; rp++) {
        const S_vector *corners = BBoxVertices((*rp)->GetBoundVolume().bbox);
        S_matrix m = (*rp)->GetMatrix() * GetInvMatrix();
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
                    level->num_vertices, level->fgroups, level->NumFaces(), (*rp)->vis_flags & VISF_PROJECTOR_2);
            else {
                const S_matrix &m = (*rp)->GetRenderMatrix();
                ClipSingleMesh(((I3D_singlemesh *)*rp)->GetVertexPos(((I3D_object *)*rp)->GetMeshLOD()),
                    level->num_vertices, level->fgroups, level->NumFaces(), (*rp)->vis_flags & VISF_PROJECTOR_2, m);
            }
        } else {
            if ((*rp)->vis_flags & VISF_WORLD_SPACE)
                ClipMeshW(level->LockVertices(0), level->num_vertices, level->fgroups, level->NumFaces(),
                    (*rp)->vis_flags & VISF_PROJECTOR_2);
            else {
                const S_matrix &m = (*rp)->GetRenderMatrix();
                ClipMesh(level->LockVertices(0), level->num_vertices, level->fgroups, level->NumFaces(),
                    (*rp)->vis_flags & VISF_PROJECTOR_2, m);
            }
        }
    }
    if (!num_clip_results) {
        num_verts = 0;
        return;
    }
    S_shadow_vertex *v;
    if (projector_lock) {
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
                verts_alloc = 0;
                vb.buffer = NULL;
                dbgPrintf("!! Out of memory  I3D_projector::ProcessRecList()  ClippedVertNum:%d", num_clip_results);
                return;
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
                verts_alloc = 0;
                dbgPrintf("!! Out of memory  I3D_projector::ProcessRecList()  ClippedVertNum:%d", num_clip_results);
                return;
            }
        }
        v = verts;
    }
    num_verts = num_clip_results;
    const S_vector *r = clip_results;
    if (orthogonal) {
        switch (mode) {
        case PROJECTOR_MODE_ADD_FADE:
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                v->u = (r->x + 1.0f) * .5f;
                v->v = .5f - r->y * .5f;
                float a = (1.0f - r->z) * alpha;
                if (a > 1.0f)
                    a = 1.0f;
                else if (a < 0.0f)
                    a = 0.0f;
                v->color = 0xff000000 | FloatToInt(a * color.x * 255.0f) << 16 |
                    FloatToInt(a * color.y * 255.0f) << 8 | FloatToInt(a * color.z * 255.0f);
            }
            break;
        case PROJECTOR_MODE_ADD_FADE_MID:
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                v->u = (r->x + 1.0f) * .5f;
                v->v = .5f - r->y * .5f;
                float a = (.5 - fabs(r->z - .5f)) * alpha * 2;
                if (a > 1.0f)
                    a = 1.0f;
                else if (a < 0.0f)
                    a = 0.0f;
                v->color = 0xff000000 | FloatToInt(a * color.x * 255.0f) << 16 |
                    FloatToInt(a * color.y * 255.0f) << 8 | FloatToInt(a * color.z * 255.0f);
            }
            break;
        case PROJECTOR_MODE_BLEND_FADE:
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                v->u = (r->x + 1.0f) * .5f;
                v->v = .5f - r->y * .5f;
                float a = (1.0f - r->z) * alpha;
                if (a > 1.0f)
                    a = 1.0f;
                else if (a < 0.0f)
                    a = 0.0f;
                v->color = FloatToInt(a * 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 |
                    FloatToInt(color.y * 255.0f) << 8 | FloatToInt(color.z * 255.0f);
            }
            break;
        case PROJECTOR_MODE_BLEND_FADE_MID:
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                v->u = (r->x + 1.0f) * .5f;
                v->v = .5f - r->y * .5f;
                float a = (.5 - fabs(r->z - .5f)) * alpha * 2;
                if (a > 1.0f)
                    a = 1.0f;
                else if (a < 0.0f)
                    a = 0.0f;
                v->color = FloatToInt(a * 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 |
                    FloatToInt(color.y * 255.0f) << 8 | FloatToInt(color.z * 255.0f);
            }
            break;
        default: {
            // An alpha below 1 is converted unscaled (the original's too)
            uint c = FloatToInt(alpha < 1.0f ? alpha : 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 |
                FloatToInt(color.y * 255.0f) << 8 | FloatToInt(color.z * 255.0f);
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                v->u = (r->x + 1.0f) * .5f;
                v->v = .5f - r->y * .5f;
                v->color = c;
            }
        } break;
        }
    } else {
        switch (mode) {
        case PROJECTOR_MODE_ADD_FADE:
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                if (IsMrgZeroLess(r->z)) {
                    v->u = r->x > 0.0f ? 1.0f : 0.0f;
                    v->u = r->y > 0.0f ? 0.0f : 1.0f;
                } else {
                    float k = .5f / r->z;
                    v->u = k * r->x + .5f;
                    v->v = .5f - k * r->y;
                }
                float a = (1.0f - r->z) * alpha;
                if (a > 1.0f)
                    a = 1.0f;
                else if (a < 0.0f)
                    a = 0.0f;
                v->color = 0xff000000 | FloatToInt(a * color.x * 255.0f) << 16 |
                    FloatToInt(a * color.y * 255.0f) << 8 | FloatToInt(a * color.z * 255.0f);
            }
            break;
        case PROJECTOR_MODE_ADD_FADE_MID:
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                if (IsMrgZeroLess(r->z)) {
                    v->u = r->x > 0.0f ? 1.0f : 0.0f;
                    v->u = r->y > 0.0f ? 0.0f : 1.0f;
                } else {
                    float k = .5f / r->z;
                    v->u = k * r->x + .5f;
                    v->v = .5f - k * r->y;
                }
                float a = (.5 - fabs(r->z - .5f)) * alpha * 2;
                if (a > 1.0f)
                    a = 1.0f;
                else if (a < 0.0f)
                    a = 0.0f;
                v->color = 0xff000000 | FloatToInt(a * color.x * 255.0f) << 16 |
                    FloatToInt(a * color.y * 255.0f) << 8 | FloatToInt(a * color.z * 255.0f);
            }
            break;
        case PROJECTOR_MODE_BLEND_FADE:
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                if (IsMrgZeroLess(r->z)) {
                    v->u = r->x > 0.0f ? 1.0f : 0.0f;
                    v->u = r->y > 0.0f ? 0.0f : 1.0f;
                } else {
                    float k = .5f / r->z;
                    v->u = k * r->x + .5f;
                    v->v = .5f - k * r->y;
                }
                float a = (1.0f - r->z) * alpha;
                if (a > 1.0f)
                    a = 1.0f;
                else if (a < 0.0f)
                    a = 0.0f;
                v->color = FloatToInt(a * 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 |
                    FloatToInt(color.y * 255.0f) << 8 | FloatToInt(color.z * 255.0f);
            }
            break;
        case PROJECTOR_MODE_BLEND_FADE_MID:
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                if (IsMrgZeroLess(r->z)) {
                    v->u = r->x > 0.0f ? 1.0f : 0.0f;
                    v->u = r->y > 0.0f ? 0.0f : 1.0f;
                } else {
                    float k = .5f / r->z;
                    v->u = k * r->x + .5f;
                    v->v = .5f - k * r->y;
                }
                float a = (.5 - fabs(r->z - .5f)) * alpha * 2;
                if (a > 1.0f)
                    a = 1.0f;
                else if (a < 0.0f)
                    a = 0.0f;
                v->color = FloatToInt(a * 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 |
                    FloatToInt(color.y * 255.0f) << 8 | FloatToInt(color.z * 255.0f);
            }
            break;
        default: {
            uint c = FloatToInt(alpha < 1.0f ? alpha : 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 |
                FloatToInt(color.y * 255.0f) << 8 | FloatToInt(color.z * 255.0f);
            for (i = num_verts; i--; v++, r++) {
                v->pos = *r;
                if (IsMrgZeroLess(r->z)) {
                    v->u = r->x > 0.0f ? 1.0f : 0.0f;
                    v->u = r->y > 0.0f ? 0.0f : 1.0f;
                } else {
                    float k = .5f / r->z;
                    v->u = k * r->x + .5f;
                    v->v = .5f - k * r->y;
                }
                v->color = c;
            }
        } break;
        }
    }
    ushort *f;
    if (projector_lock) {
        if (faces_alloc < num_clip_faces + num_clip_faces_t) {
            faces_alloc = num_clip_faces + num_clip_faces_t + 4;
            if (!cIBMan.m_Alloc(D3DUSAGE_SOFTWAREPROCESSING, faces_alloc * 3, ib)) {
                faces_alloc = 0;
                ib.buffer = NULL;
                dbgPrintf("!! Out of memory  I3D_projector::ProcessRecList()  ClippedFaceNum+ClippedFaceNumA:%d",
                    num_clip_faces + num_clip_faces_t);
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
                dbgPrintf("!! Out of memory  I3D_shadow::ProcessRecList()  ClippedFaceNum+ClippedFaceNumA:%d",
                    num_clip_faces + num_clip_faces_t);
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
    // The two face groups m_AddFGroups queues. The out of memory failure clears the vertices' count (the original's
    // too).
    if (!rfg) {
        rfg = new RENDER_FG_HELP_STRUCT[2];
        if (!rfg) {
            verts_alloc = 0;
            dbgPrintf("!! Out of memory  I3D_projector::ProcessRecList()");
            return;
        }
        rfg[0].clip = 1;
        rfg[0].vis = this;
        rfg[0].fg = NULL;
        rfg[1].clip = 1;
        rfg[1].vis = this;
        rfg[1].fg = NULL;
        rfg[1].texture = NULL;
    }
}

// The receivers: the visuals with a mesh among the queued face groups that take projectors, in the projector's sector
// or one of its group, and whose boxes meet the projector's. Their faces are clipped again when they or the projector
// changed, then queued.
// FUNCTION: LS3DF 0x10048060
void I3D_projector::ProcessShadow()
{
    if (frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED))
        update_count++;
    if (projector_lock)
        proj_flags |= PROJF_RECEIVERS_VALID;
    receiver_count = 0;
    I3D_visual *last = NULL;
    I3D_visual **rp = ShadowReceivers;
    const I3D_bvolume &bvol = GetWBoundVolume();
    uint bits = sector->group_bits;
    RENDER_FG_HELP_STRUCT *fg = &AddFGHlpStruct.fgroups[0];
    int i;
    for (i = AddFGHlpStruct.fgroups.size(); i--; fg++) {
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
        if (!(last->vis_flags & VISF_PROJECTOR))
            continue;
        if (fg->sector != sector && !(fg->sector->group_bits & bits))
            continue;
        const I3D_bbox &bb = last->GetWBoundVolume().bbox;
        if (bb.min.x > bvol.bbox.max.x || bb.min.y > bvol.bbox.max.y || bb.min.z > bvol.bbox.max.z ||
            bb.max.x < bvol.bbox.min.x || bb.max.y < bvol.bbox.min.y || bb.max.z < bvol.bbox.min.z)
            continue;
        *rp++ = last;
        if (++receiver_count >= 512) {
            dbgPrintf("!! Maximum projector receivers reached:%d", 512);
            break;
        }
    }
    if (!receiver_count) {
        num_verts = 0;
        return;
    }
    int count = 0;
    rp = ShadowReceivers;
    for (i = receiver_count; i--; rp++)
        count += (*rp)->update_count + ((I3D_object *)*rp)->GetMeshLOD();
    if (receivers_update == update_count && receiver_count == num_receivers && count == receivers_count) {
        if (num_verts)
            AddFGroups();
        return;
    }
    receivers_update = update_count;
    num_receivers = receiver_count;
    receivers_count = count;
    ProcessRecList();
    AddFGroups();
}

// FUNCTION: LS3DF 0x100482c0
void I3D_projector::MakeProjection(uint flags, I3D_frame *frame)
{
    if (!sector) {
        I3D_frame *frm = parent;
        while (true) {
            if (!frm)
                return;
            if (frm->type == FRAME_SECTOR)
                break;
            frm = frm->parent;
        }
        sector = (I3D_sector *)frm;
    }
    projection_sector = sector;
    projector_lock = I3DLOCK_RECEIVERS;
    proj_flags |= PROJF_RECEIVERS_VALID;
    receiver_count = 0;
    projection_rp = ShadowReceivers;
    projection_bbox = &GetWBoundVolume().bbox;
    projection_mode = flags;
    projection_bits = sector->group_bits;
    if (frame)
        CollectReceivers(frame);
    else if (flags & PROJF_OWN_SECTOR)
        CollectReceivers(sector);
    else {
        I3D_scene *scene = sector->scene;
        if (!scene) {
            num_verts = 0;
            return;
        }
        CollectReceivers(scene->primary_sector);
        if (flags & PROJF_OMNI_SECTOR)
            CollectReceivers(scene->omni_sector);
    }
    if (!receiver_count) {
        num_verts = 0;
        return;
    }
    ProcessRecList();
}

// The receivers in frm's hierarchy for MakeProjection: the visuals that take projectors and whose boxes meet the
// projector's, in the hierarchies whose boxes do. Other sectors only of the projector's group. A full list is reported
// for each further receiver, and the search goes on (the original's too).
// FUNCTION: LS3DF 0x100483e0
void I3D_projector::CollectReceivers(I3D_frame *frm)
{
    if (!frm->IsOn())
        return;
    const I3D_bbox &hb = frm->GetHRBoundVolumeW().bbox;
    if (hb.min.x > projection_bbox->max.x || hb.min.y > projection_bbox->max.y || hb.min.z > projection_bbox->max.z ||
        hb.max.x < projection_bbox->min.x || hb.max.y < projection_bbox->min.y || hb.max.z < projection_bbox->min.z)
        return;
    switch (frm->type) {
    case FRAME_VISUAL: {
        I3D_visual *vis = (I3D_visual *)frm;
        switch (vis->visual_type) {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_SINGLEMESH:
        case I3D_VISUAL_MORPH:
        case I3D_VISUAL_SINGLEMORPH:
            break;
        default:
            goto children;
        }
        if (!(vis->vis_flags & VISF_PROJECTOR))
            break;
        const I3D_bbox &bb = vis->GetWBoundVolume().bbox;
        if (bb.min.x > projection_bbox->max.x || bb.min.y > projection_bbox->max.y ||
            bb.min.z > projection_bbox->max.z || bb.max.x < projection_bbox->min.x ||
            bb.max.y < projection_bbox->min.y || bb.max.z < projection_bbox->min.z)
            break;
        *projection_rp++ = vis;
        if (++receiver_count >= 512)
            dbgPrintf("!! Maximum projector receivers reached:%d", 512);
    } break;
    case FRAME_SECTOR:
        if (frm != projection_sector) {
            if (projection_mode & PROJF_OWN_SECTOR)
                return;
            if (!(((I3D_sector *)frm)->group_bits & projection_bits))
                return;
        }
        break;
    case FRAME_MODEL:
        if (projection_mode & PROJF_NO_MODELS)
            return;
        break;
    }
children:
    if (projection_mode & PROJF_NO_CHILDREN)
        return;
    for (I3D_frame *f = frm->child; f; f = f->next)
        CollectReceivers(f);
}

// The receivers' faces with the projector's texture, added or blended by the mode, without z writes: the opaque ones
// (help's texture set, see AddFGroups) or the transparent ones after them. From the buffers the transparent faces' base
// vertex is moved by the opaque faces' indices (the original's too).
// FUNCTION: LS3DF 0x100485f0
void I3D_projector::m_RenderFGroup(RENDER_FG_HELP_STRUCT *help)
{
    if (!num_verts)
        return;
    SetColorOp(D3DTOP_MODULATE);
    SetColorOp1(D3DTOP_DISABLE);
    SetAlphaState(1);
    if (mode != PROJECTOR_MODE_ADD && mode != PROJECTOR_MODE_ADD_FADE && mode != PROJECTOR_MODE_ADD_FADE_MID) {
        SetBlendState(0);
        SetAlphaOp(D3DTOP_MODULATE);
    } else
        SetBlendState(2);
    SetCullState(D3DCULL_CCW);
    SetClipping(1);
    if (texture)
        m_pD3DDevice8->SetTexture(0, texture->m_pD3DTexture8);
    else
        m_pD3DDevice8->SetTexture(0, material->GetTexture()->m_pD3DTexture8);
    pGlobalIGraph->SetWorldMatrix(GetMatrix());
    SetZBias(RsZBiasProjector);
    m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    if (proj_flags & PROJF_RECEIVERS_VALID) {
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
    m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

// The projector's part of the 4ds file: the orthogonal flag, the mode and the material
// FUNCTION: LS3DF 0x10048960
void I3D_projector::LoadImage()
{
    I3D_frame::LoadImage();
    dtaRead(LoadImageHandle, &orthogonal, 1);
    dtaRead(LoadImageHandle, &mode, 1);
    int id = 0;
    dtaRead(LoadImageHandle, &id, 2);
    if (id > LoadMaterialu) {
        dbgPrintf("!! Invalid material ID:%d  materialu:%d  I3D_projector::LoadImage() %s", id, LoadMaterialu,
            ImageFileName);
        ImageErrFlg = I3DERR_FILECORRUPTED;
        return;
    }
    material = LoadMaterialy[id - 1];
    material->AddRef();
}
