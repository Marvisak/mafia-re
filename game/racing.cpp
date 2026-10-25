// The racing championship (PS2 racing.cpp; the PC's file is the one at 0x54d320-0x55997c, docs/game-files.md). Not
// written yet: only what main needs to link.

#include "racing.h"

#include "stub.h"

// GLOBAL: GAME 0x646dbc
C_racing *CRacing;

#pragma auto_inline(off)

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

#pragma auto_inline(on)
