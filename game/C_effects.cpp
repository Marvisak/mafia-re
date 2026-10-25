// The effects of tables\effects.tbl, PC only: the file at 0x448f00-0x44b4cc (docs/game-files.md), which starts with
// C_effects' constructor; the name is ours. Written so far: the constructor, the destructor and Clear, which
// C_mission's Init and UnInit run; Load is a stub.

#include "C_effects.h"

#include "stub.h"

// FUNCTION: GAME 0x449080
C_effects::C_effects()
{
    Load();
    unknown_30 = -1;
}

// FUNCTION: GAME 0x449100
C_effects::~C_effects()
{
    Clear();
}

#pragma auto_inline(off)

// The original reads the table's chunks: an effect (0x3c bytes) for each, with its parts
// STUB: GAME 0x449380
void C_effects::Load()
{
    StubLog("C_effects::Load");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x44a350
void C_effects::Clear()
{
    for (int i = 0; i < effects.size(); i++)
        delete effects[i];
    effects.clear();
}
