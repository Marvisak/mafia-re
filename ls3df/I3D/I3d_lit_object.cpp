// The lit objects (PS2 I3d_lit_object.cpp), at 0x10021570-0x1002ade0 in the original: I3D_lit_object, the PC's
// functions first (constructor, destructor, SetActiveLevel, UpdateVertices and the rectangles' accessors), Construct
// with the header's zero vector's $E (0x10026900) after it, then the rest in PS2's order, and litLod's functions. Names
// are the PS2 build's unless noted.
// A lit object has six lightings of its mesh; the active one is rendered. Each has a litLod per level of the mesh:
// colors per vertex, or lightmaps, rectangles of texels packed into a texture (lightmap.cpp) that the lit vertices map
// into. Construct computes them from the scene's lights, Load reads them from a scene file.
//
// Not matching yet, beyond the functions' own notes: the original calls more header inline functions out of line at
// Construct's end (see there), so its file has their copies, which ours inlines: I3DFloatAsInt (0x100268b0, before
// S_vector2's), the timeGetTime wrapper (0x10026920, after the $E), I3D_frame::GetName (0x10027570, after Load), the
// face group's and the material's accessors (0x1002acc0-0x1002acf0) and C_list<VERTEX_STRUCT>'s push_back and
// operator[] (0x1002ad50, 0x1002add0). S_vector2::Magnitude's copy comes after the $E in ours, where the original has
// it before Dot's: defined before the zero vector, as the original's must have been, it moves the driver's $E past
// DrawTriangles (see I3d_math.h). litLod's scalar deleting destructor comes right after the $E in ours, GetRenderMatrix
// after m_RenderFGroup, and the render state helpers in render.h's order; the original has SetCullState, SetColorOp1
// and litLod's right before ~litLod, and GetRenderMatrix at the end (the original's headers had more inline code
// before them, see tools/autopch.py).
#include "I3D/I3D.h"
#include "I3D/lightmap.h"
#include "I3D/render.h"
#include "I3D/VBManager.h"
#include "I3D/IBManager.h"
#include "IGraph/IGraph.h"
#include "dta.h"

#include <string.h>
#include <mmsystem.h>

// IGraph.cpp's
extern D3DFORMAT m_TrueColorTexturePixelFormat;
extern bool m_DisableSleeping;

// FUNCTION: LS3DF 0x10021570
I3D_lit_object::I3D_lit_object()
{
    visual_type = I3D_VISUAL_LIT_OBJECT;
    active_level = 0;
    for (int i = 0; i < 6; i++) {
        levels[i].constructed = false;
        levels[i].mode = 2;
        levels[i].resolution = 5.0f;
        levels[i].angle = 45.0f;
        memset(levels[i].lods, 0, sizeof(levels[i].lods));
    }
}

// FUNCTION: LS3DF 0x10021600
I3D_lit_object::~I3D_lit_object()
{
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 10; j++) {
            delete levels[i].lods[j];
            levels[i].lods[j] = NULL;
        }
    }
}

// Frees the vertex buffers of the active lighting's lods, which renders them again, and makes another one active
// FUNCTION: LS3DF 0x100216a0
void I3D_lit_object::SetActiveLevel(int index)
{
    S_lit_level &level = levels[active_level];
    for (int i = 0; i < 10; i++) {
        if (level.lods[i]) {
            cVBMan.m_Free(level.lods[i]->vb);
            memset(&level.lods[i]->vb, 0, sizeof(level.lods[i]->vb));
            if (level.lods[i]->mem > 0) {
                cVBMan.m_Free(level.lods[i]->pos_vb);
                memset(&level.lods[i]->pos_vb, 0, sizeof(level.lods[i]->pos_vb));
            }
            level.lods[i]->dirty = true;
            level.lods[i]->lights_changed = (uint)-1;
            level.lods[i]->light_key = (uint)-1;
        }
    }
    active_level = index;
}

// FUNCTION: LS3DF 0x10021740
void I3D_lit_object::UpdateVertices()
{
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 10; j++) {
            if (levels[i].lods[j]) {
                levels[i].lods[j]->dirty = true;
                levels[i].lods[j]->lights_changed = (uint)-1;
                levels[i].lods[j]->light_key = (uint)-1;
            }
        }
    }
}

// FUNCTION: LS3DF 0x10021790
void I3D_lit_object::UpdateVertices(int index)
{
    for (int i = 0; i < 6; i++) {
        if (levels[i].lods[index]) {
            levels[i].lods[index]->dirty = true;
            levels[i].lods[index]->lights_changed = (uint)-1;
            levels[i].lods[index]->light_key = (uint)-1;
        }
    }
}

// FUNCTION: LS3DF 0x100217d0
int I3D_lit_object::GetNumRectInfos(int lod_index, int index)
{
    S_lit_level &level = levels[index];
    if (!level.lods[lod_index])
        return 0;
    return level.lods[lod_index]->m_numRectangles;
}

// FUNCTION: LS3DF 0x10021800
RECTANGLE_INFO *I3D_lit_object::GetRectInfos(int lod_index, int index)
{
    S_lit_level &level = levels[index];
    if (!level.lods[lod_index])
        return NULL;
    return level.lods[lod_index]->m_pRectangles;
}

// Copies the rectangles' texels into the lightmap's texture, after the caller changed them (GetRectInfos), resized
// where the texture has them smaller; true color lightmaps only
// Not matching yet: the original's frame is 0x24 bytes, ours 0x14. Two of its slots nothing references; q (with the
// single color's skip), the rotation flag and the outer counter have slots of their own, where ours keeps the flag and
// q in a dead parameter's; the registers differ in turn. No declaration scope moved it.
// FUNCTION: LS3DF 0x10021830
void I3D_lit_object::UpdateLMFromRectInfos(int lod_index, int index)
{
    if (!renderState[RS_LMTRUECOLOR] || !m_TrueColorTexturePixelFormat) {
        dbgPrintf("I3D_lit_object::UpdateLMFromRectInfos() - WARNING!! 16bit realtime LM update isn't implemented "
            "yet ! Use TRUECOLOR LMaps.");
        return;
    }
    S_lit_level &level = levels[index];
    if (!level.lods[lod_index])
        return;
    LightMap *lm = level.lods[lod_index]->m_pLM;
    uint pitch;
    uchar *dst = (uchar *)lm->texture->Lock(NULL, pitch, 0);
    if (!dst)
        return;
    RECTANGLE_INFO *ri = level.lods[lod_index]->m_pRectangles;
    int num = level.lods[lod_index]->m_numRectangles;
    while (num--) {
        bool rotated;
        int x;
        if (ri->u < 2.0f) {
            rotated = false;
            x = (int)(lm->texture->width * ri->u);
        } else {
            rotated = true;
            x = (int)(lm->texture->width * (ri->u - 2.0f));
        }
        int y = (int)(lm->texture->height * ri->v);
        uchar *p = dst + y * pitch + x * 4;
        if (ri->tex_sx == 3 && ri->tex_sy == 3) {
            uint skip = pitch - ri->tex_sx * 4;
            uint c = (uint)ri->data;
            for (int j = 3; j--; ) {
                for (int k = 3; k--; ) {
                    *p = (uchar)c;
                    p++;
                    *p = (uchar)(c >> 8);
                    p++;
                    *p = (uchar)(c >> 16);
                    p += 2;
                }
                p += skip;
            }
        } else {
            uchar *resized = NULL;
            uchar *src;
            if (ri->sx != ri->tex_sx || ri->sy != ri->tex_sy) {
                lm->m_Resize(ri, ri->tex_sx, ri->tex_sy, &resized);
                src = resized;
            } else
                src = ri->data;
            if (rotated) {
                for (int j = 0; j < ri->tex_sy; j++) {
                    uchar *q = p;
                    lm->m_FillLineRGB(&src, &q, ri->tex_sx, pitch);
                    p += 4;
                }
            } else {
                uint skip = pitch - ri->tex_sx * 4;
                for (int j = 0; j < ri->tex_sy; j++) {
                    lm->m_FillLineRGB(&src, &p, ri->tex_sx, 4);
                    p += skip;
                }
            }
            delete[] resized;
        }
        ri++;
    }
    lm->texture->Unlock();
    lm->texture->AddDirtyRect(NULL);
}

// Locks the lod's index buffer (PC; PS2's locks the lightmap's texture)
// FUNCTION: LS3DF 0x10021ad0
int I3D_lit_object::GetRectInfoFlags(int lod_index, int index, int flags)
{
    litLod *l = levels[index].lods[lod_index];
    if (!l)
        return -1;
    l->ib.buffer->Lock(l->ib.base, 0);
    return 0;
}

// T&l.cpp: a texel's or a vertex's lighting
void VertexLightingProcess(S_vector &out, S_vector pos, S_vector normal, I3D_material *mat, I3D_scene *scene,
    uint flags, bool *inside, I3D_frame *frm, int x);

// The time in ms the progress is reported by (PS2 reads g_MSCount1; the name is a guess)
inline uint GetMSCount()
{
    return timeGetTime();
}

// A level's vertex for the lighting, in world space (PS2 VERTEX_STRUCT, 0x2c bytes on PC; the member names are
// guesses)
struct VERTEX_STRUCT {
    S_vector pos;                       // 0x00
    S_vector2 uv;                       // 0x0c, in the texels of the rectangle being lit
    S_vector2 lit_uv;                   // 0x14, in its rectangle, from 0 to 1
    bool done;                          // 0x1c, lit_uv set and its lit vertex not added yet; colored (per vertex)
    ushort index;                       // 0x1e, the level's
    S_vector normal;                    // 0x20
};

// A level's face (PS2 FACE_STRUCT, 0x1c bytes on PC; the member names are guesses)
struct FACE_STRUCT {
    I3D_face_group *fg;                 // 0x00
    ushort fg_index;                    // 0x04
    ushort face_index;                  // 0x06, in the face group
    I3D_triface face;                   // 0x08
    bool free;                          // 0x0e, in no rectangle yet
    S_vector normal;                    // 0x10, in world space

    // The normal of the face p0, p1, p2: (p2 - p1) x (p0 - p1), the differences scaled up first, not normalized (the
    // name is a guess)
    void SetNormal(const S_vector &p0, const S_vector &p1, const S_vector &p2)
    {
        S_vector u = (p2 - p1) * 10000.0f;
        S_vector v = (p0 - p1) * 10000.0f;
        normal.x = u.y * v.z - u.z * v.y;
        normal.y = u.z * v.x - u.x * v.z;
        normal.z = u.x * v.y - u.y * v.x;
    }
};

// A face in a rectangle (PS2 NEW_FACE_STRUCT; the member names are guesses)
struct NEW_FACE_STRUCT {
    ushort face;                        // in the face group
    ushort rect;
};

// A lit vertex: a vertex in a rectangle and its coordinates there (the names are guesses)
struct LIT_VERTEX {
    ushort vertex;                      // 0x00
    ushort rect;                        // 0x02
    float u, v;                         // 0x04
};

// How far a texel's point outside the faces is moved into the nearest one, from the nearest edge (Insanity3D's)
#define INSIDE_PUSH_RATIO .0075f

// The blur's weights of a texel's neighbours
static const float blur_kernel[9] = {
    1.0f, 2.0f, 1.0f,
    2.0f, 4.0f, 2.0f,
    1.0f, 2.0f, 1.0f,
};

template<class T>
inline void T_swap(T &a, T &b)
{
    T t = a;
    a = b;
    b = t;
}

// Computes the active lighting: the lights of the object's sector and the scene's color on the faces, in lightmaps
// of resolution texels per unit, or per vertex (the lighting's mode). cb gets CBM_PROGRESS.
// For the lightmaps, each level's faces are grouped into rectangles: from a free face, every free face that shares an
// edge with one in the group and whose normal is within the lighting's angle of the first one's. The rectangle is the
// smallest one around the group's vertices projected onto the plane of their mean normal. Its texels are lit at the
// faces (or at the nearest one's nearest edge), the texels outside the faces filled from their neighbours, and the
// rectangle is blurred (flags bit 2) or becomes a single color.
// Not matching yet: the original's inline budget runs out late in the lightmaps' branch. From there it calls out of
// line what ours inlines: the lists' destructors at the branch's end (so the lists' addresses are taken, and VC6 reads
// their sizes again after calls and keeps the copy loops of their reserve), and in the colors' branch every helper (the
// list's constructor, GetRenderMatrix, push_back and operator[], the accessors, I3DFloatAsInt, GetMSCount). About 2000
// more expression operations anywhere in the function reproduce that (dead code works too: 600 statements like
// x = x * k + 1 give the original's calls; with fewer the budget grows, as the model in CLAUDE.md says, so the cap is
// the function's total size), but no source the original could have had is found yet. The frame is 0x10 bytes larger
// in the original, and the registers and slots differ all through.
// FUNCTION: LS3DF 0x10021bb0
LS3D_RESULT I3D_lit_object::Construct(I3D_scene *scene, S_vector &color, float resolution, uint flags,
    I3D_LOAD_CB_PROC *cb, void *context)
{
    Destruct(0);
    if (!mesh) {
        dbgPrintf("!! no mesh   I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
        return I3DERR_INVALIDPARAMS;
    }
    m_DisableSleeping = true;
    if (cb)
        cb(CBM_PROGRESS, I3DFloatAsInt(0.0f), 0, context);
    S_lit_level &level = levels[active_level];
    level.resolution = resolution;
    I3D_frame *frm = parent;
    while (frm->type != FRAME_SECTOR)
        frm = frm->parent;
    LightingSetSector((I3D_sector *)frm);
    int last_time = GetMSCount();
    float faces_done = 0.0f;
    float total = 0.0f;
    for (uint i = 0; i < mesh->num_lods; i++)
        total += (uint)mesh->levels[i]->NumFaces();
    float progress = 0.0f;
    uint count = (int)total;
    for (int lod_index = 0; lod_index < mesh->num_lods; lod_index++) {
        I3D_mesh_level *ml = mesh->levels[lod_index];
        litLod *l = level.lods[lod_index];
        if (!l) {
            l = new litLod;
            if (!l) {
                dbgPrintf("!! out of memory  I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return I3DERR_OUTOFMEM;
            }
        }
        if (level.mode & 2) {
            C_list<RECTANGLE_INFO> rects;
            C_list<NEW_FACE_STRUCT> lists[100];         // each face group's faces with their rectangles
            C_list<FACE_STRUCT> faces;
            C_list<VERTEX_STRUCT> verts;
            C_list<ushort> group_verts;
            C_list<LIT_VERTEX> lit_verts;
            S_matrix m = GetRenderMatrix();
            verts.reserve(ml->num_vertices);
            I3D_vertex *vtx = ml->LockVertices(1);
            for (uint i = 0; i < ml->num_vertices; i++) {
                VERTEX_STRUCT v;
                v.pos = vtx[i].pos * m;
                v.uv = S_vector2(0.0f, 0.0f);
                v.done = false;
                v.index = i;
                v.normal = vtx[i].normal.RotateByMatrix(m);
                v.normal.Normalize();
                verts.push_back(v);
            }
            ml->UnlockVertices();
            faces.reserve(ml->NumFaces());
            I3D_face_group *fg = ml->fgroups;
            for (uint fg_index = 0; fg_index < ml->num_fgroups; fg_index++, fg = fg->next) {
                if (fg->material && fg->material->flags & MATF_TWO_SIDED)
                    dbgPrintf("I3D_lit_object::Construct() - WARNING!! Obj:%s has TWO_SIDED material. LightMap can not "
                        "be correct !!", GetName());
                for (uint j = 0; j < fg->num_faces; j++) {
                    FACE_STRUCT f;
                    f.fg = fg;
                    f.fg_index = fg_index;
                    f.face_index = j;
                    f.face = fg->GetFaces()[j];
                    f.free = true;
                    S_vector p0 = verts[f.face.vi[0]].pos;
                    S_vector p1 = verts[f.face.vi[1]].pos;
                    S_vector p2 = verts[f.face.vi[2]].pos;
                    f.SetNormal(p0, p1, p2);
                    f.normal.Normalize();
                    if (f.normal.Square() < 1e-16f) {
                        dbgPrintf("I3D_lit_object::Construct() - Obj:%s => ZERO normal detected !", GetName());
                        f.normal = S_vector(0.0f, 0.0f, 1.0f);
                    }
                    faces.push_back(f);
                }
            }
            C_list<ushort> group;
            group.reserve(faces.size());
            for (;;) {
                S_vector normal;
                if (flags & 0x10) {
                    // all the faces in one rectangle, lit along color
                    group.reset();
                    if (count > 0) {
                        normal = color;
                        for (int i = 0; i < faces.size(); i++) {
                            group.push_back(i);
                            faces[i].free = false;
                            if (cb) {
                                count--;
                                cb(CBM_PROGRESS, MAKELONG(count, 1), 5, context);
                            }
                        }
                    }
                } else {
                    group.reset();
                    for (int i = 0; i < faces.size(); i++) {
                        if (faces[i].free) {
                            if (!group.size()) {
                                group.push_back(i);
                                normal = faces[i].normal;
                                faces[i].free = false;
                                if (cb) {
                                    count--;
                                    cb(CBM_PROGRESS, MAKELONG(count, 1), 5, context);
                                }
                            } else {
                                for (int j = 0; j < group.size(); j++) {
                                    FACE_STRUCT f = faces[group[j]];
                                    uchar shared = 0;
                                    for (int a = 0; a < 3; a++) {
                                        for (int b = 0; b < 3; b++) {
                                            if ((verts[f.face.vi[a]].pos - verts[faces[i].face.vi[b]].pos).Square()
                                                    <= 1e-6)
                                                shared++;
                                        }
                                    }
                                    if (shared > 1 && faces[group[0]].normal.AngleTo(faces[i].normal) <=
                                            level.angle * PI / 180.0) {
                                        group.push_back(i);
                                        normal += faces[i].normal;
                                        if (cb) {
                                            count--;
                                            cb(CBM_PROGRESS, MAKELONG(count, group.size()), 5, context);
                                        }
                                        faces[i].free = false;
                                        i = 0;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                if (group.size()) {
                    normal.Normalize();
                    // the group's vertices
                    group_verts.reset();
                    group_verts.reserve(group.size() * 3);
                    for (int j = group.size(); j--; ) {
                        ushort *vi = faces[group[j]].face.vi;
                        for (int k = 0; k < 3; k++) {
                            if (!group_verts.find(vi[k]))
                                group_verts.push_back(vi[k]);
                        }
                    }
                    // the smallest rectangle: each edge between two of the vertices projected onto the plane is tried
                    // as its side
                    float d = -normal.Dot(verts[group_verts[0]].pos);
                    float best = 1e16f;
                    S_matrix mat;
                    mat.Identity();
                    mat(2) = -normal;
                    I3D_bbox best_bb;
                    S_matrix best_mat;
                    for (int k = group_verts.size(); k--; ) {
                        if (cb)
                            cb(CBM_PROGRESS, MAKELONG(group_verts.size(), group_verts.size() - k), 6, context);
                        S_vector p0 = verts[group_verts[k]].pos - normal * (verts[group_verts[k]].pos.Dot(normal) + d);
                        for (int n = k; n--; ) {
                            S_vector p1 = verts[group_verts[n]].pos - normal *
                                (verts[group_verts[n]].pos.Dot(normal) + d);
                            S_vector dir = p1 - p0;
                            if (IsMrgZeroLess(dir.Square()))
                                continue;
                            mat(0) = dir;
                            mat(0).Normalize();
                            mat(1) = mat(0).Cross(mat(2));
                            S_matrix m_inv = mat;
                            m_inv.Inverse(S_matrix(m_inv));
                            S_matrix inv = m_inv;
                            I3D_bbox bb;
                            int vn = group_verts.size() - 1;
                            bb.min = bb.max = verts[group_verts[vn]].pos * inv;
                            while (vn--) {
                                S_vector v = verts[group_verts[vn]].pos * inv;
                                bb.Absorb(v);
                            }
                            float area = (bb.max.x - bb.min.x) * (bb.max.y - bb.min.y);
                            if (best > area) {
                                best = area;
                                best_bb = bb;
                                best_mat = mat;
                            }
                        }
                    }
                    // its texels, a border of half a texel around the vertices; lower levels have fewer of them
                    S_vector size = best_bb.max - best_bb.min;
                    S_matrix rm = best_mat;
                    float fx = (size.x * level.resolution + 1.0f) / (lod_index + 1);
                    float nx = max(4, min(256, (int)fx));
                    float fy = (size.y * level.resolution + 1.0f) / (lod_index + 1);
                    float ny = max(4, min(256, (int)fy));
                    float du = .5f * (lod_index + 1) / level.resolution * (1.0f + fx / nx);
                    float dv = .5f * (lod_index + 1) / level.resolution * (1.0f + fy / ny);
                    best_bb.min.x -= du;
                    best_bb.min.y -= dv;
                    best_bb.max.x += du;
                    best_bb.max.y += dv;
                    S_vector corner = best_bb.min.RotateByMatrix(rm);
                    size = best_bb.max - best_bb.min;
                    S_matrix sm;
                    sm.Identity();
                    sm(0, 0) = size.x;
                    sm(1, 1) = size.y;
                    rm = sm.RotateByMatrix(rm);
                    rm(3) = corner;
                    rm.Inverse(S_matrix(rm));
                    RECTANGLE_INFO rect;
                    memset(&rect, 0, sizeof(rect));
                    int isx = (int)((size.x * level.resolution + 1.0f) / (lod_index + 1));
                    rect.tex_sx = max(4, min(256, isx));
                    int isy = (int)((size.y * level.resolution + 1.0f) / (lod_index + 1));
                    rect.tex_sy = max(4, min(256, isy));
                    // the faces' vertices in the rectangle
                    for (j = 0; j < group.size(); j++) {
                        ushort *vi = faces[group[j]].face.vi;
                        for (int k = 3; k--; ) {
                            if (!verts[vi[k]].done) {
                                S_vector p = verts[vi[k]].pos * rm;
                                verts[vi[k]].lit_uv.x = p.x;
                                verts[vi[k]].lit_uv.y = p.y;
                                verts[vi[k]].done = true;
                            }
                        }
                    }
                    for (int vn = 0; vn < verts.size(); vn++) {
                        S_vector p = verts[vn].pos * rm;
                        verts[vn].uv.x = p.x * (rect.tex_sx - 1);
                        verts[vn].uv.y = p.y * (rect.tex_sy - 1);
                    }
                    rect.sx = rect.tex_sx;
                    rect.sy = rect.tex_sy;
                    int sx = rect.sx;
                    int sy = rect.sy;
                    int area = sx * sy;
                    uchar *data = new uchar[area * 3];
                    rect.data = data;
                    memset(data, 0xff, area * 3);
                    float aa = (float)renderState[RS_LM_AA_RATIO];
                    float offset = .5 / aa - .5;
                    float step = 1.0 / aa;
                    if (cb)
                        cb(CBM_PROGRESS, I3DFloatAsInt(progress), 1, context);
                    float pct = 100.0f / total;
                    float share = group.size() * pct;
                    if (flags & 0x20) {
                        // a test pattern: borders and a checkerboard
                        int odd = 0;
                        for (int y = 0; y < sy; y++) {
                            for (int x = 0; x < sx; x++) {
                                uchar r, g, b;
                                if (!x || x == sx - 1 || !y || y == sy - 1) {
                                    r = 0xff;
                                    g = 0;
                                    b = 0;
                                } else if (x == 1 || x == sx - 2 || y == 1 || y == sy - 2) {
                                    r = 0;
                                    g = 0xff;
                                    b = 0;
                                } else if (x == 2 || x == sx - 3 || y == 2 || y == sy - 3) {
                                    r = 0;
                                    g = 0;
                                    b = 0xff;
                                } else
                                    r = g = b = (x + odd) % 2 ? 0 : 0xff;
                                data[(y * sx + x) * 3] = r;
                                data[(y * sx + x) * 3 + 1] = g;
                                data[(y * sx + x) * 3 + 2] = b;
                            }
                            odd = !odd;
                        }
                        data[0] = 0xff;
                        data[1] = 0;
                        data[2] = 0xff;
                        data[3] = 0xff;
                        data[4] = 0;
                        data[5] = 0xff;
                        data[6] = 0xff;
                        data[7] = 0;
                        data[8] = 0xff;
                    } else {
                        S_vector2 pt(0.0f, 0.0f);
                        for (int y = 0; y < sy; y++) {
                            for (int x = 0; x < sx; x++) {
                                S_vector col(0.0f, 0.0f, 0.0f);
                                int num = 0;
                                for (int ay = 0; ay < aa; ay++) {
                                    for (int ax = 0; ax < aa; ax++) {
                                        pt.x = x + offset + ax * step;
                                        pt.y = y + offset + ay * step;
                                        // the face the sample is in, else the one with the nearest edge
                                        int found = -1;
                                        int nearest = -1;
                                        int nearest_edge = 0;
                                        float nearest_dist = 1e16f;
                                        for (int j = group.size(); j--; ) {
                                            const I3D_triface &fc = faces[group[j]].face;
                                            const S_vector2 *t[3];
                                            t[0] = &verts[fc.vi[0]].uv;
                                            t[1] = &verts[fc.vi[1]].uv;
                                            t[2] = &verts[fc.vi[2]].uv;
                                            bool inside = true;
                                            int edge = -1;
                                            float min_dist = 1e16f;
                                            float dist = 0.0f;
                                            int vi0 = 0;
                                            for (int e = 3; e--; ) {
                                                int vi1 = vi0;
                                                vi0 = e;
                                                const S_vector2 &p = *t[vi0];
                                                S_vector2 dir = *t[vi1] - p;
                                                float dd = pt.DistanceToLine(p, dir);
                                                if (dd < .001f) {
                                                    inside = false;
                                                    if (dd > -2.1f) {
                                                        dd = -dd;
                                                        if (min_dist > dd) {
                                                            min_dist = dd;
                                                            edge = e;
                                                            dist += dd;
                                                        }
                                                    } else {
                                                        edge = -1;
                                                        break;
                                                    }
                                                }
                                            }
                                            if (inside) {
                                                found = j;
                                                break;
                                            }
                                            if (edge != -1 && nearest_dist > dist) {
                                                nearest_dist = dist;
                                                nearest_edge = edge;
                                                nearest = j;
                                            }
                                        }
                                        int face = found;
                                        if (face == -1)
                                            face = nearest;
                                        if (face == -1)
                                            continue;
                                        // the point in the face: through the line from a vertex through it
                                        FACE_STRUCT &f = faces[group[face]];
                                        I3D_triface tri = f.face;
                                        I3D_material *mat = f.fg->material;
                                        const S_vector2 *t0 = &verts[tri.vi[0]].uv;
                                        const S_vector2 *t1 = &verts[tri.vi[1]].uv;
                                        const S_vector2 *t2 = &verts[tri.vi[2]].uv;
                                        S_vector2 edge_dir = *t2 - *t1;
                                        S_vector2 p_dir = pt - *t0;
                                        float u, v;
                                        if (!LineLineIntersect(S_vector(t0->x, t0->y, 0.0f),
                                                S_vector(p_dir.x, p_dir.y, 0.0f), S_vector(t1->x, t1->y, 0.0f),
                                                S_vector(edge_dir.x, edge_dir.y, 0.0f), u, v) || fabs(u) < MRG_ZERO) {
                                            // another vertex first
                                            T_swap(t0, t1);
                                            T_swap(t0, t2);
                                            T_swap(tri.vi[0], tri.vi[1]);
                                            T_swap(tri.vi[0], tri.vi[2]);
                                            edge_dir = *t2 - *t1;
                                            p_dir = pt - *t0;
                                            if (!LineLineIntersect(S_vector(t0->x, t0->y, 0.0f),
                                                    S_vector(p_dir.x, p_dir.y, 0.0f), S_vector(t1->x, t1->y, 0.0f),
                                                    S_vector(edge_dir.x, edge_dir.y, 0.0f), u, v) ||
                                                    fabs(u) < MRG_ZERO) {
                                                T_swap(t0, t1);
                                                T_swap(t0, t2);
                                                T_swap(tri.vi[0], tri.vi[1]);
                                                T_swap(tri.vi[0], tri.vi[2]);
                                                edge_dir = *t2 - *t1;
                                                p_dir = pt - *t0;
                                                if (!LineLineIntersect(S_vector(t0->x, t0->y, 0.0f),
                                                        S_vector(p_dir.x, p_dir.y, 0.0f), S_vector(t1->x, t1->y, 0.0f),
                                                        S_vector(edge_dir.x, edge_dir.y, 0.0f), u, v) ||
                                                        IsAbsMrgZero(u))
                                                    continue;
                                            }
                                        }
                                        const I3D_vertex &va = vtx[tri.vi[0]];
                                        const I3D_vertex &vb = vtx[tri.vi[1]];
                                        const I3D_vertex &vc = vtx[tri.vi[2]];
                                        S_vector a = va.pos;
                                        S_vector b = vb.pos;
                                        S_vector c = vc.pos;
                                        S_vector q = b + (c - b) * v;
                                        S_vector p = a + (q - a) / u;
                                        p *= m;
                                        S_vector na = va.normal;
                                        S_vector nb = vb.normal;
                                        S_vector nc = vc.normal;
                                        S_vector nq = nb * (1.0f - v) + nc * v;
                                        u = 1.0f / u;
                                        S_vector n = na * (1.0f - u) + nq * u;
                                        n = n.RotateByMatrix(m);
                                        n.Normalize();
                                        S_vector pos;
                                        if (found == -1) {
                                            // the nearest point of the edge, moved a little into the face
                                            I3D_triface tn = faces[group[nearest]].face;
                                            ushort ia = tn.vi[nearest_edge];
                                            ushort ib = tn.vi[(nearest_edge + 1) % 3];
                                            S_vector pa = vtx[ia].pos;
                                            S_vector pb = vtx[ib].pos;
                                            S_vector2 ta = verts[ia].uv;
                                            S_vector2 tb = verts[ib].uv;
                                            S_vector2 de = tb - ta;
                                            float sq = de.Dot(de);
                                            if (sq > MRG_ZERO) {
                                                float tt = de.Dot(pt - ta) / sq;
                                                tt = Max(INSIDE_PUSH_RATIO * 2.0f,
                                                    Min(1.0f - INSIDE_PUSH_RATIO * 2.0f, tt));
                                                pos = pa + (pb - pa) * tt;
                                                pos *= m;
                                                S_vector fn;
                                                fn.GetNormal(vtx[tn.vi[0]].pos, vtx[tn.vi[1]].pos, vtx[tn.vi[2]].pos);
                                                S_vector in;
                                                in.GetNormal(pa, pb, pa - fn);
                                                in = in.RotateByMatrix(m);
                                                float len = in.Magnitude();
                                                if (len > MRG_ZERO)
                                                    pos += in * INSIDE_PUSH_RATIO / len;
                                            } else
                                                pos = p;
                                        } else
                                            pos = p;
                                        S_vector out;
                                        bool outside = false;
                                        VertexLightingProcess(out, pos, n, mat, scene, flags, &outside, this,
                                            lod_index);
                                        if (!outside) {
                                            col += out;
                                            num++;
                                        }
                                    }
                                }
                                uchar r, g, b;
                                if (!num) {
                                    // no face here
                                    r = 0xff;
                                    g = 0;
                                    b = 0xff;
                                } else {
                                    r = FloatToInt(col.x * 127.5 / (float)num);
                                    g = FloatToInt(col.y * 127.5 / (float)num);
                                    b = FloatToInt(col.z * 127.5 / (float)num);
                                }
                                data[(y * sx + x) * 3] = r;
                                data[(y * sx + x) * 3 + 1] = g;
                                data[(y * sx + x) * 3 + 2] = b;
                                if (cb) {
                                    int time = GetMSCount();
                                    if (time - last_time >= 250) {
                                        progress = (pct * faces_done + (float)(y * sx) / area * share) / 100.0;
                                        cb(CBM_PROGRESS, I3DFloatAsInt(progress), 1, context);
                                        last_time = time;
                                    }
                                }
                            }
                        }
                        // the texels outside the faces (magenta) get their neighbours' mean, again and again
                        ushort w = rect.tex_sx + 2;
                        ushort h = rect.tex_sy + 2;
                        uchar *tmp = new uchar[w * h * 3];
                        if (!tmp) {
                            delete l;
                            delete[] tmp;
                            dbgPrintf("!! out of memory  I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                            return I3DERR_OUTOFMEM;
                        }
                        if (cb)
                            cb(CBM_PROGRESS, 0, 3, context);
                        uint filled;
                        do {
                            filled = 0;
                            for (y = 0; y < h; y++) {
                                for (int x = 0; x < w; x++) {
                                    uchar r, g, b;
                                    if (!x || x == sx + 1 || !y || y == sy + 1) {
                                        r = 0xff;
                                        g = 0;
                                        b = 0xff;
                                    } else {
                                        r = data[((y - 1) * sx + x - 1) * 3];
                                        g = data[((y - 1) * sx + x - 1) * 3 + 1];
                                        b = data[((y - 1) * sx + x - 1) * 3 + 2];
                                    }
                                    tmp[(y * w + x) * 3] = r;
                                    tmp[(y * w + x) * 3 + 1] = g;
                                    tmp[(y * w + x) * 3 + 2] = b;
                                }
                            }
                            for (y = 1; y < h - 1; y++) {
                                for (int x = 1; x < w - 1; x++) {
                                    uchar *t = &tmp[(y * w + x) * 3];
                                    if (t[0] == 0xff && !t[1] && t[2] == 0xff) {
                                        filled++;
                                        S_vector col(0.0f, 0.0f, 0.0f);
                                        float num = 0.0f;
                                        for (int dy = -1; dy < 2; dy++) {
                                            for (int dx = -1; dx < 2; dx++) {
                                                if (dy || dx) {
                                                    uchar *s = &tmp[((y + dy) * w + x + dx) * 3];
                                                    uchar r = s[0];
                                                    uchar g = s[1];
                                                    uchar b = s[2];
                                                    if (r != 0xff || g || b != 0xff) {
                                                        col.x += r;
                                                        col.y += g;
                                                        col.z += b;
                                                        num += 1.0f;
                                                    }
                                                }
                                            }
                                        }
                                        if (num != 0.0f) {
                                            col *= 1.0f / num;
                                            data[((y - 1) * sx + x - 1) * 3] = (uchar)col.x;
                                            data[((y - 1) * sx + x - 1) * 3 + 1] = (uchar)col.y;
                                            data[((y - 1) * sx + x - 1) * 3 + 2] = (uchar)col.z;
                                        }
                                    }
                                }
                            }
                        } while (filled > 0 && filled != area);
                        // a rectangle of one color becomes that color
                        uchar r0 = data[0];
                        uchar g0 = data[1];
                        uchar b0 = data[2];
                        bool single = true;
                        uchar *p = data;
                        for (y = 0; y < sy; y++) {
                            for (int x = 0; x < sx; x++) {
                                if (r0 != p[0] || g0 != p[1] || b0 != p[2]) {
                                    single = false;
                                    y = sy;
                                    break;
                                }
                                p += 3;
                            }
                        }
                        if (single) {
                            rect.tex_sx = 3;
                            rect.sx = 3;
                            rect.tex_sy = 3;
                            rect.sy = 3;
                            rect.area = 9;
                            delete[] data;
                            delete[] tmp;
                            if (r0 == 0xff && !g0 && b0 == 0xff)
                                rect.data = (uchar *)0xff000000;
                            else
                                rect.data = (uchar *)(0xff000000 | (r0 << 8 | g0) << 8 | b0);
                        } else {
                            if (flags & 4) {
                                // blurred, outside the faces too
                                if (cb)
                                    cb(CBM_PROGRESS, 0, 4, context);
                                for (y = 0; y < h; y++) {
                                    for (int x = 0; x < w; x++) {
                                        uchar r, g, b;
                                        if (!x || x == sx + 1 || !y || y == sy + 1) {
                                            r = 0xff;
                                            g = 0;
                                            b = 0xff;
                                        } else {
                                            r = data[((y - 1) * sx + x - 1) * 3];
                                            g = data[((y - 1) * sx + x - 1) * 3 + 1];
                                            b = data[((y - 1) * sx + x - 1) * 3 + 2];
                                        }
                                        tmp[(y * w + x) * 3] = r;
                                        tmp[(y * w + x) * 3 + 1] = g;
                                        tmp[(y * w + x) * 3 + 2] = b;
                                    }
                                }
                                for (y = 0; y < sy; y++) {
                                    for (int x = 0; x < sx; x++) {
                                        S_vector col(0.0f, 0.0f, 0.0f);
                                        ushort weight = 0;
                                        for (int dy = -1; dy < 2; dy++) {
                                            for (int dx = -1; dx < 2; dx++) {
                                                uchar *s = &tmp[((y + 1 + dy) * w + x + 1 + dx) * 3];
                                                uchar r = s[0];
                                                uchar g = s[1];
                                                uchar b = s[2];
                                                if (r != 0xff || g || b != 0xff) {
                                                    float k = blur_kernel[(dy + 1) * 3 + dx + 1];
                                                    col.x += r * k;
                                                    col.y += g * k;
                                                    col.z += b * k;
                                                    weight += k;
                                                }
                                            }
                                        }
                                        col *= 1.0f / weight;
                                        data[(y * sx + x) * 3] = min(0xff, (uchar)col.x);
                                        data[(y * sx + x) * 3 + 1] = min(0xff, (uchar)col.y);
                                        data[(y * sx + x) * 3 + 2] = min(0xff, (uchar)col.z);
                                    }
                                }
                            }
                            delete[] tmp;
                        }
                    }
                    rects.push_back(rect);
                    // the faces' rectangle, and their vertices' lit vertices in it
                    for (j = 0; j < group.size(); j++) {
                        NEW_FACE_STRUCT nf;
                        nf.face = faces[group[j]].face_index;
                        nf.rect = rects.size() - 1;
                        lists[faces[group[j]].fg_index].push_back(nf);
                        for (int k = 0; k < 3; k++) {
                            if (verts[faces[group[j]].face.vi[k]].done) {
                                LIT_VERTEX lv;
                                lv.vertex = verts[faces[group[j]].face.vi[k]].index;
                                lv.rect = rects.size() - 1;
                                lv.u = verts[faces[group[j]].face.vi[k]].lit_uv.x;
                                lv.v = verts[faces[group[j]].face.vi[k]].lit_uv.y;
                                lit_verts.push_back(lv);
                                verts[faces[group[j]].face.vi[k]].done = false;
                            }
                        }
                    }
                    faces_done += group.size();
                    continue;
                }
                // the lit vertices, each in the slot of its vertex where it is the first one's
                l->m_pLitUV = new I3D_text_coor[lit_verts.size()];
                if (!l->m_pLitUV) {
                    delete l;
                    dbgPrintf("!! out of memory  I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                    m_DisableSleeping = false;
                    ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                    return I3DERR_OUTOFMEM;
                }
                l->m_numLVertices = lit_verts.size();
                l->m_pVI = new VERTEX_INFO[lit_verts.size()];
                if (!l->m_pVI) {
                    delete l;
                    dbgPrintf("!! out of memory  I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                    ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                    return I3DERR_OUTOFMEM;
                }
                for (int n = 0; n < l->m_numLVertices; n++)
                    l->m_pLitUV[n].u = -1.0f;
                int last = l->m_numLVertices - 1;
                for (n = 0; n < l->m_numLVertices; n++) {
                    ushort vi = lit_verts[n].vertex;
                    if (l->m_pLitUV[vi].u == -1.0) {
                        l->m_pLitUV[vi].u = lit_verts[n].u;
                        l->m_pLitUV[vi].v = lit_verts[n].v;
                        l->m_pVI[vi].vertex = vi;
                        l->m_pVI[vi].rect = lit_verts[n].rect;
                    } else {
                        while (l->m_pLitUV[last].u != -1.0 && last >= 0)
                            last--;
                        l->m_pLitUV[last].u = lit_verts[n].u;
                        l->m_pLitUV[last].v = lit_verts[n].v;
                        l->m_pVI[last].vertex = vi;
                        l->m_pVI[last].rect = lit_verts[n].rect;
                        last--;
                    }
                }
                RECTANGLE_INFO *ri = new RECTANGLE_INFO[rects.size()];
                if (!ri) {
                    dbgPrintf("!! out of memory  I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                    delete l;
                    m_DisableSleeping = false;
                    ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                    return I3DERR_OUTOFMEM;
                }
                memset(ri, 0, rects.size() * sizeof(RECTANGLE_INFO));
                l->m_pRectangles = ri;
                l->m_numRectangles = rects.size();
                for (n = 0; n < rects.size(); n++) {
                    ri->data = rects[n].data;
                    ri->sx = rects[n].sx;
                    ri->sy = rects[n].sy;
                    ri->tex_sx = rects[n].tex_sx;
                    ri->tex_sy = rects[n].tex_sy;
                    ri->area = ri->tex_sx * ri->tex_sy;
                    ri++;
                }
                if (!l->SetNumFGI(ml->num_fgroups)) {
                    dbgPrintf("!! out of memory  I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                    delete l;
                    m_DisableSleeping = false;
                    ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                    return I3DERR_OUTOFMEM;
                }
                fg = ml->fgroups;
                C_list<NEW_FACE_STRUCT> *list = lists;
                for (int k = 0; k < ml->num_fgroups; k++, fg = fg->next, list++) {
                    ushort *fg_rects = new ushort[fg->num_faces];
                    if (!fg_rects) {
                        dbgPrintf("!! out of memory   I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                        delete l;
                        m_DisableSleeping = false;
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return I3DERR_OUTOFMEM;
                    }
                    for (uint j = 0; j < fg->num_faces; j++)
                        fg_rects[(*list)[j].face] = (*list)[j].rect;
                    l->AddFGindex(fg_rects, k);
                }
                if (!l->postProcess(mesh->levels[lod_index], false)) {
                    delete l;
                    dbgPrintf("!! GENERIC  I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                    m_DisableSleeping = false;
                    return I3DERR_GENERIC;
                }
                break;
            }
        } else {
            // colors per vertex
            l->m_numLVertices = ml->num_vertices;
            l->m_pColors = new uint[ml->num_vertices];
            if (!l->m_pColors) {
                dbgPrintf("!! out of memory   I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                delete l;
                m_DisableSleeping = false;
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return I3DERR_OUTOFMEM;
            }
            C_list<VERTEX_STRUCT> verts;
            S_matrix m = GetRenderMatrix();
            I3D_vertex *vtx = ml->LockVertices(1);
            for (uint i = 0; i < ml->num_vertices; i++) {
                VERTEX_STRUCT v;
                v.pos = vtx[i].pos * m;
                v.done = false;
                v.index = i;
                v.normal = vtx[i].normal.RotateByMatrix(m);
                v.normal.Normalize();
                verts.push_back(v);
            }
            ml->UnlockVertices();
            I3D_face_group *fg = ml->fgroups;
            int num = 0;
            for (uint fg_index = 0; fg_index < ml->num_fgroups; fg_index++) {
                uint alpha = 0xff000000;
                if (fg->GetMaterial())
                    alpha = FloatToInt(fg->GetMaterial()->GetAlpha() * 255.0f) << 24;
                for (uint j = 0; j < fg->NumFaces(); j++) {
                    num++;
                    for (int k = 3; k--; ) {
                        ushort &vi = fg->GetFaces()[j].vi[k];
                        if (!verts[vi].done) {
                            S_vector col;
                            VertexLightingProcess(col, verts[vi].pos, verts[vi].normal, fg->GetMaterial(), scene,
                                flags, NULL, this, lod_index);
                            l->m_pColors[vi] = (FloatToInt(col.x * 127.5) << 8 | FloatToInt(col.y * 127.5)) << 8 |
                                alpha | FloatToInt(col.z * 127.5);
                            verts[vi].done = true;
                        }
                        if (cb) {
                            int time = GetMSCount();
                            if (time - last_time >= 250) {
                                progress = 100.0f / total * num / 100.0f;
                                cb(CBM_PROGRESS, I3DFloatAsInt(progress), 1, context);
                                last_time = time;
                            }
                        }
                    }
                }
                fg = fg->GetNext();
            }
        }
        level.lods[lod_index] = l;
    }
    level.constructed = true;
    m_DisableSleeping = false;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10026930
LS3D_RESULT I3D_lit_object::Destruct(uint flags)
{
    levels[active_level].constructed = false;
    S_lit_level &level = levels[active_level];
    for (int i = 0; i < 10; i++) {
        delete level.lods[i];
        level.lods[i] = NULL;
    }
    return I3D_OK;
}

// The header of a lighting in the file
#pragma pack(push, 1)
struct S_lm_header {
    uchar version;                      // 0x21
    uchar mode;                         // S_lit_level's
    int num_lods;                       // the mesh's
    float resolution;
    float angle;
    uchar level;                        // the lighting, from 1
};
#pragma pack(pop)

// The lightings saved in the scene file: a byte with a bit set for each, then each's header and lods
// Not matching yet: the original's maps' loop has all four callee-saved registers busy, so it computes sx * sy * 3
// again after each call and keeps dtaRead's result in a slot (its frame is 4 bytes larger); and it places the shared
// epilogue last, after the error blocks, where ours puts it after the lightings' loop's guard.
// FUNCTION: LS3DF 0x10026980
LS3D_RESULT I3D_lit_object::Load(int handle)
{
    Destruct(0);
    if (!mesh) {
        dbgPrintf("!! no mesh   I3D_lit_object::Load()  this:0x%x %s", this, GetName());
        return I3DERR_INVALIDPARAMS;
    }
    uchar num_levels;
    dtaRead(handle, &num_levels, 1);
    if (num_levels) {
        uchar n = 0;
        int bit = 1;
        for (int i = 0; i < 8; i++, bit <<= 1) {
            if (num_levels & bit)
                n++;
        }
        num_levels = n;
        for (int k = 0; k < num_levels; k++) {
            S_lm_header hdr;
            if (dtaRead(handle, &hdr, sizeof(hdr)) != sizeof(hdr)) {
                dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x", this,
                    GetName(), handle);
                return I3DERR_FILECORRUPTED;
            }
            if (hdr.level > 6)
                hdr.level = 1;
            S_lit_level &lev = levels[hdr.level - 1];
            lev.mode = hdr.mode;
            lev.resolution = hdr.resolution;
            lev.angle = hdr.angle;
            if (hdr.version != 0x21) {
                dbgPrintf("!! bad lightmap version:%d   I3D_lit_object::Load()  this:0x%x %s", hdr.version, this,
                    GetName());
                return I3DERR_INVALIDPARAMS;
            }
            if (hdr.num_lods != mesh->num_lods) {
                dbgPrintf("!! bad numLODs  I3D_lit_object::Load()  this:0x%x %s", this, GetName());
                return I3DERR_INVALIDPARAMS;
            }
            for (int j = 0; j < hdr.num_lods; j++) {
                I3D_mesh_level *level = mesh->levels[j];
                ushort lod_flags;
                if (dtaRead(handle, &lod_flags, 2) != 2) {
                    dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x", this,
                        GetName(), handle);
                    return I3DERR_FILECORRUPTED;
                }
                litLod *l = lev.lods[j];
                if (!l) {
                    l = new litLod;
                    if (!l) {
                        dbgPrintf("!! out of memory  I3D_lit_object::Load()  this:0x%x %s", this, GetName());
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return I3DERR_OUTOFMEM;
                    }
                }
                if (hdr.mode & 2) {
                    struct {
                        ushort num_rects;
                        ushort num_fgroups;
                    } counts;
                    if (dtaRead(handle, &counts, 4) != 4) {
                        dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x", this,
                            GetName(), handle);
                        delete l;
                        return I3DERR_FILECORRUPTED;
                    }
                    if (counts.num_fgroups != level->num_fgroups) {
                        dbgPrintf("!! bad num_face_groups:%d  mesh num_face_groups:%d  I3D_lit_object::Load()  "
                            "this:0x%x %s", counts.num_fgroups, level->num_fgroups, this, GetName());
                        delete l;
                        return I3DERR_INVALIDPARAMS;
                    }
                    RECTANGLE_INFO *ri = (RECTANGLE_INFO *)new uchar[counts.num_rects * sizeof(RECTANGLE_INFO)];
                    if (!ri) {
                        dbgPrintf("!! out of memory  I3D_lit_object::Load()  this:0x%x %s", this, GetName());
                        delete l;
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return I3DERR_OUTOFMEM;
                    }
                    memset(ri, 0, counts.num_rects * sizeof(RECTANGLE_INFO));
                    l->m_pRectangles = ri;
                    l->m_numRectangles = counts.num_rects;
                    for (int r = 0; r < counts.num_rects; r++, ri++) {
                        uchar single;
                        if (dtaRead(handle, &single, 1) != 1) {
                            dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x",
                                this, GetName(), handle);
                            delete l;
                            return I3DERR_FILECORRUPTED;
                        }
                        if (single) {
                            ri->tex_sx = 3;
                            ri->sx = 3;
                            ri->tex_sy = 3;
                            ri->sy = 3;
                            ri->area = 9;
                            if (dtaRead(handle, &ri->data, 4) != 4) {
                                dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  "
                                    "handle:0x%x", this, GetName(), handle);
                                delete l;
                                return I3DERR_FILECORRUPTED;
                            }
                        }
                        uint num_maps;
                        if (dtaRead(handle, &num_maps, 4) != 4) {
                            dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x",
                                this, GetName(), handle);
                            delete l;
                            return I3DERR_FILECORRUPTED;
                        }
                        for (uint m = 0; m < num_maps; m++) {
                            int sx, sy;
                            if (dtaRead(handle, &sx, 4) != 4) {
                                dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  "
                                    "handle:0x%x", this, GetName(), handle);
                                delete l;
                                return I3DERR_FILECORRUPTED;
                            }
                            if (dtaRead(handle, &sy, 4) != 4) {
                                dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  "
                                    "handle:0x%x", this, GetName(), handle);
                                delete l;
                                return I3DERR_FILECORRUPTED;
                            }
                            if (!single) {
                                uchar *data = new uchar[sx * sy * 3];
                                if (!data) {
                                    delete l;
                                    return I3DERR_OUTOFMEM;
                                }
                                if (dtaRead(handle, data, sx * sy * 3) != (uint)(sx * sy * 3)) {
                                    delete[] data;
                                    dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  "
                                        "handle:0x%x", this, GetName(), handle);
                                    delete l;
                                    return I3DERR_FILECORRUPTED;
                                }
                                if (!m) {
                                    if (sx == 3 && sy == sx) {
                                        dbgPrintf("!! LM size 1x1   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x",
                                            this, GetName(), handle);
                                        delete l;
                                        return I3DERR_FILECORRUPTED;
                                    }
                                    ri->data = data;
                                    ri->tex_sx = sx;
                                    ri->sx = sx;
                                    ri->tex_sy = sy;
                                    ri->sy = sy;
                                    ri->area = sx * sy;
                                } else
                                    delete[] data;
                            }
                        }
                    }
                    int num_lverts;
                    if (!dtaRead(handle, &num_lverts, 4)) {
                        dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x", this,
                            GetName(), handle);
                        delete l;
                        return I3DERR_FILECORRUPTED;
                    }
                    l->m_numLVertices = num_lverts;
                    l->m_pLitUV = new I3D_text_coor[num_lverts];
                    if (!l->m_pLitUV) {
                        delete l;
                        return I3DERR_OUTOFMEM;
                    }
                    if (!dtaRead(handle, l->m_pLitUV, l->m_numLVertices * sizeof(I3D_text_coor))) {
                        dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x", this,
                            GetName(), handle);
                        delete l;
                        return I3DERR_FILECORRUPTED;
                    }
                    l->m_pVI = new VERTEX_INFO[num_lverts];
                    if (!l->m_pVI) {
                        delete l;
                        return I3DERR_OUTOFMEM;
                    }
                    if (!dtaRead(handle, l->m_pVI, l->m_numLVertices * sizeof(VERTEX_INFO))) {
                        dbgPrintf("!! error reading data   I3D_lit_object::load()  this:0x%x %s  handle:0x%x", this,
                            GetName(), handle);
                        delete l;
                        return I3DERR_FILECORRUPTED;
                    }
                    int num_indices;
                    if (!dtaRead(handle, &num_indices, 4)) {
                        dbgPrintf("!! Error reading data   I3D_lit_object::load()  this:0x%x %s  handle:0x%x", this,
                            GetName(), handle);
                        delete l;
                        return I3DERR_FILECORRUPTED;
                    }
                    if (level->NumFaces() * 3 != num_indices) {
                        dbgPrintf("!! Error detected [ ( pLevel->FaceCount * 3 ) != FaceNum ]   "
                            "I3D_lit_object::load()  this:0x%x %s  handle:0x%x", this, GetName(), handle);
                        delete l;
                        return I3DERR_FILECORRUPTED;
                    }
                    if (!cIBMan.m_Alloc(D3DUSAGE_SOFTWAREPROCESSING, num_indices, l->ib)) {
                        dbgPrintf("!! Out of memory  I3D_litobject::Load()");
                        delete l;
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return I3DERR_OUTOFMEM;
                    }
                    ushort *indx = (ushort *)l->ib.buffer->Lock(l->ib.base, 0);
                    if (!dtaRead(handle, indx, num_indices * sizeof(ushort))) {
                        dbgPrintf("!! Error reading data   I3D_lit_object::read()  this:0x%x %s  handle:0x%x", this,
                            GetName(), handle);
                        delete l;
                        return I3DERR_FILECORRUPTED;
                    }
                    if (!l->SetNumFGI(counts.num_fgroups)) {
                        dbgPrintf("!! Out of memory  I3D_lit_object::Load()  this:0x%x %s", this, GetName());
                        delete l;
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return I3DERR_OUTOFMEM;
                    }
                    I3D_face_group *fg = level->fgroups;
                    for (int f = 0; f < counts.num_fgroups; f++) {
                        int num_faces;
                        if (dtaRead(handle, &num_faces, 4) != 4) {
                            dbgPrintf("!! Error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x",
                                this, GetName(), handle);
                            delete l;
                            return I3DERR_FILECORRUPTED;
                        }
                        if (num_faces != fg->num_faces) {
                            dbgPrintf("!! Different  num_faces:%d   mesh num_faces:%d   I3D_lit_object::Load() %s",
                                num_faces, fg->num_faces, GetName());
                            delete l;
                            return I3DERR_FILECORRUPTED;
                        }
                        ushort *rects = new ushort[num_faces];
                        if (!rects) {
                            dbgPrintf("!! out of memory   I3D_lit_object::Load()  this:0x%x %s", this, GetName());
                            delete l;
                            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                            return I3DERR_OUTOFMEM;
                        }
                        if (dtaRead(handle, rects, num_faces * sizeof(ushort)) != num_faces * sizeof(ushort)) {
                            dbgPrintf("!! error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x",
                                this, GetName(), handle);
                            delete l;
                            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                            return I3DERR_FILECORRUPTED;
                        }
                        l->AddFGindex(rects, f);
                        fg = fg->next;
                    }
                    if (!l->postProcess(mesh->levels[j], true)) {
                        dbgPrintf("!! lightmap can't be loaded  %s", GetName());
                        delete l;
                        return I3DERR_GENERIC;
                    }
                } else {
                    if (lev.mode & 4) {
                        dtaRead(handle, &l->m_numLVertices, 4);
                        if (l->m_numLVertices != level->num_vertices) {
                            dbgPrintf("!! Different  num_verts:%d   mesh num_verts:%d   I3D_lit_object::Load()  %s",
                                l->m_numLVertices, level->num_vertices, GetName());
                            delete l;
                            return I3DERR_FILECORRUPTED;
                        }
                    } else
                        l->m_numLVertices = level->num_vertices;
                    l->m_pColors = new uint[l->m_numLVertices];
                    if (!l->m_pColors) {
                        dbgPrintf("!! Out of memory   I3D_lit_object::Construct()  this:0x%x %s", this, GetName());
                        delete l;
                        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                        return I3DERR_OUTOFMEM;
                    }
                    if (dtaRead(handle, l->m_pColors, l->m_numLVertices * sizeof(uint)) !=
                            l->m_numLVertices * sizeof(uint)) {
                        dbgPrintf("!! Error reading data   I3D_lit_object::Load()  this:0x%x %s  handle:0x%x", this,
                            GetName(), handle);
                        delete l;
                        return I3DERR_FILECORRUPTED;
                    }
                }
                lev.lods[j] = l;
            }
            lev.constructed = true;
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10027580
LS3D_RESULT I3D_lit_object::Save(int handle)
{
    dbgPrintf("NEMUZETE UKLADAT LM, KDYZ JE #define LS3D_FINAL !!!!!!!");
    return I3DERR_GENERIC;
}

// FUNCTION: LS3DF 0x100275a0
LS3D_RESULT I3D_lit_object::SetMode(uint mode)
{
    Destruct(0);
    levels[active_level].mode = mode;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100275d0
LS3D_RESULT I3D_lit_object::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    Destruct(0);
    I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_VISUAL) {
        if (frame->GetType() == FRAME_EMITOR)
            SetMesh(((I3D_emitor *)frame)->mesh, true);
        return I3D_OK;
    }
    I3D_lit_object *obj = (I3D_lit_object *)frame;
    switch (obj->visual_type) {
    case I3D_VISUAL_LIT_OBJECT: {
        for (int i = 0; i < 6; i++) {
            levels[i].mode = obj->levels[i].mode;
            levels[i].resolution = obj->levels[i].resolution;
            levels[i].angle = obj->levels[i].angle;
        }
    }
    case I3D_VISUAL_OBJECT:
    case I3D_VISUAL_BILLBOARD:
    case I3D_VISUAL_SINGLEMESH:
    case I3D_VISUAL_LENSF:
    case I3D_VISUAL_MORPH:
    case I3D_VISUAL_SINGLEMORPH:
        break;
    default:
        return I3DERR_GENERIC;
    }
    SetMesh(obj->mesh, true);
    lightness = obj->lightness;
    transparency = obj->transparency;
    vis_flags = obj->vis_flags & 0x2ffd0;
    vis_bound = obj->vis_bound;
    if (obj->vis_flags & VISF_WORLD_SPACE)
        vis_bound_w = obj->GetWBoundVolume();
    else
        vis_flags &= ~VISF_WBOUND_VALID;
    return I3D_OK;
}

// keep_lods keeps the lightings
// FUNCTION: LS3DF 0x10027760
void I3D_lit_object::SetMesh(I3D_mesh_object *m, bool keep_lods)
{
    if (m == mesh) {
        if (!m)
            return;
    } else {
        if (!keep_lods)
            Destruct(0);
        if (mesh)
            mesh->Release();
        mesh = m;
        if (!m) {
            vis_bound.bbox.Invalidate();
            vis_bound.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
            vis_bound.bsphere.radius = 0.0f;
            return;
        }
        m->AddRef();
    }
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_bound = m->bvol;
}

// The lightmaps' memory, of all the lightings
// FUNCTION: LS3DF 0x10027830
uint I3D_lit_object::GetTotalLMMemory()
{
    uint total = 0;
    if (mesh) {
        for (int i = 0; i < 6; i++) {
            for (uint j = 0; j < mesh->num_lods; j++) {
                if (levels[i].lods[j])
                    total += levels[i].lods[j]->mem;
            }
        }
    }
    return total;
}

// Queues the face groups of the level of detail the distance picks, after bringing the active lighting's lod up to
// date (PS2's renderLitObject() in the messages)
// FUNCTION: LS3DF 0x10027880
void I3D_lit_object::m_AddFGroups()
{
    if (!mesh)
        return;
    if (vis_flags & VISF_PROXY) {
        I3D_visual *vis = (I3D_visual *)user_data;
        I3D_frame *frm = vis->parent;
        while (true) {
            if (!frm) {
                AddFGHlpStruct.sector = pCurrentScene->primary_sector;
                break;
            }
            if (frm->type == FRAME_SECTOR) {
                AddFGHlpStruct.sector = (I3D_sector *)frm;
                break;
            }
            frm = frm->parent;
        }
        int first = AddFGHlpStruct.fgroups.size();
        vis->m_AddFGroups();
        for (int i = first; i < AddFGHlpStruct.fgroups.size(); i++)
            AddFGHlpStruct.fgroups[i].vis = this;
        return;
    }
    const I3D_bvolume &bvol = GetWBoundVolume();
    float dist_2 = (bvol.bsphere.pos - AddFGHlpStruct.cam_pos).Square();
    int lod_index = GetObjectLOD(this, dist_2);
    I3D_mesh_level *level = mesh->levels[lod_index];
    if (!level || !level->fgroups)
        return;
    int num_verts = level->num_vertices;
    if (!num_verts)
        return;
    S_lit_level &lit = levels[active_level];
    litLod *l = lit.lods[lod_index];
    if (!l) {
        l = new litLod;
        if (!l) {
            dbgPrintf("!! Out of memory    renderLitObject()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return;
        }
        lit.lods[lod_index] = l;
    }
    if (l->dirty || !l->vb.vb) {
        if (!l->Create(level, mesh, lit.mode)) {
            dbgPrintf("!! Out of memory    renderLitObject()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return;
        }
        l->lights_changed = (uint)-1;
        l->light_key = (uint)-1;
    }
    bool moved;
    if (l->update_count != update_count) {
        moved = true;
        l->update_count = update_count;
    } else
        moved = false;
    uchar *lv = NULL;
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, 0);
    int base = l->vb.base;
    IDirect3DVertexBuffer8 *dest = l->vb.vb;
    RENDER_FG_HELP_STRUCT h;
    h.clip = frm_flags & FRMFLAGS_CLIP;
    h.sector = AddFGHlpStruct.sector;
    h.vis = this;

    if (!lit.constructed) {
        // Not lit yet: the level's vertices with two pairs of texture coordinates, which m_RenderFGroup renders in
        // the texture factor's color
        if (!IsAbsMrgZero(mesh->u_ofs - l->u_ofs) || !IsAbsMrgZero(mesh->v_ofs - l->v_ofs))
            l->SetUV(2, level->num_vertices, &verts->tex, sizeof(I3D_vertex), mesh->u_ofs, mesh->v_ofs);
        if ((renderCameraChanged || render_number != RenderNumberMinus1 || moved) &&
                !(m_HardwareCaps & HWCAPS_HWTL)) {
            if (frm_flags & FRMFLAGS_CLIP) {
                m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
                level->vb.buffer->Unlock();
                SetVertexFormat(VF_VERTEX);
                SetClipping(TRUE);
                m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
            } else {
                m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
                level->vb.buffer->Unlock();
                SetVertexFormat(VF_VERTEX);
                SetClipping(FALSE);
                m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
            }
            if (!(m_HardwareCaps & HWCAPS_TABLEFOG)) {
                if (!lv)
                    lv = l->vb.buffer->Lock(base, 0);
                FogProcess((uchar *)&((TLVERTEX2 *)lv)->specular + 3, &((TLVERTEX2 *)lv)->z, sizeof(TLVERTEX2),
                    num_verts, AddFGHlpStruct.sector->fog_light, 0);
            }
        }
        for (I3D_face_group *fg = level->fgroups; fg; fg = fg->next) {
            I3D_material *mat = fg->material;
            h.fg = fg;
            if (!mat) {
                h.texture = NULL;
                h.sort_value = 0;
                AddFGHlpStruct.num_opaque++;
            } else if (!(mat->flags & MATF_TEXTURE)) {
                h.texture = NULL;
                h.sort_value = 0;
                AddFGHlpStruct.num_opaque++;
            } else {
                h.texture = mat->GetTexture();
                if (mat->flags & (MATF_TXT_ALPHA | MATF_ADD_MODE)) {
                    h.sort_value = (FloatToInt(sort_dist) & 0x3fffffff) | 0x80000000;
                    AddFGHlpStruct.num_transparent++;
                } else {
                    if (mat->flags & MATF_CKEY_ALPHA)
                        h.sort_value = ((h.texture ? h.texture->uid : 0) & 0x7fff) | 0x40000000;
                    else
                        h.sort_value = (h.texture ? h.texture->uid : 0) & 0x7fff;
                    AddFGHlpStruct.num_opaque++;
                }
            }
            AddFGHlpStruct.fgroups.push_back(h);
        }
        return;
    }

    num_verts = l->m_numLVertices;
    ushort lm_uid;
    if (lit.mode & 1) {
        // Colors per vertex, which Create copied into the vertices
        h.env_texture = NULL;
        if (!IsAbsMrgZero(mesh->u_ofs - l->u_ofs) || !IsAbsMrgZero(mesh->v_ofs - l->v_ofs))
            l->SetUV(1, level->num_vertices, &verts->tex, sizeof(I3D_vertex), mesh->u_ofs, mesh->v_ofs);
        if ((renderCameraChanged || render_number != RenderNumberMinus1 || moved) &&
                !(m_HardwareCaps & HWCAPS_HWTL)) {
            if (frm_flags & FRMFLAGS_CLIP) {
                m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
                level->vb.buffer->Unlock();
                SetVertexFormat(VF_VERTEX);
                SetClipping(TRUE);
                m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
            } else {
                m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
                level->vb.buffer->Unlock();
                SetVertexFormat(VF_VERTEX);
                SetClipping(FALSE);
                m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
            }
            if (!(m_HardwareCaps & HWCAPS_TABLEFOG)) {
                if (!lv)
                    lv = l->vb.buffer->Lock(base, 0);
                FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z, sizeof(TLVERTEX),
                    num_verts, AddFGHlpStruct.sector->fog_light, 0);
            }
        }
        lm_uid = 0;
    } else {
        // Lightmaps: the lit vertices, lit by the dynamic lights (PrepareLights' colors of the level's vertices),
        // transformed from their positions' buffer without hardware T&L
        if (!IsAbsMrgZero(mesh->u_ofs - l->u_ofs) || !IsAbsMrgZero(mesh->v_ofs - l->v_ofs))
            l->SetUV(2, num_verts, l->m_pUV, sizeof(I3D_text_coor), mesh->u_ofs, mesh->v_ofs);
        int fog = 0;
        if (moved || l->lights_changed != AddFGHlpStruct.sector->lights_changed) {
            l->lights_changed = AddFGHlpStruct.sector->lights_changed;
            LightingSetSector(AddFGHlpStruct.sector);
            if (!(m_HardwareCaps & (HWCAPS_HWTL | HWCAPS_SINGLEPASS)))
                fog = 1;
            if (!lv)
                lv = l->vb.buffer->Lock(base, 0);
            static C_list<uint> colors;
            colors.reserve(level->num_vertices);
            lods[cur_lod] = lit.lods[cur_lod];
            if (PrepareLights((uchar *)&colors[0], sizeof(uint), level, (const uchar *)verts, sizeof(I3D_vertex),
                    this, GetInvMatrix(), 0x10, moved, NULL)) {
                if (m_HardwareCaps & HWCAPS_HWTL) {
                    LVERTEX2 *v = (LVERTEX2 *)lv;
                    for (int i = 0; i < num_verts; i++) {
                        v->color = colors[l->m_pVI[i].vertex];
                        v++;
                    }
                } else {
                    TLVERTEX2 *v = (TLVERTEX2 *)lv;
                    for (int i = 0; i < num_verts; i++) {
                        v->color = colors[l->m_pVI[i].vertex];
                        v++;
                    }
                }
            }
            lods[cur_lod] = NULL;
        }
        if ((renderCameraChanged || render_number != RenderNumberMinus1 || moved) &&
                !(m_HardwareCaps & HWCAPS_HWTL)) {
            fog = 1;
            if (frm_flags & FRMFLAGS_CLIP) {
                m_pD3DDevice8->SetStreamSource(0, l->pos_vb.vb, l->pos_vb.buffer->stride);
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
                l->pos_vb.buffer->Unlock();
                SetVertexFormat(VF_VERTEX);
                SetClipping(TRUE);
                m_pD3DDevice8->ProcessVertices(l->pos_vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
            } else {
                m_pD3DDevice8->SetStreamSource(0, l->pos_vb.vb, l->pos_vb.buffer->stride);
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
                l->pos_vb.buffer->Unlock();
                SetVertexFormat(VF_VERTEX);
                SetClipping(FALSE);
                m_pD3DDevice8->ProcessVertices(l->pos_vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
            }
        }
        if (fog && !(m_HardwareCaps & HWCAPS_TABLEFOG)) {
            if (!lv)
                lv = l->vb.buffer->Lock(base, 0);
            if (m_HardwareCaps & HWCAPS_SINGLEPASS)
                FogProcess((uchar *)&((TLVERTEX2 *)lv)->specular + 3, &((TLVERTEX2 *)lv)->z, sizeof(TLVERTEX2),
                    num_verts, AddFGHlpStruct.sector->fog_light, 0);
            else
                FogProcess((uchar *)&((TLVERTEX2 *)lv)->color + 3, &((TLVERTEX2 *)lv)->z, sizeof(TLVERTEX2),
                    num_verts, AddFGHlpStruct.sector->fog_light, 1);
        }
        h.env_texture = l->m_pLM->texture;
        lm_uid = h.env_texture->uid;
    }
    // The opaque face groups sort by the lightmap's texture, then by their own
    int first = 0;
    for (I3D_face_group *fg = level->fgroups; fg; fg = fg->next) {
        I3D_material *mat = fg->material;
        h.fg = fg;
        h.first = first;
        if (!mat) {
            h.texture = NULL;
            h.sort_value = 0;
            AddFGHlpStruct.num_opaque++;
        } else if (!(mat->flags & MATF_TEXTURE)) {
            h.texture = NULL;
            h.sort_value = 0;
            AddFGHlpStruct.num_opaque++;
        } else {
            h.texture = mat->GetTexture();
            if (mat->flags & (MATF_TXT_ALPHA | MATF_ADD_MODE)) {
                h.sort_value = (FloatToInt(sort_dist) & 0x3fffffff) | 0x80000000;
                AddFGHlpStruct.num_transparent++;
            } else {
                if (mat->flags & MATF_CKEY_ALPHA)
                    h.sort_value = (lm_uid | 0x8000) << 15 | (h.texture ? h.texture->uid : 0) & 0x7fff;
                else
                    h.sort_value = (lm_uid & 0x7fff) << 15 | (h.texture ? h.texture->uid : 0) & 0x7fff;
                AddFGHlpStruct.num_opaque++;
            }
        }
        AddFGHlpStruct.fgroups.push_back(h);
        first += fg->num_faces * 3;
    }
}

// Renders a face group m_AddFGroups queued: not lit yet in the texture factor's color, with its colors per vertex, or
// with its lightmap (in one pass, the lightmap in the first stage and the texture in the second, or without
// HWCAPS_SINGLEPASS in two, and a fog pass)
// Not matching yet: the original calls SetColorOp1(m_Modulate2X) of the single pass out of line (a little less inline
// budget there), and its draws read help->first before l->ib.base (the allocator's order; the sum spelled either way
// compiles the same).
// FUNCTION: LS3DF 0x10028d40
void I3D_lit_object::m_RenderFGroup(RENDER_FG_HELP_STRUCT *help)
{
    S_lit_level *lit;
    int lod_index;
    if (vis_flags & VISF_PROXY) {
        I3D_lit_object *obj = (I3D_lit_object *)user_data;
        lit = &obj->levels[obj->active_level];
        lod_index = obj->cur_lod;
    } else {
        lod_index = cur_lod;
        lit = &levels[active_level];
    }
    litLod *l = lit->lods[lod_index];
    int num_verts = mesh->levels[lod_index]->num_vertices;
    I3D_face_group *fg = help->fg;
    I3D_material *mat = fg->material;
    IDirect3DVertexBuffer8 *vb = l->vb.vb;
    uint reset = 0;
    SetAlphaOp(D3DTOP_SELECTARG1);
    if (!lit->constructed) {
        m_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, 0xff00ffff);
        SetColorOp1(D3DTOP_DISABLE);
        SetColorOp(D3DTOP_MODULATE);
        if (!mat) {
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
            SetColorOp(D3DTOP_SELECTARG1);
            SetAlphaState(0);
            m_pD3DDevice8->SetTexture(0, NULL);
            reset |= 0x10;
        } else {
            if (mat->flags & MATF_ADDRESS_U_CLAMP) {
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
                reset |= 2;
            }
            if (mat->flags & MATF_ADDRESS_V_CLAMP) {
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
                reset |= 4;
            }
            if (mat->flags & MATF_ADD_MODE) {
                SetAlphaState(1);
                SetBlendState(1);
            } else if (mat->flags & MATF_TXT_ALPHA) {
                SetAlphaState(1);
                if (*(int *)&mat->emissive.x || *(int *)&mat->emissive.y || *(int *)&mat->emissive.z)
                    SetBlendState(1);
                else
                    SetBlendState(0);
            } else if (mat->flags & MATF_CKEY_ALPHA) {
                SetAlphaState(3);
                SetBlendState(0);
            } else
                SetAlphaState(0);
            SetCullState(((~mat->flags & MATF_TWO_SIDED) | 0x8000000) >> 27);
            if (help->texture) {
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
                m_pD3DDevice8->SetTexture(0, help->texture->texture);
                reset |= 8;
            } else {
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
                SetColorOp(D3DTOP_SELECTARG1);
                SetAlphaState(0);
                m_pD3DDevice8->SetTexture(0, NULL);
                reset |= 0x10;
            }
        }
        if (m_HardwareCaps & HWCAPS_HWTL)
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
        SetVertexFormat(l->vb.buffer->vertex_format);
        m_pD3DDevice8->SetStreamSource(0, vb, l->vb.buffer->stride);
        m_pD3DDevice8->SetIndices(fg->ib.ib, l->vb.base);
        m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
        if (reset & 8)
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        if (reset & 0x10)
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        if (reset & 2)
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        if (reset & 4)
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    } else if (lit->mode & 1) {
        SetColorOp1(D3DTOP_DISABLE);
        SetColorOp(m_Modulate2X);
        num_verts = l->m_numLVertices;
        if (!mat) {
            SetAlphaState(0);
            m_pD3DDevice8->SetTexture(0, NULL);
        } else {
            if (mat->flags & MATF_ADDRESS_U_CLAMP) {
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
                reset |= 2;
            }
            if (mat->flags & MATF_ADDRESS_V_CLAMP) {
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
                reset |= 4;
            }
            if (mat->flags & MATF_ADD_MODE) {
                SetAlphaState(1);
                SetBlendState(1);
            } else if (mat->flags & MATF_TXT_ALPHA) {
                SetAlphaState(1);
                if (*(int *)&mat->emissive.x || *(int *)&mat->emissive.y || *(int *)&mat->emissive.z)
                    SetBlendState(1);
                else
                    SetBlendState(0);
            } else if (mat->flags & MATF_CKEY_ALPHA) {
                SetAlphaState(3);
                SetBlendState(0);
            } else
                SetAlphaState(0);
            SetCullState(((~mat->flags & MATF_TWO_SIDED) | 0x8000000) >> 27);
            if (help->texture)
                m_pD3DDevice8->SetTexture(0, help->texture->texture);
            else
                m_pD3DDevice8->SetTexture(0, NULL);
        }
        if (m_HardwareCaps & HWCAPS_HWTL)
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
        SetVertexFormat(l->vb.buffer->vertex_format);
        m_pD3DDevice8->SetStreamSource(0, vb, l->vb.buffer->stride);
        m_pD3DDevice8->SetIndices(fg->ib.ib, l->vb.base);
        m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
        if (reset & 2)
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        if (reset & 4)
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    } else {
        num_verts = l->m_numLVertices;
        if (m_HardwareCaps & HWCAPS_SINGLEPASS) {
            // The lightmap in the first stage, added to the vertices' colors, the texture in the second
            SetColorOp(D3DTOP_ADD);
            m_pD3DDevice8->SetTexture(0, help->env_texture->texture);
            if (!mat) {
                SetColorOp1(D3DTOP_DISABLE);
                SetAlphaState(0);
            } else {
                if (mat->flags & MATF_ADDRESS_U_CLAMP) {
                    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
                    reset |= 2;
                }
                if (mat->flags & MATF_ADDRESS_V_CLAMP) {
                    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
                    reset |= 4;
                }
                if (mat->flags & MATF_ADD_MODE) {
                    SetAlphaState(1);
                    SetBlendState(1);
                } else if (mat->flags & MATF_TXT_ALPHA) {
                    SetAlphaState(1);
                    if (*(int *)&mat->emissive.x || *(int *)&mat->emissive.y || *(int *)&mat->emissive.z)
                        SetBlendState(1);
                    else
                        SetBlendState(0);
                } else if (mat->flags & MATF_CKEY_ALPHA) {
                    SetAlphaState(3);
                    SetBlendState(0);
                } else
                    SetAlphaState(0);
                SetCullState(((~mat->flags & MATF_TWO_SIDED) | 0x8000000) >> 27);
                if (help->texture) {
                    SetColorOp1(m_Modulate2X);
                    m_pD3DDevice8->SetTexture(1, help->texture->texture);
                } else
                    SetColorOp1(D3DTOP_DISABLE);
            }
            if (m_HardwareCaps & HWCAPS_HWTL)
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
            SetVertexFormat(l->vb.buffer->vertex_format);
            m_pD3DDevice8->SetStreamSource(0, vb, l->vb.buffer->stride);
            m_pD3DDevice8->SetIndices(l->ib.ib, l->vb.base);
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, l->ib.base + help->first,
                fg->num_faces);
            if (reset & 1)
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        } else {
            // The texture first, with the second pair of coordinates, then the lightmap over it
            SetColorOp1(D3DTOP_DISABLE);
            if (m_CurrFogState == TRUE)
                m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
            if (!mat) {
                m_pD3DDevice8->SetTexture(0, NULL);
                SetAlphaState(0);
            } else {
                if (mat->flags & MATF_ADDRESS_U_CLAMP) {
                    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
                    reset |= 2;
                }
                if (mat->flags & MATF_ADDRESS_V_CLAMP) {
                    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
                    reset |= 4;
                }
                if (mat->flags & MATF_ADD_MODE) {
                    SetAlphaState(1);
                    SetBlendState(1);
                } else if (mat->flags & MATF_TXT_ALPHA) {
                    SetAlphaState(1);
                    if (*(int *)&mat->emissive.x || *(int *)&mat->emissive.y || *(int *)&mat->emissive.z)
                        SetBlendState(1);
                    else
                        SetBlendState(0);
                } else if (mat->flags & MATF_CKEY_ALPHA) {
                    SetAlphaState(3);
                    SetBlendState(0);
                } else
                    SetAlphaState(0);
                SetCullState(((~mat->flags & MATF_TWO_SIDED) | 0x8000000) >> 27);
                if (help->texture) {
                    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
                    m_pD3DDevice8->SetTexture(0, help->texture->texture);
                    SetColorOp(D3DTOP_SELECTARG1);
                    reset |= 1;
                } else {
                    SetColorOp(D3DTOP_SELECTARG1);
                    m_pD3DDevice8->SetTexture(0, NULL);
                }
            }
            SetVertexFormat(l->vb.buffer->vertex_format);
            m_pD3DDevice8->SetStreamSource(0, vb, l->vb.buffer->stride);
            m_pD3DDevice8->SetIndices(l->ib.ib, l->vb.base);
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, l->ib.base + help->first,
                fg->num_faces);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            if (m_ProblemFlags & 0x40)
                SetColorOp(D3DTOP_SELECTARG1);
            else
                SetColorOp(D3DTOP_ADD);
            if (reset & 1) {
                SetAlphaState(1);
                SetBlendState(4);
                SetAlphaOp(D3DTOP_SELECTARG1);
                m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
            } else
                SetAlphaState(0);
            m_pD3DDevice8->SetTexture(0, help->env_texture->texture);
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, l->ib.base + help->first,
                fg->num_faces);
            m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
            if (m_CurrFogState == TRUE) {
                // The fog pass, the fog color blended by the vertices' alpha
                m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
                SetAlphaOp(D3DTOP_SELECTARG2);
                SetColorOp(D3DTOP_SELECTARG2);
                m_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, m_CurrFogColor);
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
                SetAlphaState(1);
                SetBlendState(0);
                m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, l->ib.base + help->first,
                    fg->num_faces);
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
                m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, TRUE);
                m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
            }
        }
        if (reset & 2)
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        if (reset & 4)
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    }
}

// FUNCTION: LS3DF 0x10029f80
litLod::~litLod()
{
    delete[] m_pUV;
    delete[] m_pLitUV;
    delete[] (uchar *)unknown_3c;
    delete m_pLM;
    if (m_pPIndex) {
        for (int i = 0; i < m_numPIndex; i++)
            delete[] m_pPIndex[i];
        delete[] m_pPIndex;
    }
    m_pPIndex = NULL;
    RECTANGLE_INFO *ri = m_pRectangles;
    while (m_numRectangles--) {
        if (ri->tex_sx > 3) {
            if (ri->data)
                delete[] ri->data;
            if (ri->tex_data)
                delete[] (uchar *)ri->tex_data;
        }
        ri++;
    }
    delete[] (uchar *)m_pRectangles;
    cIBMan.m_Free(ib);
    delete[] m_pVI;
    if (mem > 0)
        cVBMan.m_Free(pos_vb);
    else
        delete[] m_pColors;
    mem = 0;
}

// FUNCTION: LS3DF 0x1002a0e0
bool litLod::Create(I3D_mesh_level *level, I3D_mesh_object *mesh, uint flags)
{
    if (flags & 2 && !dirty && pos_vb.vb) {
        cVBMan.m_Free(pos_vb);
        memset(&pos_vb, 0, sizeof(pos_vb));
    }
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, 0);
    if (!m_numLVertices) {
        lod::Create(0x502, level->num_vertices, &verts->pos, sizeof(I3D_vertex), &verts->tex, sizeof(I3D_vertex),
            NULL, 0, mesh->u_ofs, mesh->v_ofs);
    } else if (flags & 1) {
        if (!lod::Create(0x501, level->num_vertices, &verts->pos, sizeof(I3D_vertex), &verts->tex,
                sizeof(I3D_vertex), NULL, 0, mesh->u_ofs, mesh->v_ofs))
            return false;
        if (m_HardwareCaps & HWCAPS_HWTL) {
            LVERTEX *v = (LVERTEX *)vb.buffer->Lock(vb.base, 0);
            uint *c = m_pColors;
            for (int i = m_numLVertices; i; i--) {
                v->color = *c;
                c++;
                v++;
            }
        } else {
            TLVERTEX *v = (TLVERTEX *)vb.buffer->Lock(vb.base, 0);
            uint *c = m_pColors;
            if (m_ProblemFlags & 4) {
                for (int i = m_numLVertices; i; i--) {
                    uint col = *c;
                    uint a = col & 0xff000000;
                    uint r = col >> 15 & 0x1fe;
                    if (r > 0xff)
                        r = 0xff;
                    uint g = col >> 7 & 0x1fe;
                    if (g > 0xff)
                        g = 0xff;
                    uint b = (col & 0xff) << 1;
                    if (b > 0xff)
                        b = 0xff;
                    v->color = (r << 8 | g) << 8 | b | a;
                    c++;
                    v++;
                }
            } else {
                for (int j = m_numLVertices; j; j--) {
                    v->color = *c;
                    c++;
                    v++;
                }
            }
        }
    } else if (m_HardwareCaps & HWCAPS_HWTL) {
        if (!lod::Create(0x404, m_numLVertices, NULL, 0, m_pLitUV, sizeof(I3D_text_coor), m_pUV,
                sizeof(I3D_text_coor), mesh->u_ofs, mesh->v_ofs))
            return false;
        LVERTEX2 *v = (LVERTEX2 *)vb.buffer->Lock(vb.base, 0);
        for (int i = 0; i < m_numLVertices; i++) {
            *(S_vector *)&v->x = verts[m_pVI[i].vertex].pos;
            v++;
        }
    } else {
        if (!dirty || !pos_vb.buffer) {
            if (!cVBMan.m_Alloc(D3DFVF_XYZ, D3DUSAGE_SOFTWAREPROCESSING, sizeof(S_vector), m_numLVertices, pos_vb,
                    VF_VERTEX, false)) {
                dbgPrintf("litLod::Create() - Error create VertexBuffer for LM !!");
                return false;
            }
        }
        S_vector *pos = (S_vector *)pos_vb.buffer->Lock(pos_vb.base, 0);
        for (int i = m_numLVertices; i; ) {
            i--;
            pos[i] = verts[m_pVI[i].vertex].pos;
        }
        lod::Create(0x502, m_numLVertices, pos, sizeof(S_vector), m_pLitUV, sizeof(I3D_text_coor), m_pUV,
            sizeof(I3D_text_coor), mesh->u_ofs, mesh->v_ofs);
    }
    return true;
}

// FUNCTION: LS3DF 0x1002a790
bool litLod::postProcess(I3D_mesh_level *level, bool loaded)
{
    m_pLM = new LightMap;
    if (!m_pLM) {
        dbgPrintf("!! out of memory  litLod::postLoadProcess()  this:0x%x", this);
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return false;
    }
    ushort *indices = new ushort[m_numRectangles];
    for (int i = 0; i < m_numRectangles; i++)
        indices[i] = i;
    mem = 0;
    if (!m_pLM->m_CreateLM(indices, m_pRectangles, m_numRectangles, &mem)) {
        dbgPrintf("!! can't create LM  litLod::postLoadProcess()  this:0x%x", this);
        delete[] indices;
        return false;
    }
    delete[] indices;
    m_pUV = new I3D_text_coor[m_numLVertices];
    if (!loaded) {
        if (!cIBMan.m_Alloc(D3DUSAGE_SOFTWAREPROCESSING, level->NumFaces() * 3, ib)) {
            dbgPrintf("!! Out of memory  litLod::postLoadProcess");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return false;
        }
    }
    if (!m_pUV || !m_pLitUV) {
        dbgPrintf("!! neklaply kontroly m_pUV:0x%x  m_pLitUV:0x%x  litLod::postLoadProcess()", m_pUV, m_pLitUV);
        return false;
    }
    I3D_text_coor *uv = m_pUV;
    float *lit_uv = (float *)m_pLitUV;
    float su = 1.0f / m_pLM->texture->width;
    VERTEX_INFO *vi = m_pVI;
    float sv = 1.0f / m_pLM->texture->height;
    int n = m_numLVertices;
    while (n--) {
        I3D_vertex &vt = level->LockVertices(0)[vi->vertex];
        *uv = vt.tex;
        if (m_numRectangles <= vi->rect) {
            dbgPrintf("!! corrupted lightmap  litLod::postProcess()  m_numRectangles:%d < pVI->rect:%d",
                m_numRectangles, vi->rect);
            return false;
        }
        RECTANGLE_INFO *ri = &m_pRectangles[vi->rect];
        float u = lit_uv[0];
        float v = lit_uv[1];
        if (ri->u < 2.0f) {
            *lit_uv++ = (ri->tex_sx - 1) * u * su + ri->u + su * .5;
            *lit_uv = (ri->tex_sy - 1) * v * sv + ri->v + sv * .5;
        } else {
            *lit_uv++ = (ri->tex_sy - 1) * v * su + ri->u - 2.0f + su * .5;
            *lit_uv = (ri->tex_sx - 1) * u * sv + ri->v + sv * .5;
        }
        lit_uv++;
        vi++;
        uv++;
    }
    if (!loaded) {
        ushort *indx = (ushort *)ib.buffer->Lock(ib.base, 0);
        int fg_index = 0;
        for (I3D_face_group *fg = level->fgroups; fg; fg_index++, fg = fg->next) {
            ushort *rects = m_pPIndex[fg_index];
            int num = fg->num_faces;
            I3D_triface *faces = fg->GetFaces();
            while (num--) {
                ushort rect = *rects;
                *indx++ = m_GetLVertex(faces->vi[0], rect);
                *indx++ = m_GetLVertex(faces->vi[1], rect);
                *indx++ = m_GetLVertex(faces->vi[2], rect);
                faces++;
                rects++;
            }
        }
    }
    return true;
}
