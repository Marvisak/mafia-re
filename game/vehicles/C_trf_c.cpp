// The traffic's cars (PS2 C_trf_c.cpp, C_traffic_car's; the PC's file is the one at 0x4aa490-0x4bc35c,
// docs/game-files.md), whose constructor (0x4aa7a0) comes after functions of its own. Not written yet: the
// constructor, which C_mission::CreateActor calls, and TakATedSiVasKupim_Vsecky and VypraskeeeKaaaru are stubs.

#include "vehicles/C_trf_c.h"

#include "stub.h"

// GLOBAL: GAME 0x63d7fc
float C_traffic_car::unknown_63d7fc;
// GLOBAL: GAME 0x63d804
float C_traffic_car::unknown_63d804;

#pragma auto_inline(off)

// STUB: GAME 0x4aa7a0
C_traffic_car::C_traffic_car()
{
    StubLog("C_traffic_car::C_traffic_car");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x4bad80
void C_traffic_car::TakATedSiVasKupim_Vsecky()
{
    StubLog("C_traffic_car::TakATedSiVasKupim_Vsecky");
}

// STUB: GAME 0x4bbe90
void C_traffic_car::VypraskeeeKaaaru(C_actor *act)
{
    StubLog("C_traffic_car::VypraskeeeKaaaru");
}

#pragma auto_inline(on)

