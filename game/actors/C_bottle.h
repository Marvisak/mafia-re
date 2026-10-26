#pragma once

#include "actors/C_actor.h"

// The bottles (PS2 C_bottle, C_bottle.cpp), 0x138 bytes (PS2's 0x190). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_BOTTLE.
class C_bottle : public C_actor
{
public:
    C_bottle();

    uchar unknown_70[0xc8];             // 0x70
};
