// The script objects (PS2 C_detector.cpp; the PC's file is the one at 0x432870-0x433fdc, docs/game-files.md), whose
// constructor (0x432880) comes first. Not written yet: the constructor, which C_mission::CreateActor calls, is a
// stub.

#include "actors/C_detector.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x432880
C_detector::C_detector()
{
    StubLog("C_detector::C_detector");
}

#pragma auto_inline(on)
