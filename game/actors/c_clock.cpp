// The clocks (PS2 c_clock.cpp; the PC's file is the one at 0x56add0-0x56b17c, docs/game-files.md), which starts with
// the constructor. Written so far: the constructor and Init, which C_mission::CreateActor and OpenBin call, the
// destructor (c_clock.h), GameInit and GameDone, which C_game::Init and Done call, and AI (PS2's name), which turns
// the hands: the whole file.

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
    elapsed = 0;
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

// Folded with the other returns of true of their kind (0x4aa490, C_actor.h)
bool C_clock::SaveGameSave(void **buf)
{
    return true;
}

bool C_clock::SaveGameLoad(void **buf)
{
    return true;
}

// Each minute past turns the minute hand on by one, each of its turns the hour hand by PI / 12, half of the PI / 6 an
// hour is in GameInit (PS2's the same); the second hand turns with the time
// FUNCTION: GAME 0x56afe0
void C_clock::AI(uint time)
{
    elapsed += time;
    if (elapsed > 60000)
    {
        elapsed -= 60000;
        angle_minute += 2 * PI / 60.0f;
        if (angle_minute > 2 * PI)
        {
            angle_minute -= 2 * PI;
            angle_hour += PI / 12.0f;
            if (angle_hour > 2 * PI)
                angle_hour -= 2 * PI;
        }
        hand_hour->SetDir(S_vector(0.0f, 1.0f, 0.0f), PI - (angle_hour + angle_minute / 12.0f));
        hand_hour->Update();
        hand_minute->SetDir(S_vector(0.0f, 1.0f, 0.0f), PI - angle_minute);
        hand_minute->Update();
    }
    if (hand_second)
    {
        hand_second->SetDir(S_vector(0.0f, 1.0f, 0.0f), PI - (float)elapsed * 2 * PI / 60000.0f);
        hand_second->Update();
    }
}
