#pragma once

#include "actors/C_actor.h"

// The bridges (PS2 C_bridge, C_bridge.cpp), 0x194 bytes (PS2's 0x1a0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_BRIDGE, and ShutDown, which the script calls, as stubs.
class C_bridge : public C_actor
{
public:
    C_bridge();

    // The bridge shut down or not (PS2 name): the flag stored, the bridge's collision taken from the game's
    void ShutDown(bool shut);

    uchar unknown_70[0x124];            // 0x70
};
