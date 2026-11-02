// The wagons (PS2 C_wagon.cpp; the PC's file is the one at 0x4f1b50-0x4f5dbc, docs/game-files.md), which starts with
// the constructor. Written: SaveGameGetSize; the constructor, which C_mission::CreateActor calls, is a stub.

#include "vehicles/C_wagon.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4f1b50
C_wagon::C_wagon()
{
    StubLog("C_wagon::C_wagon");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x4f1c20
C_wagon::~C_wagon()
{
    StubLog("C_wagon::~C_wagon");
}

// The original's is C_door's, folded: it logs that name, so ours folds too
// STUB: GAME 0x4393a0 FOLDED
bool C_wagon::Init(I3D_frame *frm)
{
    StubLog("C_door::Init");
    return C_actor::Init(frm);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_wagon::NetDirect(uint time)
{
}

// FUNCTION: GAME 0x4f1ca0
int C_wagon::SaveGameGetSize()
{
    int size = C_actor::SaveGameGetSize() + 95;
    size += sizeof(uint) * unknown_a8.size();
    size += sizeof(uint) * path.size();
    return size;
}

// STUB: GAME 0x4f1cf0
bool C_wagon::SaveGameSave(void **buf)
{
    StubLog("C_wagon::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x4f1eb0
bool C_wagon::SaveGameLoad(void **buf)
{
    StubLog("C_wagon::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// STUB: GAME 0x4f3520
void C_wagon::AI(uint time)
{
    StubLog("C_wagon::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x4f48f0
void C_wagon::Update(uint time)
{
    StubLog("C_wagon::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x4f4d70
void C_wagon::GameInit()
{
    StubLog("C_wagon::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x4f5bc0
void C_wagon::GameDone()
{
    StubLog("C_wagon::GameDone");
    C_actor::GameDone();
}

#pragma auto_inline(on)
