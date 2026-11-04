// The file at 0x5c9e70-0x5cafac (docs/game-files.md), between the line tests and the sphere tests: by its place most
// likely PS2's g_c_linebs.cpp, the third of PS2's collision files (PS2 has FindCollsByFrame right before _TestLineS and
// the LineInS* tests, so which of the two files it is in there isn't certain). Not written yet: what written code calls
// is stubs.

#include "collision/g_colls.h"

#include "stub.h"

// STUB: GAME 0x5ca5c0
uint g_collision::FindCollsByFrame(I3D_frame *frm, std::vector<g_collision_header *> &colls)
{
    StubLog("g_collision::FindCollsByFrame");
    return 0;
}
