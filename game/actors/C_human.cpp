// The humans (PS2 C_human.cpp; the PC's file is the one at 0x56b180-0x58e34c, docs/game-files.md), whose constructor
// (0x56efd0) comes after functions of its own. Not written yet: the constructor, which the derived classes'
// constructors call, is a stub.

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

// STUB: GAME 0x57e500
void C_human::Intern_FromCar(I3D_frame *frm)
{
    StubLog("C_human::Intern_FromCar");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5877a0
void C_human::InitFFEffects()
{
    StubLog("C_human::InitFFEffects");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x587880
void C_human::DoneFFEffects()
{
    StubLog("C_human::DoneFFEffects");
}

#pragma auto_inline(on)


