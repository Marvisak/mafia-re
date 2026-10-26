// The humans (PS2 C_human.cpp; the PC's file is the one at 0x56b180-0x58e34c, docs/game-files.md), whose constructor
// (0x56efd0) comes after functions of its own. Not written yet: the constructor, which the derived classes'
// constructors call, is a stub.

#include "actors/C_human.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x56efd0
C_human::C_human()
{
    StubLog("C_human::C_human");
}

#pragma auto_inline(on)
