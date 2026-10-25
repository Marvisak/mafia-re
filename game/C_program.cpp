// The scripts (PS2 C_program.cpp; the PC's file is most likely the one at 0x460a50-0x47c31c, whose strings are the
// script language's, docs/game-files.md). Not written yet: only what InitSystem needs to link.

#include "C_program.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x460f20
void LoadPatchFile(const char *name)
{
    StubLog("LoadPatchFile %s", name);
}

#pragma auto_inline(on)
