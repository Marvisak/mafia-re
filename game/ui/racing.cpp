// The racing championship (PS2 racing.cpp; the PC's file is the one at 0x54d320-0x55997c, docs/game-files.md). Not
// written yet: only what main and C_game need to link.

#include "ui/racing.h"

#include "stub.h"

// GLOBAL: GAME 0x646dbc
C_racing *CRacing;

// STUB: GAME 0x551830
C_racing::C_racing()
{
    StubLog("C_racing::C_racing");
}

// STUB: GAME 0x551a70
C_racing::~C_racing()
{
    StubLog("C_racing::~C_racing");
}

// STUB: GAME 0x552e40
void C_racing::GameInit()
{
    StubLog("C_racing::GameInit");
}

// STUB: GAME 0x552f90
void C_racing::GameDone()
{
    StubLog("C_racing::GameDone");
}

// STUB: GAME 0x555e40
void C_racing::Tick(uint time)
{
    StubLog("C_racing::Tick");
}

// STUB: GAME 0x5571d0
void C_racing::LoadReplay(uint size, void *data)
{
    StubLog("C_racing::LoadReplay");
}
