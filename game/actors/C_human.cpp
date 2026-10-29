// The humans (PS2 C_human.cpp; the PC's file is the one at 0x56b180-0x58e34c, docs/game-files.md), whose constructor
// (0x56efd0) comes after functions of its own. Not written yet: the constructor, which the derived classes'
// constructors call, and the functions the script calls are stubs.

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

// STUB: GAME 0x58b870
void C_human::ResetActions()
{
    StubLog("C_human::ResetActions");
}

#pragma auto_inline(on)
