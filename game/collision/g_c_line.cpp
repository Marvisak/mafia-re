// The collisions' line tests, the file at 0x5c5040-0x5c9e6c (docs/game-files.md): most likely PS2's g_c_line.cpp, the
// second of PS2's collision files, which has the line tests (LineIn*, _TestLineV, TestLineHStatic, TestLineHDynamic,
// _TestLineH, TestLineInVehicle). Not written yet: what written code calls is stubs.

#include "collision/g_colls.h"

#include "stub.h"

// STUB: GAME 0x5c6b10
int g_collision::_TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    StubLog("g_collision::_TestLineV");
    return 0;
}

// STUB: GAME 0x5c6ff0
int g_collision::_TestLineVInnerDyn(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    StubLog("g_collision::_TestLineVInnerDyn");
    return 0;
}

// STUB: GAME 0x5c74d0
int g_collision::TestLineHStatic(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
    uint flags)
{
    StubLog("g_collision::TestLineHStatic");
    return 0;
}

// STUB: GAME 0x5c8440
tDynamicCollObject *g_collision::TestLineHDynamic(const S_vector &from, const S_vector &dir, S_vector *pos,
    S_vector *normal, uint flags)
{
    StubLog("g_collision::TestLineHDynamic");
    return NULL;
}

// STUB: GAME 0x5c8770
int g_collision::_TestLineH(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    StubLog("g_collision::_TestLineH");
    return 0;
}

// STUB: GAME 0x5c9b80
int g_collision::TestLineInVehicle(std::vector<tDynamicCollObject *> *colls, const S_vector &from, const S_vector &dir,
    S_vector *pos, S_vector *normal, uint flags)
{
    StubLog("g_collision::TestLineInVehicle");
    return 0;
}
