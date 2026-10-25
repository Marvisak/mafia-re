// The game's tools (PS2 tools.cpp, which has C_sound_table; the PC's file is most likely the one at 0x55cfa0-0x56040c,
// docs/game-files.md). Not written yet: only what InitSystem needs to link.

#include "tools.h"

#include "stub.h"

// The original's has a dynamic initializer (0x55d430)
// GLOBAL: GAME 0x646f50
C_sound_table sound_table;

#pragma auto_inline(off)

// STUB: GAME 0x55d460
void PlayLogo(const char *name, float sx, float sy)
{
    StubLog("PlayLogo %s", name);
}

// STUB: GAME 0x55f390
void C_sound_table::LoadTables(const char *name)
{
    StubLog("C_sound_table::LoadTables %s", name);
}

#pragma auto_inline(on)
