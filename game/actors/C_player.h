#pragma once

#include "actors/C_human.h"

// The player (PS2 C_player, C_player.cpp), a human, 0xb00 bytes (PS2's 0xc90). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_PLAYER.
// VTABLE: GAME 0x625878
class C_player : public C_human
{
public:
    C_player();
    virtual ~C_player();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    // The base's: the original's jumps there
    // FUNCTION: GAME 0x594050 FOLDED
    virtual void Update(uint time)
    {
        C_human::Update(time);
    }
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    // The base's: the original's jumps there
    // FUNCTION: GAME 0x58f290 FOLDED
    virtual void DuplicateFrom(C_actor *act)
    {
        C_human::DuplicateFrom(act);
    }
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    // The base's: the original's jumps there
    // FUNCTION: GAME 0x58f4a0 FOLDED
    virtual void Death()
    {
        C_human::Death();
    }
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

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
    uchar unknown_ac0[0x10];            // 0xac0
    float unknown_ad0;                  // 0xad0 (PS2 0xc50), a tenth of it off the camera's tilt (C_game::Tick)
    uchar unknown_ad4[0x2c];            // 0xad4

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

// SYNTHETIC: GAME 0x58f200
// C_player::`scalar deleting destructor'

// The player's navigation point, a frame or NULL (PS2 name), which the script's SETCOMPASS and C_game::Init set
void SetPlayerNaviPoint(I3D_frame *frm);
