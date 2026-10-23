// The single meshes (PS2 I3d_singlemesh.cpp), at 0x10054240-0x10056e80 in the original: I3D_singlemesh in PS2's order
// from its constructor, with the PC's lods (SMeshLod: each level's joints and moved vertices) and their vertex helpers
// before m_AddFGroups. The header's zero vector has its $E after m_AddFGroups (0x10056840); after it LoadImage, the
// lod's scalar deleting destructor and destructor, then the out-of-line I3D_joint::GetJointMatrix and
// S_vector::operator*=. Names are the PS2 build's unless noted.
// 1.1's (0x10051fc0-0x10054540): the same functions, without a copy of GetJointMatrix (UpdateLodVertices inlines both
// its calls) and with S_vector::operator*='s right after UpdateLodVertices, its first caller there; its $E is at
// 0x100541e0. 1.1's buffers lock without flags (VBManager.h).
//
// Still open: m_AddFGroups' face group loop (see there); GetJointMatrix's copy (I3D.h); and in UpdateBVolumeProc the
// operands of the sphere center's x sum, which come the other way round (the allocator's live-range ids: every
// spelling of the sum compiles the same). Our linker keeps this file's copy of the empty SetSpace, which it folds with
// the billboard's and the morphs', where the original keeps the billboard file's (I3D.h). In 1.1 ours has
// S_vector::operator*= after CopyMovedPositions: the file's source up to UpdateLodVertices' end is short by about 16
// statements for /YX (SMeshDef's functions and GetJointMatrix defined here, not in I3D.h, give the original's place,
// but then other files' deferred copies move: the billboard's SetAlphaState, the particles' SetSector). 1.1's
// UpdateLodVertices loads two slots in the other order (100%*) since the matrix's element accessor has its asserts
// (I3d_math.h); 1.2's is unchanged.
#include "I3D/I3D.h"
#include "I3D/render/render.h"
#include "I3D/render/VBManager.h"
#include "IGraph/IGraph.h"
#include "dta.h"

// DBuild's search for the joints (PS2 names)
// GLOBAL: LS3DF 0x101b4f80
// GLOBAL: LS3DF11 0x101b20a0
static int jointu;
// GLOBAL: LS3DF 0x101b4f88
// GLOBAL: LS3DF11 0x101b20a8
static I3D_joint *pJoint[64];

// FUNCTION: LS3DF 0x10054240
// FUNCTION: LS3DF11 0x10051fc0
I3D_singlemesh::I3D_singlemesh()
{
    num_joints = 0;
    joints = NULL;
    visual_type = I3D_VISUAL_SINGLEMESH;
    joints_changed = (uint)-300;
    bvol_joints_changed = (uint)-400;
}

// FUNCTION: LS3DF 0x100542b0
// FUNCTION: LS3DF11 0x10052030
I3D_singlemesh::~I3D_singlemesh()
{
    int i;
    for (i = 0; i < num_joints; i++) {
        if (joints[i])
            joints[i]->singlemesh = NULL;
    }
    delete[] joints;
    for (i = 0; i < 10; i++)
        delete lods[i];
    memset(lods, 0, sizeof(lods));
}

// FUNCTION: LS3DF 0x10054370
// FUNCTION: LS3DF11 0x100520f0
void I3D_singlemesh::UpdateVertices()
{
    I3D_object::UpdateVertices();
    joints_changed++;
}

// FUNCTION: LS3DF 0x10054390
// FUNCTION: LS3DF11 0x10052110
void I3D_singlemesh::UpdateVertices(int index)
{
    I3D_object::UpdateVertices(index);
    joints_changed++;
}

// FUNCTION: LS3DF 0x100543b0
// FUNCTION: LS3DF11 0x10052130
const I3D_bbox *I3D_singlemesh::GetJointBBox(int index) const
{
    if (index >= num_joints)
        return NULL;
    SMeshLod *l = (SMeshLod *)lods[0];
    if (!l)
        return NULL;
    SMeshDef *def = l->def;
    if (!def)
        return NULL;
    if (index < 0)
        return &def->bbox;
    if (def->info && index < def->num_joints)
        return &def->info[index].bbox;
    return NULL;
}

void I3D_singlemesh::SetSpace(bool world)
{
}

// The copy's lods share sm's levels: a lod for each of sm's
// FUNCTION: LS3DF 0x10054400
// FUNCTION: LS3DF11 0x10052180
LS3D_RESULT I3D_singlemesh::DuplicateSinglemeshDef(I3D_singlemesh *sm)
{
    for (int i = 0; i < 10; i++) {
        if (!sm->lods[i])
            continue;
        SMeshLod *l = new SMeshLod;
        if (!l) {
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return I3DERR_OUTOFMEM;
        }
        lods[i] = l;
        l->def = ((SMeshLod *)sm->lods[i])->def;
        if (l->def)
            l->def->ref++;
    }
    return I3D_OK;
}

// From a visual of a type with a mesh it takes the mesh, and the levels of a single mesh or single morph unless this is
// a proxy. A proxy takes the objects' case through a goto: the original has one copy of it, which the objects' cases
// and both proxy tests share. Written out in each case, VC6 hoists their shared push above the proxy tests and keeps
// three copies.
// FUNCTION: LS3DF 0x100544d0
// FUNCTION: LS3DF11 0x10052250
LS3D_RESULT I3D_singlemesh::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    DiscardHRBoundVol();
    I3D_frame::Duplicate(frame);
    joints_changed++;
    if (frame->GetType() != FRAME_VISUAL) {
        if (frame->GetType() == FRAME_EMITOR) {
            int i;
            for (i = 0; i < num_joints; i++) {
                if (joints[i])
                    joints[i]->singlemesh = NULL;
            }
            delete[] joints;
            joints = NULL;
            num_joints = 0;
            for (i = 0; i < 10; i++)
                delete lods[i];
            memset(lods, 0, sizeof(lods));
            SetMesh(((I3D_emitor *)frame)->mesh, true);
        }
        return I3D_OK;
    }
    int i;
    for (i = 0; i < num_joints; i++) {
        if (joints[i])
            joints[i]->singlemesh = NULL;
    }
    delete[] joints;
    joints = NULL;
    num_joints = 0;
    for (i = 0; i < 10; i++)
        delete lods[i];
    memset(lods, 0, sizeof(lods));
    {
        I3D_singlemesh *sm = (I3D_singlemesh *)frame;
        switch (sm->visual_type) {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_BILLBOARD:
        case I3D_VISUAL_LENSF:
        case I3D_VISUAL_MORPH:
        obj:
            SetMesh(sm->mesh, false);
            lightness = sm->lightness;
            transparency = sm->transparency;
            fade = sm->fade;
            break;
        case I3D_VISUAL_SINGLEMESH:
            if (vis_flags & VISF_PROXY)
                goto obj;
            {
                lightness = sm->lightness;
                transparency = sm->transparency;
                fade = sm->fade;
                SetMesh(sm->mesh, false);
                LS3D_RESULT r = DuplicateSinglemeshDef(sm);
                if (r < 0)
                    return r;
            }
            break;
        case I3D_VISUAL_SINGLEMORPH:
            if (vis_flags & VISF_PROXY)
                goto obj;
            {
                lightness = sm->lightness;
                transparency = sm->transparency;
                fade = sm->fade;
                SetMesh(sm->mesh, false);
                LS3D_RESULT r = DuplicateSinglemeshDef(sm);
                if (r < 0)
                    return r;
            }
            break;
        default:
            return I3DERR_GENERIC;
        }
        vis_flags = sm->vis_flags & 0x2ffd0;
        vis_bound = sm->vis_bound;
        if (sm->vis_flags & VISF_WORLD_SPACE)
            vis_bound_w = sm->GetWBoundVolume();
        else
            vis_flags &= ~VISF_WBOUND_VALID;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100547e0
// FUNCTION: LS3DF11 0x10052560
void I3D_singlemesh::UpdateBoundProc()
{
    if (bvol_joints_changed != joints_changed)
        UpdateBVolumeProc();
    else
        vis_flags |= VISF_BOUND_VALID;
}

// The first level's box, grown by each joint's box where the joint is now. A moved corner goes into a local, which
// Absorb reads from its slot; passed as the temporary, it is read through the returned pointer.
// FUNCTION: LS3DF 0x10054810
// FUNCTION: LS3DF11 0x10052590
void I3D_singlemesh::UpdateBVolumeProc()
{
    bvol_joints_changed = joints_changed;
    if (!joints && !(vis_flags & VISF_PROXY))
        DBuild();
    if (!mesh) {
        vis_bound.bbox.Invalidate();
        vis_bound.bsphere.radius = 0.0f;
        vis_flags = (vis_flags & ~VISF_WBOUND_VALID) | VISF_BOUND_VALID;
        return;
    }
    SMeshLod *l = (SMeshLod *)lods[0];
    if (!l) {
        vis_bound = mesh->bvol;
        vis_flags = (vis_flags & ~VISF_WBOUND_VALID) | VISF_BOUND_VALID;
        return;
    }
    SMeshDef *def = l->def;
    if (!def) {
        vis_bound = mesh->bvol;
        vis_flags = (vis_flags & ~VISF_WBOUND_VALID) | VISF_BOUND_VALID;
        return;
    }
    I3D_bbox bb = def->bbox;
    if (joints && def->info) {
        I3D_joint **jp = joints;
        SM_INFO *info = def->info;
        for (int i = 0; i < num_joints; i++, jp++, info++) {
            if (*jp && info->bbox.IsValid()) {
                const S_vector *v = BBoxVertices(info->bbox);
                const S_matrix &m = (*jp)->GetJointMatrix();
                for (int j = 8; j; j--, v++) {
                    S_vector p = *v * m;
                    bb.Absorb(p);
                }
            }
        }
    }
    vis_bound.bbox = bb;
    S_vector half = (bb.max - bb.min) * 0.5f;
    vis_bound.bsphere.pos = bb.min + half;
    vis_bound.bsphere.radius = (float)sqrt(half.x * half.x + half.y * half.y + half.z * half.z);
    vis_flags = (vis_flags & ~VISF_WBOUND_VALID) | VISF_BOUND_VALID;
    DiscardHRBoundVol();
}

static void findJoint(I3D_frame *frm);

// FUNCTION: LS3DF 0x10054c20
// FUNCTION: LS3DF11 0x100529a0
bool I3D_singlemesh::DBuild()
{
    int i;
    for (i = 0; i < num_joints; i++) {
        if (joints[i])
            joints[i]->singlemesh = NULL;
    }
    num_joints = 0;
    delete[] joints;
    joints = NULL;
    if (!mesh)
        return false;
    jointu = 0;
    findJoint(this);
    if (!jointu)
        return false;
    joints = new I3D_joint *[jointu];
    if (!joints) {
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return false;
    }
    num_joints = jointu;
    for (i = 0; i < num_joints; i++) {
        I3D_joint **p = pJoint;
        int k = num_joints;
        while (k--) {
            if ((*p)->index == i) {
                joints[i] = *p;
                joints[i]->singlemesh = this;
                break;
            }
            p++;
        }
    }
    UpdateBVolumeProc();
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
    return true;
}

// The joints below frm into pJoint, as many as it holds. VC6 inlines the first level into DBuild and puts the copy
// right after it.
// FUNCTION: LS3DF 0x10054d80
// FUNCTION: LS3DF11 0x10052b00
static void findJoint(I3D_frame *frm)
{
    for (I3D_frame *f = frm->m_pChild; f; f = f->m_pNext) {
        if (f->GetType() == FRAME_JOINT) {
            if (jointu >= 64)
                break;
            pJoint[jointu++] = (I3D_joint *)f;
        }
        findJoint(f);
    }
}

// FUNCTION: LS3DF 0x10054dd0
// FUNCTION: LS3DF11 0x10052b50
I3D_joint *I3D_singlemesh::GetVertexJoint(int vertex, int lod_index)
{
    SMeshLod *l = (SMeshLod *)lods[lod_index];
    if (!l || !l->def)
        return NULL;
    I3D_joint **jp = joints;
    SM_INFO *info = l->def->info;
    for (int i = num_joints; i > 0; i--, jp++, info++) {
        vertex -= info->num_verts + info->m_vertNum;
        if (vertex < 0)
            return *jp;
    }
    return NULL;
}

// FUNCTION: LS3DF 0x10054e30
// FUNCTION: LS3DF11 0x10052bb0
const S_clip_sm_vertex *I3D_singlemesh::GetVertexPos(int lod_index)
{
    I3D_mesh_level *level = mesh->levels[lod_index];
    if (!level)
        return NULL;
    if (!level->num_vertices)
        return NULL;
    SMeshLod *l = (SMeshLod *)lods[lod_index];
    if (!l)
        return NULL;
    if (l->joints_changed != joints_changed)
        UpdateLodVertices(lod_index);
    C_VBuffer *buffer = l->vb_skin.buffer;
    if (!buffer)
        return NULL;
    return (const S_clip_sm_vertex *)buffer->Lock(l->vb_skin.base, BUFFER_LOCK_FLAGS);
}

// Each joint's vertices through its matrix, the blended ones through both joints'. The lod's buffer is made at the
// first call, with the vertices no joint moves copied from the mesh. A proxy moves its visual's.
// The inline budget decides the shape: the original inlines C_list::push_back in the first and third Lock and calls it
// in the second and fourth, and calls the second GetJointMatrix. That takes the proxy's visual through GetData (an
// inline call pending between the second and the third Lock), the rigid copy counting num_rigid itself down (a
// post-decrement loop of post-increments; a counter of its own is a little more source, which inlines the second
// Lock's push_back), and jp stepped before info (the loop's last register).
// 1.1's Lock has no push_back: its shape comes from 1.1's GetJointMatrix (I3D.h), the proxy's visual read as m_UserData
// and the rigid copy counting a counter of its own down (the original's registers and operand orders, no other code).
// FUNCTION: LS3DF 0x10054f60
// FUNCTION: LS3DF11 0x10052c40
void I3D_singlemesh::UpdateLodVertices(int lod_index)
{
    if (!joints && !(vis_flags & VISF_PROXY))
        DBuild();
    SMeshLod *l = (SMeshLod *)lods[lod_index];
    if (!l)
        return;
    l->lights_changed = (uint)-700;
    l->update_count = (uint)-600;
    l->joints_changed = joints_changed;
    I3D_mesh_level *level = mesh->levels[lod_index];
    if (!level)
        return;
    SMeshDef *def = l->def;
    if (!def)
        return;
    if (!l->vb_skin.buffer) {
        if (!cVBMan.m_Alloc(D3DFVF_XYZ | D3DFVF_NORMAL, D3DUSAGE_SOFTWAREPROCESSING, sizeof(S_clip_sm_vertex),
                level->num_vertices, l->vb_skin, VF_VERTEX, false)) {
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return;
        }
        int num_rigid = def->num_rigid;
        if (num_rigid) {
            int first = level->num_vertices - num_rigid;
            const I3D_vertex *src =
                (const I3D_vertex *)level->vb.buffer->Lock(level->vb.base + first, BUFFER_LOCK_FLAGS);
            S_clip_sm_vertex *dst =
                (S_clip_sm_vertex *)l->vb_skin.buffer->Lock(l->vb_skin.base + first, BUFFER_LOCK_FLAGS);
#if ENGINE_VERSION >= 395
            while (num_rigid--)
                *dst++ = *(const S_clip_sm_vertex *)src++;
#else
            for (int k = num_rigid; k; k--)
                *dst++ = *(const S_clip_sm_vertex *)src++;
#endif
        }
    }
    if (vis_flags & VISF_PROXY) {
#if ENGINE_VERSION >= 395
        ((I3D_singlemesh *)GetData())->UpdateLodVertices(lod_index);
#else
        ((I3D_singlemesh *)m_UserData)->UpdateLodVertices(lod_index);
#endif
        return;
    }
    SM_INFO *info = def->info;
    I3D_joint **jp = joints;
    S_clip_sm_vertex *dst = (S_clip_sm_vertex *)l->vb_skin.buffer->Lock(l->vb_skin.base, BUFFER_LOCK_FLAGS);
    const I3D_vertex *src = (const I3D_vertex *)level->vb.buffer->Lock(level->vb.base, BUFFER_LOCK_FLAGS);
    __declspec(align(16)) S_matrix m, mn, m2, mn2;
    for (int i = num_joints; i; i--, jp++, info++) {
        if (info->m_vertNum || info->num_verts) {
            m = info->matrix * (*jp)->GetJointMatrix();
            mn.SetRot(m);
        }
        if (info->num_verts) {
            TransformVertices(dst, src, info->num_verts, m, mn);
            src += info->num_verts;
            dst += info->num_verts;
        }
        if (info->m_vertNum) {
            if (!info->blend_joint) {
                m2.Identity();
                mn2.Identity();
            } else {
                m2 = joints[info->blend_joint - 1]->GetJointMatrix();
                m2 = def->info[info->blend_joint - 1].matrix * m2;
                mn2.SetRot(m2);
            }
            TransformVerticesBlend(dst, src, info->m_vertNum, m, mn, info->weights, m2, mn2);
            src += info->m_vertNum;
            dst += info->m_vertNum;
        }
    }
}

// Both walk a local copy of dst; walking dst itself moves their registers
// FUNCTION: LS3DF 0x10055640
// FUNCTION: LS3DF11 0x10053440
void I3D_singlemesh::CopyRigidPositions(uchar *dst, uint stride, int lod_index)
{
    I3D_singlemesh *sm = this;
    while (sm->vis_flags & VISF_PROXY)
        sm = (I3D_singlemesh *)sm->m_UserData;
    if (!sm->joints)
        sm->DBuild();
    SMeshLod *l = (SMeshLod *)sm->lods[lod_index];
    if (!l)
        return;
    SMeshDef *def = l->def;
    if (!def)
        return;
    int num = def->num_rigid;
    if (!num)
        return;
    I3D_mesh_level *level = sm->mesh->levels[lod_index];
    if (!level)
        return;
    int first = level->num_vertices - num;
    const I3D_vertex *src = (const I3D_vertex *)level->vb.buffer->Lock(level->vb.base + first, BUFFER_LOCK_FLAGS);
    uchar *p = dst + first * stride;
    for (int i = num; i; i--) {
        *(S_vector *)p = src->pos;
        p += stride;
        src++;
    }
}

// FUNCTION: LS3DF 0x100557c0
// FUNCTION: LS3DF11 0x10053510
void I3D_singlemesh::CopyMovedPositions(uchar *dst, uint stride, int lod_index)
{
    I3D_singlemesh *sm = this;
    while (sm->vis_flags & VISF_PROXY)
        sm = (I3D_singlemesh *)sm->m_UserData;
    if (!sm->joints)
        sm->DBuild();
    SMeshLod *l = (SMeshLod *)sm->lods[lod_index];
    if (!l)
        return;
    SMeshDef *def = l->def;
    if (!def)
        return;
    I3D_mesh_level *level = sm->mesh->levels[lod_index];
    if (!level)
        return;
    int num = level->num_vertices - def->num_rigid;
    if (num <= 0)
        return;
    const S_clip_sm_vertex *src = (const S_clip_sm_vertex *)l->vb_skin.buffer->Lock(l->vb_skin.base, BUFFER_LOCK_FLAGS);
    uchar *p = dst;
    for (int i = num; i; i--) {
        *(S_vector *)p = src->pos;
        p += stride;
        src++;
    }
}

// As I3D_object's, with the lod's vertices moved by the joints: the lod's vertex buffer gets their positions, and is
// lit and transformed from them.
// The position copies and FogProcess are an if/else with a call in each branch (the original hoists lod_index's push
// above the hardware test, and pushes FogProcess' stride in the branches). The inline budget: the original inlines
// C_list::push_back in the alpha block's Lock and calls it in the environment map's, which takes no inline call
// between the two (the alpha block's and the environment test's flags read as the member; through GetFlags both
// Locks inline it) and enough pending after them: EnvProcess' two GetFlags and the face groups' GetNext and
// GetMaterial, as in the particles' m_AddFGroups (one of the last two would do; written as the members, both Locks
// inline it).
// Not matching yet: the face group loop keeps fg in edx and reloads it after the texture updates, ours reloads it
// at the loop's top; the original loads the material's alpha before fade (every spelling of the product compiles the
// same), and swaps which of num_transparent++ and the push_back's size increment are a read-modify-write; the stack
// slots of some temporaries follow. 1.1's differs the same way; there, without the Locks' push_back, the flags and the
// face group read through the accessors or as the members compile alike.
// FUNCTION: LS3DF 0x10055930
// FUNCTION: LS3DF11 0x100535d0
void I3D_singlemesh::m_AddFGroups()
{
    if (!mesh)
        return;
    if (vis_flags & VISF_PROXY) {
        I3D_visual *vis = (I3D_visual *)m_UserData;
        I3D_frame *frm = vis->m_pParent;
        while (true) {
            if (!frm) {
                AddFGHlpStruct.sector = pCurrentScene->primary_sector;
                break;
            }
            if (frm->m_Type == FRAME_SECTOR) {
                AddFGHlpStruct.sector = (I3D_sector *)frm;
                break;
            }
            frm = frm->m_pParent;
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
    SMeshLod *l = (SMeshLod *)lods[lod_index];
    if (!l)
        return;
    if (l->dirty) {
        l->dirty = false;
        joints_changed++;
    }
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, BUFFER_LOCK_FLAGS);
    if (!l->vb.vb) {
        if (!l->Create((level->GetFlags() & MESHF_ENVMAP ? 2 : 1) | 0x400, level->num_vertices, NULL, 0, &verts->tex,
                sizeof(I3D_vertex), NULL, 0, mesh->u_ofs, mesh->v_ofs)) {
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return;
        }
        l->lights_changed = (uint)-1;
        l->light_key = (uint)-1;
        l->update_count = (uint)-1;
    }

    if (lod_index == mesh->num_lods - 1 && level->lod_dist > 0.0f) {
        float f = level->lod_dist * 0.5f;
        if (mesh->num_lods > 1)
            f += mesh->levels[lod_index - 1]->lod_dist;
        float s = RSLodQuality * m_WorldScale;
        float d = 1.0f - (dist_2 / (s * s) - f) / (level->lod_dist - f);
        if (d > 1.0f)
            d = 1.0f;
        SetFade(d);
    } else
        SetFade(1.0f);

    bool moved;
    if (l->update_count != m_UpdateCount) {
        moved = true;
        l->update_count = m_UpdateCount;
    } else
        moved = false;
    int base = l->vb.base;
    IDirect3DVertexBuffer8 *dest = l->vb.vb;
    uchar *lv = NULL;
    if (vis_flags & VISF_PROXY)
        joints_changed = ((I3D_singlemesh *)m_UserData)->joints_changed;
    if (l->joints_changed != joints_changed) {
        UpdateLodVertices(lod_index);
        l->update_count = m_UpdateCount;
        moved = true;
    }
    if (moved) {
        lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
        if (m_HardwareCaps & HWCAPS_HWTL)
            CopyRigidPositions(lv, level->GetFlags() & MESHF_ENVMAP ? sizeof(LVERTEX2) : sizeof(LVERTEX), lod_index);
        else
            CopyRigidPositions(lv, level->GetFlags() & MESHF_ENVMAP ? sizeof(TLVERTEX2) : sizeof(TLVERTEX), lod_index);
        if (m_HardwareCaps & HWCAPS_HWTL)
            CopyMovedPositions(lv, level->GetFlags() & MESHF_ENVMAP ? sizeof(LVERTEX2) : sizeof(LVERTEX), lod_index);
        else
            CopyMovedPositions(lv, level->GetFlags() & MESHF_ENVMAP ? sizeof(TLVERTEX2) : sizeof(TLVERTEX), lod_index);
    }
    if (!l->vb_skin.buffer)
        return;
    const S_clip_sm_vertex *sv = (const S_clip_sm_vertex *)l->vb_skin.buffer->Lock(l->vb_skin.base, BUFFER_LOCK_FLAGS);

    if (moved || l->lights_changed != AddFGHlpStruct.sector->lights_changed) {
        l->lights_changed = AddFGHlpStruct.sector->lights_changed;
        LightingSetSector(AddFGHlpStruct.sector);
        if (!lv)
            lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
        if (m_HardwareCaps & HWCAPS_HWTL) {
            if (level->GetFlags() & MESHF_ENVMAP)
                PrepareLights((uchar *)&((LVERTEX2 *)lv)->color, sizeof(LVERTEX2), level, (const uchar *)sv,
                    sizeof(S_clip_sm_vertex), this, GetInvMatrix(), I3DLIGHTMODE_VERTEX, moved, l);
            else
                PrepareLights((uchar *)&((LVERTEX *)lv)->color, sizeof(LVERTEX), level, (const uchar *)sv,
                    sizeof(S_clip_sm_vertex), this, GetInvMatrix(), I3DLIGHTMODE_VERTEX, moved, l);
        } else {
            if (level->GetFlags() & MESHF_ENVMAP)
                PrepareLights((uchar *)&((TLVERTEX2 *)lv)->color, sizeof(TLVERTEX2), level, (const uchar *)sv,
                    sizeof(S_clip_sm_vertex), this, GetInvMatrix(), I3DLIGHTMODE_VERTEX, moved, l);
            else
                PrepareLights((uchar *)&((TLVERTEX *)lv)->color, sizeof(TLVERTEX), level, (const uchar *)sv,
                    sizeof(S_clip_sm_vertex), this, GetInvMatrix(), I3DLIGHTMODE_VERTEX, moved, l);
        }
    }
    if (l->alpha_dirty) {
        if (!lv)
            lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
        if (m_HardwareCaps & HWCAPS_HWTL) {
            if (level->mesh_flags & MESHF_ENVMAP)
                SetVertexAlpha((uchar *)&((LVERTEX2 *)lv)->color, sizeof(LVERTEX2), level, this, l);
            else
                SetVertexAlpha((uchar *)&((LVERTEX *)lv)->color, sizeof(LVERTEX), level, this, l);
        } else {
            if (level->mesh_flags & MESHF_ENVMAP)
                SetVertexAlpha((uchar *)&((TLVERTEX2 *)lv)->color, sizeof(TLVERTEX2), level, this, l);
            else
                SetVertexAlpha((uchar *)&((TLVERTEX *)lv)->color, sizeof(TLVERTEX), level, this, l);
        }
    }

    if (renderCameraChanged || m_RenderNumber != RenderNumberMinus1 || moved) {
        if (level->mesh_flags & MESHF_ENVMAP) {
            if (!lv)
                lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
            if (m_HardwareCaps & HWCAPS_HWTL)
                EnvProcess(&((LVERTEX2 *)lv)->u2, sizeof(LVERTEX2), sv, sizeof(S_clip_sm_vertex), num_verts,
                    AddFGHlpStruct.cam_pos, GetMatrix(), level->env_scale, l->env_u_ofs, l->env_v_ofs,
                    level->GetFlags() & MESHF_ENVMAP_MODE, NULL);
            else
                EnvProcess(&((TLVERTEX2 *)lv)->u2, sizeof(TLVERTEX2), sv, sizeof(S_clip_sm_vertex), num_verts,
                    AddFGHlpStruct.cam_pos, GetMatrix(), level->env_scale, l->env_u_ofs, l->env_v_ofs,
                    level->GetFlags() & MESHF_ENVMAP_MODE, NULL);
        }
        if (!(m_HardwareCaps & HWCAPS_HWTL)) {
            if (m_FrmFlags & FRMFLAGS_CLIP) {
                m_pD3DDevice8->SetStreamSource(0, l->vb_skin.vb, l->vb_skin.buffer->stride);
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
                l->vb_skin.buffer->Unlock();
                SetVertexFormat(VF_VERTEX);
                SetClipping(TRUE);
            } else {
                m_pD3DDevice8->SetStreamSource(0, l->vb_skin.vb, l->vb_skin.buffer->stride);
                m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
                l->vb_skin.buffer->Unlock();
                SetVertexFormat(VF_VERTEX);
                SetClipping(FALSE);
            }
            m_pD3DDevice8->ProcessVertices(l->vb_skin.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
            if (!(m_HardwareCaps & HWCAPS_TABLEFOG)) {
                if (!lv)
                    lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
                if (level->GetFlags() & MESHF_ENVMAP)
                    FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z, sizeof(TLVERTEX2),
                        num_verts, AddFGHlpStruct.sector->fog_light, !(m_HardwareCaps & HWCAPS_SINGLEPASS));
                else
                    FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z, sizeof(TLVERTEX),
                        num_verts, AddFGHlpStruct.sector->fog_light, !(m_HardwareCaps & HWCAPS_SINGLEPASS));
            }
        }
    }

    RENDER_FG_HELP_STRUCT h;
    h.sector = AddFGHlpStruct.sector;
    h.clip = m_FrmFlags & FRMFLAGS_CLIP;
    h.vis = this;
    for (I3D_face_group *fg = level->fgroups; fg; fg = fg->GetNext()) {
        I3D_material *mat = fg->GetMaterial();
        h.fg = fg;
        if (mat && mat->flags & MATF_TEXTURE)
            h.texture = mat->GetTexture();
        else
            h.texture = NULL;
        if (mat && mat->flags & MATF_ENVMAP)
            h.env_texture = mat->GetEnvTexture();
        else
            h.env_texture = NULL;
        if (!mat || !h.texture && mat->alpha * fade * transparency >= 1.0f) {
            h.sort_value = 0;
            AddFGHlpStruct.num_opaque++;
        } else if (mat->flags & (MATF_TXT_ALPHA | MATF_ADD_MODE) || mat->alpha * fade * transparency < 1.0f) {
            h.sort_value = (FloatToInt(sort_dist) & 0x3fffffff) | 0x80000000;
            AddFGHlpStruct.num_transparent++;
        } else {
            if (mat->flags & MATF_CKEY_ALPHA)
                h.sort_value = ((h.texture ? h.texture->m_UID : 0) & 0x7fff) | 0x40000000;
            else
                h.sort_value = (h.texture ? h.texture->m_UID : 0) & 0x7fff;
            AddFGHlpStruct.num_opaque++;
        }
        AddFGHlpStruct.fgroups.push_back(h);
    }
}

// Each lod's joints' parts of its level: a count, then the vertices no joint moves and the level's box, then each
// joint's part and its weights. A lod without joints is deleted. The lod is tested before it is assigned (eax, then
// copied into esi).
// FUNCTION: LS3DF 0x10056860
// FUNCTION: LS3DF11 0x10054200
void I3D_singlemesh::LoadImage()
{
    I3D_object::LoadImage();
    if (!mesh)
        return;
    for (uint i = 0; i < mesh->num_lods; i++) {
        int num = 0;
        dtaRead(LoadImageHandle, &num, 1);
        if (!num) {
            delete lods[i];
            lods[i] = NULL;
            continue;
        }
        SMeshLod *l;
        if (lods[i])
            l = (SMeshLod *)lods[i];
        else {
            l = new SMeshLod;
            if (!l) {
                ImageErrFlg = I3DERR_OUTOFMEM;
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return;
            }
            lods[i] = l;
        }
        SMeshDef *def = l->def;
        if (def) {
            delete[] def->info;
            def->info = NULL;
        } else {
            def = new SMeshDef;
            if (!def) {
                ImageErrFlg = I3DERR_OUTOFMEM;
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return;
            }
            l->def = def;
        }
        dtaRead(LoadImageHandle, &def->num_rigid, sizeof(def->num_rigid) + sizeof(def->bbox));
        def->num_joints = num;
        SM_INFO *info = (SM_INFO *)new uchar[num * sizeof(SM_INFO)];
        if (!info) {
            ImageErrFlg = I3DERR_OUTOFMEM;
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return;
        }
        def->info = info;
        while (num--) {
            dtaRead(LoadImageHandle, info, sizeof(SM_INFO) - sizeof(info->weights));
            if (info->m_vertNum) {
                info->weights = new float[info->m_vertNum];
                if (!info->weights) {
                    dbgPrintf("!! Out of memory I3D_singlemesh::LoadImage() m_vertNum:%d", info->m_vertNum);
                    ImageErrFlg = I3DERR_OUTOFMEM;
                    return;
                }
                dtaRead(LoadImageHandle, info->weights, info->m_vertNum * sizeof(float));
            } else
                info->weights = NULL;
            info++;
        }
    }
}

// Inline, so that its copy comes after the scalar deleting destructor that calls it (I3D.h)
// FUNCTION: LS3DF 0x10056af0
// FUNCTION: LS3DF11 0x10054490
inline SMeshLod::~SMeshLod()
{
    if (def)
        def->Release();
    cVBMan.m_Free(vb_skin);
}
