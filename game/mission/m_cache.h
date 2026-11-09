#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_chunk;
class I3D_frame;
class I3D_scene;

// cache.bin, the mission's base blocks (the names are ours): a chunk of the file's version, then a chunk for each block
// (its frame's name, bounding circle and edges, then a chunk for each model it places)
#define CHUNK_CACHE 500
#define CHUNK_CACHE_BLOCK 1000
#define CHUNK_CACHE_MODEL 2000
#define CACHE_VERSION 1

// What C_cache_base_block::Open returns (the names are ours; C_mission::Open ignores it)
#define CACHE_OPEN_OK 0
#define CACHE_OPEN_NOT_CACHE 1          // the file doesn't start with a CHUNK_CACHE
#define CACHE_OPEN_BAD_VERSION 2
#define CACHE_OPEN_NO_FRAME 3           // a block's frame isn't in the scene
#define CACHE_OPEN_NO_MODEL 4           // a model couldn't be loaded

// A copy of a model set's model (PS2 S_cache_model_one, 8 bytes; the members' names are ours)
struct S_cache_model_one
{
    I3D_frame *frame;                   // 0x0, the model
    bool used;                          // 0x4, placed by an active block's model
};

// A model the blocks place (PS2 S_cache_model_set, 0x14 bytes on PC; the members' names are ours): the copies of the
// model are made as the active blocks need them, the first the one the model cache loaded
struct S_cache_model_set
{
    char *name;                         // 0x0, the model's file name, which the telephones and signs look for
    std::vector<S_cache_model_one> instances;  // 0x4
};

// A model placed in a block (PS2 S_cache_model_inbb, 0x44 bytes on PC; the members' names are ours)
struct S_cache_model_inbb
{
    ushort block;                       // 0x0, the index of its block
    ushort model;                       // 0x2, the index of its model set, 0xffff for none
    ushort instance;                    // 0x4, the copy of the set's model it shows while its block is active, or 0xffff
    S_vector pos;                       // 0x8
    S_quat rot;                         // 0x14
    S_vector scale;                     // 0x24
    uint unknown_30;                    // 0x30, read from the file; no code reads it
    bool visible;                       // 0x34, false once destroyed (DestroyFrame), until VisibleAllFrames
    S_vector visual_scale;              // 0x38, the scale of the model's visuals
};

// A block of the city (PS2 S_cache_base_block, 0x74 bytes on PC, where PS2's has a name of 32 characters after the
// frame; the members' names are ours). The arrays' S_vector2 constructor makes it non-POD: its implicit constructor
// constructs them (Open's local)
struct S_cache_base_block
{
    I3D_frame *frame;                   // 0x0, the block's frame in the scene, off while the block is inactive
    bool active;                        // 0x4
    std::vector<I3D_frame *> visuals;   // 0x8, the frame's visual children, switched with it
    S_vector2 pos;                      // 0x18, the centre of its bounding circle, in x and z
    float radius;                       // 0x20
    S_vector2 points[4];                // 0x24, a point on each of its four edges
    S_vector2 dirs[4];                  // 0x44, the edges' directions
    std::vector<S_cache_model_inbb> models;  // 0x64
};

// The city's base blocks (PS2 C_cache_base_block, m_cache.cpp), which the mission opens with its scene and
// GameLoopSingle ticks: the blocks around the camera are active, their models placed from the copies of the model sets
class C_cache_base_block
{
public:
    // The scene's callback (PS2's S_hlp, the name of PrepareScene's local struct)
    struct S_hlp;

    C_cache_base_block();
    ~C_cache_base_block();
    void Close();
    // PS2 names
    void PrepareScene();
    // Reads the mission's cache.bin into the scene's blocks; closes first. CACHE_OPEN_*
    int Open(C_chunk *chunk, I3D_scene *scene);
    // The index of the model set of a file, loaded through the model cache if there is none: 0xffff if it can't be
    ushort OpenModel(const char *name);
    void Tick(uint time);
    void ActivateBlock(uint index);
    void DeactivateBlock(uint index);
    // A free copy of a set's model, made if there is none, and its index (PS2 names)
    I3D_frame *ShowModel(ushort model, ushort *instance);
    void HideModel(ushort model, ushort instance);
    I3D_frame *FindModel(ushort model, ushort instance);
    // Hides a block's model for good (until VisibleAllFrames)
    bool DestroyFrame(S_cache_model_inbb *m);
    void VisibleAllFrames();
    // Whether a block reaches within radius of pos
    bool IsBlockInRange(uint index, S_vector2 pos, float radius);

    // Whether the blocks are open (the name is ours; its copy is among the accessors at the end of the file at
    // 0x460a50)
    bool IsOpen() const
    {
        return open;
    }

    std::vector<S_cache_base_block> blocks;  // 0x0 (the names are ours)
    std::vector<S_cache_model_set> model_sets;  // 0x10
    uint tick_set;                      // 0x20, the model set Tick releases an unused copy of, one set a tick
    bool open;                          // 0x24, which Close clears first and GameLoopSingle tests before the Tick
    uint timer;                         // 0x28, the time since Tick last looked at the blocks
    S_vector cam_pos;                   // 0x2c, the camera's position then
};

// The one object (PS2 name)
extern C_cache_base_block base_cache;

// What the file calls out of line, its vectors' templates and their helpers: the original kept this file's copies (the
// range at 0x401030, docs/game-files.md), but for the model sets' operator=, another file's (folded). The model cache's
// items' _Ucopy and fill (0x403ee0, 0x4047c0) come from OpenModel's inlined Open; the linker folded the three caches'.
// SYNTHETIC: GAME 0x401c70
// ??1S_cache_base_block@@QAE@XZ

// TEMPLATE: GAME 0x403680
// ??1?$vector@US_cache_base_block@@V?$allocator@US_cache_base_block@@@std@@@std@@QAE@XZ

// Open's blocks.size(), which its budget leaves out of line
// TEMPLATE: GAME 0x4036c0
// ?size@?$vector@US_cache_base_block@@V?$allocator@US_cache_base_block@@@std@@@std@@QBEIXZ

// Tick's pop_back
// TEMPLATE: GAME 0x4038e0
// ?erase@?$vector@US_cache_model_one@@V?$allocator@US_cache_model_one@@@std@@@std@@QAEPAUS_cache_model_one@@PAU3@@Z

// TEMPLATE: GAME 0x403920
// ?insert@?$vector@US_cache_base_block@@V?$allocator@US_cache_base_block@@@std@@@std@@QAEPAUS_cache_base_block@@PAU3@ABU3@@Z

// TEMPLATE: GAME 0x403b20
// ?_Destroy@?$vector@US_cache_base_block@@V?$allocator@US_cache_base_block@@@std@@@std@@IAEXPAUS_cache_base_block@@0@Z

// TEMPLATE: GAME 0x403b50
// ?insert@?$vector@US_cache_model_set@@V?$allocator@US_cache_model_set@@@std@@@std@@QAEPAUS_cache_model_set@@PAU3@ABU3@@Z

// TEMPLATE: GAME 0x403d50
// ?_Destroy@?$vector@US_RoadBlock@C_game@@V?$allocator@US_RoadBlock@C_game@@@std@@@std@@IAEXPAUS_RoadBlock@C_game@@0@Z

// TEMPLATE: GAME 0x403dc0
// ?_Ucopy@?$vector@US_cache_model_inbb@@V?$allocator@US_cache_model_inbb@@@std@@@std@@IAEPAUS_cache_model_inbb@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x403e00
// ?_Ucopy@?$vector@US_cache_base_block@@V?$allocator@US_cache_base_block@@@std@@@std@@IAEPAUS_cache_base_block@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x403e40
// ?_Ufill@?$vector@US_cache_base_block@@V?$allocator@US_cache_base_block@@@std@@@std@@IAEXPAUS_cache_base_block@@IABU3@@Z

// TEMPLATE: GAME 0x403e70
// ?_Ucopy@?$vector@US_cache_model_set@@V?$allocator@US_cache_model_set@@@std@@@std@@IAEPAUS_cache_model_set@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x403eb0
// ?_Ufill@?$vector@US_cache_model_set@@V?$allocator@US_cache_model_set@@@std@@@std@@IAEXPAUS_cache_model_set@@IABU3@@Z

// TEMPLATE: GAME 0x403ee0
// ?_Ucopy@?$vector@US_itm@?$C_I3D_cache@VI3D_animation_set@@@@V?$allocator@US_itm@?$C_I3D_cache@VI3D_animation_set@@@@@std@@@std@@IAEPAUS_itm@?$C_I3D_cache@VI3D_animation_set@@@@PBU34@0PAU34@@Z

// TEMPLATE: GAME 0x403f20
// ?copy@std@@YIPAUS_cache_base_block@@PAU2@00@Z

// TEMPLATE: GAME 0x4040d0
// ?fill@std@@YIXPAUS_cache_base_block@@0ABU2@@Z

// TEMPLATE: GAME 0x404290
// ?copy_backward@std@@YIPAUS_cache_base_block@@PAU2@00@Z

// TEMPLATE: GAME 0x404440
// ?fill@std@@YIXPAUS_cache_model_set@@0ABU2@@Z

// TEMPLATE: GAME 0x4045f0
// ?copy_backward@std@@YIPAUS_cache_model_set@@PAU2@00@Z

// TEMPLATE: GAME 0x4047c0
// ?fill@std@@YIXPAUS_itm@?$C_I3D_cache@VI3D_animation_set@@@@0ABU23@@Z

// TEMPLATE: GAME 0x404810
// ?_Construct@std@@YIXPAUS_cache_model_inbb@@ABU2@@Z

// TEMPLATE: GAME 0x404840
// ?_Construct@std@@YIXPAUS_cache_base_block@@ABU2@@Z

// TEMPLATE: GAME 0x4049a0
// ?_Construct@std@@YIXPAUS_cache_model_set@@ABU2@@Z

// The models' assignment, which the blocks' copy calls
// TEMPLATE: GAME 0x404a50
// ??4?$vector@US_cache_model_inbb@@V?$allocator@US_cache_model_inbb@@@std@@@std@@QAEAAV01@ABV01@@Z

// The visuals' capacity, which the blocks' copy calls out of line (another file's copy, folded)
// TEMPLATE: GAME 0x5384e0
// std::vector<I3D_frame *,std::allocator<I3D_frame *> >::capacity

// TEMPLATE: GAME 0x5380e0
// ??4?$vector@US_cache_model_one@@V?$allocator@US_cache_model_one@@@std@@@std@@QAEAAV01@ABV01@@Z

// The S_vector2 arrays' constructor (ls3df/I3D/I3d_math.h), empty, which the blocks' implicit constructor passes to the
// vector constructor iterator
// LIBRARY: GAME 0x401c60
// ??0S_vector2@@QAE@XZ
