#pragma once

#include "actors/C_actor.h"

// The objects scattered around a frame (PC only, the name is ours): copies of up to seven of its frame's objects,
// the number of each from 0x84 on, put at random within a radius (0x80) on the ground below, 0xa0 bytes. Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_SCATTER.
class C_scatter : public C_actor
{
public:
    C_scatter();

    uchar unknown_70[0x30];             // 0x70
};
