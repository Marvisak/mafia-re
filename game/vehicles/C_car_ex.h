#pragma once

#include "vehicles/C_car.h"

// The cars with dynamic collisions (PS2 C_car_ex, C_car_ex.cpp, whose S_dyn_coll they are), a car, 0x227c bytes
// (PS2's 0x24f0). Only the constructor so far, which C_mission::CreateActor calls for ACTOR_CAR_EX.
class C_car_ex : public C_car
{
public:
    C_car_ex();

    uchar unknown_221c[0x60];           // 0x221c
};
