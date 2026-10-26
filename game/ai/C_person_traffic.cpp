// The pedestrians' generator (PS2 C_person_traffic.cpp, C_traffic_generator's; the PC's file is the one at
// 0x44c520-0x459e8c, docs/game-files.md), whose constructor (0x44d410) comes after functions of its own. Not written
// yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "ai/C_person_traffic.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x44d410
C_traffic_generator::C_traffic_generator()
{
    StubLog("C_traffic_generator::C_traffic_generator");
}

#pragma auto_inline(on)
