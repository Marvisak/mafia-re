#pragma once

#include "actors/C_actor.h"

// The race cameras (PS2 C_racecam, c_racecam.cpp), 0x88 bytes (PS2's 0xa0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_RACECAM.
class C_racecam : public C_actor
{
public:
    C_racecam();

    uchar unknown_70[0x18];             // 0x70
};
