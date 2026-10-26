// The mission's rmorf.bin (PS2 C_rmorf.cpp; the PC's file is the one at 0x499090-0x499b3c, docs/game-files.md): the
// initializer of its global C_rmorf (0x63d7f0), the sets' loader (0x4990d0, PS2 RMRF_S_SETREF::LoadRAWSet), the
// constructor, the destructor, the sets' scalar deleting destructor, then the virtual functions, PS2's order. Written
// so far: the constructor and the destructor, which the game's constructor and destructor call; the virtual functions,
// which the vtable needs, are stubs.

#include "mission/C_rmorf.h"

#include "stub.h"

// The vtable stored first: an assignment in the body (an initializer stores before it), PS2's order too
// FUNCTION: GAME 0x499280
C_rmorf::C_rmorf()
{
    sets = NULL;
}

// PS2's too: the sets deleted, then the pointer cleared whether there were any
// FUNCTION: GAME 0x499290
C_rmorf::~C_rmorf()
{
    if (sets)
        delete sets;
    sets = NULL;
}

#pragma auto_inline(off)

// STUB: GAME 0x499330
void C_rmorf::Mission_Start()
{
    StubLog("C_rmorf::Mission_Start");
}

// STUB: GAME 0x499560
void C_rmorf::Mission_Done()
{
    StubLog("C_rmorf::Mission_Done");
}

// STUB: GAME 0x4995b0
void C_rmorf::Tick(int time)
{
    StubLog("C_rmorf::Tick %i", time);
}

#pragma auto_inline(on)
