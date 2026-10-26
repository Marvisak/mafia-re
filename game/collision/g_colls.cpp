// The collisions, the file at 0x5c2840-0x5c503c (docs/game-files.md); the name is a guess, PS2's g_colls.cpp. Not
// written yet: only what main needs to link.

#include "collision/g_colls.h"

#include "stub.h"

// GLOBAL: GAME 0x647f48
g_collision gifCollision;

#pragma auto_inline(off)

// STUB: GAME 0x5c2b70
bool g_collision::LoadCollision(const char *name, bool)
{
    StubLog("g_collision::LoadCollision %s", name);
    return false;
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

// PS2's g_colls.cpp has it after g_camera.cpp's functions (12 bytes, a jump with this); the PC's is in the file at
// 0x5d5900 and loads gifCollision whatever it is called on
// FUNCTION: GAME 0x5d5940
bool g_collision::TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos, S_vector *normal,
    uint flags, uint flags2)
{
    return gifCollision._TestSphereB(from, dir, radius, pos, normal, flags, flags2);
}
