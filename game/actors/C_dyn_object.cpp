// The objects of dynamic parts (PC only, the file's name is ours, after the class; the PC's file is the one at
// 0x413d90-0x41bb4c, docs/game-files.md), whose constructor (0x418930) follows the vector math it starts with. Not
// written yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "actors/C_dyn_object.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x418930
C_dyn_object::C_dyn_object()
{
    StubLog("C_dyn_object::C_dyn_object");
}

#pragma auto_inline(on)
