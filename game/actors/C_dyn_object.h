#pragma once

#include "actors/C_actor.h"

// The objects of dynamic parts (PC only, the name is ours: it takes its model's frames named dynA, dynB and dynS,
// a part of 0xc8 bytes each from 0x130 on), 0xc9c bytes. Only the constructor so far, which C_mission::CreateActor
// calls for ACTOR_DYN_OBJECT.
class C_dyn_object : public C_actor
{
public:
    C_dyn_object();

    uchar unknown_70[0xc2c];            // 0x70
};
