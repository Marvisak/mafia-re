// The rails' turnouts (PS2 C_turnout.cpp; the PC's file is the one at 0x4bc360-0x4bd64c, docs/game-files.md), which
// starts with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "vehicles/C_turnout.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4bc360
C_turnout::C_turnout()
{
    StubLog("C_turnout::C_turnout");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4bc490
C_turnout::~C_turnout()
{
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// The original's is C_pumper's, folded: it logs that name, so ours folds too
// STUB: GAME 0x47c760 FOLDED
bool C_turnout::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x4bc4a0
void C_turnout::SaveData(C_chunk *chunk)
{
    StubLog("C_turnout::SaveData");
    C_actor::SaveData(chunk);
}

// STUB: GAME 0x4bc600
void C_turnout::LoadData(C_chunk *chunk)
{
    StubLog("C_turnout::LoadData");
    C_actor::LoadData(chunk);
}

// STUB: GAME 0x4bc830
void C_turnout::DuplicateFrom(C_actor *act)
{
    StubLog("C_turnout::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x4bc8c0
void C_turnout::GameInit()
{
    StubLog("C_turnout::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x4bcd40
void C_turnout::GameDone()
{
    StubLog("C_turnout::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x4bce10
void C_turnout::AI(uint time)
{
    StubLog("C_turnout::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x4bcfe0
void C_turnout::Update(uint time)
{
    StubLog("C_turnout::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x4bd420
int C_turnout::SaveGameGetSize()
{
    StubLog("C_turnout::SaveGameGetSize");
    return C_actor::SaveGameGetSize();
}

// STUB: GAME 0x4bd430
bool C_turnout::SaveGameSave(void **buf)
{
    StubLog("C_turnout::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x4bd470
bool C_turnout::SaveGameLoad(void **buf)
{
    StubLog("C_turnout::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

#pragma auto_inline(on)
