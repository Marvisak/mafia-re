#pragma once

#include "actors/C_actor.h"

// The clocks (PS2 C_clock, c_clock.cpp), 0x88 bytes (PS2's 0xa0): a model whose hands turn with the computer's time.
// Only the constructor, the destructor and Init so far, which C_mission::CreateActor and OpenBin call for ACTOR_CLOCK,
// and the vtable's functions from DuplicateFrom to GameDone, the last two stubs.
// VTABLE: GAME 0x625648
class C_clock : public C_actor
{
public:
    C_clock();
    // FUNCTION: GAME 0x56ae10
    virtual ~C_clock() {}
    // The hands: false without the hour's and the minute's
    virtual bool Init(I3D_frame *frm);
    // Inline: the vtable's copy (0x443860) is in another file's range and jumps to C_actor's (PS2 has a C_clock
    // DuplicateFrom too)
    // FUNCTION: GAME 0x443860
    virtual void DuplicateFrom(C_actor *act)
    {
        C_actor::DuplicateFrom(act);
    }
    // Nothing to save: the vtable has the copies of other classes' (0x409220, a xor eax, eax, and 0x4aa490, a mov al, 1)
    virtual int SaveGameGetSize()
    {
        return 0;
    }
    virtual bool SaveGameSave(void **buf)
    {
        return true;
    }
    virtual bool SaveGameLoad(void **buf)
    {
        return true;
    }
    virtual void GameInit();
    virtual void GameDone();

    // The hands, the frame's visuals "Mala", "Velka" and "Sekundova" (Czech: the small, the big and the second's; the
    // names are ours)
    I3D_frame *hand_hour;               // 0x70
    I3D_frame *hand_minute;             // 0x74
    I3D_frame *hand_second;             // 0x78, which the clock may lack
    uchar unknown_7c[0xc];              // 0x7c
};
