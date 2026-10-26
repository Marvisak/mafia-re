// The clocks (PS2 c_clock.cpp; the PC's file is the one at 0x56add0-0x56b17c, docs/game-files.md), which starts with
// the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "actors/c_clock.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x56add0
C_clock::C_clock()
{
    StubLog("C_clock::C_clock");
}

#pragma auto_inline(on)
