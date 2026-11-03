// The humans (PS2 C_human.cpp; the PC's file is the one at 0x56b180-0x58e34c, docs/game-files.md), whose constructor
// (0x56efd0) comes after functions of its own. Not written yet: the constructor, which the derived classes'
// constructors call, the functions the script calls, and Activate and Deactivate, which C_mission's FrmActBuff
// functions call, are stubs.

#include "actors/C_human.h"

#include "stub.h"

// Empty on the PC: its calls go to the empty function the linker folded it into (0x4aabe0)
void C_human::HideLegs(I3D_model *model, bool hide)
{
}

#pragma auto_inline(off)

// STUB: GAME 0x56efd0
C_human::C_human()
{
    StubLog("C_human::C_human");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5721b0
void C_human::_intern_UseCarTruck(C_car_ex *car, bool unknown)
{
    StubLog("C_human::_intern_UseCarTruck");
}

// STUB: GAME 0x579630
void C_human::PoseSetPoseNormal(S_vector pos)
{
    StubLog("C_human::PoseSetPoseNormal");
}

// STUB: GAME 0x57a560
void C_human::PoseNullRotate()
{
    StubLog("C_human::PoseNullRotate");
}

// STUB: GAME 0x57d2e0
void C_human::Do_StopAnim()
{
    StubLog("C_human::Do_StopAnim");
}

// STUB: GAME 0x57d310
void C_human::Do_Breath()
{
    StubLog("C_human::Do_Breath");
}

// STUB: GAME 0x57dd00
void C_human::ChangeAnim(char *anim1, char *anim2)
{
    StubLog("C_human::ChangeAnim");
}

// STUB: GAME 0x57e020
void C_human::Intern_UseCar(C_car *car, int seat)
{
    StubLog("C_human::Intern_UseCar");
}

// STUB: GAME 0x57e500
void C_human::Intern_FromCar(I3D_frame *frm)
{
    StubLog("C_human::Intern_FromCar");
}

// STUB: GAME 0x57ec20
void C_human::ChangeWeaponModel()
{
    StubLog("C_human::ChangeWeaponModel");
}

// STUB: GAME 0x57faa0
bool C_human::DropOutItems(std::vector<S_GameItem> &items)
{
    StubLog("C_human::DropOutItems");
    return false;
}

// STUB: GAME 0x583f40
void C_human::Do_ThrowGrenade(S_vector &dir)
{
    StubLog("C_human::Do_ThrowGrenade");
}

// STUB: GAME 0x585c60
void C_human::Do_Holster()
{
    StubLog("C_human::Do_Holster");
}

// STUB: GAME 0x585e80
int C_human::Do_PlayAnim(const char *anim, bool simple, bool unknown)
{
    StubLog("C_human::Do_PlayAnim");
    return 0;
}

// STUB: GAME 0x587190
void C_human::_intern_ChangeModel(char *name, bool unknown)
{
    StubLog("C_human::_intern_ChangeModel");
}

// STUB: GAME 0x5877a0
void C_human::InitFFEffects()
{
    StubLog("C_human::InitFFEffects");
}

// STUB: GAME 0x587880
void C_human::DoneFFEffects()
{
    StubLog("C_human::DoneFFEffects");
}

// STUB: GAME 0x58ac90
void C_human::Activate()
{
    StubLog("C_human::Activate");
}

// STUB: GAME 0x58b330
void C_human::Deactivate()
{
    StubLog("C_human::Deactivate");
}

// STUB: GAME 0x58b870
void C_human::ResetActions()
{
    StubLog("C_human::ResetActions");
}

// STUB: GAME 0x58cfb0
bool C_human::IsThisCarStealing(C_car *car)
{
    StubLog("C_human::IsThisCarStealing");
    return false;
}

// STUB: GAME 0x58e270
bool C_human::Can_Hit_FilterHumanType(C_human *shooter, C_human *human)
{
    StubLog("C_human::Can_Hit_FilterHumanType");
    return true;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x56f210
C_human::~C_human()
{
    StubLog("C_human::~C_human");
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_human::NetDirect(uint time)
{
}

// STUB: GAME 0x51af20
void C_human::GetActorSpeed(S_vector &speed)
{
    StubLog("C_human::GetActorSpeed");
    C_actor::GetActorSpeed(speed);
}

// STUB: GAME 0x56b180
bool C_human::vfunc_9()
{
    StubLog("C_human::vfunc_9");
    return C_actor::vfunc_9();
}

// STUB: GAME 0x56b1a0
int C_human::vfunc_0(int a)
{
    StubLog("C_human::vfunc_0");
    return C_actor::vfunc_0(a);
}

// STUB: GAME 0x56b670
void C_human::vfunc_1(int a)
{
    StubLog("C_human::vfunc_1");
    C_actor::vfunc_1(a);
}

// STUB: GAME 0x56b8d0
void C_human::vfunc_2(int a)
{
    StubLog("C_human::vfunc_2");
    C_actor::vfunc_2(a);
}

// STUB: GAME 0x56bec0
bool C_human::vfunc_3(int a)
{
    StubLog("C_human::vfunc_3");
    return C_actor::vfunc_3(a);
}

// STUB: GAME 0x56c6e0
void C_human::vfunc_12()
{
    StubLog("C_human::vfunc_12");
    C_actor::vfunc_12();
}

// STUB: GAME 0x56c7c0
S_vector C_human::vfunc_4(int a)
{
    StubLog("C_human::vfunc_4");
    return C_actor::vfunc_4(a);
}

// STUB: GAME 0x56cc70
S_vector C_human::vfunc_5(int a)
{
    StubLog("C_human::vfunc_5");
    return C_actor::vfunc_5(a);
}

// STUB: GAME 0x56f350
bool C_human::Init(I3D_frame *frm)
{
    StubLog("C_human::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x56f400
void C_human::DuplicateFrom(C_actor *act)
{
    StubLog("C_human::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x56f440
void C_human::SaveData(C_chunk *chunk)
{
    StubLog("C_human::SaveData");
    C_actor::SaveData(chunk);
}

// STUB: GAME 0x56f4e0
void C_human::LoadData(C_chunk *chunk)
{
    StubLog("C_human::LoadData");
    C_actor::LoadData(chunk);
}

// STUB: GAME 0x56f9e0
int C_human::SaveGameGetSize()
{
    StubLog("C_human::SaveGameGetSize");
    return C_actor::SaveGameGetSize();
}

// STUB: GAME 0x56fa20
bool C_human::SaveGameSave(void **buf)
{
    StubLog("C_human::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x56fd30
bool C_human::SaveGameLoad(void **buf)
{
    StubLog("C_human::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// STUB: GAME 0x570570
void C_human::Death()
{
    StubLog("C_human::Death");
    C_actor::Death();
}

// STUB: GAME 0x5706a0
void C_human::SoundsChange(int i)
{
    StubLog("C_human::SoundsChange");
    C_actor::SoundsChange(i);
}

// STUB: GAME 0x570740
void C_human::AI(uint time)
{
    StubLog("C_human::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x572b80
void C_human::Update(uint time)
{
    StubLog("C_human::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x5748f0
void C_human::ChangeState()
{
    StubLog("C_human::ChangeState");
    C_actor::ChangeState();
}

// STUB: GAME 0x574bd0
void C_human::GameInit()
{
    StubLog("C_human::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x5757c0
void C_human::GameDone()
{
    StubLog("C_human::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x5762a0
bool C_human::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act, uint u,
    I3D_frame *frm)
{
    StubLog("C_human::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x5784a0
bool C_human::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_human::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x578c60
bool C_human::IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll)
{
    StubLog("C_human::IsPossibleColl");
    return C_actor::IsPossibleColl(act, v1, v2, f, dist, coll);
}

// STUB: GAME 0x585f40
bool C_human::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    StubLog("C_human::GetBoundPoints2D");
    return C_actor::GetBoundPoints2D(points, frm);
}

// STUB: GAME 0x587d30
void C_human::cbCollTester(tDynamicCollObject *obj)
{
    StubLog("C_human::cbCollTester");
    C_actor::cbCollTester(obj);
}

#pragma auto_inline(on)
