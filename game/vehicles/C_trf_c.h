#pragma once

#include "actors/C_actor.h"

// The traffic's cars (PS2 C_traffic_car, C_trf_c.cpp), 0x26c bytes (PS2's 0x290). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_TRAFFIC_CAR.
class C_traffic_car : public C_actor
{
public:
    C_traffic_car();

    uchar unknown_70[0x1fc];            // 0x70
};
