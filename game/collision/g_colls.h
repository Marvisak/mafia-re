#pragma once

#include "Main.h"

struct S_vector;

// TestSphereB's flags (the names are ours, by value until the collisions' code shows their meanings): what the sphere
// stops at, and the second flags
#define COLF_UNKNOWN_2 2
#define COLF_UNKNOWN_100 0x100
#define COLF_ALL 0xffffffff
#define COLF2_UNKNOWN_100000 0x100000

// The collisions (PS2 g_collision; the file is a guess, PS2's g_colls.cpp). Only what main and the camera use so far.
class g_collision
{
public:
    // A sphere of that radius moved from from along dir (PS2 names; the parameters' names are ours): where it stops
    // into pos, the surface's normal into normal where it isn't NULL. PS2 has TestSphereB in g_colls.cpp, the wrapper of
    // _TestSphereB, another file's.
    bool TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
        uint flags, uint flags2);
    bool _TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
        uint flags, uint flags2);
    // Loads a mission's collision tree (missions\<mission>\tree.klz, "kolize")
    bool LoadCollision(const char *name, bool);
};

extern g_collision gifCollision;
