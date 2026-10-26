// The doors (PS2 C_door.cpp; the PC's file is the one at 0x439240-0x43bd8c, docs/game-files.md), whose constructor
// (0x439290) comes after a function of its own. Not written yet: the constructor, which C_mission::CreateActor
// calls, is a stub.

#include "actors/C_door.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x439290
C_door::C_door()
{
    StubLog("C_door::C_door");
}

#pragma auto_inline(on)
