#pragma once

#include "actors/C_actor.h"

// The wagons (PS2 C_wagon, C_wagon.cpp), 0x1a8 bytes (PS2's 0x1e0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_WAGON.
class C_wagon : public C_actor
{
public:
    C_wagon();

    uchar unknown_70[0x138];            // 0x70
};
