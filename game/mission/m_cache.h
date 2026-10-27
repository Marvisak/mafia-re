#pragma once

#include "Main.h"

#include <vector>

class C_chunk;
class I3D_frame;
class I3D_scene;

// A model set's 8-byte element, which the destructor destroys one by one (PS2 S_cache_model_one; the members' names
// are ours)
struct S_cache_model_one
{
    I3D_frame *frame;                   // 0x0, which Close unlinks and releases
    uchar unknown_4[4];
};

// A model of the blocks (PS2 S_cache_model_set, 0x14 bytes on PC; the members' names are ours)
struct S_cache_model_set
{
    char *name;                         // 0x0, the model's file name, which the telephones and signs look for
    std::vector<S_cache_model_one> unknown_4;  // 0x4
};

// A model placed in a block (PS2 S_cache_model_inbb, 0x44 bytes on PC; the members' names are ours)
struct S_cache_model_inbb
{
    uchar unknown_0[2];
    ushort model;                       // 0x2, the index of its model set
    uchar unknown_4[0x40];
};

// An 8-byte element of a block's two arrays (the names are ours): its copy is two dwords
struct S_cache_block_pair
{
    uint unknown_0;
    uint unknown_4;
};

// A block (PS2 S_cache_base_block, 0x74 bytes on PC; the members' names are ours). The types are what the copy
// (0x403f20) copies them as: a dword, a byte, three dwords one by one, two arrays of four 8-byte elements. Open: the
// copy's inlined operator= of unknown_8 isn't the original's yet.
struct S_cache_base_block
{
    int unknown_0;                      // 0x0
    uchar unknown_4;                    // 0x4
    std::vector<uint> unknown_8;        // 0x8, 4-byte values, whose ~vector the destructor calls
    uint unknown_18;                    // 0x18
    uint unknown_1c;                    // 0x1c
    uint unknown_20;                    // 0x20
    S_cache_block_pair unknown_24[4];   // 0x24
    S_cache_block_pair unknown_44[4];   // 0x44
    std::vector<S_cache_model_inbb> models;  // 0x64
};

// The city's base blocks (PS2 C_cache_base_block, m_cache.cpp), which the mission opens with its scene and
// GameLoopSingle ticks
class C_cache_base_block
{
public:
    C_cache_base_block();
    ~C_cache_base_block();
    void Close();
    // Reads the mission's cache.bin into the scene's blocks; closes first
    void Open(C_chunk *chunk, I3D_scene *scene);
    // PS2 names
    void VisibleAllFrames();
    void Tick(int time);

    // Whether the blocks are open (the name is ours; its copy is among the accessors at the end of the file at
    // 0x460a50)
    bool IsOpen() const
    {
        return open;
    }

    std::vector<S_cache_base_block> blocks;  // 0x0 (the names are ours)
    std::vector<S_cache_model_set> model_sets;  // 0x10
    uint unknown_20;                    // 0x20, which the constructor leaves
    bool open;                          // 0x24, which Close clears first and GameLoopSingle tests before the Tick
    int unknown_28;                     // 0x28
};

// The one object (PS2 name)
extern C_cache_base_block base_cache;

// What Close calls of the vectors out of line: the blocks' copy and destruction, the destructor they call, and the
// model sets' operator=, of which the original kept another file's copy (folded)
// TEMPLATE: GAME 0x403b20
// ?_Destroy@?$vector@US_cache_base_block@@V?$allocator@US_cache_base_block@@@std@@@std@@IAEXPAUS_cache_base_block@@0@Z

// TEMPLATE: GAME 0x403f20
// ?copy@std@@YIPAUS_cache_base_block@@PAU2@00@Z

// SYNTHETIC: GAME 0x401c70
// ??1S_cache_base_block@@QAE@XZ

// TEMPLATE: GAME 0x5380e0
// ??4?$vector@US_cache_model_one@@V?$allocator@US_cache_model_one@@@std@@@std@@QAEAAV01@ABV01@@Z
