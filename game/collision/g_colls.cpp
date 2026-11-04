// The collisions' wrappers, the file at 0x5d5900-0x5d6e4c (docs/game-files.md): PS2's g_colls.cpp, the last of PS2's
// collision files, which has GameDone, the wrappers of the tests (TestLineV, TestLineVInnerDyn, TestLineH, TestLineS,
// TestSphereB, TestDBox) and TestPanak2 and TestPanak2InVehicle, in the PC's order but for TestLineS: the PC has none
// before TestSphereB, and a function of its own after it (0x5d5960, a sphere test of the file at 0x5cafb0 on
// gifCollision). Written so far: GameDone and the wrappers; TestPanak2 and TestPanak2InVehicle, which C_human.cpp calls,
// are stubs.

#include "collision/g_colls.h"

#include "stub.h"

// The moving objects' collisions, then the level's; PS2's passes its bool on to DeleteCollsLevel
// FUNCTION: GAME 0x5d5900
void g_collision::GameDone()
{
    DynWipeOut();
    DeleteCollsLevel();
}

// Loads gifCollision whatever it is called on, as TestLineVInnerDyn and TestSphereB
// FUNCTION: GAME 0x5d5910
int g_collision::TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    return gifCollision._TestLineV(from, dir, pos, normal, flags, flags2);
}

// FUNCTION: GAME 0x5d5920
int g_collision::TestLineVInnerDyn(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    return gifCollision._TestLineVInnerDyn(from, dir, pos, normal, flags, flags2);
}

// A jump on this (PS2 name), where TestLineV and TestSphereB load gifCollision
// FUNCTION: GAME 0x5d5930
int g_collision::TestLineH(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    return _TestLineH(from, dir, pos, normal, flags, flags2);
}

// PS2's is a jump with this; the PC's loads gifCollision whatever it is called on
// FUNCTION: GAME 0x5d5940
int g_collision::TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    return gifCollision._TestSphereB(from, dir, radius, pos, normal, flags, flags2);
}

// A jump on this, as TestLineH (PS2 name), which only a vehicle's code calls (0x4e9088)
// FUNCTION: GAME 0x5d59b0
bool g_collision::TestDBox(const I3D_bbox &bbox, const S_matrix &m0, const S_matrix &m1, const S_vector &dir,
    const S_vector &v1, const S_vector &v2, uint flags, uint flags2)
{
    return _TestDBox(bbox, m0, m1, dir, v1, v2, flags, flags2);
}

// STUB: GAME 0x5d59c0
int g_collision::TestPanak2(tgcPanak2 *p)
{
    StubLog("g_collision::TestPanak2");
    return 0;
}

// STUB: GAME 0x5d6430
int g_collision::TestPanak2InVehicle(std::vector<tDynamicCollObject *> *colls, tgcPanak2 *p)
{
    StubLog("g_collision::TestPanak2InVehicle");
    return 0;
}
