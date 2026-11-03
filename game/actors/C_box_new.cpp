// The physical objects (PS2 C_box_new.cpp; the PC's file is the one at 0x5674f0-0x567cec, docs/game-files.md), which
// starts with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "actors/C_box_new.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x5674f0
C_box_new::C_box_new()
{
    StubLog("C_box_new::C_box_new");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x567530
C_box_new::~C_box_new()
{
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x567540
bool C_box_new::Init(I3D_frame *frm)
{
    StubLog("C_box_new::Init");
    return C_PhThing::Init(frm);
}

// STUB: GAME 0x567650
void C_box_new::GameInit()
{
    StubLog("C_box_new::GameInit");
    C_PhThing::GameInit();
}

// STUB: GAME 0x567730
bool C_box_new::LoadData(C_chunk *chunk)
{
    StubLog("C_box_new::LoadData");
    C_PhThing::LoadData(chunk);
    return true;
}

// STUB: GAME 0x567ab0
bool C_box_new::SaveData(C_chunk *chunk)
{
    StubLog("C_box_new::SaveData");
    C_PhThing::SaveData(chunk);
    return true;
}

// STUB: GAME 0x567c20
void C_box_new::DuplicateFrom(C_actor *act)
{
    StubLog("C_box_new::DuplicateFrom");
    C_PhThing::DuplicateFrom(act);
}

// STUB: GAME 0x567c80
bool C_box_new::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_box_new::Collision");
    return C_PhThing::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x567ca0
bool C_box_new::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_box_new::Hit");
    return C_PhThing::Hit(type, v1, v2, v3, f, act, u, frm);
}

#pragma auto_inline(on)
