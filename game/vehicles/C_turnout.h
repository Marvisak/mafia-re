#pragma once

#include "actors/C_actor.h"

// The rails' turnouts (PS2 C_turnout, C_turnout.cpp), 0x1a0 bytes (PS2's 0x1d0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_TURNOUT.
class C_turnout : public C_actor
{
public:
    C_turnout();

    uchar unknown_70[0x130];            // 0x70
};
