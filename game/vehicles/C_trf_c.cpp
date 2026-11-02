// The traffic's cars (PS2 C_trf_c.cpp, C_traffic_car's; the PC's file is the one at 0x4aa490-0x4bc35c,
// docs/game-files.md), whose constructor (0x4aa7a0) comes after functions of its own. Written so far: GetParkingCars,
// which C_traffic_generator::TryGoToAssHole calls, SaveGameGetSize and DuplicateFrom; the constructor, which
// C_mission::CreateActor calls, TakATedSiVasKupim_Vsecky, VypraskeeeKaaaru and ChlupatejVystrik, which
// C_shvestky::TickDoRiciActors calls, are stubs.

#include "vehicles/C_trf_c.h"

#include "stub.h"

// GLOBAL: GAME 0x63d7fc
float C_traffic_car::unknown_63d7fc;
// GLOBAL: GAME 0x63d804
float C_traffic_car::unknown_63d804;

// The third and the fourth of the file's six vectors, whose initializers start it (0x4aa4b0 to 0x4aa730; these
// ones' 0x4aa5b0 and 0x4aa630)
// GLOBAL: GAME 0x63d844
std::vector<C_traffic_car::S_ELEMENT *> C_traffic_car::v_elems;
// GLOBAL: GAME 0x63d834
std::vector<C_traffic_car::S_ELEMENT *> C_traffic_car::v_elems_parking;

#pragma auto_inline(off)

// STUB: GAME 0x4aa7a0
C_traffic_car::C_traffic_car()
{
    StubLog("C_traffic_car::C_traffic_car");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x4b8540
std::vector<C_traffic_car::S_ELEMENT *> &C_traffic_car::GetParkingCars()
{
    return v_elems_parking;
}

#pragma auto_inline(off)

// STUB: GAME 0x4b9560
void C_traffic_car::ChlupatejVystrik()
{
    StubLog("C_traffic_car::ChlupatejVystrik");
}

// STUB: GAME 0x4bad80
void C_traffic_car::TakATedSiVasKupim_Vsecky()
{
    StubLog("C_traffic_car::TakATedSiVasKupim_Vsecky");
}

// STUB: GAME 0x4bbe90
void C_traffic_car::VypraskeeeKaaaru(C_actor *act)
{
    StubLog("C_traffic_car::VypraskeeeKaaaru");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x4aa830
C_traffic_car::~C_traffic_car()
{
    StubLog("C_traffic_car::~C_traffic_car");
}

// STUB: GAME 0x4aa860
bool C_traffic_car::Init(I3D_frame *frm)
{
    StubLog("C_traffic_car::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x4aa8b0
void C_traffic_car::ChangeState()
{
    StubLog("C_traffic_car::ChangeState");
    C_actor::ChangeState();
}

// The fields and the names, which DuplicateFrom copies (the names are ours)
// FUNCTION: GAME 0x4aab20
void C_traffic_car::DuplicateFrom(C_actor *act)
{
    C_traffic_car *other = (C_traffic_car *)act;
    C_actor::DuplicateFrom(act);
    unknown_21c = other->unknown_21c;
    unknown_224 = other->unknown_224;
    char *old = names;
    unknown_228 = other->unknown_228;
    unknown_22c = other->unknown_22c;
    delete[] old;
    names = NULL;
    num_names = other->num_names;
    if (num_names)
        names = new char[32 * num_names];
    for (int i = 0; i < num_names; i++)
        strcpy(names + 32 * i, other->names + 32 * i);
}

// STUB: GAME 0x4aabf0
void C_traffic_car::SaveData(C_chunk *chunk)
{
    StubLog("C_traffic_car::SaveData");
    C_actor::SaveData(chunk);
}

// STUB: GAME 0x4aad20
void C_traffic_car::LoadData(C_chunk *chunk)
{
    StubLog("C_traffic_car::LoadData");
    C_actor::LoadData(chunk);
}

// FUNCTION: GAME 0x4ab120
bool C_traffic_car::vfunc_38(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l)
{
    return false;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_traffic_car::NetDirect(uint time)
{
}

// STUB: GAME 0x4afc40
void C_traffic_car::AI(uint time)
{
    StubLog("C_traffic_car::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x4b5270
void C_traffic_car::Update(uint time)
{
    StubLog("C_traffic_car::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x4b54a0
void C_traffic_car::GameInit()
{
    StubLog("C_traffic_car::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x4b8010
void C_traffic_car::GameDone()
{
    StubLog("C_traffic_car::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x4ba9c0
bool C_traffic_car::IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll)
{
    StubLog("C_traffic_car::IsPossibleColl");
    return C_actor::IsPossibleColl(act, v1, v2, f, dist, coll);
}

// STUB: GAME 0x4baad0
bool C_traffic_car::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_traffic_car::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// FUNCTION: GAME 0x4bc150
int C_traffic_car::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + 2;
}

// STUB: GAME 0x4bc160
bool C_traffic_car::SaveGameSave(void **buf)
{
    StubLog("C_traffic_car::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x4bc190
bool C_traffic_car::SaveGameLoad(void **buf)
{
    StubLog("C_traffic_car::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

#pragma auto_inline(on)
