// The doors (PS2 C_door.cpp; the PC's file is the one at 0x439240-0x43bd8c, docs/game-files.md), whose constructor
// (0x439290) comes after a function of its own. Not written yet: the constructor, which C_mission::CreateActor
// calls, and the functions the script calls are stubs.

#include "actors/C_door.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x439290
C_door::C_door()
{
    StubLog("C_door::C_door");
}

// STUB: GAME 0x439610
void C_door::SetState(E_state state, C_actor *actor, bool unknown1, bool unknown2)
{
    StubLog("C_door::SetState");
}

// STUB: GAME 0x43add0
void C_door::EnableUsingObject(bool enable)
{
    StubLog("C_door::EnableUsingObject");
}

// STUB: GAME 0x43b810
void C_door::SetOpenAngle(float open)
{
    StubLog("C_door::SetOpenAngle");
}

#pragma auto_inline(on)
