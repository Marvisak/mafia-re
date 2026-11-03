#pragma once

#include "actors/C_human.h"

// The player (PS2 C_player, C_player.cpp), a human, 0xb00 bytes (PS2's 0xc90). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_PLAYER.
// VTABLE: GAME 0x625878
class C_player : public C_human
{
public:
    // What the player's model change adds (PS2 name), which C_human::_intern_ChangeModel calls
    void _intern_ChangeModel();
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
    // The car the player strikes, whose damage the HUD shows (PS2 name)
    void SetCarDamage(C_car *car);

    uchar unknown_a9c[0x1c];            // 0xa9c
    // The player's alpha and the one it fades to, 1 from C_game::Init and FadeReset: PS2's AI fades the first to the
    // second, 0 while the camera looks from his eyes, and passes it to SetAlpha (the names are ours)
    float fade_alpha;                   // 0xab8 (PS2 0xc3c)
    float fade_target;                  // 0xabc (PS2 0xc40)
    uchar unknown_ac0[0x10];            // 0xac0
    // 0xad0 (PS2 0xc50), the shots' recoil: PS2's ShootDeAimed adds the weapon's, AI lets it down with the time, and a
    // tenth of it comes off the camera's tilt (C_game::Tick; the name is ours)
    float recoil;
    uchar unknown_ad4[7];               // 0xad4
    bool unknown_adb;                   // 0xadb (PS2 0xc5b), the car's automatic gearing off (C_human::Update)
    uchar unknown_adc[0x19];            // 0xadc
    bool unknown_af5;                   // 0xaf5, C_human::SaveGameLoad clears it with the aim
    uchar unknown_af6[2];               // 0xaf6
    I3D_sound *unknown_af8;             // 0xaf8, C_human::SoundsChange turns it off
    uchar unknown_afc[4];               // 0xafc

    // C_game::Init clears it, the script's CARDAMAGEVISIBLE sets it (PS2 name)
    static bool can_car_damage;
    // SetPlayerNaviPoint's (PS2 name, a static of PS2's C_player.cpp)
    static I3D_frame *navi_frame;

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b860
    static void SetCanCarDamage(bool on)
    {
        can_car_damage = on;
    }
};

// SYNTHETIC: GAME 0x58f200
// C_player::`scalar deleting destructor'

// The player's navigation point, a frame or NULL (PS2 name), which the script's SETCOMPASS and C_game::Init set
void SetPlayerNaviPoint(I3D_frame *frm);
