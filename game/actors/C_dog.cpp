// The dogs (PS2 C_dog.cpp; the PC's file is the one at 0x433fe0-0x43923c, docs/game-files.md), whose constructor
// (0x434030) comes after a function of its own. Not written yet: the constructor, which C_mission::CreateActor
// calls, is a stub.

#include "actors/C_dog.h"
#include "actors/C_animations.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x434030
C_dog::C_dog()
{
    StubLog("C_dog::C_dog");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x434b50
void C_dog::LoadTCK()
{
    // STRING: GAME 0x63246c
    C_animations_machine_de_luxe::LoadTrack("DturnL.tck");
    // STRING: GAME 0x632460
    C_animations_machine_de_luxe::LoadTrack("DturnR.tck");
    // STRING: GAME 0x632454
    C_animations_machine_de_luxe::LoadTrack("Ddeath.tck");
}

