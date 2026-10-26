// The rail vehicles' generator (PS2 C_rail_generator.cpp; the PC's file is the one at 0x596420-0x5983dc,
// docs/game-files.md), whose constructor (0x596620) comes after functions of its own. Not written yet: the
// constructor, which C_mission::CreateActor calls, is a stub.

#include "vehicles/C_rail_generator.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x596620
C_rail_generator::C_rail_generator()
{
    StubLog("C_rail_generator::C_rail_generator");
}

#pragma auto_inline(on)
