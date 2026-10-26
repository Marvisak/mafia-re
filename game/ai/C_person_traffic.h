#pragma once

#include "actors/C_actor.h"

// The pedestrians' generator (PS2 C_traffic_generator, C_person_traffic.cpp), 0xb8 bytes (PS2's 0xd0). Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_TRAFFIC_GENERATOR.
class C_traffic_generator : public C_actor
{
public:
    C_traffic_generator();

    uchar unknown_70[0x48];             // 0x70
};
