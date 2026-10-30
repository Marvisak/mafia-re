// The cartridge cases (PS2 C_cartridge.cpp; the PC's file is most likely the one at 0x4307d0-0x430b9c, after C_car_ex.cpp
// as the name sorts, docs/game-files.md), which starts with the constructor. Written so far: the constructor, which
// C_game::Init calls, and the sounds.

#include "actors/C_cartridge.h"
#include "stub.h"

// GLOBAL: GAME 0x63d6c8
I3D_sound *C_cartridge::sounds[3];

// FUNCTION: GAME 0x4307d0
C_cartridge::C_cartridge()
{
    unknown_fc = false;
    unknown_6d = false;
    type = ACTOR_CARTRIDGE;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x430820
C_cartridge::~C_cartridge()
{
}

#pragma auto_inline(off)

// The linker folded it with C_stone's
// STUB: GAME 0x430830 FOLDED
bool C_cartridge::Init(I3D_frame *frm)
{
    StubLog("C_cartridge::Init");
    return C_bottle::Init(frm);
}

// STUB: GAME 0x430850
void C_cartridge::AI(uint time)
{
    StubLog("C_cartridge::AI");
    C_bottle::AI(time);
}

// STUB: GAME 0x430a70
void C_cartridge::GameInit()
{
    StubLog("C_cartridge::GameInit");
    C_bottle::GameInit();
}

// STUB: GAME 0x430ad0
void C_cartridge::GameDone()
{
    StubLog("C_cartridge::GameDone");
    C_bottle::GameDone();
}

// The linker's folded copy of the out-of-line kind (0x4aa430)
int C_cartridge::SaveGameGetSize()
{
    return 0;
}

#pragma auto_inline(on)
