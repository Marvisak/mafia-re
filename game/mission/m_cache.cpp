// The city's base blocks (PS2 m_cache.cpp, the name its helper structs carry; the PC's file is most likely the one at
// 0x401030-0x404cdc, docs/game-files.md, the first after the empty one): base_cache's initializer, then PS2's
// functions in PS2's order, the constructor (0x401070), the destructor (0x4010a0), Close (0x4011f0), PrepareScene
// (0x4012e0), cbDupl (0x401307) and Open (0x4013f0, which closes first), and later the Tick GameLoopSingle calls
// (0x4020b0). Written so far: nothing; Close, which C_mission::Close calls, is a stub.

#include "mission/m_cache.h"

#include "stub.h"

// GLOBAL: GAME 0x63d268
C_cache_base_block base_cache;

#pragma auto_inline(off)

// STUB: GAME 0x4011f0
void C_cache_base_block::Close()
{
    StubLog("C_cache_base_block::Close");
}

#pragma auto_inline(on)
