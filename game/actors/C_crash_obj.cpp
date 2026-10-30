// The pieces crashes break off (PS2 C_crash_obj.cpp; the PC's file is the one at 0x430ba0-0x43286c, after
// C_cartridge.cpp as the names sort, docs/game-files.md): the helpers that make them first, then the constructor
// (0x431b40) and PS2's functions in PS2's order, GameInit (0x432460), GameDone (0x432470), SetModelFileName (0x4326d0)
// and DestroyOuterMostCrashObject among them. Written so far: the count; DestroyOuterMostCrashObject, which
// C_game::Tick calls, is a stub.

#include "actors/C_crash_obj.h"

#include "stub.h"

// GLOBAL: GAME 0x63d6d4
int C_crash_obj::count;

#pragma auto_inline(off)

// STUB: GAME 0x432740
void C_crash_obj::DestroyOuterMostCrashObject()
{
    StubLog("C_crash_obj::DestroyOuterMostCrashObject");
}

#pragma auto_inline(on)
