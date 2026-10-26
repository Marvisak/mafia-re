// The game's tables (PS2 g_material.cpp, between g_loadsave.cpp and g_textdb.cpp as the PC's file at 0x60f5c0-0x60f82c
// is, docs/game-files.md). Written so far: the tables, which C_car's loaders read; LoadMaterialsFromBin and
// UnloadMaterials aren't, as their callers (G_LoadSaveClass's Init and Close) are stubs.

#include "g_material.h"

// Zero-initialized, so not hashed: the original has them in this order, PS2's (tools/bsshash.py would scatter them)
// GLOBAL: GAME 0x6d4c0c
S_voice_def *gameVoices = NULL;
// GLOBAL: GAME 0x6d4c10
uint numOfGameVoices = 0;
// GLOBAL: GAME 0x6d4c14
S_item_def *gameItemTable = NULL;
// GLOBAL: GAME 0x6d4c18
uint numOfGameItems = 0;
// GLOBAL: GAME 0x6d4c1c
S_particle_def *gameParticleTable = NULL;
// GLOBAL: GAME 0x6d4c20
uint numOfGameParticles = 0;
// GLOBAL: GAME 0x6d4c24
S_material_def *materialList = NULL;
// GLOBAL: GAME 0x6d4c28
uint numOfMaterials = 0;
