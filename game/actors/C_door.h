#pragma once

#include "actors/C_actor.h"

// The doors (PS2 C_door, C_door.cpp), 0x1b8 bytes (PS2's 0x200). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_DOOR.
class C_door : public C_actor
{
public:
    C_door();

    uchar unknown_70[0x148];            // 0x70
};
