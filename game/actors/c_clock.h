#pragma once

#include "actors/C_actor.h"

// The clocks (PS2 C_clock, c_clock.cpp), 0x88 bytes (PS2's 0xa0): a model whose hands turn with the computer's time.
// Only the constructor, the destructor and Init so far, which C_mission::CreateActor and OpenBin call for ACTOR_CLOCK.
// VTABLE: GAME 0x625648
class C_clock : public C_actor
{
public:
    C_clock();
    // FUNCTION: GAME 0x56ae10
    virtual ~C_clock() {}
    // The hands: false without the hour's and the minute's
    virtual bool Init(I3D_frame *frm);

    // The hands, the frame's visuals "Mala", "Velka" and "Sekundova" (Czech: the small, the big and the second's; the
    // names are ours)
    I3D_frame *hand_hour;               // 0x70
    I3D_frame *hand_minute;             // 0x74
    I3D_frame *hand_second;             // 0x78, which the clock may lack
    uchar unknown_7c[0xc];              // 0x7c
};
