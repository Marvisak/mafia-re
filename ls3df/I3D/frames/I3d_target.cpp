// The targets (PS2 I3d_target.cpp), at 0x10059440-0x1005a140 in the original: globalTargetList's $E, then I3D_target
// in PS2's order, the destructor after the constructor. The header's zero vector has its $E at the end (0x1005a120).
// Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "dta.h"

// <new> declares operator delete throw(), so VC6 sets no EH state before the frame list's destructor in the target's
#include <new>

// Every target (PS2 name)
// GLOBAL: LS3DF 0x101b50b8
// GLOBAL: LS3DF11 0x101b21d8
C_list<I3D_target *> globalTargetList;

// FUNCTION: LS3DF 0x10059490
// FUNCTION: LS3DF11 0x10056b50
I3D_target::I3D_target()
{
    type = FRAME_TARGET;
    globalTargetList.push_back(this);
    target_flags = TARGETF_LOOKAT;
}

// FUNCTION: LS3DF 0x100595b0
// FUNCTION: LS3DF11 0x10056c70
I3D_target::~I3D_target()
{
    I3D_frame **p = &frames[0];
    for (int i = frames.size(); i--; p++)
        (*p)->frm_flags &= ~FRMFLAGS_TARGET;
    globalTargetList.remove(this);
}

// FUNCTION: LS3DF 0x10059680
// FUNCTION: LS3DF11 0x10056d40
LS3D_RESULT I3D_target::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_TARGET)
        return I3D_OK;
    target_flags = ((I3D_target *)frame)->target_flags;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100596d0
// FUNCTION: LS3DF11 0x10056d90
void I3D_target::SetTargetFlags(uint flags)
{
    target_flags = flags;
    UpdateFrames();
}

// A frame in another target's list leaves it first
// FUNCTION: LS3DF 0x100596f0
// FUNCTION: LS3DF11 0x10056db0
void I3D_target::AddToTarget(I3D_frame *frame)
{
    if (frame == this)
        return;
    if (frame->frm_flags & FRMFLAGS_TARGET)
        DeleteFromAnyTarget(frame);
    frames.push_back(frame);
    frame->frm_flags |= FRMFLAGS_TARGET;
}

// FUNCTION: LS3DF 0x100597f0
// FUNCTION: LS3DF11 0x10056eb0
LS3D_RESULT I3D_target::DeleteFromTarget(I3D_frame *frame)
{
    if (frames.remove(frame)) {
        if (frame)
            frame->frm_flags &= ~FRMFLAGS_TARGET;
        return I3D_OK;
    }
    return I3DERR_OBJECTNOTFOUND;
}

// FUNCTION: LS3DF 0x10059880
// FUNCTION: LS3DF11 0x10056f40
I3D_target *I3D_target::GetTarget(I3D_frame *frame)
{
    I3D_target **tp = &globalTargetList[0];
    for (int i = globalTargetList.size(); i--; tp++) {
        I3D_frame **fp = &(*tp)->frames[0];
        for (int j = (*tp)->frames.size(); j--; fp++) {
            if (*fp == frame)
                return *tp;
        }
    }
    return NULL;
}

// The same search: PS2's doesn't call GetTarget either, and both builds pass the list's element
// FUNCTION: LS3DF 0x100598d0
// FUNCTION: LS3DF11 0x10056f90
void I3D_target::DeleteFromAnyTarget(I3D_frame *frame)
{
    I3D_target **tp = &globalTargetList[0];
    for (int i = globalTargetList.size(); i--; tp++) {
        I3D_frame **fp = &(*tp)->frames[0];
        for (int j = (*tp)->frames.size(); j--; fp++) {
            if (*fp == frame) {
                (*tp)->DeleteFromTarget(*fp);
                return;
            }
        }
    }
}

// A target that is on and moved updates its frames after the frames' update (I3D_frame::Update)
// FUNCTION: LS3DF 0x10059920
// FUNCTION: LS3DF11 0x10056fe0
void I3D_target::UpdateV()
{
    if ((frm_flags & FRMFLAGS_ON) && (frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED)))
        UpdateTargetList.push_back(this);
}

// Turns the frames to the target, or gives them the parts of its world matrix the flags name, in their parents'
// space; a frame without a parent takes the whole matrix. Each frame is read through p again after every call, as
// the original does; the direction is assigned, not initialized, which computes it from x on into a temporary. The
// parent is tested before it is used (the original tests it in ecx, then copies it).
// Not matching yet: the inline budget. The original inlines GetWorldDir's Normalize (the same code as in the light's
// UpdateV), ours calls it. 64 dead statements (if (0) target_flags = 0;) at the top give the original's code exactly,
// 48 don't; so does a forced inline Normalize (see I3d_math.h), or leaving out the inline calls after GetWorldDir.
// FUNCTION: LS3DF 0x100599b0
// FUNCTION: LS3DF11 0x10057070
void I3D_target::UpdateFrames()
{
    if (!frames.size())
        return;
    S_vector pos = GetWorldPos();
    I3D_frame **p = &frames[0];
    for (int i = frames.size(); i--; p++) {
        if (target_flags & TARGETF_LOOKAT) {
            S_vector dir;
            dir = pos - (*p)->GetWorldPos();
            dir.Normalize();
            S_vector wdir = (*p)->GetWorldDir();
            if (fabs(wdir.x - dir.x) > MRG_ZERO || fabs(wdir.y - dir.y) > MRG_ZERO ||
                fabs(wdir.z - dir.z) > MRG_ZERO) {
                (*p)->SetWorldDir(dir, 0.0f);
                (*p)->Update();
            }
        } else if (target_flags & (TARGETF_ROT | TARGETF_POS | TARGETF_SCALE)) {
            if ((*p)->parent) {
                S_matrix m = (*p)->parent->GetMatrix();
                m.Inverse(S_matrix(m));
                m = GetMatrix() * m;
                if (target_flags & TARGETF_POS)
                    (*p)->SetPos(m(3));
                if (target_flags & TARGETF_SCALE)
                    (*p)->SetScale(m.GetScale());
                if (target_flags & TARGETF_ROT) {
                    I3D_frame *frm = *p;
                    frm->f_matrix.SetDir3(m(2), m(1));
                    frm->frm_flags = (frm->frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID |
                        FRMFLAGS_QUAT_VALID | FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID;
                }
            } else
                (*p)->SetLocalMatrix(GetMatrix());
        }
    }
}

// The frames are read as their 1-based indices in the file, which the loader turns into the frames once they are all
// loaded (LoadTargetList)
// FUNCTION: LS3DF 0x10059f10
// FUNCTION: LS3DF11 0x100575d0
void I3D_target::LoadImage()
{
    I3D_frame::LoadImage();
    dtaRead(LoadImageHandle, &target_flags, 2);
    int num = 0;
    dtaRead(LoadImageHandle, &num, 1);
    if (num) {
        LoadTargetList.push_back(this);
        frames.reserve(num);
        while (num--) {
            int index = 0;
            dtaRead(LoadImageHandle, &index, 2);
            if (index)
                frames.push_back((I3D_frame *)index);
        }
    }
}
