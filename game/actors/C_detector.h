#pragma once

#include <string.h>

#include "actors/C_actor.h"
#include "mission/C_program.h"

// The script objects (PS2 C_detector, C_detector.cpp), an actor and its script: C_program is the second base (at 0x70,
// PS2's 0x80, whose this adjustors PS2 names @128@), 0x128 bytes (PS2's 0x150). Only the constructor, the destructor,
// Init and LoadData so far, which C_mission::CreateActor and OpenBin call for ACTOR_DETECTOR, and the vtable's
// functions from DuplicateFrom to GameDone as stubs. PS2's save functions override C_program's too (its @128@
// thunks), which doesn't declare them yet.
// VTABLE: GAME 0x6239b0 C_actor
// VTABLE: GAME 0x623a48 C_program
class C_detector : public C_actor, public C_program
{
public:
    // Cleared through a register of its own after the other members' zero stores (PS2's memset of its 0x140): an inlined
    // memset
    struct S_unknown_108
    {
        int unknown_0[4];

        S_unknown_108() { memset(this, 0, sizeof(*this)); }
    };

    C_detector();
    virtual ~C_detector();
    // The script runs for the actor itself
    virtual bool Init(I3D_frame *frm);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    int unknown_d8;                     // 0xd8, 0 from Init, then LoadData's (PS2 0x108)
    uchar unknown_dc[0x14];             // 0xdc
    int unknown_f0;                     // 0xf0
    int unknown_f4;                     // 0xf4
    int unknown_f8;                     // 0xf8
    int unknown_fc;                     // 0xfc
    int unknown_100;                    // 0x100
    bool unknown_104;                   // 0x104
    S_unknown_108 unknown_108;          // 0x108
    uchar unknown_118[0x10];            // 0x118
};
