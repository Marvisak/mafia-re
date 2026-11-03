// The bridges (PS2 C_bridge.cpp; the PC's file is the one at 0x567cf0-0x56ad9c, docs/game-files.md), whose
// constructor (0x567d10) comes after a function of its own. Not written yet: the constructor, which
// C_mission::CreateActor calls, and ShutDown are stubs.

#include "actors/C_bridge.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x567d10
C_bridge::C_bridge()
{
    StubLog("C_bridge::C_bridge");
}

// STUB: GAME 0x56a7b0
void C_bridge::ShutDown(bool shut)
{
    StubLog("C_bridge::ShutDown");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x567e20
C_bridge::~C_bridge()
{
    StubLog("C_bridge::~C_bridge");
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_bridge::NetDirect(uint time)
{
}

// STUB: GAME 0x567e70
bool C_bridge::Init(I3D_frame *frm)
{
    StubLog("C_bridge::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x567fb0
void C_bridge::AI(uint time)
{
    StubLog("C_bridge::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x568540
void C_bridge::Update(uint time)
{
    StubLog("C_bridge::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x569510
void C_bridge::GameInit()
{
    StubLog("C_bridge::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x56a600
void C_bridge::GameDone()
{
    StubLog("C_bridge::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x56a6b0
bool C_bridge::LoadData(C_chunk *chunk)
{
    StubLog("C_bridge::LoadData");
    C_actor::LoadData(chunk);
    return true;
}

// STUB: GAME 0x56a790
bool C_bridge::SaveGameSave(void **buf)
{
    StubLog("C_bridge::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x56a7e0
bool C_bridge::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_bridge::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// The linker's folded copy of the out-of-line kind (0x4aa430)
int C_bridge::SaveGameGetSize()
{
    return 0;
}

#pragma auto_inline(on)
