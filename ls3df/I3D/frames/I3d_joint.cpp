// The single meshes' joints (PS2 I3d_joint.cpp), at 0x1001ed30-0x1001ef60 in the original: I3D_joint in PS2's order,
// the destructor after the constructor. The header's zero vector has its $E at the end (0x1001ef40). Names are the PS2
// build's unless noted.
#include "I3D/I3D.h"
#include "dta.h"

// The bone matrix's diagonal is a chain, stored from the right
// FUNCTION: LS3DF 0x1001ed30
I3D_joint::I3D_joint()
{
    type = FRAME_JOINT;
    bone_matrix.Identity();
    bone_matrix(0, 0) = bone_matrix(1, 1) = bone_matrix(2, 2) = 0.01f;
    matrix_184 = bone_matrix;
    joint_flags = 0;
    index = 0;
    singlemesh = NULL;
}

// The joint leaves its single mesh's list. The index is read once, before the stores (PS2's too).
// FUNCTION: LS3DF 0x1001ede0
I3D_joint::~I3D_joint()
{
    I3D_singlemesh *sm = singlemesh;
    if (sm) {
        uint i = index;
        if (sm->joints && sm->joints[i])
            sm->joints[i]->singlemesh = NULL;
        sm->joints[i] = NULL;
    }
}

// A moved joint's matrices are to be computed again, and its single mesh's vertices and bound volume. The first two
// stores go through a local, the others read the member again (PS2's do the same).
// FUNCTION: LS3DF 0x1001ee30
void I3D_joint::UpdateV()
{
    if (!(frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED)))
        return;
    joint_flags &= ~(JOINTF_UNKNOWN_4 | JOINTF_UNKNOWN_8 | JOINTF_JMAT_VALID);
    if (!(frm_flags & FRMFLAGS_UPDATE))
        return;
    I3D_singlemesh *sm = singlemesh;
    if (!sm)
        return;
    sm->joints_changed++;
    sm->vis_flags &= ~(VISF_BOUND_VALID | VISF_WBOUND_VALID);
    singlemesh->DiscardHRBoundVol();
    singlemesh->frm_flags = (singlemesh->frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_HR_BOUND_VALID)) |
        FRMFLAGS_UPDATE;
}

// FUNCTION: LS3DF 0x1001eeb0
LS3D_RESULT I3D_joint::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    if (frame->GetType() == FRAME_JOINT) {
        I3D_joint *j = (I3D_joint *)frame;
        bone_matrix = j->bone_matrix;
        joint_flags = j->joint_flags & JOINTF_LOADED;
        index = j->index;
    }
    return I3D_frame::Duplicate(frame);
}

// The bone matrix and the index, in one read
// FUNCTION: LS3DF 0x1001ef10
void I3D_joint::LoadImage()
{
    I3D_frame::LoadImage();
    dtaRead(LoadImageHandle, &bone_matrix, sizeof(S_matrix) + sizeof(uint));
    joint_flags = JOINTF_LOADED;
}
