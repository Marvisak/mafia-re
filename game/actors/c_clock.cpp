// The clocks (PS2 c_clock.cpp; the PC's file is the one at 0x56add0-0x56b17c, docs/game-files.md), which starts with
// the constructor. Written so far: the constructor and Init, which C_mission::CreateActor and OpenBin call, and the
// destructor (c_clock.h). Not written: GameInit (0x56ae90), GameDone (0x56afd0) and the hands' Tick (0x56afe0).

#include "actors/c_clock.h"

// FUNCTION: GAME 0x56add0
C_clock::C_clock()
{
    type = ACTOR_CLOCK;
}

// FUNCTION: GAME 0x56ae20
bool C_clock::Init(I3D_frame *frm)
{
    if (!frm || !C_actor::Init(frm))
        return false;
    // STRING: GAME 0x6368c8
    hand_hour = frame->FindChildFrame("Mala", ENUMF_VISUAL);
    // STRING: GAME 0x6368c0
    hand_minute = frame->FindChildFrame("Velka", ENUMF_VISUAL);
    // STRING: GAME 0x6368b4
    hand_second = frame->FindChildFrame("Sekundova", ENUMF_VISUAL);
    if (!hand_hour || !hand_minute)
        return false;
    return true;
}
