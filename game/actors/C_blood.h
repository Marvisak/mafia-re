#pragma once

#include "Main.h"

#include <vector>

class C_blood;

// The blood the humans leave (PS2 C_blood_manager, C_blood.cpp), the game's 0x2d90, 0x10 bytes: its C_blood objects.
// Only what the game's constructor, destructor, Init, Done and Tick call is written so far.
class C_blood_manager
{
public:
    std::vector<C_blood *> bloods;      // 0x0, GameDone (0x412050) deletes them

    C_blood_manager();
    ~C_blood_manager();

    // PS2 names: C_game's Init and Done call them
    void GameInit();
    void GameDone();
    // The stains by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // A stain (PS2 name and parameters; the names are ours), which C_human::Hit adds where a hit's blood falls
    void AddBlood(S_vector &pos, S_vector &dir, float size, int unknown, bool unknown2);
};

// The destructor, folded with C_tshift's: the name our PDB keeps for the two changes with the build (foldfix)
// FUNCTION: GAME 0x412000
// ??1C_blood_manager@@QAE@XZ
