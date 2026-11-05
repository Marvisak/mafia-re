// The rail vehicles (most likely PS2 C_railway.cpp; the PC's file is the one at 0x4826c0-0x49908c,
// docs/game-files.md), whose constructor (0x4865b0) comes after functions of its own. Written: DuplicateFrom,
// SaveGameGetSize and GetMaterialID; the constructor, which C_mission::CreateActor calls, is a stub.

#include "vehicles/C_railway.h"

#include "stub.h"

// STUB: GAME 0x4865b0
C_railway::C_railway()
{
    StubLog("C_railway::C_railway");
}

// STUB: GAME 0x486760
C_railway::~C_railway()
{
    StubLog("C_railway::~C_railway");
}

// STUB: GAME 0x4868c0
bool C_railway::Init(I3D_frame *frm)
{
    StubLog("C_railway::Init");
    return C_actor::Init(frm);
}

// FUNCTION: GAME 0x4868f0
void C_railway::DuplicateFrom(C_actor *act)
{
    C_actor::DuplicateFrom(act);
    unknown_1a8 = ((C_railway *)act)->unknown_1a8;
    unknown_1b4 = ((C_railway *)act)->unknown_1b4;
}

// STUB: GAME 0x486920
bool C_railway::SaveData(C_chunk *chunk)
{
    StubLog("C_railway::SaveData");
    return C_actor::SaveData(chunk);
}

// STUB: GAME 0x486a20
bool C_railway::LoadData(C_chunk *chunk)
{
    StubLog("C_railway::LoadData");
    return C_actor::LoadData(chunk);
}

// STUB: GAME 0x488140
void C_railway::AI(uint time)
{
    StubLog("C_railway::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x4894d0
void C_railway::Update(uint time)
{
    StubLog("C_railway::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x48a390
void C_railway::GameInit()
{
    StubLog("C_railway::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x48cfa0
void C_railway::GameDone()
{
    StubLog("C_railway::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x48d530
bool C_railway::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_railway::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x48d890
bool C_railway::IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll)
{
    StubLog("C_railway::IsPossibleColl");
    return C_actor::IsPossibleColl(act, v1, v2, f, dist, coll);
}

// STUB: GAME 0x490b40
bool C_railway::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_railway::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// FUNCTION: GAME 0x496e40
int C_railway::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + 80;
}

// STUB: GAME 0x496e50
bool C_railway::SaveGameSave(void **buf)
{
    StubLog("C_railway::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x496f80
bool C_railway::SaveGameLoad(void **buf)
{
    StubLog("C_railway::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// STUB: GAME 0x498060
void C_railway::ChangeState()
{
    StubLog("C_railway::ChangeState");
    C_actor::ChangeState();
}

// STUB: GAME 0x498180
bool C_railway::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    StubLog("C_railway::GetBoundPoints2D");
    return C_actor::GetBoundPoints2D(points, frm);
}

// FUNCTION: GAME 0x498a70
int C_railway::GetMaterialID(I3D_frame *frm)
{
    return material;
}

// STUB: GAME 0x498a80
void C_railway::GetActorSpeed(S_vector &speed)
{
    StubLog("C_railway::GetActorSpeed");
    C_actor::GetActorSpeed(speed);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_railway::NetDirect(uint time)
{
}

// STUB: GAME 0x495100
void C_railway::AddOuterEntries()
{
    StubLog("C_railway::AddOuterEntries");
}

// STUB: GAME 0x495160
void C_railway::AddInnerEntries()
{
    StubLog("C_railway::AddInnerEntries");
}

// STUB: GAME 0x4951c0
void C_railway::RemoveOuterEntries()
{
    StubLog("C_railway::RemoveOuterEntries");
}

// STUB: GAME 0x495220
void C_railway::RemoveInnerEntries()
{
    StubLog("C_railway::RemoveInnerEntries");
}

// STUB: GAME 0x497990
void C_railway::Unlock()
{
    StubLog("C_railway::Unlock");
}

// STUB: GAME 0x4989a0
void C_railway::SetRainDummy(bool in, I3D_frame *frm)
{
    StubLog("C_railway::SetRainDummy");
}

// STUB: GAME 0x495d30
bool C_railway::GetEntry(C_actor *act, S_vector *pos, S_vector *dir, uint *entry)
{
    StubLog("C_railway::GetEntry");
    return false;
}

// STUB: GAME 0x496e10
I3D_frame *C_railway::GetVagon(uint wagon)
{
    StubLog("C_railway::GetVagon");
    return NULL;
}

// STUB: GAME 0x497980
void C_railway::Lock()
{
    StubLog("C_railway::Lock");
}
