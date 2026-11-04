// The collisions' sphere tests, the file at 0x5cafb0-0x5d58fc (docs/game-files.md): most likely PS2's g_c_sph.cpp, the
// fourth of PS2's collision files, which has the sphere tests (SphereIn*, _TestSphereB, TestSphereInVehicle). Not
// written yet: what written code calls is stubs.

#include "collision/g_colls.h"

#include "stub.h"

// STUB: GAME 0x5d39a0
int g_collision::_TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos,
    S_vector *normal, uint flags, uint flags2)
{
    StubLog("g_collision::_TestSphereB");
    return false;
}
