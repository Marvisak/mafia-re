#pragma once

#include "actors/C_actor.h"

// The rail vehicles (PS2 C_railway, with doors, windows and seats; C_railway.cpp, or C_rail_traffic.cpp, whose seats
// they use), 0x370 bytes (PS2's 0x390). Only the constructor so far, which C_mission::CreateActor calls for
// ACTOR_RAILWAY.
class C_railway : public C_actor
{
public:
    C_railway();

    uchar unknown_70[0x300];            // 0x70
};
