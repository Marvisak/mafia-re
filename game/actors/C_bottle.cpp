// The bottles (PS2 C_bottle.cpp; the PC's file is the one at 0x412350-0x413d8c, docs/game-files.md), which starts
// with the constructor. Written so far: the constructor, which C_mission::CreateActor calls.

#include "actors/C_bottle.h"
#include "stub.h"

// PS2's, its stores in this order
// FUNCTION: GAME 0x412350
C_bottle::C_bottle()
{
    unknown_fc = true;
    type = ACTOR_BOTTLE;
    unknown_6d = true;
    unknown_128 = true;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4123b0
C_bottle::~C_bottle()
{
}

#pragma auto_inline(off)

// STUB: GAME 0x4123c0
bool C_bottle::Init(I3D_frame *frm)
{
    StubLog("C_bottle::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x4123f0
void C_bottle::AI(uint time)
{
    StubLog("C_bottle::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x413040
void C_bottle::Update(uint time)
{
    StubLog("C_bottle::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x413120
void C_bottle::GameInit()
{
    StubLog("C_bottle::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x413440
void C_bottle::GameDone()
{
    StubLog("C_bottle::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x413470
int C_bottle::SaveGameGetSize()
{
    StubLog("C_bottle::SaveGameGetSize");
    return C_actor::SaveGameGetSize();
}

// STUB: GAME 0x413480
bool C_bottle::SaveGameSave(void **buf)
{
    StubLog("C_bottle::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x413560
bool C_bottle::SaveGameLoad(void **buf)
{
    StubLog("C_bottle::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// STUB: GAME 0x413840
bool C_bottle::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_bottle::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x413ac0
bool C_bottle::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_bottle::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_bottle::NetDirect(uint time)
{
}

#pragma auto_inline(on)
