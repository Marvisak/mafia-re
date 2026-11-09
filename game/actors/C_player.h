#pragma once

#include "actors/C_human.h"

// The saved game's data of the player, after C_human's (C_player::SaveGameSave's and SaveGameLoad's), 0x22 bytes,
// packed: the members the fields copy, in their order (the struct's name is ours)
#define PLAYER_SAVEGAME_VERSION 1

// C_player::primary_controls' bits, the car's controls whose primary binding was pressed last (the names are
// ours): AI_drive takes the secondary binding's value only once it moved more than 0.5 from where it was then,
// and tells the car which of its inputs are the secondary's (C_Vehicle::analog_steer and the others)
#define PRIMARY_STEER 0x1
#define PRIMARY_ACCELERATE 0x2
#define PRIMARY_BRAKE 0x4
#define PRIMARY_CLUTCH 0x8

#pragma pack(push, 1)
struct S_player_savegame
{
    uchar version;                      // 0x0, PLAYER_SAVEGAME_VERSION
    ushort primary_controls;            // 0x1
    float turn_last;                    // 0x3
    float turn_idle;                    // 0x7
    float turn_pending;                 // 0xb
    float fade_alpha;                   // 0xf, which SaveGameLoad doesn't read
    float fade_target;                  // 0x13, nor this
    float recoil;                       // 0x17
    int aim_time;                       // 0x1b
    bool horn;                          // 0x1f
    bool always_run;                    // 0x20
    bool controls_locked;               // 0x21
};
#pragma pack(pop)

// The player (PS2 C_player, C_player.cpp), a human, 0xb00 bytes (PS2's 0xc90), which C_mission::CreateActor creates
// for ACTOR_PLAYER.
// VTABLE: GAME 0x625878
class C_player : public C_human
{
public:
    // What the player's model change adds (PS2 name), which C_human::_intern_ChangeModel calls
    void _intern_ChangeModel();
    C_player();
    virtual ~C_player();
    // The vtable's other overrides (the slots' names are C_actor's)
    virtual void AI(uint time);
    // The base's: the original's jumps there
    // FUNCTION: GAME 0x594050 FOLDED
    virtual void Update(uint time)
    {
        C_human::Update(time);
    }
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
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

    // The player's controls by the time passed, values and pressed C_controls::GetValues' (PS2 names; PS2's take its
    // G_ConsoleControls): on foot, driving, and shooting from a car's passenger seat
    void AI_walk(uint time, float *values, float *pressed);
    void AI_drive(uint time, float *values, float *pressed);
    void AI_car_fire(uint time, float *values, float *pressed);
    // The action button on foot and in a car (PS2 names)
    void Do_AB_OwnerNULL();
    void Do_AB_OwnerCar();
    // Gets into the car's seat, or steals the car (PS2 name; the parameters' names are ours)
    void Can_UseCar(C_car *car, int seat);
    // The BOXER cheat (C_game's cheats, 0x10): the hands twice their size (PC only, the name is ours)
    void SetBoxer(bool on);
    // PS2 names
    void UpdateControls();
    void SetSniperMode(bool on);
    // The frames the player's camera fades with him, the weapons' models (PS2 names)
    void AddAlphaObject(I3D_frame *frm);
    void DelAlphaObject(I3D_frame *frm);
    void SetAlpha(float alpha);
    void FadeReset();
    // Which the script's DIALOG_END calls on the game's player
    void IndicatorSetAimed();
    // The player's controls locked or unlocked, the script's PLAYER_LOCKCONTROLS
    void LockControls(bool lock);
    // The car the player strikes, whose damage the HUD shows (PS2 name)
    void SetCarDamage(C_car *car);
    // The car gone (PS2 name): the HUD stops showing its damage
    void InvalidateCarDamage(C_car *car);
    // The name of the weapon in the hand in the HUD's console (PS2 name)
    void ShowWeaponName();
    // The player's shot of the weapon (PS2 name; C_human::Shooting): its recoil added, the HUD's accuracy
    void ShootDeAimed(S_item_def *weapon);

    // 0xa9c, PRIMARY_*: the car's controls whose primary binding was pressed last, 0xffff (all) from GameInit
    // (the name is ours)
    ushort primary_controls;
    uchar unknown_a9e[2];               // 0xa9e
    // The mouse's turn (AI_walk's; the names are ours): its last sideways move but 0, whose sign chooses the
    // turn animation (ANIM_LEFT1 or ANIM_RIGHT1), the seconds since it last moved sideways (past 0.12 s the turn
    // still to make is dropped), and the turn still to make, at most the profile's aiming_speed_x * 40 a second
    float turn_last;                    // 0xaa0
    float turn_idle;                    // 0xaa4
    float turn_pending;                 // 0xaa8
    // 0xaac, AI_walk's check every 0x218 ms (GameInit's period) for an object to use within reach, the HUD's
    // INDF_OBJECT_NEAR (the name is ours)
    S_timer use_timer;
    // 0xab4, the frame "base" of the model if it is a single mesh or a single morph (GameInit, _intern_ChangeModel), or
    // NULL: SetAlpha gives it the alpha first (the name is ours)
    I3D_object *alpha_base;
    // The player's alpha and the one it fades to, 1 from C_game::Init and FadeReset: PS2's AI fades the first to the
    // second, 0 while the camera looks from his eyes, and passes it to SetAlpha (the names are ours)
    float fade_alpha;                   // 0xab8 (PS2 0xc3c)
    float fade_target;                  // 0xabc (PS2 0xc40)
    // 0xac0, the frames AddAlphaObject adds (the weapons' models) which SetAlpha fades with him (the name is ours)
    std::vector<I3D_object *> alpha_objects;
    // 0xad0 (PS2 0xc50), the shots' recoil: PS2's ShootDeAimed adds the weapon's, AI lets it down with the time, and a
    // tenth of it comes off the camera's tilt (C_game::Tick; the name is ours)
    float recoil;
    // 0xad4 (PS2 0xc54), the time the weapon stays aimed after a shot or a strike: C_human::Do_Shoot sets 3000,
    // PS2's AI counts it down and lowers the aim at 0 (the name is ours)
    int aim_time;
    // 0xad8, the horn held (CTRL_HORN), which AI_drive passes to the car's (the name is ours)
    bool horn;
    // 0xad9, CTRL_TOGGLE_RUN's: he runs unless CTRL_WALK is held, and walks while it is held where it is
    // cleared; true from GameInit (the name is ours)
    bool always_run;
    // 0xada (PS2 0xc5a), LockControls' (the script's PLAYER_LOCKCONTROLS): while it is set the police's
    // SemTrefenaLocklaKurva must let him be hit (the name is ours)
    bool controls_locked;
    // 0xadb (PS2 0xc5b), the car's automatic gearing off (C_human::Update), which CTRL_GEARING toggles (the name
    // is ours)
    bool manual_gearing;
    // 0xadc, the time the HUD shows the damage of the car the player strikes, 3000 from SetCarDamage and -1 from
    // GameInit (the name is ours)
    int car_damage_time;
    // 0xae0, the car SetCarDamage was given, which InvalidateCarDamage forgets (the name is ours)
    C_car *damage_car;
    // The side roll (AI_walk's, the profile's side_roll; the names are ours): 1 after a side key alone, 2 after it
    // was let go within 200 ms, 3 once pressed again within 200 ms, which rolls; the game time of the last
    // change; and the side, left
    int side_step_state;                // 0xae4
    uint side_step_time;                // 0xae8
    bool side_step_left;                // 0xaec
    uchar unknown_aed[3];               // 0xaed
    // 0xaf0, the weapon ShowWeaponName showed the name of last, -1 from GameInit (the name is ours)
    int shown_weapon;
    // 0xaf4 (PS2 0xc74), set where the car the player gets into or steals was chosen in the item pick-up menu out of
    // more than one object (Do_AB_OwnerNULL): AI doesn't let a move cancel the stealing then while the mouse's
    // first button is down (igraph->GetMouseButtons(); the name is a guess)
    bool menu_choice;
    // 0xaf5, the camera's free look on for the aim in a car's other seat (AI_car_fire, for a weapon that doesn't shoot
    // from the car): AI doesn't count aim_time down while it is set; cleared with the free look off (C_human's
    // SaveGameLoad, the car functions; the name is ours)
    bool car_free_look;
    uchar unknown_af6[2];               // 0xaf6
    // 0xaf8, the sound the horn's control plays on foot (7b_zip.wav), AI_walk's, created at the first press;
    // C_human::SoundsChange turns it off, GameDone releases it (the name is ours)
    I3D_sound *horn_sound;
    uint horn_time;                     // 0xafc, the game time it played last, at most every 2 s

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
