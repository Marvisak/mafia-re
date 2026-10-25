// The scripts (PS2 C_program.cpp; the PC's file is most likely the one at 0x460a50-0x47c31c, whose strings are the
// script language's, docs/game-files.md). Not written yet: only what InitSystem and CloseSystem need to link.

#include "C_program.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x460f20
void LoadPatchFile(const char *name)
{
    StubLog("LoadPatchFile %s", name);
}

// The original deletes the four buffers of each entry LoadPatchFile added (0x63d778, 16 bytes each) and clears the list
// STUB: GAME 0x461310
void UnloadPatchFiles()
{
    StubLog("UnloadPatchFiles");
}

#pragma auto_inline(on)
