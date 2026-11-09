// The bones the shots test against (PS2 C_ShotSkeleton, whose file sorts between g_menu.cpp and g_textdb.cpp on PS2;
// the file's name is ours: the PC's file is the one at 0x5d6f80-0x5d767c, docs/game-files.md). The class is in
// actors/C_ShotSkeleton.h, a member of C_human.

#include "actors/C_human.h"

// The frames of the joints (PS2 name), which Init finds in the human's model
// GLOBAL: GAME 0x637f98
const char *jointName[SHOT_JOINT_COUNT] = {
    "r_hand",
    "r_elbow",
    "r_arm",
    "r_foot",
    "r_shin",
    "r_thigh",
    "l_hand",
    "l_elbow",
    "l_arm",
    "l_foot",
    "l_shin",
    "l_thigh",
    "base",
    "back1",
    // STRING: GAME 0x6380c8
    "back3",
    "neck",
};

// The bones every skeleton gets (PS2 name, "the model bones")
// GLOBAL: GAME 0x637fd8
S_shot_bone vzoroveKosti[SHOT_BONE_COUNT] = {
    {SHOT_JOINT_R_HAND, SHOT_JOINT_R_ELBOW, -.5f, 1.2f, .06f, HIT_PART_HAND_R},
    {SHOT_JOINT_L_HAND, SHOT_JOINT_L_ELBOW, -.5f, 1.2f, .06f, HIT_PART_HAND_L},
    {SHOT_JOINT_R_ELBOW, SHOT_JOINT_R_ARM, 0.0f, 1.3f, .08f, HIT_PART_HAND_R},
    {SHOT_JOINT_L_ELBOW, SHOT_JOINT_L_ARM, 0.0f, 1.3f, .08f, HIT_PART_HAND_L},
    {SHOT_JOINT_R_FOOT, SHOT_JOINT_R_SHIN, -.3f, 1.1f, .1f, HIT_PART_LEG_R},
    {SHOT_JOINT_L_FOOT, SHOT_JOINT_L_SHIN, -.3f, 1.1f, .1f, HIT_PART_LEG_L},
    {SHOT_JOINT_R_SHIN, SHOT_JOINT_R_THIGH, .1f, 1.2f, .12f, HIT_PART_LEG_R},
    {SHOT_JOINT_L_SHIN, SHOT_JOINT_L_THIGH, .1f, 1.2f, .12f, HIT_PART_LEG_L},
    {SHOT_JOINT_BACK1, SHOT_JOINT_BACK3, -.5f, 1.2f, .17f, HIT_PART_BODY},
    {SHOT_JOINT_BACK3, SHOT_JOINT_NECK, .5f, 3.0f, .12f, HIT_PART_HEAD},
};

// FUNCTION: GAME 0x5d6f80
void C_ShotSkeleton::Init(I3D_frame *frm)
{
    int i;
    for (i = 0; i < SHOT_JOINT_COUNT; i++)
        joints[i] = frm->FindChildFrame(jointName[i], ENUMF_ALL);
    for (i = 0; i < SHOT_BONE_COUNT; i++)
        bones[i] = vzoroveKosti[i];
}

// Each bone is a rectangle facing the line: along the bone from start to end, radius wide on either side of it across
// the line. The line has to pass between its long edges and between its ends, and then meets its plane; the bones
// later in the list count a little farther.
// The five vectors are declared together and assigned: the original builds each in a temporary and copies it, and
// pos among them (its copy is the same either way) gives the budget that inlines the second test's from - p0. A
// failed t >= best continues (and eax, 0x100; je), where t < best around the stores tests ah, 5.
// FUNCTION: GAME 0x5d6fe0
int C_ShotSkeleton::TestCollision(const S_vector &from, const S_vector &dir, float dist2)
{
    int part = 0;
    float best = dist2;
    for (int i = 0; i < SHOT_BONE_COUNT; i++) {
        const S_shot_bone &b = bones[i];
        S_vector pos, axis, side, p0, p1;
        pos = joints[b.joint0]->GetWorldPos();
        axis = joints[b.joint1]->GetWorldPos() - pos;
        side = axis.Cross(dir);
        side.Normalize();
        side *= b.radius;
        p0 = pos + axis * b.start;
        p1 = pos + axis * b.end;
        S_vector p0_side = p0 + side;
        // Never read (PS2 computes it too)
        S_vector p1_side = p1 + side;
        p0 -= side;
        p1 -= side;
        if (side.Dot(from - p0) * side.Dot(p0_side - from) < 0.0f)
            continue;
        S_vector across = dir.Cross(side);
        if (across.Dot(from - p0) * across.Dot(p1 - from) < 0.0f)
            continue;
        S_vector normal = side.Cross(axis);
        float d = -normal.Dot(p0);
        float den = normal.Dot(dir);
        if (fabs(den) < MRG_ZERO)
            continue;
        float t = (normal.Dot(from) + d) * (-1.0f / den);
        t += i * .01f;
        t *= t;
        if (t >= best)
            continue;
        best = t;
        part = b.part;
    }
    return part;
}
