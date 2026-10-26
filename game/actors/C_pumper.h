#pragma once

#include "actors/C_actor.h"

// The petrol pump attendants (PS2 C_pumper, C_pumper.cpp), 0x2ac bytes (PS2's 0x2e0). Only the constructor so far,
// which C_mission::CreateActor calls for ACTOR_PUMPER.
class C_pumper : public C_actor
{
public:
    C_pumper();

    uchar unknown_70[0x23c];            // 0x70
};
