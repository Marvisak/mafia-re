#pragma once

#include "Main.h"

// The game of the loaded mission (PS2 game.cpp). PS2's Init takes two bools and loads the mission's collisions itself;
// the PC's takes none, main loads them before it.
class C_game
{
public:
    void Init();
};
