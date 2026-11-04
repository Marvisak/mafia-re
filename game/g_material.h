#pragma once

#include "Main.h"

// The game's tables of materials, particles, items and voices (PS2 g_material.cpp), each a file read whole into an
// array: tables\materials.def, particles.def, predmety.def and voices.def. LoadMaterialsFromBin, which
// G_LoadSaveClass::Init calls, loads them, UnloadMaterials, which G_LoadSaveClass::Close calls, frees them. The tables'
// structs and their names are ours, after their files.

// The materials the code names (materialList's indices, g_collision_header::material; the names are ours, after
// materials.def's "Kovove konstrukce" (metal structures), "Sklo" and "CRASH COLLISION"): the airplanes'
// (C_airplane::GetMaterialID's) and those the shots treat apart
#define MAT_METAL_STRUCTURE 0x13
#define MAT_GLASS 0x18
#define MAT_WATER_DEATH 0x1f            // "Voda Death": MovementIntern drowns the human
#define MAT_DEATH 0x28                  // "DEATH COLLISION": the player's fall fails the game
#define MAT_CRASH 0x29
#define MAT_UNKNOWN_2B 0x2b            // a strike on it makes no debris (C_human::AI; by value)

// A material of the scene's surfaces (materials.def), 0x170 bytes. The cars take their part (C_car::LoadMaterials);
// their meanings aren't known, C_car::LoadMaterials clamps them.
struct S_material_def
{
    char name[0x20];                    // 0x0, the editor's, in Czech ("Asfalt", "Trava", ...)
    int step_sound;                     // 0x20, the sound of a step on it (C_sound_table's id), 0 for none (ours)
    int shot_sound;                     // 0x24, the sound of a shot's hit (C_sound_table's id), -1 for none
    uchar unknown_28[8];                // 0x28
    // 0x30, the sound of a strike on it (C_sound_table's id), by the weapon in the right hand: the others, items 3,
    // 0x19, 0x1d and 0x1f, items 0 and 2 (C_human::AI; the name is ours)
    int strike_sounds[3];
    // 0x3c, the texture of a shot's track on it (C_game::track_textures' index) and the model of its holes
    // (C_game::hole_models'), -1 for none
    int hole;
    int particle;                       // 0x40, the effects' particle of a shot's hit, -1 for none
    uchar unknown_44[0xc4];             // 0x44
    float unknown_108;                  // 0x108, the cars' unknown_18
    float unknown_10c;                  // 0x10c, unknown_1c
    float unknown_110;                  // 0x110, unknown_08
    float unknown_114;                  // 0x114, unknown_04
    float unknown_118;                  // 0x118, unknown_10
    float unknown_11c;                  // 0x11c, unknown_0c
    float unknown_120;                  // 0x120, unknown_14
    int unknown_124;                    // 0x124, unknown_20
    int unknown_128;                    // 0x128, unknown_24
    int unknown_12c;                    // 0x12c, unknown_28
    uchar unknown_130[0x40];
};

// A particle emitter of the cars (particles.def), 0xa4 bytes: the model whose first frame is its particles' visual, and
// its I3D_emitor parameters (C_car::LoadParticles)
struct S_particle_def
{
    char name[0x20];                    // 0x0, the model ("smoke_particle2.I3D")
    uint emit_time;                     // 0x20, PP_EMIT_TIME
    uint emit_time_rnd;                 // 0x24
    uint life_time;                     // 0x28
    uint life_time_rnd;                 // 0x2c
    float grow_1;                       // 0x30
    float grow_2;                       // 0x34
    float pos_rnd;                      // 0x38
    float dir_rnd;                      // 0x3c, passed converted to an integer, where the emitter takes its bits
    uint dir_time;                      // 0x40
    uint dir_time_rnd;                  // 0x44
    float dir_time_stray;               // 0x48
    float fade_in;                      // 0x4c
    float fade_out;                     // 0x50
    float velocity[3];                  // 0x54
    float velocity_end[3];              // 0x60
    uint flags;                         // 0x6c, PP_FLAGS
    int max_particles;                  // 0x70
    uint unknown_74;
    uint unknown_78;
    float rot_speed_1;                  // 0x7c
    float rot_speed_2;                  // 0x80
    float scale;                        // 0x84
    uint unknown_88;
    float color[3];                     // 0x8c
    float color_end[3];                 // 0x98
};

// A voice (voices.def), 0xa4 bytes (the members' names are ours): what C_human::NewScream says
struct S_voice_def
{
    uint priority;                      // 0x0, a voice of a higher one interrupts it (50 for most)
    // 0x4, its wave files' numbers ("%08lu.wav") in each of the humans' voices (C_human::voice_type), 0 for none
    uint sounds[40];
};

// S_item_def::flags (the name is a guess): what G_Inventory::HaveSomeWeapon tests, which the scripts' ANYWEAPONINHAND
// and ANYWEAPONININVENTORY pass
#define ITEMF_WEAPON 0x20
// The weapons that kill at close range (C_human::Hit: a shot from within 3 kills, from within 5 throws the human
// down; the name is a guess)
#define ITEMF_CLOSE_RANGE 0x2
// The weapon has a sniper's scope (the Mosin-Nagant's): C_human::ChangeWeaponModel turns the player's sniper mode off
// for any other (the name is a guess)
#define ITEMF_SCOPE 0x4000

// Weapons of gameItemTable (predmety.def), by their names there: the parts of their models C_human's weapon effects
// move ("cylinder", "zaver1", "clip", "foregrip", "hlavne") follow them
#define GAMEITEM_KNUCKLEDUSTER 2        // "Knuckleduster"
#define GAMEITEM_KNIFE 3                // "Knife"
#define GAMEITEM_BASEBALL_BAT 4         // "Baseball Bat"
#define GAMEITEM_MOLOTOV 5              // "Molotov cocktail"
#define GAMEITEM_COLT_DETECTIVE 6       // "Colt Detective Special"
#define GAMEITEM_SW_MAGNUM 7            // "S&W model 27 Magnum"
#define GAMEITEM_SW_MP 8                // "S&W model 10 M&P"
#define GAMEITEM_COLT_1911 9            // "Colt 1911"
#define GAMEITEM_THOMPSON 10            // "Thompson 1928"
#define GAMEITEM_SHOTGUN 11             // "Pump shotgun", which reloads a shell at a time
#define GAMEITEM_SAWED_OFF 12           // "Saw off shotgun"
#define GAMEITEM_SPRINGFIELD 13         // "US Rifle M1903 Springfield"
#define GAMEITEM_MOSIN_NAGANT 14        // "Mosin-Nagant 1891/30"
#define GAMEITEM_GRENADE 15             // "Grenade"
// Other items C_human::DropOutItems gives a falling sound of their own
#define GAMEITEM_BUCKET 17              // "Bucket"
#define GAMEITEM_FLASHLIGHT 18          // "Flashlight"
#define GAMEITEM_BAR 20                 // "Bar"
#define GAMEITEM_CROWBAR 25             // "Crowbar"
#define GAMEITEM_BOARD 28               // "Deska"
#define GAMEITEM_BOTTLE 29              // "Bottle"
#define GAMEITEM_UNKNOWN_31 31          // its name is a single byte, 0x85

// gameItemTable's first item, "Special Action (nesahat!!!)" ("don't touch"), which C_human::ChangeState puts on a dead
// body's using objects, with the values 0x7dc and 0x7dd where a weapon has its ammunition (the name is ours)
#define GAMEITEM_SPECIAL_ACTION 1

// A step of what a weapon's fire does (the struct's and the members' names are ours): an item's list (S_item_def's
// fire) and the hands' own, PS2's _fire_action_*_ lists, file statics of C_human.cpp; each list ends with
// FIRE_ACTION_END. C_human keeps the step it is at and the time left of it.
struct S_fire_action
{
    int action;                         // 0x0, FIRE_ACTION_*
    ushort time;                        // 0x4, in milliseconds, as PS2's (C_human::AI and Do_Reload read a word)
};
#define FIRE_ACTION_END 3               // the lists' last step (the name is ours)
// The other steps (C_human::AI; the names are guesses): the reload's animation and the ammunition reloaded at the
// end, skipped while the weapon has rounds or there are none in reserve (the weapons' lists have it after the shot
// with their reload's time, 3 to 4.2 seconds, the pump shotgun's 0.4 a shell); a cartridge thrown out; the time
// counted again
#define FIRE_ACTION_RELOAD 1
#define FIRE_ACTION_CARTRIDGE 2
#define FIRE_ACTION_RESTART 4

// An item (predmety.def, "items"), 0xbc bytes, not read yet but its flags
struct S_item_def
{
    uchar unknown_0[0x20];              // 0x0
    uint flags;                         // 0x20, ITEMF_*
    char model[0x28];                   // 0x24, the weapon's model, the file ChangeWeaponModel opens (the name is ours)
    int gun_anim;                       // 0x4c, the number of the reload's animations, gun0%i (the name is ours)
    // 0x50, the number of the fire's animations, gun0%i ... Fire.i3d (C_human::Shooting), 4 for the weapons the corner
    // fire has an animation of its own for (the name is ours)
    int fire_anim;
    uint unknown_54;                    // 0x54, the sound table's sound of the weapon (NOTE_WEAPON_SOUND)
    uint unknown_58;                    // 0x58, the sound table's sound C_human::ChangeWeaponModel gives unknown_6c4
    // 0x5c, the weapon's kind, C_human::weapon_kind's: 0 (the fists) and 5 to 7 strike, 7 the bat, 1 to 3 shoot (2
    // and 3 with both hands), 4 is thrown; a strike breaks crash objects with 7, glass but with 6 (the name is ours)
    int kind;
    // The most ammunition in the weapon and in reserve, which C_human::InventoryTruncate cuts an item's to (the names
    // are ours)
    uint ammo_max;                      // 0x60
    uint spare_max;                     // 0x64
    uchar unknown_68[0xc];              // 0x68
    // The shot's (C_human::Shooting; the names are ours): its range, the length of the line C_game::NewShoot follows,
    // and its power, NewShoot's and a railway vehicle's hit's damage
    float range;                        // 0x74
    float power;                        // 0x78
    uchar unknown_7c[8];                // 0x7c
    // What a shot adds to the player's recoil (C_player::ShootDeAimed), and the weapon's accuracy: a shot strays
    // from its line by up to 1 - accuracy times the human's shooting (HUMAN_PROPERTY_SHOOTING; the names are ours)
    float recoil;                       // 0x84
    float accuracy;                     // 0x88
    S_fire_action fire[6];              // 0x8c, the weapon's fire steps, which C_human's Shooting and Do_Reload follow
};

// The tables (PS2 names) and their sizes in entries
extern S_voice_def *gameVoices;
extern uint numOfGameVoices;
extern S_item_def *gameItemTable;
extern uint numOfGameItems;
extern S_particle_def *gameParticleTable;
extern uint numOfGameParticles;
extern S_material_def *materialList;
extern uint numOfMaterials;

// Reads the four tables (PS2's names); false if a file is missing, the tables before it read
bool LoadMaterialsFromBin();
// Frees them
void UnloadMaterials();
