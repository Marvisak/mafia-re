// The doors (PS2 C_door.cpp; the PC's file is the one at 0x439240-0x43bd8c, docs/game-files.md), whose constructor
// (0x439290) comes after a function of its own. Not written yet: the constructor, which C_mission::CreateActor
// calls, and the functions the script calls are stubs.

#include "actors/C_door.h"

#include "stub.h"

// STUB: GAME 0x439290
C_door::C_door()
{
    StubLog("C_door::C_door");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x439390
C_door::~C_door()
{
}

// STUB: GAME 0x439610
void C_door::SetState(E_state state, C_actor *actor, bool unknown1, bool unknown2)
{
    StubLog("C_door::SetState");
}

// STUB: GAME 0x43add0
void C_door::EnableUsingObject(bool enable)
{
    StubLog("C_door::EnableUsingObject");
}

// STUB: GAME 0x43b810
void C_door::SetOpenAngle(float open)
{
    StubLog("C_door::SetOpenAngle");
}

// The linker folded it with C_traffic_generator, C_wagon's
// STUB: GAME 0x4393a0 FOLDED
bool C_door::Init(I3D_frame *frm)
{
    StubLog("C_door::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x4393c0
void C_door::DuplicateFrom(C_actor *act)
{
    StubLog("C_door::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x439480
bool C_door::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act, uint u,
    I3D_frame *frm)
{
    StubLog("C_door::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x4399c0
void C_door::AI(uint time)
{
    StubLog("C_door::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x439e90
void C_door::NetDirect(uint time)
{
    StubLog("C_door::NetDirect");
    C_actor::NetDirect(time);
}

// STUB: GAME 0x439eb0
void C_door::GameInit()
{
    StubLog("C_door::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x43ad60
void C_door::GameDone()
{
    StubLog("C_door::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x43ae50
bool C_door::SaveData(C_chunk *chunk)
{
    StubLog("C_door::SaveData");
    return C_actor::SaveData(chunk);
}

// STUB: GAME 0x43b030
bool C_door::LoadData(C_chunk *chunk)
{
    StubLog("C_door::LoadData");
    return C_actor::LoadData(chunk);
}

// STUB: GAME 0x43b4f0
int C_door::SaveGameGetSize()
{
    StubLog("C_door::SaveGameGetSize");
    return C_actor::SaveGameGetSize();
}

// STUB: GAME 0x43b500
bool C_door::SaveGameSave(void **buf)
{
    StubLog("C_door::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x43b560
bool C_door::SaveGameLoad(void **buf)
{
    StubLog("C_door::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// STUB: GAME 0x43bab0
bool C_door::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    StubLog("C_door::GetBoundPoints2D");
    return C_actor::GetBoundPoints2D(points, frm);
}

// STUB: GAME 0x43bcc0
void C_door::ChangeState()
{
    StubLog("C_door::ChangeState");
    C_actor::ChangeState();
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_door::Update(uint time)
{
}
