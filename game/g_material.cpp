// The game's tables (PS2 g_material.cpp, between g_loadsave.cpp and g_textdb.cpp as the PC's file at 0x60f5c0-0x60f82c
// is, docs/game-files.md): the tables, which C_car's loaders read, LoadMaterialsFromBin and UnloadMaterials, all of it.

#include "g_material.h"

#include "rw_data.h"

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

// FUNCTION: GAME 0x60f5c0
bool LoadMaterialsFromBin()
{
    // STRING: GAME 0x6399a4
    int file = dtaOpen("tables\\predmety.def", false);
    if (file < 0)
        return false;
    if (gameItemTable)
        delete[] gameItemTable;
    int size = dtaSeek(file, 0, SEEK_END);
    dtaSeek(file, 0, SEEK_SET);
    gameItemTable = (S_item_def *)new char[size];
    dtaRead(file, gameItemTable, size);
    dtaClose(file);
    numOfGameItems = size / sizeof(S_item_def);

    // STRING: GAME 0x63998c
    file = dtaOpen("tables\\materials.def", false);
    if (file < 0)
        return false;
    if (materialList)
        delete[] materialList;
    size = dtaSeek(file, 0, SEEK_END);
    dtaSeek(file, 0, SEEK_SET);
    materialList = (S_material_def *)new char[size];
    dtaRead(file, materialList, size);
    dtaClose(file);
    numOfMaterials = size / sizeof(S_material_def);

    // STRING: GAME 0x639974
    file = dtaOpen("tables\\particles.def", false);
    if (file < 0)
        return false;
    if (gameParticleTable)
        delete[] gameParticleTable;
    size = dtaSeek(file, 0, SEEK_END);
    dtaSeek(file, 0, SEEK_SET);
    gameParticleTable = (S_particle_def *)new char[size];
    dtaRead(file, gameParticleTable, size);
    dtaClose(file);
    numOfGameParticles = size / sizeof(S_particle_def);

    // STRING: GAME 0x639960
    file = dtaOpen("tables\\voices.def", false);
    if (file < 0)
        return false;
    if (gameVoices)
        delete[] gameVoices;
    size = dtaSeek(file, 0, SEEK_END);
    dtaSeek(file, 0, SEEK_SET);
    gameVoices = (S_voice_def *)new char[size];
    dtaRead(file, gameVoices, size);
    dtaClose(file);
    numOfGameVoices = size / sizeof(S_voice_def);
    return true;
}

// FUNCTION: GAME 0x60f780
void UnloadMaterials()
{
    if (gameItemTable)
        delete[] gameItemTable;
    gameItemTable = NULL;
    numOfGameItems = 0;
    if (materialList)
        delete[] materialList;
    materialList = NULL;
    numOfMaterials = 0;
    if (gameParticleTable)
        delete[] gameParticleTable;
    gameParticleTable = NULL;
    numOfGameParticles = 0;
    if (gameVoices)
        delete[] gameVoices;
    gameVoices = NULL;
    numOfGameVoices = 0;
}
