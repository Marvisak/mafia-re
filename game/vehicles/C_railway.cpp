// The rail vehicles (most likely PS2 C_railway.cpp; the PC's file is the one at 0x4826c0-0x49908c,
// docs/game-files.md), whose constructor (0x4865b0) comes after functions of its own. Written: S_rail_human (Kill;
// SetAnim is a stub), DuplicateFrom, SaveGameGetSize, Lock, Unlock and GetMaterialID; the constructor, which
// C_mission::CreateActor calls, is a stub.

#include "vehicles/C_railway.h"

#include "stub.h"

// GLOBAL: GAME 0x63d7ac
bool C_railway::salina_v_kine;

// STUB: GAME 0x4841a0
void S_rail_human::SetAnim(uint animation)
{
    StubLog("S_rail_human::SetAnim");
}

// FUNCTION: GAME 0x484230
void S_rail_human::Kill(C_railway *tram)
{
    // Panicking (22): back to the state it saved, so the death animation fits what it was really doing
    if (passenger_flag == 22)
        passenger_flag = old_passenger_flag;

    switch (passenger_flag)
    {
    case 0:  // picks the entry to board at
    case 1:  // walking to the tram
    case 2:  // at the entry, turning to the door
    case 3:  // boarding, the tram's animation
    case 4:  // walking in
    case 5:  // attaching to the vehicle
    case 10: // standing up from the seat
    case 11: // walking to the exit
    case 12: // turning to the exit
    case 13: // alighting
    case 14: // walking out
    case 15: // detaching from the vehicle
    case 16: // walking away, back to a pedestrian
        // On the way in (0-5) or out (10-16): the passenger holds a lock on the tram (lock_count), which it
        // would give up on getting in or out; dying it never does, so release it here
        tram->Unlock();
        // fall through
    case 6:  // looking for a free seat
    case 7:  // walking to the seat
    case 8:  // sitting down
        new_animation_number = rnd(5) + 7;  // a death on foot random animation from range 7-11
        break;
    case 9:  // sitting
    case 21: // scared while sitting (the fear timer returns it to 9)
        new_animation_number = rnd(2) + 12; // a sitting death, 12-13
        break;
    case 19: // the driver (17 ends in 20 for him, not 18)
        new_animation_number = rnd(2) + 23; // the driver's deaths, 23-24
        break;
    default: // 17, 18, 20: already dying or dead, keep the animation
        break;
    }

    // Dying (17): the update plays the death animation to its last 250 ms, then 18 (a passenger) or 20 (the driver)
    passenger_flag = 17;
    SetAnim(1); // send the new animation to the engine
}

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

// STUB: GAME 0x4928e0
bool C_railway::AddPerson(I3D_model *model, uint wagon, uint entry)
{
    StubLog("C_railway::AddPerson");
    return false;
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

// Gives a lock back (lock_count--), never below 0: a passenger that got in or out, or Kill for one caught dying
// while boarding or leaving, so no lock is left held
// FUNCTION: GAME 0x497990
void C_railway::Unlock()
{
    if ((uint)lock_count > 0)
        --lock_count;
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

// Takes a lock on the tram (lock_count++): a passenger on its way in or out holds one, so the tram is held while
// any is boarding or leaving; Kill gives it back for one caught dying
// FUNCTION: GAME 0x497980
void C_railway::Lock()
{
    ++lock_count;
}
