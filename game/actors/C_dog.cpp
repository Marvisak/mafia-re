// The dogs (PS2 C_dog.cpp; the PC's file is the one at 0x433fe0-0x43923c, docs/game-files.md), whose constructor
// (0x434030) comes after a function of its own. Not written yet: the constructor, which C_mission::CreateActor
// calls, is a stub.

#include "actors/C_dog.h"
#include "actors/C_animations.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x434030
C_dog::C_dog()
{
    StubLog("C_dog::C_dog");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x434b50
void C_dog::LoadTCK()
{
    // STRING: GAME 0x63246c
    C_animations_machine_de_luxe::LoadTrack("DturnL.tck");
    // STRING: GAME 0x632460
    C_animations_machine_de_luxe::LoadTrack("DturnR.tck");
    // STRING: GAME 0x632454
    C_animations_machine_de_luxe::LoadTrack("Ddeath.tck");
}

#pragma auto_inline(off)

// STUB: GAME 0x434250
C_dog::~C_dog()
{
    StubLog("C_dog::~C_dog");
}

// STUB: GAME 0x4342d0
bool C_dog::SaveData(C_chunk *chunk)
{
    StubLog("C_dog::SaveData");
    C_actor::SaveData(chunk);
    return true;
}

// STUB: GAME 0x434410
bool C_dog::LoadData(C_chunk *chunk)
{
    StubLog("C_dog::LoadData");
    C_actor::LoadData(chunk);
    return true;
}

// STUB: GAME 0x434630
void C_dog::DuplicateFrom(C_actor *act)
{
    StubLog("C_dog::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x434690
void C_dog::GameInit()
{
    StubLog("C_dog::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x434b70
void C_dog::GameDone()
{
    StubLog("C_dog::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x4352a0
bool C_dog::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act, uint u,
    I3D_frame *frm)
{
    StubLog("C_dog::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x435660
void C_dog::Death()
{
    StubLog("C_dog::Death");
    C_actor::Death();
}

// STUB: GAME 0x4356e0
void C_dog::AI(uint time)
{
    StubLog("C_dog::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x437070
void C_dog::Update(uint time)
{
    StubLog("C_dog::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x438960
int C_dog::SaveGameGetSize()
{
    StubLog("C_dog::SaveGameGetSize");
    return C_actor::SaveGameGetSize();
}

// STUB: GAME 0x438970
bool C_dog::SaveGameSave(void **buf)
{
    StubLog("C_dog::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x438b50
bool C_dog::SaveGameLoad(void **buf)
{
    StubLog("C_dog::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// STUB: GAME 0x438e20
bool C_dog::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, I3D_frame *frm)
{
    StubLog("C_dog::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x438eb0
void C_dog::cbCollTester(tDynamicCollObject *obj)
{
    StubLog("C_dog::cbCollTester");
    C_actor::cbCollTester(obj);
}

// The original's is C_pumper's, folded: it logs that name, so ours folds too
// STUB: GAME 0x47c760 FOLDED
bool C_dog::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

#pragma auto_inline(on)
