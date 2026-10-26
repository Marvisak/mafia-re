// The cars with dynamic collisions (PS2 C_car_ex.cpp; the PC's file is the one at 0x42e240-0x4307cc,
// docs/game-files.md), which starts with the constructor. Not written yet: the constructor, which
// C_mission::CreateActor calls, is a stub.

#include "vehicles/C_car_ex.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x42e240
C_car_ex::C_car_ex()
{
    StubLog("C_car_ex::C_car_ex");
}

#pragma auto_inline(on)
