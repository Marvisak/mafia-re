// The traffic's cars (PS2 C_trf_c.cpp, C_traffic_car's; the PC's file is the one at 0x4aa490-0x4bc35c,
// docs/game-files.md), whose constructor (0x4aa7a0) comes after functions of its own. Written: the constructor, which
// C_mission::CreateActor calls, the destructor, GetParkingCars, which C_traffic_generator::TryGoToAssHole calls,
// SaveGameGetSize, DuplicateFrom, SaveGameSave, SaveGameLoad and ChlupatejVystrik, which C_shvestky::TickDoRiciActors
// calls; TakATedSiVasKupim_Vsecky and VypraskeeeKaaaru are stubs.

#include "vehicles/C_trf_c.h"
#include "C_chunk.h"

#include "stub.h"

// GLOBAL: GAME 0x63d7fc
float C_traffic_car::elems_coefff;
// GLOBAL: GAME 0x63d800
float C_traffic_car::mafian_coefff;
// GLOBAL: GAME 0x63d804
float C_traffic_car::police_coefff;
// GLOBAL: GAME 0x63d810
int C_traffic_car::count;

// The third and the fourth of the file's six vectors, whose initializers start it (0x4aa4b0 to 0x4aa730; these
// ones' 0x4aa5b0 and 0x4aa630)
// GLOBAL: GAME 0x63d844
std::vector<C_traffic_car::S_ELEMENT *> C_traffic_car::v_elems;
// GLOBAL: GAME 0x63d834
std::vector<C_traffic_car::S_ELEMENT *> C_traffic_car::v_elems_parking;

#pragma auto_inline(off)

// FUNCTION: GAME 0x4aa7a0
C_traffic_car::C_traffic_car()
{
    unknown_220 = 0;
    unknown_21c = 0;
    names = NULL;
    num_names = 0;
    type = ACTOR_TRAFFIC_CAR;
    unknown_228 = 150.0f;
    unknown_22c = 170.0f;
    unknown_224 = 180.0f;
    police_coefff = 1.0f;
    mafian_coefff = 1.0f;
    elems_coefff = 1.0f;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x4b8540
std::vector<C_traffic_car::S_ELEMENT *> &C_traffic_car::GetParkingCars()
{
    return v_elems_parking;
}

#pragma auto_inline(off)

// FUNCTION: GAME 0x4b9560
void C_traffic_car::ChlupatejVystrik()
{
    if (--count < 0)
        count = 0;
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

// FUNCTION: GAME 0x4aa830
C_traffic_car::~C_traffic_car()
{
    delete[] names;
    names = NULL;
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
        names = new char[TRAFFIC_NAME_SIZE * num_names];
    for (int i = 0; i < num_names; i++)
        strcpy(names + TRAFFIC_NAME_SIZE * i, other->names + TRAFFIC_NAME_SIZE * i);
}

// Writes the type, the values and the names (PS2 name)
// FUNCTION: GAME 0x4aabf0
bool C_traffic_car::SaveData(C_chunk *chunk)
{
    int five = 5;
    chunk->Write(&five, 4);
    chunk->Write(&unknown_224, 4);
    chunk->Write(&unknown_228, 4);
    chunk->Write(&unknown_22c, 4);
    chunk->Write(&unknown_21c, 4);
    chunk->Write(&num_names, 4);
    for (int i = 0; i < num_names; i++)
        chunk->Write(names + i * TRAFFIC_NAME_SIZE, TRAFFIC_NAME_SIZE);
    return true;
}

// STUB: GAME 0x4aad20
bool C_traffic_car::LoadData(C_chunk *chunk)
{
    StubLog("C_traffic_car::LoadData");
    C_actor::LoadData(chunk);
    return true;
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

// FUNCTION: GAME 0x4bc160
bool C_traffic_car::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    *(ushort *)*buf = 1;
    *buf = (uchar *)*buf + 2;
    return true;
}

// FUNCTION: GAME 0x4bc190
bool C_traffic_car::SaveGameLoad(void **buf)
{
    if (!C_actor::SaveGameLoad(buf))
        return false;
    if (*(ushort *)*buf != 1)
        return false;
    *buf = (uchar *)*buf + 2;
    return true;
}

#pragma auto_inline(on)
