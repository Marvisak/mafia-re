#pragma once

#include "Main.h"

// The game's tables of materials, particles, items and voices (PS2 g_material.cpp), each a file read whole into an
// array: tables\materials.def, particles.def, predmety.def and voices.def. LoadMaterialsFromBin (0x60f5c0, PS2's name),
// which G_LoadSaveClass::Init calls, loads them, UnloadMaterials (0x60f780) frees them; not written yet. The tables'
// structs and their names are ours, after their files.

// A material of the scene's surfaces (materials.def), 0x170 bytes. The cars take their part (C_car::LoadMaterials);
// their meanings aren't known, C_car::LoadMaterials clamps them.
struct S_material_def
{
    char name[0x20];                    // 0x0, the editor's, in Czech ("Asfalt", "Trava", ...)
    uchar unknown_20[0xe8];
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

// The voices (voices.def, 0xa4 bytes each) and the items (predmety.def, 0xbc bytes), not read yet
struct S_voice_def;
struct S_item_def;

// The tables (PS2 names) and their sizes in entries
extern S_voice_def *gameVoices;
extern uint numOfGameVoices;
extern S_item_def *gameItemTable;
extern uint numOfGameItems;
extern S_particle_def *gameParticleTable;
extern uint numOfGameParticles;
extern S_material_def *materialList;
extern uint numOfMaterials;
