#pragma once

#include "Main.h"
#include "I3D/I3d_math.h"

#include <vector>

// The AI's check segments in clusters of four (PS2 check_seg, check_seg_cluster and check_seg_clusters; PS2's
// jp_ai_segments.cpp, most likely the PC's file at 0x53e4b0-0x53ef4c), read from the mission's clusters.seg. The
// classes' and the methods' names are PS2's, the members' are ours: those whose meaning isn't known are named by their
// offset, the types by their values.

// A segment's type (the low 4 bits of its flags) and its specials' (4 bits each after them): bits, the names by value
#define CHECK_SEG_UNKNOWN_1 1
#define CHECK_SEG_UNKNOWN_2 2
#define CHECK_SEG_UNKNOWN_4 4
#define CHECK_SEG_UNKNOWN_8 8

// What a segment's data points to, by its type (the names are ours; PS2 reads them into a temporary and copies the
// members, which shows them). Allocated as raw memory: an S_vector's constructor, empty and inline, would be a pending
// inline call, and Load leaves its vector's _Destroy out of line with one.
struct check_seg_data_1
{
    S_vector unknown_0;                 // 0x0
    S_vector unknown_c;                 // 0xc
    S_vector unknown_18;                // 0x18
    int unknown_24;                     // 0x24
};
struct check_seg_data_2
{
    S_vector unknown_0;                 // 0x0
    S_vector unknown_c;                 // 0xc
    S_vector unknown_18;                // 0x18
    S_vector unknown_24;                // 0x24
    int unknown_30;                     // 0x30
};
struct check_seg_data_4
{
    S_vector unknown_0;                 // 0x0
    int unknown_c;                      // 0xc
};

// What a segment's specials point to, by their types (the names are ours): a position and the radius isInSpecial
// (0x53e570) tests against, allocated as raw memory too
struct check_seg_special_1
{
    S_vector pos;                       // 0x0
    uchar unknown_c[0x20];              // 0xc
    int unknown_2c;                     // 0x2c
    float radius;                       // 0x30
    int unknown_34;                     // 0x34
};
struct check_seg_special_2
{
    S_vector pos;                       // 0x0
    float unknown_c[6];                 // 0xc
    float radius;                       // 0x24
    float unknown_28;                   // 0x28
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30, cleared after it's read
    int unknown_34;                     // 0x34, cleared after it's read
};
struct check_seg_special_4
{
    S_vector pos;                       // 0x0
    int unknown_c;                      // 0xc
    float radius;                       // 0x10
    int unknown_14;                     // 0x14
};
struct check_seg_special_8
{
    S_vector pos;                       // 0x0
    uchar unknown_c[0x18];              // 0xc
    float radius;                       // 0x24
    int unknown_28;                     // 0x28
};

// A segment (PS2 check_seg), 0x2c bytes (PS2's 0x30). Its constructor (0x53ea00) and destructor (0x53ea20, folded with
// G_Inventory's) are implicit.
class check_seg
{
public:
    S_vector pos;                       // 0x0
    std::vector<uint> unknown_c;        // 0xc, sorted: existIndex (0x53e4b0) searches it
    // 0x1c: the type in the low 4 bits, each special's type in the next 4 bits, the specials' count in the top 4; 0
    // for a free slot
    uint flags;
    int unknown_20;                     // 0x20
    void *data;                         // 0x24, a check_seg_data_* of the type
    void **specials;                    // 0x28, check_seg_special_* of their types

    // PS2 names
    uint NumSpecials();
    void GetType(uint *type, void **data);
    void GetSpecial(uint index, uint *type, void **data);
};

// Four segments (PS2 check_seg_cluster), 0xb0 bytes; its constructor (0x53e9d0) and destructor (0x53e9f0) are implicit
struct check_seg_cluster
{
    check_seg segs[4];
};

// The segments (PS2 check_seg_clusters), a member of AIManager, 8 bytes
class check_seg_clusters
{
public:
    uint num_clusters;                  // 0x0
    check_seg_cluster *clusters;        // 0x4, new[]

    // PS2 names
    void AddSegment(check_seg *seg);
    void Init();
    void Clear();
    // Reads the mission's clusters.seg: false if it isn't one
    bool Load();
};

// The cluster's implicit constructor and destructor, which Init's new[] and Clear's delete[] pass to the CRT's
// iterators, and the segment's constructor, which the cluster's constructor passes
// SYNTHETIC: GAME 0x53e9d0
// ??0check_seg_cluster@@QAE@XZ

// SYNTHETIC: GAME 0x53e9f0
// ??1check_seg_cluster@@QAE@XZ

// SYNTHETIC: GAME 0x53ea00
// ??0check_seg@@QAE@XZ
