// The airplanes (PS2 C_airplane.cpp; the PC's file is the one at 0x409390-0x40df6c, docs/game-files.md), which
// starts with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, and the functions
// the scripts call are stubs.

#include "vehicles/C_airplane.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x409390
C_airplane::C_airplane()
{
    StubLog("C_airplane::C_airplane");
}

// STUB: GAME 0x40c610
void C_airplane::Start(uint unknown, float speed)
{
    StubLog("C_airplane::Start %u", unknown);
}

// STUB: GAME 0x40c7f0
void C_airplane::Start2()
{
    StubLog("C_airplane::Start2");
}

// STUB: GAME 0x40d6c0
bool C_airplane::IsDestroyed()
{
    StubLog("C_airplane::IsDestroyed");
    return false;
}

// STUB: GAME 0x40ddd0
void C_airplane::ShowDamageIndicator(bool show)
{
    StubLog("C_airplane::ShowDamageIndicator %d", show);
}

#pragma auto_inline(on)
