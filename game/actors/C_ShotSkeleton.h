#pragma once

#include "I3D/I3D.h"

// The joints of the skeleton, jointName's indices: the frames of the human's model by those names (the names are
// ours)
#define SHOT_JOINT_R_HAND 0
#define SHOT_JOINT_R_ELBOW 1
#define SHOT_JOINT_R_ARM 2
#define SHOT_JOINT_R_FOOT 3
#define SHOT_JOINT_R_SHIN 4
#define SHOT_JOINT_R_THIGH 5
#define SHOT_JOINT_L_HAND 6
#define SHOT_JOINT_L_ELBOW 7
#define SHOT_JOINT_L_ARM 8
#define SHOT_JOINT_L_FOOT 9
#define SHOT_JOINT_L_SHIN 10
#define SHOT_JOINT_L_THIGH 11
#define SHOT_JOINT_BASE 12
#define SHOT_JOINT_BACK1 13
#define SHOT_JOINT_BACK3 14
#define SHOT_JOINT_NECK 15
#define SHOT_JOINT_COUNT 16

// One bone the shots test against (PS2 vzoroveKosti's entries; the names are ours), 0x18 bytes: the line through two
// joints, from start to end times their distance from the first, as wide as radius on either side
struct S_shot_bone {
    int joint0;                         // 0x0, SHOT_JOINT_*
    int joint1;                         // 0x4, SHOT_JOINT_*
    float start;                        // 0x8
    float end;                          // 0xc
    float radius;                       // 0x10
    int part;                           // 0x14, HIT_PART_*, what a hit takes
};

// The bones (the name is ours)
#define SHOT_BONE_COUNT 10

// The bones the shots test against (PS2 C_ShotSkeleton; the name of its file isn't known, PC 0x5d6f80-0x5d767c), a
// member of C_human, 0x130 bytes
class C_ShotSkeleton
{
public:
    // The line from from along dir against the bones: the part of the bone it crosses nearest to from, whose distance
    // along dir squared is below dist2, 0 for none (PS2 name; the parameters' names and the result are ours)
    int TestCollision(const S_vector &from, const S_vector &dir, float dist2);

    // Empty, in the class: C_human's constructor and destructor unwind the skeleton through the linker's folded
    // empty function of that kind (0x404990), and construct nothing
    ~C_ShotSkeleton()
    {
    }
    // Takes the joints of the human's model and the bones (PS2 name), which GameInit and _intern_ChangeModel call
    void Init(I3D_frame *frm);

    I3D_frame *joints[SHOT_JOINT_COUNT];    // 0x0, jointName's frames of the model (the name is ours)
    S_shot_bone bones[SHOT_BONE_COUNT];     // 0x40, vzoroveKosti's (the name is ours)
};
