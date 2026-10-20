// The morphs (PS2 I3d_morph.cpp), at 0x10034370-0x100367b0 in the original: morphBase from its constructor (PS2's
// BogusConstructor), then I3D_morph, then MorphDef's and morph_region's functions, in PS2's order, with the PC's second
// UpdateMorphLod. The header's zero vector has its $E after m_AddFGroups (0x10036520). Names are the PS2 build's
// unless noted.
#include "I3D/I3D.h"
#include "I3D/render.h"
#include "I3D/VBManager.h"
#include "IGraph/IGraph.h"
#include "dta.h"
#include <malloc.h>

// FUNCTION: LS3DF 0x10034370
morphBase::morphBase()
{
    flags = MORPHF_DIRTY | MORPHF_UNKNOWN_4;
    m_numLevels = 0;
    m_numRegions = 0;
    weights = NULL;
    norm_weights = NULL;
    m_pMorphDef = NULL;
    unknown_18 = NULL;
}

// FUNCTION: LS3DF 0x10034390
void morphBase::Clear()
{
    flags = (flags & ~MORPHF_LOADED) | MORPHF_DIRTY;
    m_numLevels = 0;
    m_numRegions = 0;
    delete[] weights;
    weights = NULL;
    delete unknown_18;
    unknown_18 = NULL;
    if (m_pMorphDef) {
        m_pMorphDef->Release();
        m_pMorphDef = NULL;
    }
}

// FUNCTION: LS3DF 0x10034460
LS3D_RESULT morphBase::SetWeightB(int region, int level, float weight)
{
    weights[region * m_numLevels + level] = weight;
    flags |= MORPHF_DIRTY;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10034490
float morphBase::GetWeightB(int region, int level) const
{
    return weights[region * m_numLevels + level];
}

// The region's weights are addressed twice (&weights[base]): VC6 keeps the address in a register of its own and copies
// it into each loop's pointer, where one pointer walked in both loops shares the register
// FUNCTION: LS3DF 0x100344b0
void morphBase::NormalizeWeights()
{
    int base = 0;
    for (int i = m_numRegions; i--; ) {
        float sum = 0.0f;
        const float *p = &weights[base];
        for (int j = m_numLevels; j--; )
            sum += (float)fabs(*p++);
        if (sum < MRG_ZERO) {
            float *nw = &norm_weights[base];
            *nw++ = 1.0f;
            for (j = m_numLevels - 1; j; j--)
                *nw++ = 0.0f;
        } else {
            float f = 1.0f / sum;
            float *nw = &norm_weights[base];
            const float *q = &weights[base];
            for (j = m_numLevels; j--; ) {
                *nw = *q * f;
                q++;
                nw++;
            }
        }
        base += m_numLevels;
    }
}

// A region's indices, data and count are three locals declared in that order: the count in the for statement, or any
// other order, moves the loads. Not matching yet: two fmuls take their operands the other way round (the normal's x
// before the loop, the position's x in it), which goes by the allocator's live range ids (tools/c2trace.py); the second
// UpdateMorphLod has the second of them.
// FUNCTION: LS3DF 0x10034570
void morphBase::UpdateMorphLod(int lod, I3D_mesh_object *mesh)
{
    if (!(flags & MORPHF_DIRTY) || !(flags & MORPHF_LOADED))
        return;
    NormalizeWeights();
    if (!m_pMorphDef)
        return;
    morph_region *r = NULL;
    if (lod < m_pMorphDef->num_lods)
        r = &m_pMorphDef->regions[lod * m_pMorphDef->num_regions];
    if (!r || !mesh)
        return;
    I3D_mesh_level *level = mesh->GetLOD(lod);
    if (!level)
        return;
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, 0);
    const float *w = norm_weights;
    for (int i = m_numRegions; i--; r++) {
        const ushort *ind = r->indices;
        const S_vector *d = r->verts;
        int n = r->num_verts;
        for (; n--; ind++) {
            S_vector pos, normal;
            pos = d[0] * w[0];
            normal = d[1] * w[0];
            d += 2;
            const float *wp = w + 1;
            for (int j = m_numLevels - 1; j; j--, wp++, d += 2) {
                if (*wp > MRG_ZERO) {
                    pos += d[0] * *wp;
                    normal += d[1] * *wp;
                }
            }
            I3D_vertex &v = verts[*ind];
            v.pos = pos;
            v.normal = normal;
        }
        w += m_numLevels;
    }
    flags &= ~MORPHF_DIRTY;
}

// PC only (the name is the first one's): the positions go into a lod's vertices too
// FUNCTION: LS3DF 0x10034810
void morphBase::UpdateMorphLod(void *pos_out, uint stride, int lod, I3D_mesh_object *mesh)
{
    if (!(flags & MORPHF_DIRTY) || !(flags & MORPHF_LOADED))
        return;
    NormalizeWeights();
    if (!m_pMorphDef)
        return;
    morph_region *r = NULL;
    if (lod < m_pMorphDef->num_lods)
        r = &m_pMorphDef->regions[lod * m_pMorphDef->num_regions];
    if (!r || !mesh)
        return;
    I3D_mesh_level *level = mesh->GetLOD(lod);
    if (!level)
        return;
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, 0);
    const float *w = norm_weights;
    for (int i = m_numRegions; i--; r++) {
        const ushort *ind = r->indices;
        const S_vector *d = r->verts;
        int n = r->num_verts;
        for (; n--; ind++) {
            S_vector pos, normal;
            pos = d[0] * w[0];
            normal = d[1] * w[0];
            d += 2;
            const float *wp = w + 1;
            for (int j = m_numLevels - 1; j; j--, wp++, d += 2) {
                if (*wp > MRG_ZERO) {
                    pos += d[0] * *wp;
                    normal += d[1] * *wp;
                }
            }
            I3D_vertex &v = verts[*ind];
            v.pos = pos;
            v.normal = normal;
            *(S_vector *)((uchar *)pos_out + *ind * stride) = pos;
        }
        w += m_numLevels;
    }
    flags &= ~MORPHF_DIRTY;
}

// FUNCTION: LS3DF 0x10034ad0
LS3D_RESULT morphBase::DuplicateMorphDef(morphBase *mb)
{
    if (!mb->m_pMorphDef) {
        dbgPrintf("!! No m_pMorphDef  morphBase::DuplicateMorphDef()");
        return I3DERR_GENERIC;
    }
    if (m_pMorphDef)
        m_pMorphDef->Release();
    m_pMorphDef = mb->m_pMorphDef;
    m_pMorphDef->ref++;
    m_numLevels = mb->m_numLevels;
    m_numRegions = mb->m_numRegions;
    delete[] weights;
    weights = new float[m_numLevels * m_numRegions * 2];
    if (!weights) {
        dbgPrintf("!! Out of memory  morphBase::DuplicateMorphDef()");
        return I3DERR_OUTOFMEM;
    }
    norm_weights = weights + m_numLevels * m_numRegions;
    memcpy(weights, mb->weights, m_numLevels * m_numRegions * sizeof(float));
    return I3D_OK;
}

// The levels (a byte), the regions (a byte) and the lods (a byte), then each region of each lod: its vertices (a word),
// their positions and normals in every level and, if a byte says so, their indices; the weights start on the first
// level. The bound volume comes last.
// FUNCTION: LS3DF 0x10034bd0
void morphBase::LoadImageB()
{
    dtaRead(LoadImageHandle, &m_numLevels, 1);
    if (!m_numLevels)
        return;
    dtaRead(LoadImageHandle, &m_numRegions, 1);
    int num_lods = 0;
    dtaRead(LoadImageHandle, &num_lods, 1);
    m_pMorphDef = new MorphDef(num_lods, m_numRegions);
    if (!m_pMorphDef) {
        dbgPrintf("!! Out of memory  morphBase::LoadImageB()");
        m_numLevels = 0;
        m_numRegions = 0;
        ImageErrFlg = I3DERR_OUTOFMEM;
        return;
    }
    morph_region *r = NULL;
    if (m_pMorphDef->num_lods > 0)
        r = m_pMorphDef->regions;
    for (int i = m_numRegions * num_lods; i--; r++) {
        r->num_verts = 0;
        dtaRead(LoadImageHandle, &r->num_verts, 2);
        if (r->num_verts) {
            r->verts = (S_vector *)_aligned_malloc(m_numLevels * r->num_verts * 2 * sizeof(S_vector), 16);
            if (!r->verts) {
                dbgPrintf("!! Out of memory  morphBase::LoadImageB()");
                m_numLevels = 0;
                m_numRegions = 0;
                ImageErrFlg = I3DERR_OUTOFMEM;
                return;
            }
            dtaRead(LoadImageHandle, r->verts, r->num_verts * m_numLevels * 2 * sizeof(S_vector));
            bool has_indices;
            dtaRead(LoadImageHandle, &has_indices, 1);
            if (has_indices) {
                r->indices = new ushort[r->num_verts];
                if (!r->indices) {
                    dbgPrintf("!! Out of memory  morphBase::LoadImageB()");
                    m_numLevels = 0;
                    m_numRegions = 0;
                    ImageErrFlg = I3DERR_OUTOFMEM;
                    return;
                }
                dtaRead(LoadImageHandle, r->indices, r->num_verts * sizeof(ushort));
            } else
                r->indices = NULL;
        }
    }
    weights = new float[m_numRegions * m_numLevels * 2];
    if (!weights) {
        dbgPrintf("!! Out of memory  morphBase::LoadImageB()");
        m_numLevels = 0;
        m_numRegions = 0;
        ImageErrFlg = I3DERR_OUTOFMEM;
        return;
    }
    norm_weights = weights + m_numLevels * m_numRegions;
    float *w = weights;
    for (i = m_numRegions; i > 0; i--) {
        *w++ = 1.0f;
        for (int j = m_numLevels - 1; j > 0; j--)
            *w++ = 0.0f;
    }
    flags |= MORPHF_LOADED;
    dtaRead(LoadImageHandle, &m_pMorphDef->bvol, sizeof(I3D_bvolume));
}

// FUNCTION: LS3DF 0x10034e30
I3D_morph::I3D_morph()
{
    visual_type = I3D_VISUAL_MORPH;
}

// Empty, but defined here: the original's comes right after the constructor's scalar deleting destructor, where an
// inline one, in the precompiled header, would be deferred (/YX)
// FUNCTION: LS3DF 0x10034eb0
I3D_morph::~I3D_morph()
{
}

void I3D_morph::SetSpace(bool world)
{
}

// FUNCTION: LS3DF 0x10034fe0
int I3D_morph::NumLevels() const
{
    return m_numLevels;
}

// FUNCTION: LS3DF 0x10034ff0
int I3D_morph::NumRegions() const
{
    return m_numRegions;
}

// FUNCTION: LS3DF 0x10035000
LS3D_RESULT I3D_morph::SetWeight(int region, int level, float weight)
{
    if (region >= 0 && region < m_numRegions && level >= 0 && level < m_numLevels) {
        update_count++;
        SetWeightB(region, level, weight);
        // Both branches the same, as PS2's (VC6 merges them after loading the flags; Duplicate has it too)
        if (!(vis_flags & VISF_WORLD_SPACE))
            vis_flags &= ~VISF_WBOUND_VALID;
        else
            vis_flags &= ~VISF_WBOUND_VALID;
        return I3D_OK;
    }
    dbgPrintf("!! index out of range  I3D_morph::SetWeight()  this:0x%x  %s  region:%d m_numRegions:%d  level:%d "
        "m_numLevels:%d", this, GetName(), region, m_numRegions, level, m_numLevels);
    return I3DERR_INVALIDPARAMS;
}

// FUNCTION: LS3DF 0x100350b0
float I3D_morph::GetWeight(int region, int level) const
{
    if (region >= 0 && region < m_numRegions && level >= 0 && level < m_numLevels)
        return GetWeightB(region, level);
    dbgPrintf("!! index out of range  I3D_morph::GetWeight()  this:0x%x  %s  region:%d m_numRegions:%d  level:%d "
        "m_numLevels:%d", this, GetName(), region, m_numRegions, level, m_numLevels);
    return 0.0f;
}

// FUNCTION: LS3DF 0x10035130
void I3D_morph::SetMesh(I3D_mesh_object *m, bool keep_lods)
{
    I3D_object::SetMesh(m, keep_lods);
    if (m_pMorphDef)
        vis_bound = m_pMorphDef->bvol;
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
}

// From a morph, a copy of its mesh that shares its levels; from another visual with a mesh (or a morph's proxy),
// that mesh; from an emitter, its particles' mesh. PS2's returns for anything but a visual, the PC's takes the emitters
// inside that test, as the objects' and billboards' do. The inline accessors after the second Clear (GetVisualType, two
// GetMesh) keep Release out of line in both Clears, as the original has it; without them VC6 inlines it.
// FUNCTION: LS3DF 0x10035180
LS3D_RESULT I3D_morph::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
    else
        vis_flags &= ~VISF_WBOUND_VALID;
    I3D_frame::Duplicate(frame);
    flags &= ~(MORPHF_LOADED | MORPHF_UNKNOWN_4);
    if (frame->GetType() != FRAME_VISUAL) {
        if (frame->GetType() == FRAME_EMITOR) {
            memset(lods, 0, sizeof(lods));
            Clear();
            SetMesh(((I3D_emitor *)frame)->mesh, true);
            delete unknown_18;
            unknown_18 = NULL;
        }
        return I3D_OK;
    }
    memset(lods, 0, sizeof(lods));
    Clear();
    I3D_object *obj = (I3D_object *)frame;
    switch (obj->GetVisualType()) {
    case I3D_VISUAL_MORPH:
    case I3D_VISUAL_SINGLEMORPH:
        if (!(vis_flags & VISF_PROXY)) {
            LS3D_RESULT r = DuplicateMorphDef((I3D_morph *)frame);
            if (r < 0) {
                SetMesh(obj->GetMesh(), false);
                return r;
            }
            I3D_mesh_object *m = pI3DDriver->CreateMesh();
            if (!m) {
                dbgPrintf("!! Out of memory  I3D_morph::Duplicate()");
                return I3DERR_OUTOFMEM;
            }
            r = m->Duplicate(obj->mesh);
            if (r < 0) {
                m->Release();
                return r;
            }
            flags |= MORPHF_LOADED;
            SetMesh(m, false);
            m->Release();
            lightness = obj->lightness;
            transparency = obj->transparency;
            break;
        }
    case I3D_VISUAL_OBJECT:
    case I3D_VISUAL_LIT_OBJECT:
    case I3D_VISUAL_BILLBOARD:
    case I3D_VISUAL_SINGLEMESH:
    case I3D_VISUAL_LENSF:
        SetMesh(obj->GetMesh(), false);
        lightness = obj->lightness;
        transparency = obj->transparency;
        break;
    default:
        return I3DERR_GENERIC;
    }
    vis_flags = obj->vis_flags & 0x2ffd0;
    vis_bound = obj->vis_bound;
    if (obj->vis_flags & VISF_WORLD_SPACE)
        vis_bound_w = obj->GetWBoundVolume();
    else
        vis_flags &= ~VISF_WBOUND_VALID;
    delete unknown_18;
    unknown_18 = NULL;
    return I3D_OK;
}

// I3D_object::m_AddFGroups without the lods that follow the textures loaded (VISF_UNKNOWN_10): the vertices are
// summed into the lod's when the weights changed, which lights it again
// Not matching yet, as I3D_object::m_AddFGroups: the original keeps this in ebx, uses no ebp and has one more stack
// slot, and the blocks are laid out after that (the lod's allocation after the first Lock's first part).
// FUNCTION: LS3DF 0x100354f0
void I3D_morph::m_AddFGroups()
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
    lod *l = lods[lod_index];
    if (!l || l->dirty) {
        if (!l) {
            l = new lod;
            if (!l) {
                dbgPrintf("!! Out of memory  I3D_morph::m_AddFGroups()");
                return;
            }
        }
        lods[lod_index] = l;
    }
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, 0);
    if (!l->vb.vb) {
        if (!l->Create((level->GetFlags() & MESHF_ENVMAP ? 2 : 1) | 0x500, level->num_vertices, &verts->pos,
                sizeof(I3D_vertex), &verts->tex, sizeof(I3D_vertex), NULL, 0, mesh->u_ofs, mesh->v_ofs)) {
            dbgPrintf("!! Out of memory    renderObject()");
            return;
        }
        l->lights_changed = (uint)-1;
        l->light_key = (uint)-1;
    }

    if (lod_index == mesh->num_lods - 1 && level->lod_dist > 0.0f) {
        float f = level->lod_dist * 0.5f;
        if (mesh->num_lods > 1)
            f += mesh->levels[lod_index - 1]->lod_dist;
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
    if (!IsAbsMrgZero(mesh->u_ofs - l->u_ofs) || !IsAbsMrgZero(mesh->v_ofs - l->v_ofs))
        l->SetUV(level->GetFlags() & MESHF_ENVMAP ? 2 : 1, level->num_vertices, &verts->tex, sizeof(I3D_vertex),
            mesh->u_ofs, mesh->v_ofs);

    if (moved) {
        lv = l->vb.buffer->Lock(base, 0);
        if (m_HardwareCaps & HWCAPS_HWTL) {
            if (level->GetFlags() & MESHF_ENVMAP)
                UpdateMorphLod(lv, sizeof(LVERTEX2), lod_index, mesh);
            else
                UpdateMorphLod(lv, sizeof(LVERTEX), lod_index, mesh);
        } else {
            if (level->GetFlags() & MESHF_ENVMAP)
                UpdateMorphLod(lv, sizeof(TLVERTEX2), lod_index, mesh);
            else
                UpdateMorphLod(lv, sizeof(TLVERTEX), lod_index, mesh);
        }
    }
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

    if (level->GetFlags() & MESHF_ENVMAP) {
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
        } else {
            m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, &GetRenderMatrix());
            level->vb.buffer->Unlock();
            SetVertexFormat(VF_VERTEX);
            SetClipping(FALSE);
        }
        m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
        if (!(m_HardwareCaps & HWCAPS_TABLEFOG)) {
            if (!lv)
                lv = l->vb.buffer->Lock(base, 0);
            FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z,
                level->GetFlags() & MESHF_ENVMAP ? sizeof(TLVERTEX2) : sizeof(TLVERTEX), num_verts,
                AddFGHlpStruct.sector->fog_light, !(m_HardwareCaps & HWCAPS_SINGLEPASS));
        }
    }

    RENDER_FG_HELP_STRUCT h;
    h.sector = AddFGHlpStruct.sector;
    h.clip = frm_flags & FRMFLAGS_CLIP;
    h.vis = this;
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
        if (!mat || !h.texture && !(mat->alpha * fade * transparency < 1.0f)) {
            h.sort_value = 0;
            AddFGHlpStruct.num_opaque++;
        } else if (mat->flags & (MATF_TXT_ALPHA | MATF_ADD_MODE) || mat->alpha * fade * transparency < 1.0f) {
            h.sort_value = (FloatToInt(sort_dist) & 0x3fffffff) | 0x80000000;
            AddFGHlpStruct.num_transparent++;
        } else {
            if (mat->flags & MATF_CKEY_ALPHA)
                h.sort_value = ((h.texture ? h.texture->uid : 0) & 0x7fff) | 0x40000000;
            else
                h.sort_value = (h.texture ? h.texture->uid : 0) & 0x7fff;
            AddFGHlpStruct.num_opaque++;
        }
        AddFGHlpStruct.fgroups.push_back(h);
    }
}

// PC only. After the marker: the morph's file defines it after m_AddFGroups, and Clear inlines it all the same.
// FUNCTION: LS3DF 0x10036540
S_morph_unknown::~S_morph_unknown()
{
    for (int i = 0; i < 8; i++) {
        if (unknown_00[i])
            unknown_00[i]->Release();
    }
    for (i = 0; i < 16; i++) {
        if (unknown_20[i])
            unknown_20[i]->Release();
    }
}

// FUNCTION: LS3DF 0x10036580
MorphDef::MorphDef(int lods, int regs)
{
    ref = 1;
    if (lods > 0 && regs > 0) {
        regions = new morph_region[lods * regs];
        if (!regions) {
            dbgPrintf("!! Out of memory  MorphDef constructor");
            num_lods = 0;
            num_regions = 0;
        } else {
            num_lods = lods;
            num_regions = regs;
            bvol.bbox.Invalidate();
            bvol.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
            bvol.bsphere.radius = 0.0f;
        }
    } else {
        dbgPrintf("!! Invalid parameter  MorphDef constructor");
        num_lods = 0;
        num_regions = 0;
        regions = NULL;
    }
}

// Inlined everywhere (Release, and Clear's copy of it): the linker drops the copy
MorphDef::~MorphDef()
{
    delete[] regions;
}

// FUNCTION: LS3DF 0x100366b0
int MorphDef::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! RefCount:0x%x    (MorphDef)", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// FUNCTION: LS3DF 0x10036710
morph_region::morph_region()
{
    num_verts = 0;
    indices = NULL;
    verts = NULL;
}

// FUNCTION: LS3DF 0x10036720
morph_region::~morph_region()
{
    delete[] indices;
    if (verts)
        _aligned_free(verts);
}

// FUNCTION: LS3DF 0x10036750
void I3D_morph::LoadImage()
{
    flags &= ~MORPHF_UNKNOWN_4;
    I3D_object::LoadImage();
    LoadImageB();
    if (m_pMorphDef)
        vis_bound = m_pMorphDef->bvol;
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
}
