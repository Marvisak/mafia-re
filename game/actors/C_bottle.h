#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"

// The bottles (PS2 C_bottle, C_bottle.cpp), 0x138 bytes (PS2's 0x190). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_BOTTLE.
// VTABLE: GAME 0x6233f0
class C_bottle : public C_actor
{
public:
    C_bottle();

    uchar unknown_70[0x44];             // 0x70
    // Its constructor clears it before the vtable is stored (PS2's 0x60 bytes at 0x100)
    tDynamicCollObject coll;            // 0xb4 (the name is ours)
    bool unknown_fc;                    // 0xfc, true from the constructor
    uchar unknown_fd[0x2b];             // 0xfd
    bool unknown_128;                   // 0x128, true from the constructor
    uchar unknown_129[0xf];             // 0x129
};
