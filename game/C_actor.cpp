// The actors (PS2 C_actor.cpp, which has InitAnimIndex; the PC's file is most likely the one at 0x404ce0-0x40938c,
// docs/game-files.md). Not written yet: only what InitSystem and CloseSystem need to link.

#include "C_actor.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4086b0
void InitAnimIndex()
{
    StubLog("InitAnimIndex");
}

// The original deletes the index (0x63d2cc)
// STUB: GAME 0x408750
void DoneAnimIndex()
{
    StubLog("DoneAnimIndex");
}

#pragma auto_inline(on)
