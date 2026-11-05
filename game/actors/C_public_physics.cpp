// The public physics (PS2 C_public_physics.cpp; the PC's file is the one at 0x47c320-0x47c53c, docs/game-files.md),
// whose constructor (0x47c3a0) comes after a function of its own. Not written yet: the constructor, which
// C_mission::CreateActor calls, is a stub.

#include "actors/C_public_physics.h"

#include "stub.h"

// GLOBAL: GAME 0x63d78c
std::vector<uint> C_public_physics::unknown_63d78c;

// STUB: GAME 0x47c3a0
C_public_physics::C_public_physics()
{
    StubLog("C_public_physics::C_public_physics");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x47c3e0
C_public_physics::~C_public_physics()
{
}

// STUB: GAME 0x47c3f0
void C_public_physics::GameInit()
{
    StubLog("C_public_physics::GameInit");
    C_PhThing::GameInit();
}

// STUB: GAME 0x47c4d0
void C_public_physics::GameDone()
{
    StubLog("C_public_physics::GameDone");
    C_PhThing::GameDone();
}

// STUB: GAME 0x47c4f0
bool C_public_physics::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_public_physics::Collision");
    return C_PhThing::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x47c500
bool C_public_physics::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    C_actor *act, uint u, I3D_frame *frm)
{
    StubLog("C_public_physics::Hit");
    return C_PhThing::Hit(type, v1, v2, v3, f, act, u, frm);
}
