// The city's base blocks (PS2 m_cache.cpp, the name its helper structs carry; the PC's file is most likely the one at
// 0x401030-0x404cdc, docs/game-files.md, the first after the empty one): base_cache's initializer, then PS2's
// functions in PS2's order, the constructor (0x401070), the destructor (0x4010a0), Close (0x4011f0), PrepareScene
// (0x4012e0), cbDupl (0x401307) and Open (0x4013f0, which closes first), and later the Tick GameLoopSingle calls
// (0x4020b0). Written so far: the constructor, the destructor and Close; Open, which C_mission::Open calls, is a stub.

#include "mission/m_cache.h"

#include "I3D/I3D.h"
#include "stub.h"

// GLOBAL: GAME 0x63d268
C_cache_base_block base_cache;

// FUNCTION: GAME 0x401070
C_cache_base_block::C_cache_base_block()
{
    open = false;
    unknown_28 = 0;
}

// FUNCTION: GAME 0x4010a0
C_cache_base_block::~C_cache_base_block()
{
    Close();
}

// PS2's without the model sets' own model, which PS2 releases in place of their frames, and the list of frames after
// them (0x34), which the PC doesn't have
// FUNCTION: GAME 0x4011f0
void C_cache_base_block::Close()
{
    open = false;
    blocks.clear();
    for (int i = model_sets.size(); i--;)
    {
        delete[] model_sets[i].name;
        for (int j = model_sets[i].unknown_4.size(); j--;)
        {
            if (model_sets[i].unknown_4[j].frame)
            {
                model_sets[i].unknown_4[j].frame->LinkTo(NULL, 0);
                model_sets[i].unknown_4[j].frame->Release();
            }
        }
    }
    model_sets.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x4013f0
void C_cache_base_block::Open(C_chunk *chunk, I3D_scene *scene)
{
    StubLog("C_cache_base_block::Open");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x4020b0
void C_cache_base_block::Tick(int time)
{
    StubLog("C_cache_base_block::Tick");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x402dc0
void C_cache_base_block::VisibleAllFrames()
{
    StubLog("C_cache_base_block::VisibleAllFrames");
}

#pragma auto_inline(on)


