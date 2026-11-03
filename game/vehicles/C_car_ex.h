#pragma once

#include "vehicles/C_car.h"

// The cars with dynamic collisions (PS2 C_car_ex, C_car_ex.cpp, whose S_dyn_coll they are), a car, 0x227c bytes
// (PS2's 0x24f0). Written: Init and what the scripts read; the constructor, which C_mission::CreateActor calls for
// ACTOR_CAR_EX, is a stub.
// VTABLE: GAME 0x6237d4 C_Vehicle
// VTABLE: GAME 0x6237d8 C_actor
class C_car_ex : public C_car
{
public:
    C_car_ex();
    virtual ~C_car_ex();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void Update(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    // The base's: the original's jumps there
    // FUNCTION: GAME 0x42f150 FOLDED
    virtual void DuplicateFrom(C_actor *act)
    {
        C_car::DuplicateFrom(act);
    }
    virtual void GameInit();
    virtual void GameDone();

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

// SYNTHETIC: GAME 0x42e2e0
// C_car_ex::`scalar deleting destructor'
