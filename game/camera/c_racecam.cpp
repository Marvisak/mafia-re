// The race cameras (PS2 c_racecam.cpp; the PC's file is the one at 0x596200-0x59641c, docs/game-files.md), which
// starts with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "camera/c_racecam.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x596200
C_racecam::C_racecam()
{
    StubLog("C_racecam::C_racecam");
}

#pragma auto_inline(on)
