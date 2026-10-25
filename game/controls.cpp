// The player's controls (PC only, the name is ours; the file at 0x4efb40-0x4f1b4c, which starts with the class's
// constructor, docs/game-files.md). Not written yet: only what InitSystem needs to link.

#include "controls.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4efde0
bool C_controls::Init(IGraph *igraph, IJoy **joys, int count)
{
    StubLog("C_controls::Init");
    return false;
}

#pragma auto_inline(on)
