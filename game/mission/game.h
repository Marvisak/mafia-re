#pragma once

#include "Main.h"

class C_mission;

// The game of the loaded mission (PS2 game.cpp), 0x36ac bytes, the size C_mission::Init creates it with. PS2's Init
// takes two bools and loads the mission's collisions itself; the PC's takes none, main loads them before it.
class C_game
{
public:
    uchar unknown_0[0x40];              // 0x0
    // While set, G_LoadSaveClass::LanguageSelect(-1) keeps the loaded texts (PS2 0x58)
    bool unknown_40;                    // 0x40
    uchar unknown_41[3];                // 0x41
    C_mission *mission;                 // 0x44, the mission that created it (PS2 0x5c)
    uchar unknown_48[0x36ac - 0x48];    // 0x48

    // Open: in the original both are inline, as PS2's C_mission::Init and UnInit show (they have them inlined), and the
    // copies the PC's calls go to are in Mission.cpp's range. They construct and destroy some 50 members (vectors and
    // objects of their own), which aren't known yet, so here they are stubs in game.cpp.
    C_game();
    ~C_game();

    void Init();
};
