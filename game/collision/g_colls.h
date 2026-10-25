#pragma once

#include "Main.h"

// The collisions (PS2 g_collision; the file is a guess, PS2's g_colls.cpp). Only what main uses so far.
class g_collision
{
public:
    // Loads a mission's collision tree (missions\<mission>\tree.klz, "kolize")
    bool LoadCollision(const char *name, bool);
};

extern g_collision gifCollision;
