// The visuals' base (PS2 I3d_visual.cpp), at 0x1005a200-0x1005a7a0 in the original: I3D_visual in PS2's order, then
// I3D_void_interface's functions. PS2's GetVisualType (a virtual function) and SetBoundBox aren't in LS3DF. The
// header's zero vector has its $E at the end (0x1005a780). Names are the PS2 build's unless noted.
#include "I3D/I3D.h"

// FUNCTION: LS3DF 0x1005a200
I3D_visual::I3D_visual()
{
    type = FRAME_VISUAL;
    inv_matrix.Identity();
    vis_bound.bbox.Invalidate();
    vis_bound.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
    vis_bound.bsphere.radius = 0.0f;
    vis_bound_w.bbox.Invalidate();
    vis_bound_w.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
    vis_bound_w.bsphere.radius = 0.0f;
    vis_flags = VISF_SHADOW | VISF_PROJECTOR;
}

// Empty, but declared: the implicit destructor wouldn't set the vtables again
// FUNCTION: LS3DF 0x1005a370
I3D_visual::~I3D_visual()
{
}

// FUNCTION: LS3DF 0x1005a390
void I3D_visual::UpdateV()
{
    if (!(vis_flags & VISF_WORLD_SPACE) && frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED))
        vis_flags &= ~(VISF_WBOUND_VALID | VISF_UNKNOWN_80000 | VISF_INV_MAT_VALID);
}

// FUNCTION: LS3DF 0x1005a3c0
void I3D_visual::SetSpace(bool world)
{
    if (vis_flags & VISF_WORLD_SPACE) {
        if (world)
            return;
    } else {
        if (!world)
            return;
    }
    dbgPrintf("!! Unsupported for visual type:%s  I3D_visual::SetSpace()\tthis:0x%x  %s",
        pI3DDriver->VisualType2String(visual_type), this, GetName());
}

// FUNCTION: LS3DF 0x1005a410
void I3D_visual::SetShadowMode(SHADOW_MODE mode)
{
    switch (mode) {
    case SHADOW_MODE_0:
        vis_flags &= ~(VISF_SHADOW | VISF_SHADOW_2);
        break;
    case SHADOW_MODE_1:
        vis_flags = (vis_flags & ~VISF_SHADOW_2) | VISF_SHADOW;
        break;
    case SHADOW_MODE_2:
        vis_flags |= VISF_SHADOW | VISF_SHADOW_2;
        break;
    }
}

// FUNCTION: LS3DF 0x1005a460
SHADOW_MODE I3D_visual::GetShadowMode() const
{
    if (vis_flags & VISF_SHADOW_2)
        return SHADOW_MODE_2;
    return (SHADOW_MODE)((vis_flags & VISF_SHADOW) != 0);
}

// FUNCTION: LS3DF 0x1005a480
void I3D_visual::SetProjectorMode(SHADOW_MODE mode)
{
    switch (mode) {
    case SHADOW_MODE_0:
        vis_flags &= ~(VISF_PROJECTOR | VISF_PROJECTOR_2);
        break;
    case SHADOW_MODE_1:
        vis_flags = (vis_flags & ~VISF_PROJECTOR_2) | VISF_PROJECTOR;
        break;
    case SHADOW_MODE_2:
        vis_flags |= VISF_PROJECTOR | VISF_PROJECTOR_2;
        break;
    }
}

// FUNCTION: LS3DF 0x1005a4d0
SHADOW_MODE I3D_visual::GetProjectorMode() const
{
    if (vis_flags & VISF_PROJECTOR_2)
        return SHADOW_MODE_2;
    return (SHADOW_MODE)((vis_flags & VISF_PROJECTOR) != 0);
}

// FUNCTION: LS3DF 0x1005a4f0
void I3D_visual::UpdateBoundProc()
{
    vis_flags |= VISF_BOUND_VALID;
}

// The visual's box in world space, and the sphere around it: I3D_frame::UpdateHRBoundVolW's code
// FUNCTION: LS3DF 0x1005a510
void I3D_visual::UpdateWBoundProc()
{
    vis_flags |= VISF_WBOUND_VALID;
    vis_bound_w.bbox.Invalidate();
    if (!(vis_flags & VISF_BOUND_VALID))
        UpdateBoundProc();
    if (vis_bound.bbox.IsValid()) {
        const S_vector *v = BBoxVertices(vis_bound.bbox);
        vis_bound_w.bbox.min = vis_bound_w.bbox.max = *v * GetMatrix();
        for (int i = 7; i--; ) {
            v++;
            vis_bound_w.bbox.Absorb(*v * GetMatrix());
        }
        S_vector half = (vis_bound_w.bbox.max - vis_bound_w.bbox.min) * 0.5f;
        vis_bound_w.bsphere.pos = vis_bound_w.bbox.min + half;
        vis_bound_w.bsphere.radius = (float)sqrt(half.Square());
    } else {
        vis_bound_w.bsphere.pos.Zero();
        vis_bound_w.bsphere.radius = 0.0f;
    }
}

// FUNCTION: LS3DF 0x1005a730
void I3D_visual::SetBoundVolume(const I3D_bvolume &bvol)
{
    vis_bound = bvol;
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
}

int I3D_void_interface::NumLevels() const
{
    return 0;
}

int I3D_void_interface::NumRegions() const
{
    return 0;
}

// FUNCTION: LS3DF 0x1005a760
LS3D_RESULT I3D_void_interface::SetWeight(int level, int region, float weight)
{
    return I3DERR_GENERIC;
}

// FUNCTION: LS3DF 0x1005a770
float I3D_void_interface::GetWeight(int level, int region) const
{
    return 0.0f;
}
