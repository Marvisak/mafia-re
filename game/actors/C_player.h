#pragma once

#include "actors/C_human.h"

// The player (PS2 C_player, C_player.cpp), a human, 0xb00 bytes (PS2's 0xc90). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_PLAYER.
class C_player : public C_human
{
public:
    C_player();

    // PS2 names
    void UpdateControls();
    void SetSniperMode(bool on);
    void SetAlpha(float alpha);
    void FadeReset();
    // The name is ours (a free function's in C_player.cpp, __fastcall)
    static void SetUnknown6478b4(int value);

    uchar unknown_a9c[0x1c];            // 0xa9c
    float unknown_ab8;                  // 0xab8, 1 from C_game::Init and FadeReset (PS2 0xc3c), SetAlpha's
    float unknown_abc;                  // 0xabc, 1 from C_game::Init and FadeReset (PS2 0xc40)
    uchar unknown_ac0[0x40];            // 0xac0

    // C_game::Init clears it (the name is ours)
    static bool unknown_6478b0;
    // SetUnknown6478b4's (the name is ours)
    static int unknown_6478b4;
};
