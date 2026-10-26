#pragma once

#include "actors/C_actor.h"

// The airplanes (PS2 C_airplane, C_airplane.cpp), 0x3f0 bytes (PS2's 0x400). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_AIRPLANE.
class C_airplane : public C_actor
{
public:
    C_airplane();

    uchar unknown_70[0x380];            // 0x70
};
