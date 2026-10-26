// The player (PS2 C_player.cpp; the PC's file is the one at 0x58f160-0x5961fc, docs/game-files.md), whose
// constructor (0x58f170) comes first. Not written yet: the constructor, which C_mission::CreateActor calls, is a
// stub.

#include "actors/C_player.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x58f170
C_player::C_player()
{
    StubLog("C_player::C_player");
}

#pragma auto_inline(on)
