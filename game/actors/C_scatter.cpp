// The objects scattered around a frame (PC only, the file's name is ours, after the class; the PC's file is the one
// at 0x58e350-0x58f15c, docs/game-files.md), which starts with the constructor. Not written yet: the constructor,
// which C_mission::CreateActor calls, is a stub.

#include "actors/C_scatter.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x58e350
C_scatter::C_scatter()
{
    StubLog("C_scatter::C_scatter");
}

#pragma auto_inline(on)
