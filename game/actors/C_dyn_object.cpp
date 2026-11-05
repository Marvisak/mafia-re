// The objects of dynamic parts (PC only, the file's name is ours, after the class; the PC's file is the one at
// 0x413d90-0x41bb4c, docs/game-files.md), whose constructor (0x418930) follows the vector math it starts with. Not
// written yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "actors/C_dyn_object.h"

#include "stub.h"

// STUB: GAME 0x418930
C_dyn_object::C_dyn_object()
{
    StubLog("C_dyn_object::C_dyn_object");
}

// STUB: GAME 0x418e00
C_dyn_object::~C_dyn_object()
{
    StubLog("C_dyn_object::~C_dyn_object");
}

// STUB: GAME 0x414aa0
bool C_dyn_object::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_dyn_object::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x414e00
bool C_dyn_object::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_dyn_object::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x4189d0
bool C_dyn_object::Init(I3D_frame *frm)
{
    StubLog("C_dyn_object::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x4195f0
void C_dyn_object::GameInit()
{
    StubLog("C_dyn_object::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x419b00
void C_dyn_object::GameDone()
{
    StubLog("C_dyn_object::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x419b20
bool C_dyn_object::SaveData(C_chunk *chunk)
{
    StubLog("C_dyn_object::SaveData");
    return C_actor::SaveData(chunk);
}

// STUB: GAME 0x419b80
bool C_dyn_object::LoadData(C_chunk *chunk)
{
    StubLog("C_dyn_object::LoadData");
    return C_actor::LoadData(chunk);
}

// STUB: GAME 0x419c70
void C_dyn_object::AI(uint time)
{
    StubLog("C_dyn_object::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x41b6b0
bool C_dyn_object::AddToCutscene()
{
    StubLog("C_dyn_object::AddToCutscene");
    return C_actor::AddToCutscene();
}

// STUB: GAME 0x41b6d0
int C_dyn_object::RecordKey(int a)
{
    StubLog("C_dyn_object::RecordKey");
    return C_actor::RecordKey(a);
}

// The linker folded it with C_actor_shot's
// STUB: GAME 0x41bae0 FOLDED
S_vector C_dyn_object::GetRecordPos(int a)
{
    StubLog("C_dyn_object::GetRecordPos");
    return C_actor::GetRecordPos(a);
}

// The linker folded it with C_actor_shot's
// STUB: GAME 0x41bb00 FOLDED
S_quat C_dyn_object::GetRecordRot(int a)
{
    StubLog("C_dyn_object::GetRecordRot");
    return C_actor::GetRecordRot(a);
}

// STUB: GAME 0x443ff0
void C_dyn_object::PlayStep(int a)
{
    StubLog("C_dyn_object::PlayStep");
    C_actor::PlayStep(a);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_dyn_object::PlayRecord(int a)
{
}
