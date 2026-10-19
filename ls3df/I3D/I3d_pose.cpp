// The poses (PS2 I3d_pose.cpp), at 0x10045930-0x10045e30 in the original: I3D_pose, then I3D_pose_set, each from its
// constructor, the PS2's out-of-line functions in PS2's order. The header's zero vector has its $E at the end
// (0x10045e10). Names are the PS2 build's unless noted.
// A pose holds a position, a scale and a rotation for one frame, each set or not (POSEF_* in I3D.h); a model's or
// scene's interpolator blends it into the frame's animations. A pose set gives its poses the names of their frames.
#include "I3D/I3D.h"

#include <string.h>

// FUNCTION: LS3DF 0x10045930
I3D_pose::I3D_pose()
{
    ref = 1;
    pose_flags = 0;
    pos.Zero();
    scale = UnitVector;
    rot.Identity();
    name = NULL;
}

// Out of line on PS2; VC6 inlines it into Release and the linker drops our copy
I3D_pose::~I3D_pose()
{
    delete[] name;
}

// FUNCTION: LS3DF 0x10045980
int I3D_pose::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! RefCount:0x%x   I3D_pose", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// FUNCTION: LS3DF 0x100459d0
void I3D_pose::Clear()
{
    pose_flags = 0;
}

// FUNCTION: LS3DF 0x100459e0
void I3D_pose::SetRot(const S_quat *q)
{
    if (q) {
        rot = *q;
        pose_flags = (pose_flags & ~POSEF_ROT) | POSEF_ROT_ADD;
    } else
        pose_flags &= ~POSEF_ROT_ADD;
}

// FUNCTION: LS3DF 0x10045a30
void I3D_pose::SetRotAbs(const S_quat *q)
{
    if (q) {
        rot = *q;
        pose_flags = (pose_flags & ~POSEF_ROT_ADD) | POSEF_ROT;
    } else
        pose_flags &= ~(POSEF_ROT_ADD | POSEF_ROT);
}

// FUNCTION: LS3DF 0x10045a80
void I3D_pose::SetDir(const S_vector *dir, float roll)
{
    if (dir) {
        rot.SetDir(*dir, roll);
        pose_flags = (pose_flags & ~POSEF_ROT) | POSEF_ROT_ADD;
    } else
        pose_flags &= ~(POSEF_ROT_ADD | POSEF_ROT);
}

// Leaves POSEF_ROT as it is, unlike the others (PS2's too)
// FUNCTION: LS3DF 0x10045ac0
void I3D_pose::SetDir(const S_vector &dir, float roll)
{
    rot.SetDir(dir, roll);
    pose_flags |= POSEF_ROT_ADD;
}

// FUNCTION: LS3DF 0x10045ae0
void I3D_pose::SetDirAbs(const S_vector &dir, float roll)
{
    rot.SetDir(dir, roll);
    pose_flags = (pose_flags & ~POSEF_ROT_ADD) | POSEF_ROT;
}

// FUNCTION: LS3DF 0x10045b10
void I3D_pose::SetPos(const S_vector *p)
{
    if (p) {
        pos = *p;
        pose_flags |= POSEF_POS;
    } else
        pose_flags &= ~POSEF_POS;
}

// FUNCTION: LS3DF 0x10045b50
void I3D_pose::SetScale(const S_vector *s)
{
    if (s) {
        scale = *s;
        pose_flags |= POSEF_SCALE;
    } else
        pose_flags &= ~POSEF_SCALE;
}

// VC6 inlines it into AddPose and the linker drops our copy, but the copy puts its message ahead of the pose set's in
// .data, as in the original. Without a name the old one stays, freed (the original's too).
void I3D_pose::SetName(const char *n)
{
    delete[] name;
    if (n) {
        name = new char[strlen(n) + 1];
        if (!name)
            dbgPrintf("!! Out of memory  I3D_pose::SetName()");
        else
            strcpy(name, n);
    }
}

// FUNCTION: LS3DF 0x10045b90
I3D_pose_set::I3D_pose_set()
{
    ref = 1;
}

// The file doesn't include <new>: the original sets the EH state to -1 before the list's destructor frees the array
// FUNCTION: LS3DF 0x10045bc0
I3D_pose_set::~I3D_pose_set()
{
    I3D_pose **p = &poses[0];
    for (int i = poses.size(); i--; p++)
        (*p)->Release();
}

// FUNCTION: LS3DF 0x10045c40
int I3D_pose_set::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! RefCount:0x%x    I3D_pose_set", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// The pose's GetName written out: through the inline function VC6 loads the pose into eax, not edx
// FUNCTION: LS3DF 0x10045c80
const char *I3D_pose_set::GetFrameName(int index) const
{
    if (index >= poses.size())
        return "";
    return poses[index]->name ? poses[index]->name : "";
}

// FUNCTION: LS3DF 0x10045cb0
void I3D_pose_set::AddPose(I3D_pose *pose, const char *frame_name)
{
    pose->SetName(frame_name);
    poses.push_back(pose);
    pose->ref++;
}

// FUNCTION: LS3DF 0x10045d80
LS3D_RESULT I3D_pose_set::DeletePose(I3D_pose *pose)
{
    if (poses.remove(pose)) {
        pose->Release();
        return I3D_OK;
    }
    return I3DERR_OBJECTNOTFOUND;
}
