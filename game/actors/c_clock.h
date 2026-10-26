#pragma once

#include "actors/C_actor.h"

// The clocks (PS2 C_clock, c_clock.cpp), 0x88 bytes (PS2's 0xa0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_CLOCK.
class C_clock : public C_actor
{
public:
    C_clock();

    uchar unknown_70[0x18];             // 0x70
};
