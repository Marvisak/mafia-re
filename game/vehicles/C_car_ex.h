#pragma once

#include "vehicles/C_car.h"

// The cars with dynamic collisions (PS2 C_car_ex, C_car_ex.cpp, whose S_dyn_coll they are), a car, 0x227c bytes
// (PS2's 0x24f0). Only the constructor so far, which C_mission::CreateActor calls for ACTOR_CAR_EX, and what the
// scripts read.
class C_car_ex : public C_car
{
public:
    C_car_ex();

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours): whether a
    // human can get into the truck, the scripts' HUMAN_ENTERTOTRUCK's test
    // FUNCTION: GAME 0x47b6d0
    bool CanUseTruck() const
    {
        return unknown_223c && unknown_2258;
    }

    uchar unknown_221c[0x20];           // 0x221c
    bool unknown_223c;                  // 0x223c, CanUseTruck's
    uchar unknown_223d[0x1b];           // 0x223d
    uint unknown_2258;                  // 0x2258, CanUseTruck's, tested as a whole (a count or a pointer)
    uchar unknown_225c[0x20];           // 0x225c
};
