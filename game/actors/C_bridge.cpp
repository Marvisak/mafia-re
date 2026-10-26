// The bridges (PS2 C_bridge.cpp; the PC's file is the one at 0x567cf0-0x56ad9c, docs/game-files.md), whose
// constructor (0x567d10) comes after a function of its own. Not written yet: the constructor, which
// C_mission::CreateActor calls, is a stub.

#include "actors/C_bridge.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x567d10
C_bridge::C_bridge()
{
    StubLog("C_bridge::C_bridge");
}

#pragma auto_inline(on)
