#pragma once

#include "Main.h"

// The game's tables of materials, particles, items and voices (PS2 g_material.cpp), each a file read whole into an
// array: tables\materials.def, particles.def, predmety.def and voices.def. LoadMaterialsFromBin, which
// G_LoadSaveClass::Init calls, loads them, UnloadMaterials, which G_LoadSaveClass::Close calls, frees them. The tables'
// structs and their names are ours, after their files.

// The materials the shots treat apart (materialList's indices, g_collision_header::material; the names are ours, after
// materials.def's "Sklo" and "CRASH COLLISION")
#define MAT_GLASS 0x18
#define MAT_CRASH 0x29

// A material of the scene's surfaces (materials.def), 0x170 bytes. The cars take their part (C_car::LoadMaterials);
// their meanings aren't known, C_car::LoadMaterials clamps them.
struct S_material_def
{
    char name[0x20];                    // 0x0, the editor's, in Czech ("Asfalt", "Trava", ...)
    int unknown_20;                     // 0x20
    int shot_sound;                     // 0x24, the sound of a shot's hit (C_sound_table's id), -1 for none
    uchar unknown_28[0x14];             // 0x28
    // 0x3c, the texture of a shot's track on it (C_game::unknown_29bc's index) and the model of its holes
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

// A voice (voices.def), 0xa4 bytes, not read yet
struct S_voice_def
{
    uchar unknown_0[0xa4];
};

// S_item_def::flags (the name is a guess): what G_Inventory::HaveSomeWeapon tests, which the scripts' ANYWEAPONINHAND
// and ANYWEAPONININVENTORY pass
#define ITEMF_WEAPON 0x20

// An item (predmety.def, "items"), 0xbc bytes, not read yet but its flags
struct S_item_def
{
    uchar unknown_0[0x20];              // 0x0
    uint flags;                         // 0x20, ITEMF_*
    uchar unknown_24[0x98];             // 0x24
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
