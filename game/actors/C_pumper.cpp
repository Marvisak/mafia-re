// The petrol pump attendants (PS2 C_pumper.cpp; the PC's file is the one at 0x47c540-0x48192c, docs/game-files.md),
// which starts with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, and
// ScriptCanPump are stubs.

#include "actors/C_pumper.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x47c540
C_pumper::C_pumper()
{
    StubLog("C_pumper::C_pumper");
}

// STUB: GAME 0x481840
int C_pumper::ScriptCanPump(uint id)
{
    StubLog("C_pumper::ScriptCanPump");
    return 0;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x47c700
C_pumper::~C_pumper()
{
    StubLog("C_pumper::~C_pumper");
}

// The linker folded it with C_dog, C_irenka, C_seagull, C_airplane, C_turnout's
// STUB: GAME 0x47c760 FOLDED
bool C_pumper::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x47c780
bool C_pumper::SaveData(C_chunk *chunk)
{
    StubLog("C_pumper::SaveData");
    return C_actor::SaveData(chunk);
}

// STUB: GAME 0x47c860
bool C_pumper::LoadData(C_chunk *chunk)
{
    StubLog("C_pumper::LoadData");
    return C_actor::LoadData(chunk);
}

// STUB: GAME 0x47ca60
void C_pumper::DuplicateFrom(C_actor *act)
{
    StubLog("C_pumper::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x47caa0
void C_pumper::GameInit()
{
    StubLog("C_pumper::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x47e6e0
void C_pumper::GameDone()
{
    StubLog("C_pumper::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x47e940
bool C_pumper::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_pumper::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x47e9c0
bool C_pumper::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_pumper::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x47ea30
void C_pumper::AI(uint time)
{
    StubLog("C_pumper::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x47f560
void C_pumper::Update(uint time)
{
    StubLog("C_pumper::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x481190
int C_pumper::SaveGameGetSize()
{
    StubLog("C_pumper::SaveGameGetSize");
    return C_actor::SaveGameGetSize();
}

// STUB: GAME 0x4811a0
bool C_pumper::SaveGameSave(void **buf)
{
    StubLog("C_pumper::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x4813e0
bool C_pumper::SaveGameLoad(void **buf)
{
    StubLog("C_pumper::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

#pragma auto_inline(on)
