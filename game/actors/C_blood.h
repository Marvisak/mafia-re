#pragma once

#include "Main.h"

#include <vector>

class C_blood;

// The blood the humans leave (PS2 C_blood_manager, C_blood.cpp), the game's 0x2d90, 0x10 bytes: its C_blood objects.
// Only the constructor and the destructor are written so far.
class C_blood_manager
{
public:
    std::vector<C_blood *> bloods;      // 0x0, GameDone (0x412050) deletes them

    C_blood_manager();
    ~C_blood_manager();
};
