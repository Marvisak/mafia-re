// The collisions, the file at 0x5c2840-0x5c503c (docs/game-files.md); the name is a guess, PS2's g_colls.cpp. Not
// written yet: only what main needs to link.

#include "collision/g_colls.h"

#include "stub.h"

// GLOBAL: GAME 0x647f48
g_collision gifCollision;

#pragma auto_inline(off)

// STUB: GAME 0x5c2b70
bool g_collision::LoadCollision(const char *name, bool)
{
    StubLog("g_collision::LoadCollision %s", name);
    return false;
}

#pragma auto_inline(on)
