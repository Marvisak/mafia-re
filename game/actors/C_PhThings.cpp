// The physical things (PS2 C_PhThings.cpp; the PC's file is the one at 0x459e90-0x460a1c, docs/game-files.md), which
// starts with the constructor. Not written yet: the constructor, which the derived classes' constructors call, and the
// functions the script calls are stubs.

#include "actors/C_PhThings.h"

#include "stub.h"

// STUB: GAME 0x459e90
C_PhThing::C_PhThing()
{
    StubLog("C_PhThing::C_PhThing");
}

// STUB: GAME 0x45ee90
void C_PhThing::InitBox(S_THING_BOX *box)
{
    StubLog("C_PhThing::InitBox");
}

// STUB: GAME 0x460000
void C_PhThing::ExternalImpuls(const S_vector &impuls, const S_vector &pos)
{
    StubLog("C_PhThing::ExternalImpuls");
}

// STUB: GAME 0x459f90
C_PhThing::~C_PhThing()
{
    StubLog("C_PhThing::~C_PhThing");
}

// STUB: GAME 0x459fd0
bool C_PhThing::Init(I3D_frame *frm)
{
    StubLog("C_PhThing::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x45a120
void C_PhThing::GameInit()
{
    StubLog("C_PhThing::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x45a630
void C_PhThing::GameDone()
{
    StubLog("C_PhThing::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x45a690
void C_PhThing::AI(uint time)
{
    StubLog("C_PhThing::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x45a7a0
void C_PhThing::Update(uint time)
{
    StubLog("C_PhThing::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x45adc0
void C_PhThing::ChangeState()
{
    StubLog("C_PhThing::ChangeState");
    C_actor::ChangeState();
}

// STUB: GAME 0x45f520
bool C_PhThing::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_PhThing::Collision");
    return C_actor::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x45f760
bool C_PhThing::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_PhThing::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// STUB: GAME 0x4603b0
int C_PhThing::SaveGameGetSize()
{
    StubLog("C_PhThing::SaveGameGetSize");
    return C_actor::SaveGameGetSize();
}

// STUB: GAME 0x4603c0
bool C_PhThing::SaveGameSave(void **buf)
{
    StubLog("C_PhThing::SaveGameSave");
    return C_actor::SaveGameSave(buf);
}

// STUB: GAME 0x4605a0
bool C_PhThing::SaveGameLoad(void **buf)
{
    StubLog("C_PhThing::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_PhThing::NetDirect(uint time)
{
}

// STUB: GAME 0x460140
void C_PhThing::InvalidateActor(C_actor *act)
{
    StubLog("C_PhThing::InvalidateActor");
}
