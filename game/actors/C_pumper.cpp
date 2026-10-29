// The petrol pump attendants (PS2 C_pumper.cpp; the PC's file is the one at 0x47c540-0x48192c, docs/game-files.md),
// which starts with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, and
// ScriptCanPump are stubs.

#include "actors/C_pumper.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x47c540
C_pumper::C_pumper()
{
    StubLog("C_pumper::C_pumper");
}

// STUB: GAME 0x481840
int C_pumper::ScriptCanPump(uint id)
{
    StubLog("C_pumper::ScriptCanPump");
    return 0;
}

#pragma auto_inline(on)
