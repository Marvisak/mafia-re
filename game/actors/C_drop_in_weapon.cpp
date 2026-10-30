// The weapons put down (PS2 C_drop_in_weapon.cpp; the PC's file is most likely the one at 0x4435b0-0x443a2c,
// docs/game-files.md, between C_driver.cpp and C_ff_manager.cpp as PS2's sorts), which starts with the constructor. Not
// written yet: the constructor, which the script's UNLINKFROMHAND calls, is a stub.

#include "actors/C_drop_in_weapon.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4435b0
C_drop_in_weapon::C_drop_in_weapon()
{
    StubLog("C_drop_in_weapon::C_drop_in_weapon");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x443640
C_drop_in_weapon::~C_drop_in_weapon()
{
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x443660
void C_drop_in_weapon::Update(uint time)
{
    StubLog("C_drop_in_weapon::Update");
    C_PhThing::Update(time);
}

// STUB: GAME 0x4436c0
void C_drop_in_weapon::ChangeState()
{
    StubLog("C_drop_in_weapon::ChangeState");
    C_PhThing::ChangeState();
}

// STUB: GAME 0x443870
void C_drop_in_weapon::GameInit()
{
    StubLog("C_drop_in_weapon::GameInit");
    C_PhThing::GameInit();
}

// STUB: GAME 0x443950
void C_drop_in_weapon::GameDone()
{
    StubLog("C_drop_in_weapon::GameDone");
    C_PhThing::GameDone();
}

#pragma auto_inline(on)
