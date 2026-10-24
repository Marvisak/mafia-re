// The startup's Windows helpers, the file at 0x5661f0-0x566f3c (docs/game-files.md); the name is a guess, PS2's
// Winstuff.cpp. Not written yet: only what main needs to link.

#include "Winstuff.h"

#include "ls3df.h"

#pragma auto_inline(off)

// STUB: GAME 0x5663d0
bool WaitForCD()
{
    dbgPrintf("STUB: WaitForCD");
    return false;
}

#pragma auto_inline(on)
