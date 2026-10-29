// The physical things (PS2 C_PhThings.cpp; the PC's file is the one at 0x459e90-0x460a1c, docs/game-files.md), which
// starts with the constructor. Not written yet: the constructor, which the derived classes' constructors call, and the
// functions the script calls are stubs.

#include "actors/C_PhThings.h"

#include "stub.h"

#pragma auto_inline(off)

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

#pragma auto_inline(on)
