// The collisions, the file at 0x5c2840-0x5c503c (docs/game-files.md); the name is a guess, PS2's g_colls.cpp. Written
// so far: GameDone, TestLineV and TestSphereB (in the file at 0x5d5900 on the PC); the rest of what main, the game and
// the script's commands call is stubs.

#include "collision/g_colls.h"

#include "stub.h"

// GLOBAL: GAME 0x647f48
g_collision gifCollision;

#pragma auto_inline(off)

// STUB: GAME 0x5c28d0
void g_collision::DeleteCollsLevel()
{
    StubLog("g_collision::DeleteCollsLevel");
}

// STUB: GAME 0x5c2b70
bool g_collision::LoadCollision(const char *name, bool)
{
    StubLog("g_collision::LoadCollision %s", name);
    return false;
}

// STUB: GAME 0x5c38b0
void g_collision::DynErase(tDynamicCollObject *obj)
{
    StubLog("g_collision::DynErase");
}

// STUB: GAME 0x5c3da0
void g_collision::DynWipeOut()
{
    StubLog("g_collision::DynWipeOut");
}

// The PC's is in the file at 0x5c5040 (docs/game-files.md)
// STUB: GAME 0x5c6b10
int g_collision::_TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    StubLog("g_collision::_TestLineV");
    return 0;
}

// The PC's is in the file at 0x5c9e70 (docs/game-files.md)
// STUB: GAME 0x5ca5c0
uint g_collision::FindCollsByFrame(I3D_frame *frm, std::vector<g_collision_header *> &colls)
{
    StubLog("g_collision::FindCollsByFrame");
    return 0;
}

// PS2's is another file's than TestSphereB; the PC's is in the file at 0x5cafb0 (docs/game-files.md)
// STUB: GAME 0x5d39a0
bool g_collision::_TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos,
    S_vector *normal, uint flags, uint flags2)
{
    StubLog("g_collision::_TestSphereB");
    return false;
}

#pragma auto_inline(on)

// The moving objects' collisions, then the level's; PS2's passes its bool on to DeleteCollsLevel
// FUNCTION: GAME 0x5d5900
void g_collision::GameDone()
{
    DynWipeOut();
    DeleteCollsLevel();
}

// Loads gifCollision whatever it is called on, as TestSphereB; PS2 has the wrappers in the PC's order (TestLineV,
// TestLineVInnerDyn, TestLineH, TestSphereB)
// FUNCTION: GAME 0x5d5910
int g_collision::TestLineV(const S_vector &from, const S_vector &dir, S_vector *pos, S_vector *normal, uint flags,
    uint flags2)
{
    return gifCollision._TestLineV(from, dir, pos, normal, flags, flags2);
}

// PS2's g_colls.cpp has it after g_camera.cpp's functions (12 bytes, a jump with this); the PC's is in the file at
// 0x5d5900 and loads gifCollision whatever it is called on
// FUNCTION: GAME 0x5d5940
bool g_collision::TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    return gifCollision._TestSphereB(from, dir, radius, pos, normal, flags, flags2);
}
