// The in-engine cutscenes (PC only, the file's name is ours; the PC's file is the one at 0x5983e0-0x5a075c,
// docs/game-files.md): the keys' interpolation (0x598410, 0x5984e0), the object's initializer (0x59cc20) and constructor
// (0x59cc40) mid-file, Close (0x59c6a0), Tick (0x599610). Written so far: nothing; Close, which C_mission::Close calls,
// is a stub.

#include "mission/C_cutscene.h"

#include "stub.h"

// GLOBAL: GAME 0x647900
C_cutscene cutscene;

#pragma auto_inline(off)

// STUB: GAME 0x59c6a0
void C_cutscene::Close()
{
    StubLog("C_cutscene::Close");
}

#pragma auto_inline(on)
