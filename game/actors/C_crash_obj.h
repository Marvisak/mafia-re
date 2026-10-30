#pragma once

#include "actors/C_PhThings.h"

// The pieces a crash breaks off (PS2 C_crash_obj, C_crash_obj.cpp), a physical thing of type 0x1a, 0x3c0 bytes: its
// constructor (0x431b40, vtable 0x623918) is called by a helper of the file's (0x4311d0) that walks a model's frames.
// Only the count and DestroyOuterMostCrashObject so far, which C_game::Tick uses.
class C_crash_obj : public C_PhThing
{
public:
    // Of the game's temporary actors, the crash object farthest from the camera gets flag 0x8000 at its 0x38c (PS2
    // name): C_game::Tick calls it while there are more than 20
    static void DestroyOuterMostCrashObject();

    uchar unknown_38c[0x34];            // 0x38c

    // How many there are in the game (PS2 name): GameInit (0x432460) counts it up, GameDone (0x432470) down, as PS2's,
    // and C_game::Init clears it
    static int count;
};
