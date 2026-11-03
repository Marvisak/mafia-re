// The cars' road network (PS2 road_web.cpp; the PC's file is the one at 0x559cd0-0x55cf3c, which starts with the
// constructor, docs/game-files.md). Written so far: the constructor and the destructor, which C_mission's Init and
// UnInit call; the virtual functions, which the vtable needs, and LoadData, which C_mission::Open calls, are stubs.

#include "ai/road_web.h"

#include "stub.h"

// FUNCTION: GAME 0x559cd0
C_roads::C_roads()
{
}

// FUNCTION: GAME 0x559d00
C_roads::~C_roads()
{
}

#pragma auto_inline(off)

// STUB: GAME 0x559d40
bool C_roads::SaveData(C_chunk *chunk)
{
    StubLog("C_roads::SaveData");
    return true;
}

// STUB: GAME 0x559e60
bool C_roads::LoadData(C_chunk *chunk)
{
    StubLog("C_roads::LoadData");
    return true;
}

// STUB: GAME 0x55a210
void C_roads::MergeData(C_chunk *chunk)
{
    StubLog("C_roads::MergeData");
}

// STUB: GAME 0x55a620
void C_roads::RecompileDistances()
{
    StubLog("C_roads::RecompileDistances");
}

// STUB: GAME 0x55b370
ushort C_roads::GetNearNode(S_vector pos)
{
    StubLog("C_roads::GetNearNode");
    return 0xffff;
}

// STUB: GAME 0x55c4e0
bool C_roads::FindPath(ushort from, ushort to, std::vector<ushort> &path)
{
    StubLog("C_roads::FindPath");
    return false;
}

// STUB: GAME 0x55c890
void C_roads::SortNodesByX()
{
    StubLog("C_roads::SortNodesByX");
}

#pragma auto_inline(on)
