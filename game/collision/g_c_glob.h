#pragma once

// The level's collisions as LoadCollision loads them (missions\<mission>\tree.klz) and the tests read them, PS2
// g_c_glob.cpp's types (the names are PS2's where its symbols name them, ours otherwise). The engine's header is
// included here and not in g_colls.h, which the game includes more widely: its math gives each includer the engine's
// zero vector, which the original's game files don't have (docs/game-files.md), and so far only the collisions' own
// files need the volumes' vectors.

#include "collision/g_colls.h"

#include <I3D/I3D.h>

class I3D_frame;

// g_volume_triface::flags (the names are ours): a volume that isn't a face, and one LoadCollision turned off (a face
// whose vertices nearly coincide or whose frame has no vertices, its flags set to this alone); the tests leave out both,
// and a nonface only while off
#define VOLF_DISABLED 0x20
#define VOLF_NONFACE 0x80
// A face whose vertices run the other way round its normal (PS2 builds its plane from them in the other order, the
// PC's level stores it): SphereInFace's inside tests expect the other sign (the name is a guess)
#define VOLF_REVERSED 0x100
// A face's edges in SphereInFace (the names are ours): the first vertex to the second, the second to the third, the
// third to the first, a bit each; a vertex goes by the bit of the edge opposite it (SphereInEdge's vertex codes)
#define FACE_EDGE_01 1
#define FACE_EDGE_12 2
#define FACE_EDGE_20 4
#define FACE_EDGES (FACE_EDGE_01 | FACE_EDGE_12 | FACE_EDGE_20)
// The low byte of a face's flags: GrantCoherence's coherence code, from the axis nearest the face's normal and whether
// its first edge (the first vertex to the second) runs along another axis (the names are ours, by what the codes
// mean): a face facing x with the edge level (the same y at both ends) and one with it not, one facing y with the edge
// along z (the same x), along x (the same z) or neither, one facing z with the edge upright (the same x) or not
#define VOLF_COHERENCE 0xff
#define COHERENCE_X_LEVEL 1
#define COHERENCE_X 2
#define COHERENCE_Y_ALONG_Z 3
#define COHERENCE_Y_ALONG_X 4
#define COHERENCE_Y 5
#define COHERENCE_Z_UPRIGHT 6
#define COHERENCE_Z 7
// GrantCoherence's axes, the one nearest the face's normal, before the first edge picks the code (the names are ours)
#define COHERENCE_AXIS_X 8
#define COHERENCE_AXIS_Y 9
#define COHERENCE_AXIS_Z 10

// The type byte of a cell's entries in the file (the high byte of each, the low 24 bits the volume's offset in its array;
// the names are ours, after the volumes')
#define VOLTYPE_TRIFACE 0x00
#define VOLTYPE_XTOBB 0x80
#define VOLTYPE_AABB 0x81
#define VOLTYPE_SPHERE 0x82
#define VOLTYPE_OBB 0x83
#define VOLTYPE_CYLINDER 0x84

// A face of the level's collisions (PS2 g_volume_triface, 0x20 bytes, the level's 0x34): its flags, the three vertices
// and the face's plane. PS2's tests take the other volumes ("nonfaces") as this type too: they begin with the same flags.
struct g_volume_triface
{
    uint flags;                         // 0x0, VOLF_* (the name is ours, as the members' below)
    // 0x4, in the file each a vertex index in its low word and the frame's in the high one, KLZ_LEVEL_VERTICES for the
    // level's own vertices (only the first's frame counts); LoadCollision makes them pointers, a frame's into the vertices
    // of its mesh's first lod (an I3D_vertex's position)
    S_vector *vertices[3];
    S_vector normal;                    // 0x10
    float d;                            // 0x1c
};

// The volumes that aren't faces (the names are ours, after the file's types; 0x4 the frame, in the file the index of its
// name): a box with an oriented box in it (0xb8 bytes, the level's 0x3c), an axis-aligned box (0x20, 0x44), a sphere
// (0x18, 0x4c), an oriented box (0xa0, 0x54) and an upright cylinder (0x14, 0x5c)
struct g_volume_xtobb
{
    uint flags;                         // 0x0
    I3D_frame *frame;                   // 0x4
    I3D_bbox bbox;                      // 0x8, the box around it, which BoxInNonFace tests as the AABB's
    // 0x20, the oriented box, as the OBB's: its extents in its own space, which LineInNonface tests the line against
    // through LineInAABox, its matrix, which turns the hit's normal back out, and the matrix's inverse, which moves the
    // line into that space
    I3D_bbox extents;
    S_matrix matrix;                    // 0x38
    S_matrix inv_matrix;                // 0x78
};

struct g_volume_aabb
{
    uint flags;                         // 0x0
    I3D_frame *frame;                   // 0x4
    I3D_bbox bbox;                      // 0x8
};

struct g_volume_sphere
{
    uint flags;                         // 0x0
    I3D_frame *frame;                   // 0x4
    S_vector pos;                       // 0x8
    float radius;                       // 0x14
};

struct g_volume_obb
{
    uint flags;                         // 0x0
    I3D_frame *frame;                   // 0x4
    // 0x8, its extents in its own space, which BoxInNonFace tests the box's center against in that space, its height
    // from 0 (its y extents are left out, PS2's the same)
    I3D_bbox extents;
    S_matrix matrix;                    // 0x20, out of its own space: LineInNonface turns the hit's normal back by it
    S_matrix inv_matrix;                // 0x60, into its own space
};

struct g_volume_cylinder
{
    uint flags;                         // 0x0
    I3D_frame *frame;                   // 0x4
    float x, z;                         // 0x8, its axis's place
    float radius;                       // 0x10
};

// A cell of the level's grid (PS2 tCollTreeLevel, 0x10 bytes and its arrays after it, the names ours): its volumes
struct tCollTreeLevel
{
    uint num_volumes;                   // 0x0
    // 0x4, the cell's volumes, right after the cell (in the file the type byte and offset of each, VOLTYPE_*)
    g_volume_triface **volumes;
    uchar *unknown_8;                   // 0x8, a byte for each volume, after them
    float max_y;                        // 0xc, the highest point of its volumes (the name is ours)
};

// The level's collisions (PS2 tCollisionLevel; the names are ours): tree.klz's data after its header and the frames'
// names, read whole; LoadCollision sets the arrays' pointers. They follow the header in this order: grid_x, grid_z,
// the faces, AABBs, XTOBBs, cylinders, OBBs, spheres and vertices (an array the file has none of is NULL), a dword,
// then the grid's cells row by row, each a tCollTreeLevel, its volumes and a byte for each, the next aligned to 4.
struct tCollisionLevel
{
    float min_x, min_z;                 // 0x0, the grid's bounds
    float max_x, max_z;                 // 0x8
    float cell_size_x, cell_size_z;     // 0x10
    int width, height;                  // 0x18, the cells in a row and the rows
    float dyn_cell_size;                // 0x20, the size of the moving objects' grid's cells
    // 0x24, width + 1 floats, right after the header (0x68): the columns' edges in x, which the line tests walking the
    // grid (TestLineHStatic, _TestLineH) step from column to column by
    float *grid_x;
    float *grid_z;                      // 0x28, height + 1 floats, the rows' edges in z the same way, and a dword
    tCollTreeLevel **cells;             // 0x2c, row by row (LoadCollision allocates it)
    uint num_trifaces;                  // 0x30
    g_volume_triface *trifaces;         // 0x34
    uint num_xtobbs;                    // 0x38
    g_volume_xtobb *xtobbs;             // 0x3c
    uint num_aabbs;                     // 0x40
    g_volume_aabb *aabbs;               // 0x44
    uint num_spheres;                   // 0x48
    g_volume_sphere *spheres;           // 0x4c
    uint num_obbs;                      // 0x50
    g_volume_obb *obbs;                 // 0x54
    uint num_cylinders;                 // 0x58
    g_volume_cylinder *cylinders;       // 0x5c
    uint num_vertices;                  // 0x60
    S_vector *vertices;                 // 0x64
};

// tree.klz's header (0x18 bytes, the names are ours): KLZ_MAGIC, the version, the offset of the level's data (after
// the header, the frames' table and their names), the number of frames
#define KLZ_MAGIC 0x43666947            // 'GifC'
struct S_klz_header
{
    uint magic;                         // 0x0
    uint version;                       // 0x4
    uint data_offset;                   // 0x8
    uint num_frames;                    // 0xc
    uchar unknown_10[8];
};

// A frame the level's volumes name (the names are ours): the file has a table of offsets (from the header's start) to
// each one's flags and name; LoadCollision makes the offsets pointers and puts the frame found by the name in place of
// the flags, then the vertices of a visual's mesh (its first lod's) in place of the frame, NULL for the other frames
#define KLZ_LEVEL_VERTICES 0x7fff       // a face's frame index for the level's own vertices
// The frame has to be a visual (another frame of the name is dropped), whose vertices LoadCollision then puts into world
// space (SetSpace; the name is ours)
#define KLZF_VISUAL 0x1
struct S_klz_frame
{
    union
    {
        uint flags;                     // 0x0, KLZF_*
        I3D_frame *frame;
        I3D_vertex *vertices;
    };
    char name[1];                       // 0x4
};

// What _TestDBox tests the volumes against (PS2 tCBoxParam), a local of it that BoxInDynamic and BoxInTreeLevel read:
// only what those three use (the names are ours)
struct tCBoxParam
{
    I3D_bbox bbox;                      // 0x0, the box's bounds at both matrices
    uchar unknown_18[0x24];             // 0x18
    S_vector center;                    // 0x3c, the box's center at the second matrix
    // 0x48, that center moved along v2 twenty times (the name is ours, as the members' below)
    S_vector center2;
    // 0x54, the square of center's distance to a corner, though BoxInNonFace takes it for the radius
    float radius;
    float radius2;                      // 0x58, the same of center2, which BoxInTreeLevel compares squares with
    uchar unknown_5c[0x18];             // 0x5c
    uint flags;                         // 0x74, _TestDBox's flags: the moving objects' flags that BoxInDynamic leaves out
    uint flags2;                        // 0x78
};

// tCLineParam::walked (the names are ours): the directions the walk over the grid's cells has stepped in, so the
// neighbours of the cell under test that were tested before it
#define LINEWALK_POS_Z 1
#define LINEWALK_POS_X 2
#define LINEWALK_NEG_Z 4
#define LINEWALK_NEG_X 8

// What the line tests test the volumes against (PS2 tCLineParam, 0x5c bytes, the names ours): _TestLineV fills it in,
// the tests of the cells, the volumes and the moving objects put the nearest hit into it
struct tCLineParam
{
    S_vector from;                      // 0x0
    S_vector dir;                       // 0xc
    uchar unknown_18[4];                // 0x18
    // 0x1c, the line's bounds in x and z, the lower ends, then the higher ones
    float min_x, min_z;
    float max_x, max_z;                 // 0x24
    S_vector to;                        // 0x2c, from + dir
    uint flags2;                        // 0x38, the volumes' flags the test leaves out, the second flags and VOLF_DISABLED
    uint flags;                         // 0x3c, the moving objects' flags the test leaves out
    float t;                            // 0x40, the nearest hit's place along dir, 1.0 for none
    float dir_sq;                       // 0x44, dir's square
    S_vector normal;                    // 0x48, the hit's normal
    g_collision_header *hit;            // 0x54, the volume or the moving object hit, NULL for none
    // 0x58, LINEWALK_*: _TestLineV sets LINEWALK_POS_X after each cell and LINEWALK_POS_Z after each row, the
    // horizontal tests clear their inner walk's direction before each column or row and set it after each cell, and the
    // outer one's after each column or row; nothing reads it (PS2 neither; the name is a guess)
    uchar walked;
    uchar unknown_59[3];
};

// What the sphere tests test the volumes against (PS2 tCSphereParam, 0x88 bytes by SphereInDynamics's copies of it, the
// names ours): _TestSphereB fills it in, the tests of the cells, the volumes and the moving objects put the nearest hit
// into it
struct tCSphereParam
{
    S_vector from;                      // 0x0
    S_vector dir;                       // 0xc
    float radius;                       // 0x18
    // 0x1c, the move's bounds in x and z widened by the radius, the lower ends, then the higher ones
    float min_x, min_z;
    float max_x, max_z;                 // 0x24
    S_vector to;                        // 0x2c, from + dir
    S_vector dir_n;                     // 0x38, dir normalized
    uint flags2;                        // 0x44, the volumes' flags the test leaves out, the second flags and VOLF_DISABLED
    uint flags;                         // 0x48, the moving objects' flags the test leaves out
    uchar unknown_4c[4];                // 0x4c
    // 0x50, the square of the nearest hit's distance along the move, dir's square for none (lastSphereDistance)
    float dist_sq;
    // 0x54, the horizontal perpendicular of dir_n ((0, 1, 0) crossed with it, normalized; (1, 0, 0) for an upright
    // move), and the d of the planes along the move through the sphere's two sides, the lower one first (0x60)
    S_vector side;
    float side_d_min, side_d_max;
    S_vector pos;                       // 0x68, the nearest hit's place
    S_vector normal;                    // 0x74, its normal, reversed into the caller's
    g_collision_header *hit;            // 0x80, the volume or the moving object hit, NULL for none
    bool unknown_84;                    // 0x84
    uchar unknown_85[3];
};

// A point of a curve the racing's cars drive along (PS2 g_collision::gcol_CurveSphereStruct, 0x10 bytes, the names
// ours): its place and its time
struct g_collision::gcol_CurveSphereStruct
{
    S_vector pos;                       // 0x0
    float t;                            // 0xc
};

// Sets the faces' coherence from their normals once the level is loaded (PS2's, GrantCoherence__FP15tCollisionLevel)
void GrantCoherence(tCollisionLevel *level);
