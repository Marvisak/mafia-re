// The game of the loaded mission (PS2 game.cpp; the PC's file is the one at 0x5a0760-0x5bcc0c, docs/game-files.md).
// Not written yet: only what main and C_mission need to link.

#include "mission/game.h"

#include "stub.h"

#pragma auto_inline(off)

// Inline in the original (game.h), its copy in Mission.cpp's range
// STUB: GAME 0x5472f0
C_game::C_game()
{
    StubLog("C_game::C_game");
}

// Inline in the original (game.h), its copy in Mission.cpp's range. It first calls three of the game's methods
// (0x5b1da0, 0x5b2590 and 0x5b2320, PS2's InventoryStackClear, CharacterStackClear and FloatStackClear).
// STUB: GAME 0x547a60
C_game::~C_game()
{
    StubLog("C_game::~C_game");
}

// STUB: GAME 0x5a0810
void C_game::Init()
{
    StubLog("C_game::Init");
}

#pragma auto_inline(on)
