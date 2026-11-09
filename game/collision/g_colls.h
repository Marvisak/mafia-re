#pragma once

#include "Main.h"

#include <string.h>
#include <vector>

class I3D_frame;

struct I3D_bbox;

struct S_matrix;

struct S_vector;

struct tCBoxParam;

struct tCollTreeLevel;

struct tCollisionLevel;

struct tDynamicPolicko;

struct g_volume_triface;

struct tgcPanak2;

// TestSphereB's flags (the names are ours, by value until the collisions' code shows their meanings): what the sphere
// stops at, and the second flags
#define COLF_UNKNOWN_1 1
#define COLF_UNKNOWN_2 2
#define COLF_UNKNOWN_100 0x100
#define COLF_UNKNOWN_80000 0x80000
#define COLF_ALL 0xffffffff
#define COLF2_UNKNOWN_100000 0x100000
#define COLF2_UNKNOWN_80000 0x80000
#define COLF2_UNKNOWN_800000 0x800000
// The second flags of the player's lines from the camera, the shot's and the aim's (C_player's AI_walk, AI_drive and
// AI_car_fire; by value)
#define COLF2_UNKNOWN_200000 0x200000
#define COLF2_UNKNOWN_400000 0x400000

// tDynamicCollObject::type (the name is ours, by value): a human's two spheres
#define DYNCOLL_UNKNOWN_42 0x42
// The other types BoxInDynamic tests (the names are ours, by value): 0x40 and 0x44 by the sphere, as 0x42, and 0x41
// by the box
#define DYNCOLL_UNKNOWN_40 0x40
#define DYNCOLL_UNKNOWN_41 0x41
#define DYNCOLL_UNKNOWN_44 0x44
// The tracks' (C_game::CreateTrackEffect's, by value): the blood's, which the humans' steps through take blood from
#define DYNCOLL_UNKNOWN_48 0x48
// tDynamicCollObject::flags (the names are ours, by value where the meaning isn't known): a human's spheres, the lower
// one (the legs) and the upper one; ActivateColls puts in a dead human's upper one, UpdateDynColl moves the upper one
// of a dead human only and leaves out the lower one while the human's lower_coll_off is set
#define DYNCOLLF_LOWER 0x1
#define DYNCOLLF_UPPER 0x2
#define DYNCOLLF_UNKNOWN_100 0x100
// A bottle's (C_bottle::GameInit)
#define DYNCOLLF_UNKNOWN_400 0x400
// A box's (C_game::BoxPlacementAdd)
#define DYNCOLLF_UNKNOWN_1000 0x1000
// The inter-city boxes' (C_game::InterCity_GameInit's)
#define DYNCOLLF_UNKNOWN_4000 0x4000
// The tracks' (C_game::CreateTrackEffect's, with DYNCOLLF_LOWER; by value)
#define DYNCOLLF_UNKNOWN_200000 0x200000

// A moving object's collision (PS2 tDynamicCollObject, 0x60 bytes), 0x48 bytes by the game's arrays of them; what it
// holds is known only in part
struct tDynamicCollObject
{
    uint type;                          // 0x0, DYNCOLL_* (the name is ours)
    I3D_frame *frame;                   // 0x4, the frame it is the collision of (the name is ours)
    uint flags;                         // 0x8, DYNCOLLF_* (the name is ours)
    // 0xc, the cell of the moving objects' grid it is in, NULL while it is in none (the name is ours): DynInsert puts it
    // into the cell of its sphere's position, DynErase takes it out
    tDynamicPolicko *cell;
    // 0x10, the sphere around it, an I3D_bsphere (bytes, as the box): the detector's Update sets it, DynUpdate finds
    // the object's cell by its position (the name is ours); its radius is the float at 0xc
    uchar bsphere[0x10];
    uchar unknown_20[4];
    // 0x24, the box the radar draws, an I3D_bbox as PS2's: bytes here, as including the engine's math would give each
    // includer the engine header's statics, which the original's game files don't have
    uchar bbox[0x18];
    S_matrix *matrix;                   // 0x3c, the radar places the box by it
    S_matrix *inv_matrix;               // 0x40, matrix's inverse, a box's (C_game::BoxPlacementAdd; the name is ours)
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
// A moving object's collision most likely: C_human::MovementIntern tests the ground again on the next move where
// it stands on one with it or 0x40 (the name is ours, by value)
#define COLLHF_UNKNOWN_10000 0x10000
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

// The grid of the moving objects' cells (0x1c bytes, the names are ours), which LoadCollision lays over the level's
// bounds: an object's cell is the one its sphere's position falls in, the last row and column taking what lies beyond
struct S_dyn_grid
{
    float min_x, min_z;                 // 0x0, the level's
    float cell_size_x, cell_size_z;     // 0x8
    uint width;                         // 0x10, the cells in a row
    uint height;                        // 0x14, the rows
    tDynamicPolicko **cells;            // 0x18, row by row, NULL where a cell has no objects
};

// The collisions (PS2 g_collision; the file is a guess, PS2's g_colls.cpp), 0x2c8 bytes (the constructor's last
// store). Only what main, the camera, the game and the script's commands use so far.
class g_collision
{
public:
    // Its copy is the file's last function before the file's end (0x5c4fa0)
    // FUNCTION: GAME 0x5c4fa0
    g_collision() : level(0), dyn_grid(NULL), next_coll_callback(0), next_coll_owner(0), next_coll_owners(0),
        num_next_coll_owners(0), step_actor(NULL)
    {
    }

    // A sphere of that radius moved from from along dir (PS2 names; the parameters' names are ours): where it stops
    // into pos, the surface's normal into normal where it isn't NULL. PS2 has TestSphereB in g_colls.cpp, the wrapper of
    // _TestSphereB, another file's.
    int TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
        uint flags, uint flags2);
    int _TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
        uint flags, uint flags2);
    // A line from from along dir (PS2 names; the parameters' names are ours, its flags most likely TestSphereB's):
    // nonzero for a hit, its place into pos and the surface's normal into normal. TestLineV is the wrapper of
    // _TestLineV, as TestSphereB is of _TestSphereB, which returns 0 at once without a level.
    int TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
        uint flags2);
    // The same against the moving objects' collisions too (PS2 name; PS2's wrappers are in the PC's order): the
    // collision hit, a g_collision_header, 0 for none, as TestLineV's
    int TestLineVInnerDyn(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
        uint flags2);
    int _TestLineVInnerDyn(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
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
    // The level's static collisions within radius of pos whose material is from mat_min to mat_max, into colls (PS2
    // name; the parameters' names are ours), which C_game::NewExplosion calls
    void FindStaticCollsInLocality(const S_vector &pos, float radius, uint mat_min, uint mat_max,
        std::vector<g_collision_header *> &colls);
    // Loads a mission's collision tree (missions\<mission>\tree.klz, "kolize"), and with dyn the moving objects'
    // grid over it (the parameters' names are ours; PS2's returns nothing either)
    void LoadCollision(const char *name, bool dyn);
    // A box moved from one matrix to the other (PS2 names and parameters, their names ours): nonzero where it hits the
    // level or a moving object. TestDBox is the wrapper of _TestDBox, PS2's g_colls.cpp's as the other wrappers.
    bool TestDBox(const I3D_bbox &bbox, const S_matrix &m0, const S_matrix &m1, const S_vector &dir,
        const S_vector &v1, const S_vector &v2, uint flags, uint flags2);
    bool _TestDBox(const I3D_bbox &bbox, const S_matrix &m0, const S_matrix &m1, const S_vector &dir,
        const S_vector &v1, const S_vector &v2, uint flags, uint flags2);
    // _TestDBox's tests of the moving objects, a cell's volumes and a volume that isn't a face (PS2 names)
    bool BoxInDynamic(tCBoxParam &p);
    bool BoxInTreeLevel(tCollTreeLevel *cell, tCBoxParam &p);
    bool BoxInNonFace(g_volume_triface *vol, tCBoxParam &p);
    // A line from from along dir against the moving objects' collisions (PS2 name; PS2's takes one more uint): the
    // object hit, NULL for none, its place into pos and the normal into normal
    tDynamicCollObject *TestLineHDynamic(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
        uint flags);
    // The objects the next test leaves out, and the callback it asks (PS2 names; PS2's callback takes the object and the
    // sphere test's tCSphereParam and returns a gcCallbackRet, ours is a void * until the tests show its values)
    void DynSetNextCollOwnerMult(tDynamicCollObject **objs, uint num, void *callback);
    void DynSetNextCollOwner(tDynamicCollObject *obj, void *callback);
    // A line from from along dir against the collisions of a vehicle a human is in (PS2 name): the collision hit, as
    // TestLineV's
    int TestLineInVehicle(std::vector<tDynamicCollObject *> *colls, const S_vector &from, const S_vector &dir,
        S_vector *pos, S_vector *normal, uint flags);
    // The human's spheres moved (PS2 names): against the level and the moving objects, or against the collisions of a
    // vehicle the human is in (the truck's, the rail vehicle's wagon's); nonzero for a hit
    int TestPanak2(tgcPanak2 *p);
    int TestPanak2InVehicle(std::vector<tDynamicCollObject *> *colls, tgcPanak2 *p);
    // Puts a moving object into the collisions (PS2 name)
    void DynInsert(tDynamicCollObject *obj);
    // Takes a moving object out of the collisions (PS2 name)
    void DynErase(tDynamicCollObject *obj);
    // A moving object's collision moved: its cell found again by its sphere's position (PS2 name)
    void DynUpdate(tDynamicCollObject *obj);
    // Takes a moving object out of a vector of them (PS2 name; the parameters' names are ours)
    void DynGetOut(tDynamicCollObject *obj, std::vector<tDynamicCollObject *> *colls);
    // Puts a moving object into a vector of them (PS2 name; the parameters' names are ours)
    void DynGetIn(tDynamicCollObject *obj, std::vector<tDynamicCollObject *> *colls);
    // The moving objects' collisions optimized each frame, which C_game::Tick calls (PS2 name)
    void DynOptimizeTick();
    // PS2 name (PS2's takes a bool)
    void GameDone();
    // PS2 names, GameDone's: the moving objects' collisions out, and the level's freed (PS2's DeleteCollsLevel takes a
    // bool, the PC's nothing)
    void DynWipeOut();
    void DeleteCollsLevel();

    uchar unknown_0[0x284];
    tCollisionLevel *level;             // 0x284 (PS2 0x3e4; the name is ours)
    uint level_size;                    // 0x288, the bytes LoadCollision read into it (the name is ours)
    S_dyn_grid *dyn_grid;               // 0x28c (PS2 0x3ec; the names are ours, as the next two's)
    // 0x290, the cells taken out of the grid once empty (PS2 0x3f0, its gif::vector<tDynamicPolicko *, 8>)
    std::vector<tDynamicPolicko *> free_cells;
    // 0x2a0, the cells to check, which DynOptimizeTick empties (PS2 0x3fc)
    std::vector<tDynamicPolicko *> dyn_queue;
    // 0x2b0, what DynSetNextCollOwner and DynSetNextCollOwnerMult set (the names are ours, after them): the callback,
    // the one object (0x2b4) that DynSetNextCollOwner points the list at, the list (0x2b8) and its count (0x2bc)
    void *next_coll_callback;
    tDynamicCollObject *next_coll_owner;
    tDynamicCollObject **next_coll_owners;
    uint num_next_coll_owners;
    uchar unknown_2c0[4];               // 0x2c0
    // 0x2c4 (PS2 0x41c), the human whose step C_human::MovementIntern tests ahead of him with TestLineV, NULL
    // otherwise (the name is ours)
    C_actor *step_actor;
};

// The implicit destructor (the two vectors'), which the initializer's atexit helper jumps to, after the initializer
// SYNTHETIC: GAME 0x5c2880
// ??1g_collision@@QAE@XZ

extern g_collision gifCollision;
