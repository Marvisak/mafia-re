#pragma once

#include "actors/C_actor.h"

// The rail vehicles' generator (PS2 C_rail_generator, C_rail_generator.cpp), 0x84 bytes (PS2's 0xa0). Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_RAIL_GENERATOR.
class C_rail_generator : public C_actor
{
public:
    C_rail_generator();

    uchar unknown_70[0x14];             // 0x70
};
