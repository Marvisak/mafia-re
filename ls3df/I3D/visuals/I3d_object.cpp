// The objects (PS2 I3d_object.cpp), at 0x100367b0-0x1003a430 in the original: I3D_object close to PS2's order, with the
// PC's lods (each level's own vertex buffer) after Duplicate. Its marker $E (0x10039260) comes before m_RenderFGroup.
// Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "I3D/render/render.h"
#include "I3D/render/VBManager.h"
#include "IGraph/IGraph.h"

// FUNCTION: LS3DF 0x100367b0
I3D_object::I3D_object()
{
    vis_flags |= VISF_UNKNOWN_800;
    mesh = NULL;
    cur_lod = 0;
    lightness = 1.0f;
    transparency = 1.0f;
    fade = 1.0f;
    visual_type = I3D_VISUAL_OBJECT;
    for (int i = 0; i < 10; i++)
        lods[i] = NULL;
}

// FUNCTION: LS3DF 0x10036840
I3D_object::~I3D_object()
{
    for (int i = 0; i < 10; i++)
        delete lods[i];
    memset(lods, 0, sizeof(lods));
    if (mesh)
        mesh->Release();
}

// FUNCTION: LS3DF 0x100368e0
void I3D_object::SetMesh(I3D_mesh_object *m, bool keep_lods)
{
    if (m == mesh) {
        if (!m)
            return;
    } else {
        if (!keep_lods) {
            for (int i = 0; i < 10; i++)
                delete lods[i];
            memset(lods, 0, sizeof(lods));
        } else if (!m || !m->IsCompatible(mesh)) {
            for (int i = 0; i < 10; i++)
                delete lods[i];
            memset(lods, 0, sizeof(lods));
        }
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

// FUNCTION: LS3DF 0x10036a20
void I3D_object::UpdateVertices()
{
    for (int i = 0; i < 10; i++) {
        if (lods[i])
            lods[i]->dirty = true;
    }
}

// FUNCTION: LS3DF 0x10036a50
void I3D_object::UpdateVertices(int index)
{
    if (lods[index])
        lods[index]->dirty = true;
}

// FUNCTION: LS3DF 0x10036a70
LS3D_RESULT I3D_object::DetachMesh()
{
    if (mesh && mesh->ref >= 2) {
        I3D_mesh_object *m = pI3DDriver->CreateMesh();
        if (!m) {
            dbgPrintf("!! Out of memory  I3D_object::DetachMesh()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return I3DERR_OUTOFMEM;
        }
        LS3D_RESULT result = m->Duplicate(mesh);
        if (result < 0) {
            m->Release();
            dbgPrintf("!! Can't duplicate mesh  I3D_object::DetachMesh()");
            return result;
        }
        mesh->Release();
        mesh = m;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10036b10
LS3D_RESULT I3D_object::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_VISUAL) {
        if (frame->GetType() == FRAME_EMITOR) {
            SetMesh(((I3D_emitor *)frame)->mesh, true);
            vis_flags |= VISF_UNKNOWN_800;
        }
        return I3D_OK;
    }
    {
        I3D_object *obj = (I3D_object *)frame;
        switch (obj->visual_type) {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_BILLBOARD:
        case I3D_VISUAL_SINGLEMESH:
        case I3D_VISUAL_LENSF:
        case I3D_VISUAL_MORPH:
        case I3D_VISUAL_SINGLEMORPH:
        case I3D_VISUAL_PART_BASE:
        case I3D_VISUAL_PART_ELEMENT:
            break;
        default:
            return I3DERR_GENERIC;
        }
        SetMesh(obj->mesh, false);
        lightness = obj->lightness;
        transparency = obj->transparency;
        fade = obj->fade;
        vis_flags = obj->vis_flags & 0x2ffd0;
        vis_bound = obj->vis_bound;
        if (obj->vis_flags & VISF_WORLD_SPACE)
            vis_bound_w = obj->GetWBoundVolume();
        else
            vis_flags &= ~VISF_WBOUND_VALID;
        vis_flags |= VISF_UNKNOWN_800;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10036c70
lod::lod()
{
    lights_changed = (uint)-1;
    update_count = (uint)-1;
    light_key = (uint)-1;
    memset(&vb, 0, sizeof(vb));
    dirty = false;
    alpha_dirty = false;
    alpha = 0xff;
    u_ofs = 0.0f;
    v_ofs = 0.0f;
    env_u_ofs = 0.0f;
    env_v_ofs = 0.0f;
}

// FUNCTION: LS3DF 0x10036ce0
lod::~lod()
{
    cVBMan.m_Free(vb);
}

// FUNCTION: LS3DF 0x10036d00
void lod::SetUV(uint format, int num, const I3D_text_coor *uv, uint stride, float uo, float vo)
{
    u_ofs = uo;
    v_ofs = vo;
    format &= 3;
    if (m_HardwareCaps & HWCAPS_HWTL) {
        switch (format) {
        case 1: {
            LVERTEX *v = (LVERTEX *)vb.buffer->Lock(vb.base, 0);
            if (!uv || !num)
                break;
            while (num--) {
                v->u = uv->u + uo;
                v->v = uv->v + vo;
                uv = (const I3D_text_coor *)((const uchar *)uv + stride);
                v++;
            }
            break;
        }
        case 2: {
            LVERTEX2 *v = (LVERTEX2 *)vb.buffer->Lock(vb.base, 0);
            if (!uv || !num)
                break;
            while (num--) {
                v->u = uv->u + uo;
                v->v = uv->v + vo;
                uv = (const I3D_text_coor *)((const uchar *)uv + stride);
                v++;
            }
            break;
        }
        }
    } else {
        switch (format) {
        case 1: {
            TLVERTEX *v = (TLVERTEX *)vb.buffer->Lock(vb.base, 0);
            if (!uv || !num)
                break;
            while (num--) {
                v->u = uv->u + uo;
                v->v = uv->v + vo;
                uv = (const I3D_text_coor *)((const uchar *)uv + stride);
                v++;
            }
            break;
        }
        case 2: {
            TLVERTEX2 *v = (TLVERTEX2 *)vb.buffer->Lock(vb.base, 0);
            if (!uv || !num)
                break;
            while (num--) {
                v->u = uv->u + uo;
                v->v = uv->v + vo;
                uv = (const I3D_text_coor *)((const uchar *)uv + stride);
                v++;
            }
            break;
        }
        }
    }
}

// The copy loops walk their pointers, the parameters included, in place and count a copy of num down: VC6 then keeps
// the vertex pointer on a member (lea eax, [edx + 8]), as in SetUV
// FUNCTION: LS3DF 0x10037030
bool lod::Create(uint flags, uint num, const S_vector *pos, uint pos_stride, const I3D_text_coor *uv, uint uv_stride,
    const I3D_text_coor *uv2, uint uv2_stride, float uo, float vo)
{
    u_ofs = uo;
    v_ofs = vo;
    uint fvf = 0;
    uint format = flags & 7;
    uint stride;
    uint vf;
    if (m_HardwareCaps & HWCAPS_HWTL) {
        if (!vb.vb) {
            switch (format) {
            case 1:
                fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
                stride = sizeof(LVERTEX);
                vf = VF_LVERTEX;
                break;
            case 2:
            case 4:
                fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2;
                stride = sizeof(LVERTEX2);
                vf = VF_LVERTEX2;
                break;
            }
            uint usage = D3DUSAGE_WRITEONLY;
            if (!(flags & 0x400))
                usage = D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC;
            if (flags & 0x200)
                usage |= D3DUSAGE_SOFTWAREPROCESSING;
            if (!cVBMan.m_Alloc(fvf, usage, stride, num, vb, (VERTEX_FORMAT)vf, false)) {
                dbgPrintf("!! Out of memory  lod::Create()");
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return false;
            }
        }
        switch (format) {
        case 4: {
            LVERTEX2 *verts = (LVERTEX2 *)vb.buffer->Lock(vb.base, 0);
            if (flags & 0x100) {
                LVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->x = pos->x;
                    v->y = pos->y;
                    v->z = pos->z;
                    pos = (const S_vector *)((const uchar *)pos + pos_stride);
                    v++;
                }
            }
            if (uv) {
                LVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->u = uv->u + uo;
                    v->v = uv->v + vo;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            }
            if (uv2) {
                LVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->u2 = uv2->u;
                    v->v2 = uv2->v;
                    uv2 = (const I3D_text_coor *)((const uchar *)uv2 + uv2_stride);
                    v++;
                }
            }
            break;
        }
        case 2: {
            LVERTEX2 *verts = (LVERTEX2 *)vb.buffer->Lock(vb.base, 0);
            if (flags & 0x100) {
                LVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->x = pos->x;
                    v->y = pos->y;
                    v->z = pos->z;
                    pos = (const S_vector *)((const uchar *)pos + pos_stride);
                    v++;
                }
            }
            if (uv) {
                LVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->u = uv->u + uo;
                    v->v = uv->v + vo;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            }
            if (uv2) {
                LVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->u2 = uv2->u;
                    v->v2 = uv2->v;
                    uv2 = (const I3D_text_coor *)((const uchar *)uv2 + uv2_stride);
                    v++;
                }
            }
            break;
        }
        case 1: {
            LVERTEX *verts = (LVERTEX *)vb.buffer->Lock(vb.base, 0);
            if (flags & 0x100) {
                LVERTEX *v = verts;
                for (uint n = num; n; n--) {
                    v->x = pos->x;
                    v->y = pos->y;
                    v->z = pos->z;
                    pos = (const S_vector *)((const uchar *)pos + pos_stride);
                    v++;
                }
            }
            if (uv) {
                LVERTEX *v = verts;
                for (uint n = num; n; n--) {
                    v->u = uv->u + uo;
                    v->v = uv->v + vo;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            }
            break;
        }
        }
    } else {
        if (!vb.vb) {
            switch (format) {
            case 1:
                fvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1;
                stride = sizeof(TLVERTEX);
                vf = VF_TLVERTEX;
                break;
            case 2:
                fvf = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2;
                stride = sizeof(TLVERTEX2);
                vf = VF_TLVERTEX2;
                break;
            }
            uint usage = D3DUSAGE_WRITEONLY | D3DUSAGE_SOFTWAREPROCESSING;
            if (!(flags & 0x400))
                usage = D3DUSAGE_WRITEONLY | D3DUSAGE_SOFTWAREPROCESSING | D3DUSAGE_DYNAMIC;
            if (!cVBMan.m_Alloc(fvf, usage, stride, num, vb, (VERTEX_FORMAT)vf, false)) {
                dbgPrintf("!! Out of memory  lod::Create()");
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return false;
            }
        }
        switch (format) {
        case 2: {
            TLVERTEX2 *verts = (TLVERTEX2 *)vb.buffer->Lock(vb.base, 0);
            if (uv) {
                TLVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->u = uv->u + uo;
                    v->v = uv->v + vo;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            }
            if (uv2) {
                TLVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->u2 = uv2->u;
                    v->v2 = uv2->v;
                    uv2 = (const I3D_text_coor *)((const uchar *)uv2 + uv2_stride);
                    v++;
                }
            }
            {
                TLVERTEX2 *v = verts;
                for (uint n = num; n; n--) {
                    v->specular = 0;
                    v++;
                }
            }
            break;
        }
        case 1: {
            TLVERTEX *verts = (TLVERTEX *)vb.buffer->Lock(vb.base, 0);
            if (uv) {
                TLVERTEX *v = verts;
                for (uint n = num; n; n--) {
                    v->u = uv->u + uo;
                    v->v = uv->v + vo;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            }
            {
                TLVERTEX *v = verts;
                for (uint n = num; n; n--) {
                    v->specular = 0;
                    v++;
                }
            }
            break;
        }
        }
    }
    dirty = false;
    return true;
}

// FUNCTION: LS3DF 0x100378b0
void I3D_object::SetSpace(bool world)
{
    if (vis_flags & VISF_WORLD_SPACE) {
        if (world)
            return;
        vis_flags &= ~VISF_WORLD_SPACE;
        if (!mesh)
            return;
        DetachMesh();
        I3D_mesh_object *ms = mesh;
        S_matrix m = GetInvMatrix();
        for (uint i = 0; i < ms->num_lods; i++) {
            I3D_mesh_level *level = ms->GetLOD(i);
            if (!level)
                continue;
            I3D_vertex *v = level->LockVertices(0);
            for (int n = level->num_vertices; n; n--, v++) {
                v->pos *= m;
                v->normal = v->normal.RotateByMatrix(m);
                v->normal.Normalize();
            }
            UpdateVertices(i);
            level->UpdateBBox();
        }
        ms->UpdateBoundVolume();
    } else {
        if (!world)
            return;
        GetWBoundVolume();
        if (!mesh) {
            vis_flags = (vis_flags & ~VISF_WBOUND_VALID) | VISF_WORLD_SPACE;
            return;
        }
        {
            DetachMesh();
            I3D_mesh_object *ms = mesh;
            S_matrix m = GetRenderMatrix();
            for (uint i = 0; i < ms->num_lods; i++) {
                I3D_mesh_level *level = ms->GetLOD(i);
                if (!level)
                    continue;
                I3D_vertex *v = level->LockVertices(0);
                for (int n = level->num_vertices; n; n--, v++) {
                    v->pos *= m;
                    v->normal = v->normal.RotateByMatrix(m);
                    v->normal.Normalize();
                }
                UpdateVertices(i);
                level->UpdateBBox();
            }
            ms->UpdateBoundVolume();
        }
        vis_flags = (vis_flags & ~VISF_WBOUND_VALID) | VISF_WORLD_SPACE;
    }
}

// FUNCTION: LS3DF 0x10037d50
void I3D_object::SetLightness(float l)
{
    if (!mesh)
        return;
    if (IsAbsMrgZero(lightness - l))
        return;
    lightness = l;
    for (int i = mesh->num_lods; i--; ) {
        if (lods[i]) {
            lods[i]->lights_changed = (uint)-1;
            lods[i]->light_key = (uint)-1;
        }
    }
}

// FUNCTION: LS3DF 0x10037dd0
void I3D_object::SetTransparency(float t)
{
    if (!mesh)
        return;
    if (FloatToInt(transparency * 255.0f) == FloatToInt(t * 255.0f)) {
        transparency = t;
        return;
    }
    transparency = t;
    for (int i = mesh->num_lods; i--; ) {
        if (lods[i])
            lods[i]->alpha_dirty = true;
    }
}

// FUNCTION: LS3DF 0x10037e60
void I3D_object::ResetLightInfo()
{
    if (!mesh)
        return;
    for (int i = mesh->num_lods; i--; ) {
        if (lods[i]) {
            lods[i]->lights_changed = (uint)-1;
            lods[i]->light_key = (uint)-1;
        }
    }
}

// FUNCTION: LS3DF 0x10037ea0
void I3D_object::SetFade(float f)
{
    if (!mesh)
        return;
    if (FloatToInt(fade * 255.0f) == FloatToInt(f * 255.0f)) {
        fade = f;
        return;
    }
    fade = f;
    for (int i = mesh->num_lods; i--; ) {
        if (lods[i])
            lods[i]->alpha_dirty = true;
    }
}

// FUNCTION: LS3DF 0x10037f30
uchar *I3D_object::LockVertices(int index, uint flags)
{
    lod *l = lods[index];
    if (!l)
        return NULL;
    return l->vb.buffer->Lock(l->vb.base, 0);
}

// Queues the face groups of the level of detail the distance picks, after bringing its lod up to date: created with
// the level's vertices, the texture coordinates of the level shown (VISF_UNKNOWN_10), lit, its alpha, the environment
// map and, without hardware T&L, transformed and fogged. ProcessVertices is in both clipping branches and FogProcess
// called in each branch of the stride (the original pushes 0x28 and 0x20 in two arms, tail-merged): the copies' extra
// references to num_verts give the original's stack slots. The help struct's clip is assigned first, as the original
// stores it.
// Not matching yet: the distance's squares come out x, z, y, the original's z, y, x. That is the sum written out
// (S_vector d = ...; d.x * d.x + d.y * d.y + d.z * d.z), but its source then leaves the alpha Lock's push_back inline
// and the four PrepareLights calls unmerged; Dot and a GetWBoundVolume() without the reference give Square()'s order.
// FUNCTION: LS3DF 0x10038020
void I3D_object::m_AddFGroups()
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
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, 0);
    lod *l = lods[lod_index];
    bool created = false;
    if (!l || l->dirty || !l->vb.vb) {
        if (!l) {
            l = new lod;
            if (!l) {
                dbgPrintf("!! Out of memory    renderObject()");
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return;
            }
        }
        if (!l->Create((level->GetFlags() & MESHF_ENVMAP ? 2 : 1) | 0x500, level->num_vertices, &verts->pos,
                sizeof(I3D_vertex), &verts->tex, sizeof(I3D_vertex), NULL, 0, mesh->u_ofs, mesh->v_ofs)) {
            dbgPrintf("!! Out of memory    renderObject()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return;
        }
        lods[lod_index] = l;
        l->lights_changed = (uint)-1;
        l->light_key = (uint)-1;
        created = true;
        if (vis_flags & VISF_UNKNOWN_10)
            vis_flags |= VISF_LOD_CHANGED;
    }

    if (lod_index == mesh->num_lods - 1 && level->lod_dist > 0.0f) {
        float f = level->lod_dist * 0.5f;
        if (mesh->num_lods > 1)
            f += mesh->levels[lod_index - 1]->lod_dist * 0.5f;
        float s = RSLodQuality * world_scale;
        float d = 1.0f - (dist_2 / (s * s) - f) / (level->lod_dist - f);
        if (d > 1.0f)
            d = 1.0f;
        SetFade(d);
    } else
        SetFade(1.0f);

    bool moved;
    if (l->update_count != update_count) {
        moved = true;
        l->update_count = update_count;
    } else
        moved = false;
    int base = l->vb.base;
    IDirect3DVertexBuffer8 *dest = l->vb.vb;
    uchar *lv = NULL;
    if (vis_flags & VISF_LOD_CHANGED) {
        vis_flags &= ~VISF_LOD_CHANGED;
        lv = l->vb.buffer->Lock(base, 0);
        const I3D_text_coor *uv;
        uint uv_stride;
        if (cur_lod > 0) {
            uv = (const I3D_text_coor *)mesh->levels[cur_lod]->vb.buffer;
            uv_stride = sizeof(I3D_text_coor);
        } else {
            uv_stride = sizeof(I3D_vertex);
            uv = &verts->tex;
        }
        if (m_HardwareCaps & HWCAPS_HWTL) {
            if (level->GetFlags() & MESHF_ENVMAP) {
                LVERTEX2 *v = (LVERTEX2 *)lv;
                for (int n = num_verts; n; n--) {
                    v->u = uv->u;
                    v->v = uv->v;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            } else {
                LVERTEX *v = (LVERTEX *)lv;
                for (int n = num_verts; n; n--) {
                    v->u = uv->u;
                    v->v = uv->v;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            }
        } else {
            if (level->GetFlags() & MESHF_ENVMAP) {
                TLVERTEX2 *v = (TLVERTEX2 *)lv;
                for (int n = num_verts; n; n--) {
                    v->u = uv->u;
                    v->v = uv->v;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            } else {
                TLVERTEX *v = (TLVERTEX *)lv;
                for (int n = num_verts; n; n--) {
                    v->u = uv->u;
                    v->v = uv->v;
                    uv = (const I3D_text_coor *)((const uchar *)uv + uv_stride);
                    v++;
                }
            }
        }
    }
    if (!IsAbsMrgZero(mesh->u_ofs - l->u_ofs) || !IsAbsMrgZero(mesh->v_ofs - l->v_ofs))
        l->SetUV(level->GetFlags() & MESHF_ENVMAP ? 2 : 1, level->num_vertices, &verts->tex, sizeof(I3D_vertex),
            mesh->u_ofs, mesh->v_ofs);

    if (moved || l->lights_changed != AddFGHlpStruct.sector->lights_changed) {
        l->lights_changed = AddFGHlpStruct.sector->lights_changed;
        LightingSetSector(AddFGHlpStruct.sector);
        if (!lv)
            lv = l->vb.buffer->Lock(base, 0);
        if (m_HardwareCaps & HWCAPS_HWTL) {
            if (level->GetFlags() & MESHF_ENVMAP)
                PrepareLights((uchar *)&((LVERTEX2 *)lv)->color, sizeof(LVERTEX2), level, (const uchar *)verts,
                    sizeof(I3D_vertex), this, GetInvMatrix(), I3DLIGHTMODE_VERTEX, moved, l);
            else
                PrepareLights((uchar *)&((LVERTEX *)lv)->color, sizeof(LVERTEX), level, (const uchar *)verts,
                    sizeof(I3D_vertex), this, GetInvMatrix(), I3DLIGHTMODE_VERTEX, moved, l);
        } else {
            if (level->GetFlags() & MESHF_ENVMAP)
                PrepareLights((uchar *)&((TLVERTEX2 *)lv)->color, sizeof(TLVERTEX2), level, (const uchar *)verts,
                    sizeof(I3D_vertex), this, GetInvMatrix(), I3DLIGHTMODE_VERTEX, moved, l);
            else
                PrepareLights((uchar *)&((TLVERTEX *)lv)->color, sizeof(TLVERTEX), level, (const uchar *)verts,
                    sizeof(I3D_vertex), this, GetInvMatrix(), I3DLIGHTMODE_VERTEX, moved, l);
        }
    }
    if (l->alpha_dirty) {
        if (!lv)
            lv = l->vb.buffer->Lock(base, 0);
        if (m_HardwareCaps & HWCAPS_HWTL) {
            if (level->GetFlags() & MESHF_ENVMAP)
                SetVertexAlpha((uchar *)&((LVERTEX2 *)lv)->color, sizeof(LVERTEX2), level, this, l);
            else
                SetVertexAlpha((uchar *)&((LVERTEX *)lv)->color, sizeof(LVERTEX), level, this, l);
        } else {
            if (level->GetFlags() & MESHF_ENVMAP)
                SetVertexAlpha((uchar *)&((TLVERTEX2 *)lv)->color, sizeof(TLVERTEX2), level, this, l);
            else
                SetVertexAlpha((uchar *)&((TLVERTEX *)lv)->color, sizeof(TLVERTEX), level, this, l);
        }
    }

    if (level->GetFlags() & MESHF_ENVMAP && ((level->GetFlags() & MESHF_ENVMAP_MODE) < 2 || created)) {
        if (render_number != RenderNumberMinus1 || moved || renderCameraChanged ||
                !IsAbsMrgZero(mesh->env_u_ofs - l->env_u_ofs) || !IsAbsMrgZero(mesh->env_v_ofs - l->env_v_ofs)) {
            l->env_u_ofs = mesh->env_u_ofs;
            l->env_v_ofs = mesh->env_v_ofs;
            if (!lv)
                lv = l->vb.buffer->Lock(base, 0);
            if (m_HardwareCaps & HWCAPS_HWTL)
                EnvProcess(&((LVERTEX2 *)lv)->u2, sizeof(LVERTEX2), verts, sizeof(I3D_vertex), num_verts,
                    AddFGHlpStruct.cam_pos, GetMatrix(), level->env_scale, l->env_u_ofs, l->env_v_ofs,
                    level->GetFlags() & MESHF_ENVMAP_MODE, NULL);
            else
                EnvProcess(&((TLVERTEX2 *)lv)->u2, sizeof(TLVERTEX2), verts, sizeof(I3D_vertex), num_verts,
                    AddFGHlpStruct.cam_pos, GetMatrix(), level->env_scale, l->env_u_ofs, l->env_v_ofs,
                    level->GetFlags() & MESHF_ENVMAP_MODE, NULL);
        }
    }

    if ((renderCameraChanged || render_number != RenderNumberMinus1 || moved) && !(m_HardwareCaps & HWCAPS_HWTL)) {
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
            if (level->GetFlags() & MESHF_ENVMAP)
                FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z, sizeof(TLVERTEX2), num_verts,
                    AddFGHlpStruct.sector->fog_light, !(m_HardwareCaps & HWCAPS_SINGLEPASS));
            else
                FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z, sizeof(TLVERTEX), num_verts,
                    AddFGHlpStruct.sector->fog_light, !(m_HardwareCaps & HWCAPS_SINGLEPASS));
        }
    }

    RENDER_FG_HELP_STRUCT h;
    h.clip = frm_flags & FRMFLAGS_CLIP;
    h.sector = AddFGHlpStruct.sector;
    h.vis = this;
    for (I3D_face_group *fg = level->fgroups; fg; fg = fg->next) {
        I3D_material *mat = fg->material;
        h.fg = fg;
        if (mat && mat->flags & MATF_TEXTURE) {
            if (vis_flags & VISF_UNKNOWN_10)
                h.texture = mesh->levels[cur_lod]->fgroups->material->GetTexture();
            else
                h.texture = mat->GetTexture();
        } else
            h.texture = NULL;
        if (mat && mat->flags & MATF_ENVMAP) {
            if (vis_flags & VISF_UNKNOWN_10)
                h.env_texture = mesh->levels[cur_lod]->fgroups->material->GetEnvTexture();
            else
                h.env_texture = mat->GetEnvTexture();
        } else
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

// Renders a face group m_AddFGroups queued: the material's states, its texture and environment map (in one pass, or
// in two where the hardware can't blend them, without HWCAPS_SINGLEPASS), and a fog pass that renders the
// fog color over the face group with the vertices' fog in their alpha (SwapVertexBytes)
// The cull mode goes through render.h's SetCullState, the multipass's last SetColorOp is written out and the fog pass
// calls SwapVertexBytes in each branch of the stride (the original pushes 0x28 and 0x20 in two arms), as in
// I3D_billboard's. The out-of-line calls, and with them the registers, need the render-state helpers' asserts (I3D.h).
// FUNCTION: LS3DF 0x10039280
void I3D_object::m_RenderFGroup(RENDER_FG_HELP_STRUCT *h)
{
    I3D_face_group *fg = h->fg;
    I3D_material *mat = fg->material;
    lod *l;
    int num_verts;
    if (vis_flags & VISF_PROXY) {
        I3D_object *obj = (I3D_object *)user_data;
        if (mat && mat->flags & MATF_LOD0) {
            l = obj->lods[0];
            num_verts = mesh->levels[0]->num_vertices;
        } else {
            l = obj->lods[obj->cur_lod];
            num_verts = mesh->levels[obj->cur_lod]->num_vertices;
        }
    } else if (mat && mat->flags & MATF_LOD0) {
        l = lods[0];
        num_verts = mesh->levels[0]->num_vertices;
    } else {
        l = lods[cur_lod];
        num_verts = mesh->levels[cur_lod]->num_vertices;
    }
    IDirect3DVertexBuffer8 *vb = l->vb.vb;
    uint reset = 0;
    SetAlphaOp(D3DTOP_MODULATE);
    if (m_HardwareCaps & HWCAPS_SINGLEPASS) {
        if (!mat) {
            SetColorOp(m_Modulate2X);
            SetColorOp1(D3DTOP_DISABLE);
            SetAlphaState(0);
            m_pD3DDevice8->SetTexture(0, NULL);
        } else {
            if (h->env_texture) {
                if (h->texture) {
                    switch ((mat->flags >> 8) & 7) {
                    case 1:
                        SetColorOp1(D3DTOP_BLENDFACTORALPHA);
                        m_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, (uchar)(mat->env_alpha * 255.0f) << 24);
                        break;
                    case 2:
                        SetColorOp1(D3DTOP_BLENDCURRENTALPHA);
                        break;
                    case 3:
                        SetColorOp1(D3DTOP_MODULATE);
                        break;
                    case 4:
                        SetColorOp1(m_Modulate2X);
                        break;
                    case 5:
                        SetColorOp1(D3DTOP_ADD);
                        break;
                    default:
                        dbgPrintf("renderObject() - Undefined environment blend type: %i !!", (mat->flags >> 8) & 7);
                    }
                    m_pD3DDevice8->SetTexture(1, h->env_texture->m_pD3DTexture8);
                } else {
                    switch ((mat->flags >> 8) & 7) {
                    case 1:
                        SetColorOp(D3DTOP_BLENDFACTORALPHA);
                        m_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, (uchar)(mat->env_alpha * 255.0f) << 24);
                        break;
                    case 2:
                        SetColorOp(D3DTOP_BLENDCURRENTALPHA);
                        break;
                    case 3:
                        SetColorOp(D3DTOP_MODULATE);
                        break;
                    case 4:
                        SetColorOp(m_Modulate2X);
                        break;
                    case 5:
                        SetColorOp(D3DTOP_ADD);
                        break;
                    default:
                        dbgPrintf("renderObject() - Undefined environment blend type: %i !!", (mat->flags >> 8) & 7);
                    }
                    m_pD3DDevice8->SetTexture(0, h->env_texture->m_pD3DTexture8);
                    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
                    SetColorOp1(D3DTOP_DISABLE);
                    reset = 1;
                }
            } else
                SetColorOp1(D3DTOP_DISABLE);
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
            } else if (mat->flags & MATF_TXT_ALPHA || fade * transparency * mat->alpha < 1.0f) {
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
            SetCullState(mat->flags & MATF_TWO_SIDED ? D3DCULL_NONE : D3DCULL_CCW);
            if (h->texture) {
                SetColorOp(m_Modulate2X);
                m_pD3DDevice8->SetTexture(0, h->texture->m_pD3DTexture8);
            } else if (!h->env_texture) {
                SetColorOp(m_Modulate2X);
                m_pD3DDevice8->SetTexture(0, NULL);
            }
        }
        if (m_HardwareCaps & HWCAPS_HWTL)
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
        SetVertexFormat(l->vb.buffer->vertex_format);
        m_pD3DDevice8->SetStreamSource(0, vb, l->vb.buffer->stride);
        m_pD3DDevice8->SetIndices(fg->ib.ib, l->vb.base);
        m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
    } else {
        SetColorOp1(D3DTOP_DISABLE);
        int base = l->vb.base;
        C_VBuffer *buffer = l->vb.buffer;
        if (m_CurrFogState == TRUE)
            m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
        if (!mat) {
            SetColorOp(m_Modulate2X);
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
            } else if (mat->flags & MATF_TXT_ALPHA || fade * transparency * mat->alpha < 1.0f) {
                SetAlphaState(1);
                if (FloatToInt(mat->emissive.x) || FloatToInt(mat->emissive.y) || FloatToInt(mat->emissive.z))
                    SetBlendState(1);
                else
                    SetBlendState(0);
            } else if (mat->flags & MATF_CKEY_ALPHA) {
                SetAlphaState(3);
                SetBlendState(0);
            } else
                SetAlphaState(0);
            SetCullState(mat->flags & MATF_TWO_SIDED ? D3DCULL_NONE : D3DCULL_CCW);
            if (h->texture) {
                SetColorOp(m_Modulate2X);
                m_pD3DDevice8->SetTexture(0, h->texture->m_pD3DTexture8);
            } else if (h->env_texture && (!(m_ProblemFlags & 0x10) || (mat->flags & MATF_ENVMAP_BLEND) != 0x100) &&
                    (!(m_ProblemFlags & 0x20) || (mat->flags & MATF_ENVMAP_BLEND) != 0x200) &&
                    (!(m_ProblemFlags & 0x40) || (mat->flags & MATF_ENVMAP_BLEND) != 0x500)) {
                switch ((mat->flags >> 8) & 7) {
                case 1:
                    SetColorOp(D3DTOP_BLENDFACTORALPHA);
                    m_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, (uchar)(mat->env_alpha * 255.0f) << 24);
                    break;
                case 2:
                    SetColorOp(D3DTOP_BLENDCURRENTALPHA);
                    break;
                case 3:
                    SetColorOp(D3DTOP_MODULATE);
                    break;
                case 4:
                    SetColorOp(m_Modulate2X);
                    break;
                case 5:
                    SetColorOp(D3DTOP_ADD);
                    break;
                default:
                    dbgPrintf("renderObject() - Undefined environment blend type: %i !!", (mat->flags >> 8) & 7);
                }
                m_pD3DDevice8->SetTexture(0, h->env_texture->m_pD3DTexture8);
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
                reset |= 1;
            } else {
                if (m_CurrColorOp != m_Modulate2X) {
                    m_CurrColorOp = m_Modulate2X;
                    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLOROP, m_Modulate2X);
                }
                m_pD3DDevice8->SetTexture(0, NULL);
            }
        }
        SetVertexFormat(buffer->vertex_format);
        m_pD3DDevice8->SetStreamSource(0, vb, buffer->stride);
        m_pD3DDevice8->SetIndices(fg->ib.ib, base);
        m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
        if (h->env_texture && !(reset & 1) && (mat->flags & MATF_ENVMAP_BLEND) > 0x200) {
            // The environment map as a second pass
            m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
            SetAlphaState(1);
            switch ((mat->flags >> 8) & 7) {
            case 3:
                SetBlendState(3);
                break;
            case 4:
                SetBlendState(4);
                break;
            case 5:
                SetBlendState(2);
                break;
            default:
                dbgPrintf("renderObject() - Undefined environment blend type: %i !!", (mat->flags >> 8) & 7);
            }
            m_pD3DDevice8->SetTexture(0, h->env_texture->m_pD3DTexture8);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
            reset |= 1;
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
            m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        }
        if (m_CurrFogState == TRUE) {
            // The fog pass: the fog color, blended by the fog the vertices' specular alpha holds
            m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
            SetAlphaOp(D3DTOP_SELECTARG2);
            SetColorOp(D3DTOP_SELECTARG2);
            m_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, m_CurrFogColor);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            SetAlphaState(1);
            SetBlendState(0);
            const I3D_triface *faces = fg->GetFaces();
            uchar *v = buffer->Lock(base, 0);
            if (buffer->vertex_format == VF_TLVERTEX2)
                SwapVertexBytes(v + 0x13, v + 0x17, sizeof(TLVERTEX2), faces, fg->num_faces, 0);
            else
                SwapVertexBytes(v + 0x13, v + 0x17, sizeof(TLVERTEX), faces, fg->num_faces, 0);
            fg->UnlockFaces();
            buffer->Unlock();
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
            faces = fg->GetFaces();
            v = buffer->Lock(base, 0);
            if (buffer->vertex_format == VF_TLVERTEX2)
                SwapVertexBytes(v + 0x13, v + 0x17, sizeof(TLVERTEX2), faces, fg->num_faces, 1);
            else
                SwapVertexBytes(v + 0x13, v + 0x17, sizeof(TLVERTEX), faces, fg->num_faces, 1);
            fg->UnlockFaces();
            buffer->Unlock();
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        }
    }
    if (reset & 1)
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    if (reset & 2)
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    if (reset & 4)
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
}

// FUNCTION: LS3DF 0x1003a400
void I3D_object::LoadImage()
{
    I3D_visual::LoadImage();
    I3D_mesh_object *m = LoadMeshImage();
    SetMesh(m, false);
    if (m)
        m->Release();
}

