// The dogs (PS2 C_dog.cpp; the PC's file is the one at 0x433fe0-0x43923c, docs/game-files.md), whose constructor
// (0x434030) comes after a function of its own. Not written yet: the constructor, which C_mission::CreateActor
// calls, is a stub.

#include "actors/C_dog.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x434030
C_dog::C_dog()
{
    StubLog("C_dog::C_dog");
}

#pragma auto_inline(on)
