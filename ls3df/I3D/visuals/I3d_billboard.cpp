// The billboards and lens flares (PS2 I3d_billboard.cpp), at 0x10005db0-0x1000b0c0 in the original: I3D_billboard and
// I3D_lensflare in PS2's order. The marker $E (0x10008390) comes before the billboard's m_RenderFGroup, and the copies
// of the render state helpers the linker kept follow the lens flare's Draw. Names are the PS2 build's unless noted.
// Draw's frame is 4 bytes larger in the original. The kept copies come in the original's order
// only with SetBlendState between SetAlphaState and SetColorOp in the header the driver sees, and the driver then
// inlines it into its draws whatever its size (16 dead statements didn't stop it); ours also come due earlier (after
// the billboard's m_RenderFGroup, in the original after Draw), the headers' inline content again (see autopch.py).
// Patch 1.1's (0x10005da0-0x1000ac60) differs in m_AddFGroups's ProcessVertices and in the buffers' locks.
#include "I3D/I3D.h"
#include "I3D/render/render.h"
#include "I3D/render/VBManager.h"
#include "IGraph/IGraph.h"
#include "dta.h"
#include <mmsystem.h>

// The type first, as in the other visuals' constructors: only that order gives the original's stores
// FUNCTION: LS3DF 0x10005db0
// FUNCTION: LS3DF11 0x10005da0
I3D_billboard::I3D_billboard()
{
    visual_type = I3D_VISUAL_BILLBOARD;
    rot_axis = 2;
    axis_mode = 0;
    env_dummy = NULL;
}

// Empty, but defined here: the original's comes right after the constructor's scalar deleting destructor, where an
// inline one, in the precompiled header, would be deferred (/YX)
// FUNCTION: LS3DF 0x10005e10
// FUNCTION: LS3DF11 0x10005e00
I3D_billboard::~I3D_billboard()
{
}

void I3D_billboard::SetSpace(bool world)
{
}

// The mesh's box made a cube around the mesh's extremes (the vertices turn), and its sphere
// FUNCTION: LS3DF 0x10005e40
// FUNCTION: LS3DF11 0x10005e20
void I3D_billboard::UpdateBoundProc()
{
    vis_flags |= VISF_BOUND_VALID;
    if (mesh && mesh->bvol.bbox.IsValid()) {
        float mn = mesh->bvol.bbox.min.x;
        for (int i = 1; i < 3; i++) {
            if (mn > (&mesh->bvol.bbox.min.x)[i])
                mn = (&mesh->bvol.bbox.min.x)[i];
        }
        float mx = mesh->bvol.bbox.max.x;
        for (int j = 1; j < 3; j++) {
            if (mx < (&mesh->bvol.bbox.max.x)[j])
                mx = (&mesh->bvol.bbox.max.x)[j];
        }
        vis_bound.bbox.min = S_vector(mn, mn, mn);
        vis_bound.bbox.max = S_vector(mx, mx, mx);
        vis_bound.bsphere = mesh->bvol.bsphere;
    } else
        vis_bound = mesh->bvol;
}

// FUNCTION: LS3DF 0x10005f40
// FUNCTION: LS3DF11 0x10005f20
void I3D_billboard::SetMesh(I3D_mesh_object *m, bool keep_lods)
{
    I3D_object::SetMesh(m, keep_lods);
    UpdateBoundProc();
}

// FUNCTION: LS3DF 0x10005f60
// FUNCTION: LS3DF11 0x10005f40
LS3D_RESULT I3D_billboard::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_VISUAL) {
        if (frame->GetType() == FRAME_EMITOR)
            SetMesh(((I3D_emitor *)frame)->mesh, true);
        return I3D_OK;
    }
    I3D_billboard *bb = (I3D_billboard *)frame;
    vis_flags = bb->vis_flags & 0x2ffd0;
    vis_bound = bb->vis_bound;
    if (bb->vis_flags & VISF_WORLD_SPACE)
        vis_bound_w = bb->GetWBoundVolume();
    else
        vis_flags &= ~VISF_WBOUND_VALID;
    switch (bb->visual_type) {
    case I3D_VISUAL_BILLBOARD:
        rot_axis = bb->rot_axis;
        axis_mode = bb->axis_mode;
    case I3D_VISUAL_OBJECT:
    case I3D_VISUAL_LIT_OBJECT:
    case I3D_VISUAL_SINGLEMESH:
    case I3D_VISUAL_LENSF:
    case I3D_VISUAL_MORPH:
    case I3D_VISUAL_SINGLEMORPH:
        SetMesh(bb->mesh, false);
        return I3D_OK;
    }
    return I3DERR_GENERIC;
}

// The world matrix turned to the camera at cam_pos into m: modes 0 and 1 turn the billboard's local axes to the
// camera's direction in local space, around the rot_axis (mode 1) or also tilted to it (0); mode 2 takes the camera's
// axes scaled by the billboard's, mode 3 the local matrix times the camera's. The rows a rotation keeps are copied with
// assembly. Modes 0 and 1 write the rotation times GetRenderMatrix() out element by element, where PS2 builds the
// rotation with Identity() and multiplies; the normalized vectors are PS2's (a = v; a.y = 0.0f; a.Normalize(), VC6
// folding the zero). The sums under e's square roots are written z first: the squares are then the ones a.Normalize()
// computed, kept on the FPU stack as in the original (x first reorders both). Mode 2's scale is GetWorldScale(): a
// conditional expression copies through a temporary, 12 bytes more frame.
// Not matching yet: the order VC6 loads the two terms of a product sum in (a.x's term first in the original, a.z's in
// ours; writing them the other way round changes nothing there but reorders the rest of the function), e.Normalize's
// sum in two cases, a factor order in mode 1 and in the inlined UpdateMatrix of mode 3. 64 combinations of the square
// roots' term orders and e's initialization (constructor or assignment) don't do better.
// FUNCTION: LS3DF 0x10006090
// FUNCTION: LS3DF11 0x10006070
void I3D_billboard::ProductBillMatrix(S_vector &cam_pos, S_matrix *m)
{
    switch (axis_mode) {
    case 0:
        switch (rot_axis) {
        case 0: {
            S_vector v = cam_pos * GetInvMatrix();
            S_vector a = v;
            a.y = 0.0f;
            a.Normalize();
            S_vector e((float)sqrt(v.z * v.z + v.x * v.x), v.y, 0.0f);
            e.Normalize();
            const S_matrix &mat = GetRenderMatrix();
            (*m)(0, 0) = e.y * mat(1, 0) + (a.x * mat(0, 0) + a.z * mat(2, 0)) * e.x;
            (*m)(0, 1) = e.y * mat(1, 1) + (a.x * mat(0, 1) + a.z * mat(2, 1)) * e.x;
            (*m)(0, 2) = e.y * mat(1, 2) + (a.x * mat(0, 2) + a.z * mat(2, 2)) * e.x;
            (*m)(0, 3) = e.y * mat(1, 3) + (a.x * mat(0, 3) + a.z * mat(2, 3)) * e.x;
            (*m)(1, 0) = e.x * mat(1, 0) - (a.x * mat(0, 0) + a.z * mat(2, 0)) * e.y;
            (*m)(1, 1) = e.x * mat(1, 1) - (a.x * mat(0, 1) + a.z * mat(2, 1)) * e.y;
            (*m)(1, 2) = e.x * mat(1, 2) - (a.x * mat(0, 2) + a.z * mat(2, 2)) * e.y;
            (*m)(1, 3) = e.x * mat(1, 3) - (a.x * mat(0, 3) + a.z * mat(2, 3)) * e.y;
            (*m)(2, 0) = a.x * mat(2, 0) - a.z * mat(0, 0);
            (*m)(2, 1) = a.x * mat(2, 1) - a.z * mat(0, 1);
            (*m)(2, 2) = a.x * mat(2, 2) - a.z * mat(0, 2);
            (*m)(2, 3) = a.x * mat(2, 3) - a.z * mat(0, 3);
            __asm {
                pushf
                cld
                mov esi, mat
                add esi, 0x30
                mov edi, m
                add edi, 0x30
                mov ecx, 4
                rep movsd
                popf
            }
            break;
        }
        case 1: {
            S_vector v = cam_pos * GetInvMatrix();
            S_vector e;
            e = S_vector((float)sqrt(v.z * v.z + v.x * v.x), v.y, 0.0f);
            e.Normalize();
            S_vector a = v;
            a.y = 0.0f;
            a.Normalize();
            const S_matrix &mat = GetRenderMatrix();
            (*m)(0, 0) = (a.x * mat(0, 0) + a.z * mat(2, 0)) * e.y - e.x * mat(1, 0);
            (*m)(0, 1) = (a.x * mat(0, 1) + a.z * mat(2, 1)) * e.y - e.x * mat(1, 1);
            (*m)(0, 2) = (a.x * mat(0, 2) + a.z * mat(2, 2)) * e.y - e.x * mat(1, 2);
            (*m)(0, 3) = (a.x * mat(0, 3) + a.z * mat(2, 3)) * e.y - e.x * mat(1, 3);
            (*m)(1, 0) = (a.x * mat(0, 0) + a.z * mat(2, 0)) * e.x + e.y * mat(1, 0);
            (*m)(1, 1) = (a.x * mat(0, 1) + a.z * mat(2, 1)) * e.x + e.y * mat(1, 1);
            (*m)(1, 2) = (a.x * mat(0, 2) + a.z * mat(2, 2)) * e.x + e.y * mat(1, 2);
            (*m)(1, 3) = (a.x * mat(0, 3) + a.z * mat(2, 3)) * e.x + e.y * mat(1, 3);
            (*m)(2, 0) = a.x * mat(2, 0) - a.z * mat(0, 0);
            (*m)(2, 1) = a.x * mat(2, 1) - a.z * mat(0, 1);
            (*m)(2, 2) = a.x * mat(2, 2) - a.z * mat(0, 2);
            (*m)(2, 3) = a.x * mat(2, 3) - a.z * mat(0, 3);
            __asm {
                pushf
                cld
                mov esi, mat
                add esi, 0x30
                mov edi, m
                add edi, 0x30
                mov ecx, 4
                rep movsd
                popf
            }
            break;
        }
        case 2: {
            S_vector v = cam_pos * GetInvMatrix();
            S_vector a = v;
            a.y = 0.0f;
            a.Normalize();
            S_vector e((float)sqrt(v.z * v.z + v.x * v.x), v.y, 0.0f);
            e.Normalize();
            const S_matrix &mat = GetRenderMatrix();
            (*m)(0, 0) = a.x * mat(2, 0) - a.z * mat(0, 0);
            (*m)(0, 1) = a.x * mat(2, 1) - a.z * mat(0, 1);
            (*m)(0, 2) = a.x * mat(2, 2) - a.z * mat(0, 2);
            (*m)(0, 3) = a.x * mat(2, 3) - a.z * mat(0, 3);
            (*m)(1, 0) = e.x * mat(1, 0) - (a.x * mat(0, 0) + a.z * mat(2, 0)) * e.y;
            (*m)(1, 1) = e.x * mat(1, 1) - (a.x * mat(0, 1) + a.z * mat(2, 1)) * e.y;
            (*m)(1, 2) = e.x * mat(1, 2) - (a.x * mat(0, 2) + a.z * mat(2, 2)) * e.y;
            (*m)(1, 3) = e.x * mat(1, 3) - (a.x * mat(0, 3) + a.z * mat(2, 3)) * e.y;
            (*m)(2, 0) = -(e.y * mat(1, 0)) - (a.x * mat(0, 0) + a.z * mat(2, 0)) * e.x;
            (*m)(2, 1) = -(e.y * mat(1, 1)) - (a.x * mat(0, 1) + a.z * mat(2, 1)) * e.x;
            (*m)(2, 2) = -(e.y * mat(1, 2)) - (a.x * mat(0, 2) + a.z * mat(2, 2)) * e.x;
            (*m)(2, 3) = -(e.y * mat(1, 3)) - (a.x * mat(0, 3) + a.z * mat(2, 3)) * e.x;
            __asm {
                pushf
                cld
                mov esi, mat
                add esi, 0x30
                mov edi, m
                add edi, 0x30
                mov ecx, 4
                rep movsd
                popf
            }
            break;
        }
        }
        break;
    case 1:
        switch (rot_axis) {
        case 0: {
            const S_matrix &inv = GetInvMatrix();
            S_vector d(0.0f, cam_pos.x * inv(0, 1) + cam_pos.y * inv(1, 1) + cam_pos.z * inv(2, 1) + inv(3, 1),
                cam_pos.x * inv(0, 2) + cam_pos.y * inv(1, 2) + cam_pos.z * inv(2, 2) + inv(3, 2));
            d.Normalize();
            const S_matrix &mat = GetRenderMatrix();
            __asm {
                pushf
                cld
                mov esi, mat
                mov edi, m
                mov ecx, 4
                rep movsd
                add esi, 0x20
                add edi, 0x20
                mov ecx, 4
                rep movsd
                popf
            }
            (*m)(1, 0) = d.y * mat(2, 0) - d.z * mat(1, 0);
            (*m)(1, 1) = d.y * mat(2, 1) - d.z * mat(1, 1);
            (*m)(1, 2) = d.y * mat(2, 2) - d.z * mat(1, 2);
            (*m)(1, 3) = d.y * mat(2, 3) - d.z * mat(1, 3);
            (*m)(2, 0) = -(d.y * mat(1, 0)) - d.z * mat(2, 0);
            (*m)(2, 1) = -(d.y * mat(1, 1)) - d.z * mat(2, 1);
            (*m)(2, 2) = -(d.y * mat(1, 2)) - d.z * mat(2, 2);
            (*m)(2, 3) = -(d.y * mat(1, 3)) - d.z * mat(2, 3);
            break;
        }
        case 1: {
            const S_matrix &inv = GetInvMatrix();
            S_vector a(cam_pos.x * inv(0, 0) + cam_pos.y * inv(1, 0) + cam_pos.z * inv(2, 0) + inv(3, 0), 0.0f,
                cam_pos.x * inv(0, 2) + cam_pos.y * inv(1, 2) + cam_pos.z * inv(2, 2) + inv(3, 2));
            a.Normalize();
            const S_matrix &mat = GetRenderMatrix();
            (*m)(0, 0) = a.x * mat(2, 0) - a.z * mat(0, 0);
            (*m)(0, 1) = a.x * mat(2, 1) - a.z * mat(0, 1);
            (*m)(0, 2) = a.x * mat(2, 2) - a.z * mat(0, 2);
            (*m)(0, 3) = a.x * mat(2, 3) - a.z * mat(0, 3);
            __asm {
                pushf
                cld
                mov esi, mat
                add esi, 0x10
                mov edi, m
                add edi, 0x10
                mov ecx, 4
                rep movsd
                add esi, 0x10
                add edi, 0x10
                mov ecx, 4
                rep movsd
                popf
            }
            (*m)(2, 0) = -(a.x * mat(0, 0)) - a.z * mat(2, 0);
            (*m)(2, 1) = -(a.x * mat(0, 1)) - a.z * mat(2, 1);
            (*m)(2, 2) = -(a.x * mat(0, 2)) - a.z * mat(2, 2);
            (*m)(2, 3) = -(a.x * mat(0, 3)) - a.z * mat(2, 3);
            break;
        }
        case 2: {
            const S_matrix &inv = GetInvMatrix();
            S_vector d(cam_pos.x * inv(0, 0) + cam_pos.y * inv(1, 0) + cam_pos.z * inv(2, 0) + inv(3, 0),
                cam_pos.x * inv(0, 1) + cam_pos.y * inv(1, 1) + cam_pos.z * inv(2, 1) + inv(3, 1), 0.0f);
            d.Normalize();
            const S_matrix &mat = GetRenderMatrix();
            (*m)(0, 0) = d.y * mat(0, 0) - d.x * mat(1, 0);
            (*m)(0, 1) = d.y * mat(0, 1) - d.x * mat(1, 1);
            (*m)(0, 2) = d.y * mat(0, 2) - d.x * mat(1, 2);
            (*m)(0, 3) = d.y * mat(0, 3) - d.x * mat(1, 3);
            (*m)(1, 0) = d.x * mat(0, 0) + d.y * mat(1, 0);
            (*m)(1, 1) = d.x * mat(0, 1) + d.y * mat(1, 1);
            (*m)(1, 2) = d.x * mat(0, 2) + d.y * mat(1, 2);
            (*m)(1, 3) = d.x * mat(0, 3) + d.y * mat(1, 3);
            __asm {
                pushf
                cld
                mov esi, mat
                add esi, 0x20
                mov edi, m
                add edi, 0x20
                mov ecx, 8
                rep movsd
                popf
            }
            break;
        }
        }
        break;
    case 2: {
        *m = AddFGHlpStruct.camera->view_inv;
        S_vector s = GetWorldScale();
        S_vector r1 = (*m)(1);
        (*m)(0) *= s.x;
        (*m)(1) = (*m)(2) * s.y;
        (*m)(2) = r1 * s.z;
        (*m)(3) = GetMatrix()(3);
        break;
    }
    case 3:
        *m = GetLocalMatrix() * AddFGHlpStruct.camera->view_inv;
        (*m)(3) = GetMatrix()(3);
        break;
    }
}

// Like I3D_object's, with the matrix turned to the camera, which the environment map and the transformation take
// too; a mode 5 environment map maps into the parent's first dummy child. The fade adds the previous level's whole
// distance (fadd without the fmul by 0.5), and FogProcess is called in each branch of the stride (push 0x28 and push
// 0x20 in two arms, as the original's); the help structure is filled in the original's store order.
// 1.2's lod::Create gets num_verts, where 1.1's reads level->num_vertices again (as ours there): that gives 1.2's flag
// test in memory and lod_index copied, not read twice.
// Not matching yet: the stack slots (this, lod_index, num_verts and the Lock's push_back temporary sit elsewhere), and
// the count argument: the original loads it from [level + 8] before the constant pushes, ours from num_verts' slot,
// so 1.2's argument is still another spelling.
// FUNCTION: LS3DF 0x100073c0
// FUNCTION: LS3DF11 0x100073a0
void I3D_billboard::m_AddFGroups()
{
    if (!mesh)
        return;
    if (vis_flags & VISF_PROXY) {
        I3D_billboard *vis = (I3D_billboard *)m_UserData;
        rot_axis = vis->rot_axis;
        axis_mode = vis->axis_mode;
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
        ProductBillMatrix(AddFGHlpStruct.cam_pos, &g_matrix);
        return;
    }
    const I3D_bvolume &bvol = GetWBoundVolume();
    float dist_2 = (bvol.bsphere.pos - AddFGHlpStruct.cam_pos).Square();
    int lod_index = GetObjectLOD(this, dist_2);
    I3D_mesh_level *level = mesh->levels[lod_index];
    if (!level)
        return;
    int num_verts = level->num_vertices;
    if (!num_verts)
        return;
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, BUFFER_LOCK_FLAGS);
    lod *l = lods[lod_index];
    if (!l || l->dirty || !l->vb.vb) {
        if (!l) {
            l = new lod;
            if (!l) {
                dbgPrintf("!! Out of memory    renderObject()");
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return;
            }
        }
#if ENGINE_VERSION >= 395
        if (!l->Create((level->GetFlags() & MESHF_ENVMAP ? 2 : 1) | 0x500, num_verts, &verts->pos,
                sizeof(I3D_vertex), &verts->tex, sizeof(I3D_vertex), NULL, 0, mesh->u_ofs, mesh->v_ofs)) {
#else
        if (!l->Create((level->GetFlags() & MESHF_ENVMAP ? 2 : 1) | 0x500, level->num_vertices, &verts->pos,
                sizeof(I3D_vertex), &verts->tex, sizeof(I3D_vertex), NULL, 0, mesh->u_ofs, mesh->v_ofs)) {
#endif
            dbgPrintf("!! Out of memory    renderObject()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return;
        }
        lods[lod_index] = l;
        l->lights_changed = (uint)-1;
        l->light_key = (uint)-1;
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
    uchar *lv = NULL;
    int base = l->vb.base;
    IDirect3DVertexBuffer8 *dest = l->vb.vb;
    if (!IsAbsMrgZero(mesh->u_ofs - l->u_ofs) || !IsAbsMrgZero(mesh->v_ofs - l->v_ofs))
        l->SetUV(level->GetFlags() & MESHF_ENVMAP ? 2 : 1, level->num_vertices, &verts->tex, sizeof(I3D_vertex),
            mesh->u_ofs, mesh->v_ofs);
    ProductBillMatrix(AddFGHlpStruct.cam_pos, &g_matrix);

    if (moved || renderCameraChanged || l->lights_changed != AddFGHlpStruct.sector->lights_changed) {
        l->lights_changed = AddFGHlpStruct.sector->lights_changed;
        LightingSetSector(AddFGHlpStruct.sector);
        if (!lv)
            lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
        S_matrix m = g_matrix;
        m.Inverse(S_matrix(m));
        if (m_HardwareCaps & HWCAPS_HWTL) {
            if (level->GetFlags() & MESHF_ENVMAP)
                PrepareLights((uchar *)&((LVERTEX2 *)lv)->color, sizeof(LVERTEX2), level, (const uchar *)verts,
                    sizeof(I3D_vertex), this, m, I3DLIGHTMODE_VERTEX, moved || renderCameraChanged, l);
            else
                PrepareLights((uchar *)&((LVERTEX *)lv)->color, sizeof(LVERTEX), level, (const uchar *)verts,
                    sizeof(I3D_vertex), this, m, I3DLIGHTMODE_VERTEX, moved || renderCameraChanged, l);
        } else {
            if (level->GetFlags() & MESHF_ENVMAP)
                PrepareLights((uchar *)&((TLVERTEX2 *)lv)->color, sizeof(TLVERTEX2), level, (const uchar *)verts,
                    sizeof(I3D_vertex), this, m, I3DLIGHTMODE_VERTEX, moved || renderCameraChanged, l);
            else
                PrepareLights((uchar *)&((TLVERTEX *)lv)->color, sizeof(TLVERTEX), level, (const uchar *)verts,
                    sizeof(I3D_vertex), this, m, I3DLIGHTMODE_VERTEX, moved || renderCameraChanged, l);
        }
    }
    if (l->alpha_dirty) {
        if (!lv)
            lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
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

    if (level->GetFlags() & MESHF_ENVMAP) {
        if ((level->GetFlags() & MESHF_ENVMAP_MODE) == 5 && !env_dummy && m_pParent) {
            env_dummy = m_pParent->GetChild(0);
            while (env_dummy && env_dummy->m_Type != FRAME_DUMMY)
                env_dummy = env_dummy->m_pNext;
        }
        if (m_RenderNumber != RenderNumberMinus1 || moved || renderCameraChanged ||
                !IsAbsMrgZero(mesh->env_u_ofs - l->env_u_ofs) || !IsAbsMrgZero(mesh->env_v_ofs - l->env_v_ofs)) {
            l->env_u_ofs = mesh->env_u_ofs;
            l->env_v_ofs = mesh->env_v_ofs;
            if (!lv)
                lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
            if (m_HardwareCaps & HWCAPS_HWTL)
                EnvProcess(&((LVERTEX2 *)lv)->u2, sizeof(LVERTEX2), verts, sizeof(I3D_vertex), num_verts,
                    AddFGHlpStruct.cam_pos, g_matrix, level->env_scale, l->env_u_ofs, l->env_v_ofs,
                    level->GetFlags() & MESHF_ENVMAP_MODE, env_dummy);
            else
                EnvProcess(&((TLVERTEX2 *)lv)->u2, sizeof(TLVERTEX2), verts, sizeof(I3D_vertex), num_verts,
                    AddFGHlpStruct.cam_pos, g_matrix, level->env_scale, l->env_u_ofs, l->env_v_ofs,
                    level->GetFlags() & MESHF_ENVMAP_MODE, env_dummy);
        }
    }

    if ((renderCameraChanged || m_RenderNumber != RenderNumberMinus1 || moved) && !(m_HardwareCaps & HWCAPS_HWTL)) {
        if (m_FrmFlags & FRMFLAGS_CLIP) {
            m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, &g_matrix);
            level->vb.buffer->Unlock();
            SetVertexFormat(VF_VERTEX);
            SetClipping(TRUE);
#if ENGINE_VERSION < 395
            // 1.1 calls it in each branch: VC6 merges the two, but their references weigh in the frame (dest's slot)
            m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
#endif
        } else {
            m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, &g_matrix);
            level->vb.buffer->Unlock();
            SetVertexFormat(VF_VERTEX);
            SetClipping(FALSE);
#if ENGINE_VERSION < 395
            m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
#endif
        }
#if ENGINE_VERSION >= 395
        m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
#endif
        if (!(m_HardwareCaps & HWCAPS_TABLEFOG)) {
            if (!lv)
                lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
            if (level->GetFlags() & MESHF_ENVMAP)
                FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z, sizeof(TLVERTEX2), num_verts,
                    AddFGHlpStruct.sector->fog_light, !(m_HardwareCaps & HWCAPS_SINGLEPASS));
            else
                FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z, sizeof(TLVERTEX), num_verts,
                    AddFGHlpStruct.sector->fog_light, !(m_HardwareCaps & HWCAPS_SINGLEPASS));
        }
    }

    RENDER_FG_HELP_STRUCT h;
    h.clip = m_FrmFlags & FRMFLAGS_CLIP;
    h.vis = this;
    h.sector = AddFGHlpStruct.sector;
    for (I3D_face_group *fg = level->fgroups; fg; fg = fg->next) {
        I3D_material *mat = fg->material;
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

// Like I3D_object's, rendered with g_matrix: the cull mode set once (none for a proxy, else by the material), and no
// z writes for rot_axis 4. Both cull modes go through render.h's SetCullState (the proxy's folds to the written-out
// test, but spends the inline budget the original spent). The fog pass calls SwapVertexBytes in each branch of the
// stride: the original pushes 0x28 or 0x20 in two arms where the conditional expression gives sete/lea.
// The multipass switch calls cases 4 and 5's SetColorOp out of line as the original does only with the render-state
// helpers' asserts (I3D.h). 1.1's fog pass locks with D3DLOCK_NOSYSLOCK, which its out-of-line Lock takes as an
// argument; the constant also gives the texture factor of the single pass's first blend switch 1.1's registers.
// FUNCTION: LS3DF 0x100083b0
// FUNCTION: LS3DF11 0x10008030
void I3D_billboard::m_RenderFGroup(RENDER_FG_HELP_STRUCT *h)
{
    I3D_face_group *fg = h->fg;
    I3D_material *mat = fg->material;
    lod *l;
    int num_verts;
    if (vis_flags & VISF_PROXY) {
        I3D_object *obj = (I3D_object *)m_UserData;
        l = obj->lods[obj->cur_lod];
        SetCullState(D3DCULL_NONE);
        num_verts = mesh->levels[obj->cur_lod]->num_vertices;
    } else {
        l = lods[cur_lod];
        SetCullState(mat && mat->flags & MATF_TWO_SIDED ? D3DCULL_NONE : D3DCULL_CCW);
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
                        dbgPrintf("renderBillboard() - Undefined environment blend type: %i !!", (mat->flags >> 8) & 7);
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
                        dbgPrintf("renderBillboard() - Undefined environment blend type: %i !!", (mat->flags >> 8) & 7);
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
                if ((*(int *)&mat->emissive.x || *(int *)&mat->emissive.y || *(int *)&mat->emissive.z) &&
                        *(int *)&mat->diffuse.x && *(int *)&mat->diffuse.y && *(int *)&mat->diffuse.z)
                    SetBlendState(1);
                else
                    SetBlendState(0);
            } else if (mat->flags & MATF_CKEY_ALPHA) {
                SetAlphaState(3);
                SetBlendState(0);
            } else
                SetAlphaState(0);
            if (h->texture) {
                SetColorOp(m_Modulate2X);
                m_pD3DDevice8->SetTexture(0, h->texture->m_pD3DTexture8);
            } else if (!h->env_texture) {
                SetColorOp(m_Modulate2X);
                m_pD3DDevice8->SetTexture(0, NULL);
            }
        }
        if (m_HardwareCaps & HWCAPS_HWTL)
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, &g_matrix);
        if (rot_axis == 4)
            m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
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
                if ((*(int *)&mat->emissive.x || *(int *)&mat->emissive.y || *(int *)&mat->emissive.z) &&
                        *(int *)&mat->diffuse.x && *(int *)&mat->diffuse.y && *(int *)&mat->diffuse.z)
                    SetBlendState(1);
                else
                    SetBlendState(0);
            } else if (mat->flags & MATF_CKEY_ALPHA) {
                SetAlphaState(3);
                SetBlendState(0);
            } else
                SetAlphaState(0);
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
                    dbgPrintf("renderBillboard() - Undefined environment blend type: %i !!", (mat->flags >> 8) & 7);
                }
                m_pD3DDevice8->SetTexture(0, h->env_texture->m_pD3DTexture8);
                m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
                reset |= 1;
            } else {
                // Written out: the original calls cases 4 and 5's SetColorOp out of line but inlines this one (and so
                // does I3D_object's), which an equal call after them can't give
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
                dbgPrintf("renderBillboard() - Undefined environment blend type: %i !!", (mat->flags >> 8) & 7);
            }
            m_pD3DDevice8->SetTexture(0, h->env_texture->m_pD3DTexture8);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
            reset |= 1;
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
            m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        }
        if (m_CurrFogState == TRUE) {
            // The fog pass, as I3D_object's
            m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
            SetAlphaOp(D3DTOP_SELECTARG2);
            SetColorOp(D3DTOP_SELECTARG2);
            m_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, m_CurrFogColor);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            SetAlphaState(1);
            SetBlendState(0);
            const I3D_triface *faces = fg->GetFaces();
            uchar *v = buffer->Lock(base, BUFFER_LOCK_FLAGS);
            if (buffer->vertex_format == VF_TLVERTEX2)
                SwapVertexBytes(v + 0x13, v + 0x17, sizeof(TLVERTEX2), faces, fg->num_faces, 0);
            else
                SwapVertexBytes(v + 0x13, v + 0x17, sizeof(TLVERTEX), faces, fg->num_faces, 0);
            fg->UnlockFaces();
            buffer->Unlock();
            m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
            faces = fg->GetFaces();
            v = buffer->Lock(base, BUFFER_LOCK_FLAGS);
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
    if (rot_axis == 4)
        m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    if (reset & 1)
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    if (reset & 2)
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    if (reset & 4)
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
}

// The axis and the mode's low byte (5 bytes): an axis out of range gives axis 0 in mode 2, a mode out of range mode 1
// FUNCTION: LS3DF 0x100094d0
// FUNCTION: LS3DF11 0x10009150
void I3D_billboard::LoadImage()
{
    I3D_object::LoadImage();
    dtaRead(LoadImageHandle, &rot_axis, 5);
    if (rot_axis > 2) {
        rot_axis = 0;
        axis_mode = 2;
    }
    if (axis_mode > 3)
        axis_mode = 1;
}

// FUNCTION: LS3DF 0x10009520
// FUNCTION: LS3DF11 0x100091a0
I3D_lensflare::I3D_lensflare()
{
    visual_type = I3D_VISUAL_LENSF;
    unknown_224 = 0;
    test_interval = 150;
    last_check_col_time = 0;
    last_update = 0;
    visibility = 0.0f;
    target_visibility = 1.0f;
    target_color = S_vector(1.0f, 1.0f, 1.0f);
    color = S_vector(1.0f, 1.0f, 1.0f);
    size = 1.0f;
    intensity = 1.0f;
    fade_in = 15.0f;
    fade_out = 8.0f;
    range = 300.0f;
    near_dist = 0.0f;
    far_dist = 0.0f;
    near_scale = 1.0f;
}

// FUNCTION: LS3DF 0x10009630
// FUNCTION: LS3DF11 0x100092b0
I3D_lensflare::~I3D_lensflare()
{
    int i = glows.size();
    if (i) {
        GLOW_STRUCT *g = &glows[0];
        while (i--) {
            if (g->mat)
                g->mat->Release();
            g++;
        }
    }
}

// FUNCTION: LS3DF 0x100096e0
// FUNCTION: LS3DF11 0x10009370
LS3D_RESULT I3D_lensflare::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_VISUAL)
        return I3D_OK;
    I3D_lensflare *lf = (I3D_lensflare *)frame;
    if (lf->vis_flags & VISF_WORLD_SPACE)
        vis_flags |= VISF_WORLD_SPACE;
    if (lf->visual_type != I3D_VISUAL_LENSF)
        return I3D_OK;
    test_interval = lf->test_interval;
    near_scale = lf->near_scale;
    size = lf->size;
    fade_in = lf->fade_in;
    fade_out = lf->fade_out;
    range = lf->range;
    near_dist = lf->near_dist;
    far_dist = lf->far_dist;
    glows = lf->glows;
    vis_flags = lf->vis_flags & 0x2ffd0;
    vis_bound = lf->vis_bound;
    if (lf->vis_flags & VISF_WORLD_SPACE)
        vis_bound_w = lf->GetWBoundVolume();
    else
        vis_flags &= ~VISF_WBOUND_VALID;
    int i = glows.size();
    if (i) {
        GLOW_STRUCT *g = &glows[0];
        while (i--) {
            if (g->mat)
                g->mat->AddRef();
            g++;
        }
    }
    return I3D_OK;
}

// Draws the glows, faded by the occlusion test (made with test, every test_interval ms): a single glow on the light,
// sized by the distance, or several along the light's direction from a point in front of the camera, the last one
// enlarged and the others faded by the angle to the view direction. Spellings the original's code shows: windows.h's
// min and max, the constant first where the original loads it first (max(1.0f, ...)); the viewport's half sizes in
// parentheses; the time step multiplied in each arm of the ternary; if (time - last_update);
// target_color.Dot(target_color); each vertex's color assigned first and the four specular colors last. The single
// glow's position is a local of its own (p): with pos's address taken by the out-of-line operator*=, VC6 kept pos in
// memory and made ebx the zero register from the start, where the original keeps pos.x and pos.y in ebp and ebx and
// zeroes ebx only after timeGetTime. SetTexture is called in each branch (the original pushes in two arms); sx and sy
// are declared once for both branches (same code, closer slots).
// Not matching yet: the frame (4 bytes larger in the original, its slots sorted otherwise), the order VC6 sums the
// squares of dir's and pos's Normalize in (the original's x first lets it hoist fld pos.x above the glow count test;
// no spelling of the else branch or of Normalize moved it), the loop's visibility * size operand order and the order
// of some stores (the single glow's first vertex color first).
// FUNCTION: LS3DF 0x10009890
// FUNCTION: LS3DF11 0x10009520
void I3D_lensflare::Draw(bool test)
{
    S_vector pos = GetMatrix()(3);
    S_vector cam_pos = pCurrentCamera->GetMatrix()(3);
    if (!glows.size())
        return;
    S_vector dir;
    float dist;
    int sx, sy;
    if (glows.size() == 1) {
        dir = pos - cam_pos;
        dist = (float)sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (dist > range) {
            visibility = 0.0f;
            return;
        }
    } else {
        dir = pos;
        dir.Normalize();
        dir *= 1000.0f;
    }
    uint time = timeGetTime();
    if (test) {
        if (time - last_check_col_time >= test_interval) {
            target_color = S_vector(1.0f, 1.0f, 1.0f);
            last_check_col_time = time;
            target_visibility = 1.0f;
            if (pCurrentScene->TestColGeo(cam_pos, dir, 0x61030000, NULL, NULL, NULL, &target_color, TexelCallback,
                    m_pParent, 0) && IsMrgZeroLess(target_color.Dot(target_color)))
                target_visibility = 0.0f;
        }
    } else
        target_visibility = 0.0f;
    float m = max(target_color.x, max(target_color.y, target_color.z));
    if (m > 1.0)
        target_color *= 1.0f / m;
    if (time - last_update) {
        float f = (IsMrgZeroLess(target_visibility) ? (time - last_update) * fade_out : (time - last_update) * fade_in) * 0.001f;
        last_update = time;
        if (visibility < target_visibility)
            visibility = min(visibility + f, target_visibility);
        else if (visibility > target_visibility)
            visibility = max(visibility - f, target_visibility);
        if (color.x < target_color.x)
            color.x = min(color.x + f, target_color.x);
        else if (color.x > target_color.x)
            color.x = max(color.x - f, target_color.x);
        if (color.y < target_color.y)
            color.y = min(color.y + f, target_color.y);
        else if (color.y > target_color.y)
            color.y = max(color.y - f, target_color.y);
        if (color.z < target_color.z)
            color.z = min(color.z + f, target_color.z);
        else if (color.z > target_color.z)
            color.z = max(color.z - f, target_color.z);
    }
    if (IsMrgZeroLess(visibility))
        return;
    if (glows.size() == 1) {
        S_vector p = GetWBoundVolume().bsphere.pos;
        I3D_material *mat = glows[0].mat;
        if (!mat->texture)
            sx = sy = 64;
        else {
            sx = mat->texture->m_Width;
            sy = mat->texture->m_Height;
        }
        float s;
        if (dist < near_dist)
            s = max(1.0f, (1.0f - dist / near_dist) * near_scale);
        else if (dist > far_dist)
            s = 1.0f - (dist - far_dist) / (range - far_dist);
        else
            s = 1.0f;
        float k = size * visibility;
        float w = sx * k * s;
        float h = sy * k * s;
        const S_matrix &vp = *pViewProjMatrix;
        float pw = p.x * vp(0, 3) + p.y * vp(1, 3) + p.z * vp(2, 3) + vp(3, 3);
        if (pw <= 0.0f) {
            visibility = 0.0f;
            return;
        }
        float rw = 1.0f / pw;
        p *= vp;
        p.x = (p.x * rw + 1.0f) * (m_D3DViewPort8.Width * 0.5f) + m_D3DViewPort8.X;
        p.y = (1.0f - p.y * rw) * (m_D3DViewPort8.Height * 0.5f) + m_D3DViewPort8.Y;
        float x0 = p.x - w * 0.5f;
        float y0 = p.y - h * 0.5f;
        float x1 = p.x + w * 0.5f;
        float y1 = p.y + h * 0.5f;
        uint c = FloatToInt(intensity * visibility * 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 |
            FloatToInt(color.y * 255.0f) << 8 | FloatToInt(color.z * 255.0f);
        TLVERTEX v[4];
        v[0].color = c;
        v[0].x = x0;
        v[0].y = y0;
        v[0].z = 0.0f;
        v[0].rhw = 1.0f;
        v[0].u = 0.0f;
        v[0].v = 0.0f;
        v[1].color = c;
        v[1].x = x1;
        v[1].y = y0;
        v[1].z = 0.0f;
        v[1].rhw = 1.0f;
        v[1].u = 1.0f;
        v[1].v = 0.0f;
        v[2].color = c;
        v[2].x = x0;
        v[2].y = y1;
        v[2].z = 0.0f;
        v[2].rhw = 1.0f;
        v[2].u = 0.0f;
        v[2].v = 1.0f;
        v[3].color = c;
        v[3].x = x1;
        v[3].y = y1;
        v[3].z = 0.0f;
        v[3].rhw = 1.0f;
        v[3].u = 1.0f;
        v[3].v = 1.0f;
        v[0].specular = 0;
        v[1].specular = 0;
        v[2].specular = 0;
        v[3].specular = 0;
        if (mat->texture)
            m_pD3DDevice8->SetTexture(0, mat->GetTexture()->m_pD3DTexture8);
        else
            m_pD3DDevice8->SetTexture(0, NULL);
        SetVertexFormat(VF_TLVERTEX);
        m_pD3DDevice8->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(TLVERTEX));
        return;
    }
    pos.Normalize();
    S_vector cam_dir = pCurrentCamera->GetMatrix()(2);
    cam_dir.Normalize();
    float half_fov = pCurrentCamera->fov_a * 0.5f;
    float fade = (float)max(0.0, (1.0 - acos(cam_dir.Dot(pos)) / half_fov) * 0.8 + 0.2f);
    S_vector center = cam_dir * 2.0f + cam_pos;
    for (int i = glows.size(); i--; ) {
        S_vector p = pos * glows[i].pos + center;
        I3D_material *mat = glows[i].mat;
        if (!mat->texture)
            sx = sy = 64;
        else {
            sx = mat->texture->m_Width;
            sy = mat->texture->m_Height;
        }
        float k = visibility * size;
        float w = sx * k;
        float h = sy * k;
        if (!i) {
            w *= fade + 1.0f;
            h *= fade + 1.0f;
        }
        const S_matrix &vp = *pViewProjMatrix;
        float pw = p.x * vp(0, 3) + p.y * vp(1, 3) + p.z * vp(2, 3) + vp(3, 3);
        if (pw <= 0.0f)
            continue;
        float rw = 1.0f / pw;
        p *= vp;
        p.x = (p.x * rw + 1.0f) * (m_D3DViewPort8.Width * 0.5f) + m_D3DViewPort8.X;
        p.y = (1.0f - p.y * rw) * (m_D3DViewPort8.Height * 0.5f) + m_D3DViewPort8.Y;
        // Halved in place: the halves stay on the FPU stack for x1 and y1 (written w * 0.5f four times, as in the
        // single glow, VC6 adds them from memory)
        w *= 0.5f;
        float x0 = p.x - w;
        h *= 0.5f;
        float y0 = p.y - h;
        float x1 = p.x + w;
        float y1 = p.y + h;
        uint c = FloatToInt((!i ? 1.0f : fade) * intensity * visibility * 255.0f) << 24 |
            FloatToInt(color.x * 255.0f) << 16 | FloatToInt(color.y * 255.0f) << 8 | FloatToInt(color.z * 255.0f);
        TLVERTEX v[4];
        v[0].color = c;
        v[0].x = x0;
        v[0].y = y0;
        v[0].z = 0.0f;
        v[0].rhw = 1.0f;
        v[0].u = 0.0f;
        v[0].v = 0.0f;
        v[1].color = c;
        v[1].x = x1;
        v[1].y = y0;
        v[1].z = 0.0f;
        v[1].rhw = 1.0f;
        v[1].u = 1.0f;
        v[1].v = 0.0f;
        v[2].color = c;
        v[2].x = x0;
        v[2].y = y1;
        v[2].z = 0.0f;
        v[2].rhw = 1.0f;
        v[2].u = 0.0f;
        v[2].v = 1.0f;
        v[3].color = c;
        v[3].x = x1;
        v[3].y = y1;
        v[3].z = 0.0f;
        v[3].rhw = 1.0f;
        v[3].u = 1.0f;
        v[3].v = 1.0f;
        v[0].specular = 0;
        v[1].specular = 0;
        v[2].specular = 0;
        v[3].specular = 0;
        if (mat->texture)
            m_pD3DDevice8->SetTexture(0, mat->GetTexture()->m_pD3DTexture8);
        else
            m_pD3DDevice8->SetTexture(0, NULL);
        SetVertexFormat(VF_TLVERTEX);
        m_pD3DDevice8->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(TLVERTEX));
    }
}

// The glows (their places and materials) and a small box
// FUNCTION: LS3DF 0x1000ac90
// FUNCTION: LS3DF11 0x1000a920
void I3D_lensflare::LoadImage()
{
    I3D_frame::LoadImage();
    uint num = 0;
    dtaRead(LoadImageHandle, &num, 1);
    if (!num)
        return;
    while (num--) {
        GLOW_STRUCT g;
        dtaRead(LoadImageHandle, &g.pos, 4);
        int id = 0;
        dtaRead(LoadImageHandle, &id, 2);
        if (id > LoadMaterialu) {
            dbgPrintf("!! Invalid material ID:%d  materialu:%d  I3D_lensflare::LoadImage() %s", id, LoadMaterialu,
                ImageFileName);
            ImageErrFlg = I3DERR_FILECORRUPTED;
            g.mat = NULL;
        } else {
            g.mat = LoadMaterialy[id - 1];
            g.mat->AddRef();
        }
        glows.push_back(g);
    }
    vis_bound.bsphere.radius = 0.01f;
    vis_bound.bbox.min = S_vector(-0.05f, -0.05f, -0.05f);
    vis_bound.bbox.max = S_vector(0.05f, 0.05f, 0.05f);
}
