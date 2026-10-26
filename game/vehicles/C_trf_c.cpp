// The traffic's cars (PS2 C_trf_c.cpp, C_traffic_car's; the PC's file is the one at 0x4aa490-0x4bc35c,
// docs/game-files.md), whose constructor (0x4aa7a0) comes after functions of its own. Not written yet: the
// constructor, which C_mission::CreateActor calls, is a stub.

#include "vehicles/C_trf_c.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4aa7a0
C_traffic_car::C_traffic_car()
{
    StubLog("C_traffic_car::C_traffic_car");
}

#pragma auto_inline(on)
