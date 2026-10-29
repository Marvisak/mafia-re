#pragma once

#include "Main.h"

#include <string.h>
#include <vector>

class I3D_frame;

struct S_matrix;

struct S_vector;

// TestSphereB's flags (the names are ours, by value until the collisions' code shows their meanings): what the sphere
// stops at, and the second flags
#define COLF_UNKNOWN_2 2
#define COLF_UNKNOWN_100 0x100
#define COLF_ALL 0xffffffff
#define COLF2_UNKNOWN_100000 0x100000

// A moving object's collision (PS2 tDynamicCollObject, 0x60 bytes), 0x48 bytes by the game's arrays of them; what it
// holds isn't known yet
struct tDynamicCollObject
{
    uchar unknown_0[0x24];
    // 0x24, the box the radar draws, an I3D_bbox as PS2's: bytes here, as including the engine's math would give each
    // includer the engine header's statics, which the original's game files don't have
    uchar bbox[0x18];
    S_matrix *matrix;                   // 0x3c, the radar places the box by it
    uchar unknown_40[8];

    // PS2's, which clears it
    tDynamicCollObject() { memset(this, 0, sizeof(*this)); }
};

// g_collision_header::flags (the name is ours, after the command): DISABLECOLLS sets it
#define COLLHF_DISABLED 0x20

// A collision of the level (PS2 g_collision_header): only its flags are known so far, the first dword
struct g_collision_header
{
    uint flags;                         // 0x0, COLLHF_* (the name is ours)
};

// The collisions (PS2 g_collision; the file is a guess, PS2's g_colls.cpp). Only what main, the camera, the game and
// the script's commands use so far.
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
    // A line from from along dir (PS2 names; the parameters' names are ours, its flags most likely TestSphereB's):
    // nonzero for a hit, its place into pos and the surface's normal into normal. TestLineV is the wrapper of
    // _TestLineV, as TestSphereB is of _TestSphereB, which returns 0 at once without a level.
    int TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
        uint flags2);
    int _TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
        uint flags2);
    // The level's collisions of a frame into colls (PS2 name), and their number
    uint FindCollsByFrame(I3D_frame *frm, std::vector<g_collision_header *> &colls);
    // Loads a mission's collision tree (missions\<mission>\tree.klz, "kolize")
    bool LoadCollision(const char *name, bool);
    // Takes a moving object out of the collisions (PS2 name)
    void DynErase(tDynamicCollObject *obj);
    // PS2 name (PS2's takes a bool)
    void GameDone();
    // PS2 names, GameDone's: the moving objects' collisions out, and the level's freed (PS2's DeleteCollsLevel takes a
    // bool, the PC's nothing)
    void DynWipeOut();
    void DeleteCollsLevel();
};

extern g_collision gifCollision;
