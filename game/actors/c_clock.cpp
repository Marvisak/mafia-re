// The clocks (PS2 c_clock.cpp; the PC's file is the one at 0x56add0-0x56b17c, docs/game-files.md), which starts with
// the constructor. Written so far: the constructor and Init, which C_mission::CreateActor and OpenBin call, the
// destructor (c_clock.h), and GameInit and GameDone, which C_game::Init and Done call. Not written: the hands' Tick
// (0x56afe0).

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

// The hour hand turns by the minutes too, the second hand starts at 0 (PS2's the same)
// FUNCTION: GAME 0x56ae90
void C_clock::GameInit()
{
    angle_hour = 0.0f;
    angle_minute = 0.0f;
    time = 0;
    C_actor::GameInit();
    unknown_65 = true;
    SYSTEMTIME st;
    GetLocalTime(&st);
    angle_hour = (float)(st.wHour * 2) * PI / 12.0f;
    angle_minute = (float)(st.wMinute * 2) * PI / 60.0f;
    hand_hour->SetDir(S_vector(0.0f, 1.0f, 0.0f), PI - (angle_hour + angle_minute / 12.0f));
    hand_hour->Update();
    hand_minute->SetDir(S_vector(0.0f, 1.0f, 0.0f), PI - angle_minute);
    hand_minute->Update();
    if (hand_second)
    {
        hand_second->SetDir(S_vector(0.0f, 1.0f, 0.0f), 0.0f);
        hand_second->Update();
    }
}

// FUNCTION: GAME 0x56afd0
void C_clock::GameDone()
{
    C_actor::GameDone();
}
