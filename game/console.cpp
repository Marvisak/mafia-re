// The console's text (PS2 console.cpp; the PC's file is most likely the one at 0x4f5dc0-0x4f6a2c, which loads
// editfont.bmp and fnt_matrix.bin, docs/game-files.md). Not written yet: only what InitSystem and CloseSystem need to
// link.

#include "console.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4f6980
void ConsolesInit()
{
    StubLog("ConsolesInit");
}

// The original releases the font's texture (0x646b38) if there is one, as PS2's
// STUB: GAME 0x4f6990
void ConsolesDone()
{
    StubLog("ConsolesDone");
}

#pragma auto_inline(on)
