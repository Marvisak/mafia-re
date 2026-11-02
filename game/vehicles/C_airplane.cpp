// The airplanes (PS2 C_airplane.cpp; the PC's file is the one at 0x409390-0x40df6c, docs/game-files.md), which
// starts with the constructor. Written: SaveGameGetSize, GetMaterialID and IsDestroyed; the constructor, which
// C_mission::CreateActor calls, and the scripts' other functions are stubs.

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

// PS2 name; the scripts' AIRPLANE_ISDESTROYED. Both of its values are under 0.01
// FUNCTION: GAME 0x40d6c0
bool C_airplane::IsDestroyed()
{
    bool destroyed = unknown_1cc < 0.01 && unknown_1c8 < 0.01;
    return destroyed;
}

// STUB: GAME 0x40ddd0
void C_airplane::ShowDamageIndicator(bool show)
{
    StubLog("C_airplane::ShowDamageIndicator %d", show);
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x409520
C_airplane::~C_airplane()
{
    StubLog("C_airplane::~C_airplane");
}

// STUB: GAME 0x409580
void C_airplane::SaveData(C_chunk *chunk)
{
    StubLog("C_airplane::SaveData");
    C_actor::SaveData(chunk);
}

// STUB: GAME 0x4097c0
void C_airplane::LoadData(C_chunk *chunk)
{
    StubLog("C_airplane::LoadData");
    C_actor::LoadData(chunk);
}

// STUB: GAME 0x409a40
void C_airplane::DuplicateFrom(C_actor *act)
{
    StubLog("C_airplane::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x409b00
void C_airplane::GameInit()
{
    StubLog("C_airplane::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x40a800
void C_airplane::GameDone()
{
    StubLog("C_airplane::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x40bc10
void C_airplane::AI(uint time)
{
    StubLog("C_airplane::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x40bec0
void C_airplane::Update(uint time)
{
    StubLog("C_airplane::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x40c880
bool C_airplane::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_airplane::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// FUNCTION: GAME 0x40d6f0
int C_airplane::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + 124;
}

// STUB: GAME 0x40d700
bool C_airplane::SaveGameSave(void **buf)
{
    StubLog("C_airplane::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x40d830
bool C_airplane::SaveGameLoad(void **buf)
{
    StubLog("C_airplane::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// STUB: GAME 0x40dce0
void C_airplane::ChangeState()
{
    StubLog("C_airplane::ChangeState");
    C_actor::ChangeState();
}

// FUNCTION: GAME 0x40df30
int C_airplane::GetMaterialID(I3D_frame *frm)
{
    return 19;
}

// The original's is C_pumper's, folded: it logs that name, so ours folds too
// STUB: GAME 0x47c760 FOLDED
bool C_airplane::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_airplane::NetDirect(uint time)
{
}

#pragma auto_inline(on)
