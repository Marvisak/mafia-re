// The stones (PC only; the file is the one at 0x4aa2b0-0x4aa48c, between C_sledovacka.cpp and C_trf_c.cpp, its name
// ours, docs/game-files.md), which starts with the constructor. Written so far: the constructor, which C_game::Init
// calls.

#include "actors/C_stone.h"
#include "stub.h"

// FUNCTION: GAME 0x4aa2b0
C_stone::C_stone()
{
    unknown_fc = false;
    unknown_6d = false;
    type = ACTOR_STONE;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4aa300
C_stone::~C_stone()
{
}

#pragma auto_inline(off)

// The original's is C_cartridge's, folded: it logs that name, so ours folds too
// STUB: GAME 0x430830 FOLDED
bool C_stone::Init(I3D_frame *frm)
{
    StubLog("C_cartridge::Init");
    return C_bottle::Init(frm);
}

// STUB: GAME 0x4aa310
void C_stone::AI(uint time)
{
    StubLog("C_stone::AI");
    C_bottle::AI(time);
}

// STUB: GAME 0x4aa380
void C_stone::Update(uint time)
{
    StubLog("C_stone::Update");
    C_bottle::Update(time);
}

// STUB: GAME 0x4aa3a0
void C_stone::GameInit()
{
    StubLog("C_stone::GameInit");
    C_bottle::GameInit();
}

// STUB: GAME 0x4aa400
void C_stone::GameDone()
{
    StubLog("C_stone::GameDone");
    C_bottle::GameDone();
}

// The linker's folded copy of the out-of-line kind (0x4aa430)
int C_stone::SaveGameGetSize()
{
    return 0;
}

#pragma auto_inline(on)
