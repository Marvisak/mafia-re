// The objects scattered around a frame (PC only, the file's name is ours, after the class; the PC's file is the one
// at 0x58e350-0x58f15c, docs/game-files.md), which starts with the constructor. Not written yet: the constructor,
// which C_mission::CreateActor calls, is a stub.

#include "actors/C_scatter.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x58e350
C_scatter::C_scatter()
{
    StubLog("C_scatter::C_scatter");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x58e3c0
C_scatter::~C_scatter()
{
    StubLog("C_scatter::~C_scatter");
}

// The linker's folded copy (0x4aa490)
bool C_scatter::SaveGameSave(void **buf)
{
    return true;
}

// The linker's folded copy (0x4aa490)
bool C_scatter::SaveGameLoad(void **buf)
{
    return true;
}

// STUB: GAME 0x58e3f0
bool C_scatter::Init(I3D_frame *frm)
{
    StubLog("C_scatter::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x58e420
bool C_scatter::SaveData(C_chunk *chunk)
{
    StubLog("C_scatter::SaveData");
    C_actor::SaveData(chunk);
    return true;
}

// STUB: GAME 0x58e470
bool C_scatter::LoadData(C_chunk *chunk)
{
    StubLog("C_scatter::LoadData");
    C_actor::LoadData(chunk);
    return true;
}

// STUB: GAME 0x58e550
void C_scatter::DuplicateFrom(C_actor *act)
{
    StubLog("C_scatter::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x58e590
void C_scatter::GameInit()
{
    StubLog("C_scatter::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x58e9d0
void C_scatter::GameDone()
{
    StubLog("C_scatter::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x58ead0
void C_scatter::AI(uint time)
{
    StubLog("C_scatter::AI");
    C_actor::AI(time);
}

#pragma auto_inline(on)
