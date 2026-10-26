// The rail vehicles (most likely PS2 C_railway.cpp; the PC's file is the one at 0x4826c0-0x49908c,
// docs/game-files.md), whose constructor (0x4865b0) comes after functions of its own. Not written yet: the
// constructor, which C_mission::CreateActor calls, is a stub.

#include "vehicles/C_railway.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4865b0
C_railway::C_railway()
{
    StubLog("C_railway::C_railway");
}

#pragma auto_inline(on)
