// The race cameras (PS2 c_racecam.cpp; the PC's file is the one at 0x596200-0x59641c, docs/game-files.md), which
// starts with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "camera/c_racecam.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x596200
C_racecam::C_racecam()
{
    StubLog("C_racecam::C_racecam");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x596240
C_racecam::~C_racecam()
{
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// The linker's folded copy (0x4aa490)
bool C_racecam::SaveGameSave(void **buf)
{
    return true;
}

// The linker's folded copy (0x4aa490)
bool C_racecam::SaveGameLoad(void **buf)
{
    return true;
}

// STUB: GAME 0x596250
bool C_racecam::Init(I3D_frame *frm)
{
    StubLog("C_racecam::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x5962b0
bool C_racecam::SaveData(C_chunk *chunk)
{
    StubLog("C_racecam::SaveData");
    C_actor::SaveData(chunk);
    return true;
}

// STUB: GAME 0x5962e0
bool C_racecam::LoadData(C_chunk *chunk)
{
    StubLog("C_racecam::LoadData");
    C_actor::LoadData(chunk);
    return true;
}

// STUB: GAME 0x596360
void C_racecam::DuplicateFrom(C_actor *act)
{
    StubLog("C_racecam::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x596380
void C_racecam::GameInit()
{
    StubLog("C_racecam::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x5963d0
void C_racecam::GameDone()
{
    StubLog("C_racecam::GameDone");
    C_actor::GameDone();
}

#pragma auto_inline(on)
