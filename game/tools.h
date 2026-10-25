#pragma once

#include "Main.h"

// The game's sounds by number (PS2 C_sound_table, tools.cpp), from tables\sounds.dat. Only what InitSystem uses so far.
class C_sound_table
{
public:
    void LoadTables(const char *name);
};

extern C_sound_table sound_table;

// Plays one of the logo videos at the start, in the size given for an 800x600 screen (PC only, the name is ours)
void PlayLogo(const char *name, float sx, float sy);
