#pragma once

#include "actors/C_actor.h"

// PS2's C_actor_shot (C_actor_shot.cpp), 0x8c bytes (PS2's 0xa0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_ACTOR_SHOT.
class C_actor_shot : public C_actor
{
public:
    C_actor_shot();

    uchar unknown_70[0x1c];             // 0x70
};
