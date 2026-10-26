// The public physics (PS2 C_public_physics.cpp; the PC's file is the one at 0x47c320-0x47c53c, docs/game-files.md),
// whose constructor (0x47c3a0) comes after a function of its own. Not written yet: the constructor, which
// C_mission::CreateActor calls, is a stub.

#include "actors/C_public_physics.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x47c3a0
C_public_physics::C_public_physics()
{
    StubLog("C_public_physics::C_public_physics");
}

#pragma auto_inline(on)
