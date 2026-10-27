#pragma once

#include "actors/C_actor.h"

// The doors (PS2 C_door, C_door.cpp), 0x1b8 bytes (PS2's 0x200). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_DOOR.
class C_door : public C_actor
{
public:
    C_door();

    // The name is ours: C_game::Init's, its stores in this order
    void ResetUnknown110()
    {
        unknown_114 = 0;
        unknown_118 = 0;
        unknown_110 = 0;
    }

    uchar unknown_70[0xa0];             // 0x70
    int unknown_110;                    // 0x110
    int unknown_114;                    // 0x114
    int unknown_118;                    // 0x118
    uchar unknown_11c[0x9c];            // 0x11c
};
