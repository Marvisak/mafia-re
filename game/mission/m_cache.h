#pragma once

#include "Main.h"

class C_chunk;
class I3D_scene;

// The city's base blocks (PS2 C_cache_base_block, m_cache.cpp), which the mission opens with its scene and
// GameLoopSingle ticks. Only what C_mission's Open and Close call so far: the PC's has a vector at 0x0, a vector of
// 0x14-byte elements at 0x10 and a flag at 0x24 that Close clears first and GameLoopSingle tests before the Tick.
class C_cache_base_block
{
public:
    void Close();
    // Reads the mission's cache.bin into the scene's blocks; closes first
    void Open(C_chunk *chunk, I3D_scene *scene);
};

// The one object (PS2 name)
extern C_cache_base_block base_cache;
