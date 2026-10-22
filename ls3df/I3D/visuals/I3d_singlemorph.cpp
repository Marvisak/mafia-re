// The single morphs (PS2 I3d_singlemorph.cpp), at 0x10056e80-0x10057650 in the original: I3D_singlemorph in PS2's order
// from its constructor, with the PC's UpdateLodVertices where PS2 has GetVertexPos and m_AddFGroups (the PC's are the
// single mesh's). The header's zero vector has its $E last (0x10057630). Names are the PS2 build's unless noted.
//
// Still open: SetWeight's and Duplicate's joints_changed++, which the original increments with one inc on memory and
// ours with a load, an inc and a store that VC6 schedules around the next statement's. An inline function that
// reaches the counter through an address (a reference or a pointer parameter, or a member function with a local
// reference) gives the original's code in both; a plain member function or any spelling of the statement doesn't.
// The original's source for it isn't known.
#include "I3D/I3D.h"

// FUNCTION: LS3DF 0x10056e80
I3D_singlemorph::I3D_singlemorph()
{
    visual_type = I3D_VISUAL_SINGLEMORPH;
}

// Empty, but defined here: the original's comes right after the constructor's scalar deleting destructor, as the
// morph's
// FUNCTION: LS3DF 0x10056f10
I3D_singlemorph::~I3D_singlemorph()
{
}

// FUNCTION: LS3DF 0x10056f70
int I3D_singlemorph::NumLevels() const
{
    return m_numLevels;
}

// FUNCTION: LS3DF 0x10056f80
int I3D_singlemorph::NumRegions() const
{
    return m_numRegions;
}

// FUNCTION: LS3DF 0x10056f90
LS3D_RESULT I3D_singlemorph::SetWeight(int region, int level, float weight)
{
    if (region >= 0 && region < m_numRegions && level >= 0 && level < m_numLevels) {
        joints_changed++;
        // Both branches the same, as the morph's
        if (!(vis_flags & VISF_WORLD_SPACE))
            vis_flags &= ~VISF_WBOUND_VALID;
        else
            vis_flags &= ~VISF_WBOUND_VALID;
        SetWeightB(region, level, weight);
        return I3D_OK;
    }
    dbgPrintf("!! index out of range  I3D_singlemorph::SetWeight()  this:0x%x  %s  region:%d m_numRegions:%d  level:%d "
        "m_numLevels:%d", this, GetName(), region, m_numRegions, level, m_numLevels);
    return I3DERR_INVALIDPARAMS;
}

// FUNCTION: LS3DF 0x10057030
float I3D_singlemorph::GetWeight(int region, int level) const
{
    if (region >= 0 && region < m_numRegions && level >= 0 && level < m_numLevels)
        return GetWeightB(region, level);
    dbgPrintf("!! index out of range  I3D_singlemorph::GetWeight()  this:0x%x  %s  region:%d m_numRegions:%d  level:%d "
        "m_numLevels:%d", this, GetName(), region, m_numRegions, level, m_numLevels);
    return 0.0f;
}

// FUNCTION: LS3DF 0x100570b0
void I3D_singlemorph::SetMesh(I3D_mesh_object *m, bool keep_lods)
{
    I3D_object::SetMesh(m, keep_lods);
    if (m_pMorphDef)
        vis_bound = m_pMorphDef->m_BVolume;
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
}

// From a morph or single morph, a copy of its mesh that shares its levels (and a single morph's joints' parts); from
// a single mesh its joints' parts, without its mesh (PS2 the same); from another visual with a mesh, or as a proxy,
// that mesh; from an emitter, its particles' mesh. A proxy takes the objects' case through a goto, as the single
// mesh's.
// FUNCTION: LS3DF 0x10057100
LS3D_RESULT I3D_singlemorph::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
    else
        vis_flags &= ~VISF_WBOUND_VALID;
    I3D_frame::Duplicate(frame);
    joints_changed++;
    m_Flags &= ~(MORPHF_LOADED | MORPHF_UNKNOWN_4);
    if (frame->GetType() != FRAME_VISUAL) {
        if (frame->GetType() == FRAME_EMITOR) {
            Clear();
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
            frame->UpdateV();
            SetMesh(((I3D_emitor *)frame)->mesh, true);
        }
        return I3D_OK;
    }
    Clear();
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
    frame->UpdateV();
    {
        I3D_singlemorph *sm = (I3D_singlemorph *)frame;
        switch (sm->visual_type) {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_BILLBOARD:
        case I3D_VISUAL_LENSF:
        obj:
            SetMesh(sm->mesh, false);
            lightness = sm->lightness;
            transparency = sm->transparency;
            break;
        case I3D_VISUAL_MORPH:
            if (vis_flags & VISF_PROXY)
                goto obj;
            {
                lightness = sm->lightness;
                transparency = sm->transparency;
                I3D_morph *mo = (I3D_morph *)frame;
                if (!(mo->m_Flags & MORPHF_LOADED))
                    SetMesh(sm->mesh, false);
                else {
                    LS3D_RESULT r = DuplicateMorphDef(mo);
                    if (r < 0) {
                        SetMesh(sm->mesh, false);
                        return r;
                    }
                    I3D_mesh_object *m = pI3DDriver->CreateMesh();
                    if (!m) {
                        dbgPrintf("!! Out of memory  I3D_singlemorph::Duplicate()");
                        return I3DERR_OUTOFMEM;
                    }
                    r = m->Duplicate(sm->mesh);
                    if (r < 0) {
                        m->Release();
                        return r;
                    }
                    m_Flags |= MORPHF_LOADED;
                    SetMesh(m, false);
                    m->Release();
                }
            }
            break;
        case I3D_VISUAL_SINGLEMESH:
            if (vis_flags & VISF_PROXY)
                goto obj;
            {
                lightness = sm->lightness;
                transparency = sm->transparency;
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
                if (!(sm->m_Flags & MORPHF_LOADED))
                    SetMesh(sm->mesh, false);
                else {
                    LS3D_RESULT r = DuplicateMorphDef(sm);
                    if (r < 0) {
                        SetMesh(sm->mesh, false);
                        return r;
                    }
                    I3D_mesh_object *m = pI3DDriver->CreateMesh();
                    if (!m) {
                        dbgPrintf("!! Out of memory  I3D_singlemorph::Duplicate()");
                        return I3DERR_OUTOFMEM;
                    }
                    r = m->Duplicate(sm->mesh);
                    if (r < 0) {
                        m->Release();
                        return r;
                    }
                    m_Flags |= MORPHF_LOADED;
                    SetMesh(m, false);
                    m->Release();
                }
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

// FUNCTION: LS3DF 0x10057580
void I3D_singlemorph::UpdateBoundProc()
{
    I3D_singlemesh::UpdateBoundProc();
}

// FUNCTION: LS3DF 0x10057590
void I3D_singlemorph::UpdateLodVertices(int lod_index)
{
    if (m_Flags & MORPHF_DIRTY) {
        UpdateMorphLod(lod_index, mesh);
        UpdateVertices(lod_index);
    }
    I3D_singlemesh::UpdateLodVertices(lod_index);
}

// The single mesh's, then the levels, unless the single mesh's failed
// FUNCTION: LS3DF 0x100575d0
void I3D_singlemorph::LoadImage()
{
    m_Flags &= ~MORPHF_UNKNOWN_4;
    I3D_singlemesh::LoadImage();
    if (ImageErrFlg)
        return;
    LoadImageB();
    if (m_pMorphDef)
        vis_bound = m_pMorphDef->m_BVolume;
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
}
