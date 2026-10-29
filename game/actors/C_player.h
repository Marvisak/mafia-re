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
    // Which the script's DIALOG_END calls on the game's player
    void IndicatorSetAimed();
    // The player's controls locked or unlocked, the script's PLAYER_LOCKCONTROLS
    void LockControls(bool lock);

    uchar unknown_a9c[0x1c];            // 0xa9c
    float unknown_ab8;                  // 0xab8, 1 from C_game::Init and FadeReset (PS2 0xc3c), SetAlpha's
    float unknown_abc;                  // 0xabc, 1 from C_game::Init and FadeReset (PS2 0xc40)
    uchar unknown_ac0[0x40];            // 0xac0

    // C_game::Init clears it, the script's CARDAMAGEVISIBLE sets it (PS2 a global set the same; the name is ours)
    static bool unknown_6478b0;
    // SetPlayerNaviPoint's (the name is ours)
    static I3D_frame *unknown_6478b4;

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b860
    static void SetUnknown6478b0(bool on)
    {
        unknown_6478b0 = on;
    }
};

// The player's navigation point, a frame or NULL (PS2 name), which the script's SETCOMPASS and C_game::Init set
void SetPlayerNaviPoint(I3D_frame *frm);
