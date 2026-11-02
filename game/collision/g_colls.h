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
// holds is known only in part
struct tDynamicCollObject
{
    uchar unknown_0[4];
    I3D_frame *frame;                   // 0x4, the frame it is the collision of (the name is ours)
    uchar unknown_8[8];
    // 0x10, the sphere around it, an I3D_bsphere (bytes, as the box): the detector's Update sets it, DynUpdate finds
    // the object's cell by its position (the name is ours)
    uchar bsphere[0x10];
    uchar unknown_20[4];
    // 0x24, the box the radar draws, an I3D_bbox as PS2's: bytes here, as including the engine's math would give each
    // includer the engine header's statics, which the original's game files don't have
    uchar bbox[0x18];
    S_matrix *matrix;                   // 0x3c, the radar places the box by it
    uchar unknown_40[4];
    C_actor *owner;                     // 0x44, the actor whose collision it is (the name is ours)

    // PS2's, which clears it
    tDynamicCollObject() { memset(this, 0, sizeof(*this)); }
};

// g_collision_header::flags (the name is ours, after the command): DISABLECOLLS sets it
#define COLLHF_DISABLED 0x20
// The flags TickShoot tests (the names are ours, by value where the use isn't known): NewShoot keeps the frame of a
// glass or crash collision with 0x40 or 0x80
#define COLLHF_UNKNOWN_40 0x40
#define COLLHF_UNKNOWN_80 0x80
// A control signal's collision: a shot at it runs C_game::DestroyControlSignal, which PS2's tests the flag in again
#define COLLHF_CONTROL_SIGNAL 0x20000

// A collision of the level (PS2 g_collision_header): only its flags are known so far, the first dword, and the actor
// whose collision it is, which C_game::TickHydrant reads of what TestLineV returns (PS2 0x48, compared with ACTOR_CAR
// as an actor's type), so most likely this struct
struct g_collision_header
{
    union
    {
        uint flags;                     // 0x0, COLLHF_* (the name is ours)
        struct
        {
            uchar unknown_0[3];
            uchar material;             // 0x3, the surface's, an index of materialList (MAT_*, g_material.h)
        };
    };
    I3D_frame *frame;                   // 0x4 (the name is ours)
    uchar unknown_8[0x3c];              // 0x8
    C_actor *owner;                     // 0x44 (the name is ours)
};

// A cell of the grid of the moving objects' collisions (PS2 tDynamicPolicko, Czech "little field"): its place in the
// grid and the objects in it (the members' names are ours)
struct tDynamicPolicko
{
    int x, y;                           // 0x0
    std::vector<tDynamicCollObject *> objects; // 0x8
};

// The grid of the moving objects' cells (the names are ours): only what DynOptimizeTick, DynWipeOut and
// DeleteCollsLevel use is known so far
struct S_dyn_grid
{
    uchar unknown_0[0x10];
    int width;                          // 0x10, the cells in a row
    int height;                         // 0x14, the rows
    tDynamicPolicko **cells;            // 0x18, row by row, NULL where a cell has no objects
};

// The level's collisions, LoadCollision's (PS2 0x3e4, the names are ours): only what DeleteCollsLevel frees is known so
// far
struct S_coll_level
{
    uchar unknown_0[0x2c];
    uchar *unknown_2c;                  // 0x2c, an array
};

// The collisions (PS2 g_collision; the file is a guess, PS2's g_colls.cpp), 0x2c8 bytes (the constructor's last
// store). Only what main, the camera, the game and the script's commands use so far.
class g_collision
{
public:
    // Its copy is the file's last function before the file's end (0x5c4fa0)
    // FUNCTION: GAME 0x5c4fa0
    g_collision() : level(0), dyn_grid(NULL), unknown_2b0(0), unknown_2b4(0), unknown_2b8(0), unknown_2bc(0),
        unknown_2c4(0)
    {
    }

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
    // A horizontal line (PS2 names), the same way; TestLineH is the wrapper of _TestLineH
    int TestLineH(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
        uint flags2);
    int _TestLineH(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
        uint flags2);
    // A line from from along dir against the level's static collisions only (PS2 name and parameters, their names
    // ours, as TestLineV's): nonzero for a hit
    int TestLineHStatic(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags);
    // The level's collisions of a frame into colls (PS2 name), and their number
    uint FindCollsByFrame(I3D_frame *frm, std::vector<g_collision_header *> &colls);
    // Loads a mission's collision tree (missions\<mission>\tree.klz, "kolize")
    bool LoadCollision(const char *name, bool);
    // A line from from along dir against the moving objects' collisions (PS2 name; PS2's takes one more uint): the
    // object hit, NULL for none, its place into pos and the normal into normal
    tDynamicCollObject *TestLineHDynamic(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
        uint flags);
    // The objects the next test leaves out, and the callback it asks (PS2 names; the callback's type is ours)
    void DynSetNextCollOwnerMult(tDynamicCollObject **objs, uint num, void *callback);
    void DynSetNextCollOwner(tDynamicCollObject *obj, void *callback);
    // Takes a moving object out of the collisions (PS2 name)
    void DynErase(tDynamicCollObject *obj);
    // A moving object's collision moved: its cell found again by its sphere's position (PS2 name)
    void DynUpdate(tDynamicCollObject *obj);
    // The moving objects' collisions optimized each frame, which C_game::Tick calls (PS2 name)
    void DynOptimizeTick();
    // PS2 name (PS2's takes a bool)
    void GameDone();
    // PS2 names, GameDone's: the moving objects' collisions out, and the level's freed (PS2's DeleteCollsLevel takes a
    // bool, the PC's nothing)
    void DynWipeOut();
    void DeleteCollsLevel();

    uchar unknown_0[0x284];
    S_coll_level *level;                // 0x284 (PS2 0x3e4; the name is ours)
    uchar unknown_288[4];               // 0x288
    S_dyn_grid *dyn_grid;               // 0x28c (PS2 0x3ec; the names are ours, as the next two's)
    // 0x290, the cells taken out of the grid once empty (PS2 0x3f0, its gif::vector<tDynamicPolicko *, 8>)
    std::vector<tDynamicPolicko *> free_cells;
    // 0x2a0, the cells to check, which DynOptimizeTick empties (PS2 0x3fc)
    std::vector<tDynamicPolicko *> dyn_queue;
    int unknown_2b0;                    // 0x2b0
    int unknown_2b4;                    // 0x2b4
    int unknown_2b8;                    // 0x2b8
    int unknown_2bc;                    // 0x2bc
    uchar unknown_2c0[4];               // 0x2c0
    int unknown_2c4;                    // 0x2c4
};

// The implicit destructor (the two vectors'), which the initializer's atexit helper jumps to, after the initializer
// SYNTHETIC: GAME 0x5c2880
// ??1g_collision@@QAE@XZ

extern g_collision gifCollision;
