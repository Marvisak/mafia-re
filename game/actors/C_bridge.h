#pragma once

#include "actors/C_actor.h"

// The bridges (PS2 C_bridge, C_bridge.cpp), 0x194 bytes (PS2's 0x1a0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_BRIDGE.
class C_bridge : public C_actor
{
public:
    C_bridge();

    uchar unknown_70[0x124];            // 0x70
};
