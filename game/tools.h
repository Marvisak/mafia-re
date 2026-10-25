#pragma once

#include "Main.h"

// The game's sounds by number (PS2 C_sound_table, tools.cpp), from tables\sounds.dat. Only what InitSystem and
// CloseSystem use so far.
class C_sound_table
{
public:
    void FreeTables();
    void LoadTables(const char *name);
};

extern C_sound_table sound_table;

// Plays one of the logo videos at the start until a key is pressed or it ends: the name without its extension, and the
// video's size on an 800x600 screen, whose ratio sets its height. False if no file was found or none opened. PC only,
// the name is ours.
bool PlayLogo(const char *name, float sx, float sy);
