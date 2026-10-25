// The walking paths' web (PS2 pathweb.cpp; the PC's file is the one at 0x549f90-0x54d31c, which starts with the node's
// destructor, docs/game-files.md). Written so far: the node's and the path's destructors, which C_mission::UnInit
// runs, and the three empty virtual functions; the other virtual functions, which the path's vtable needs, are stubs.
// PS2's file has more (the pathfinding's help memory, AllocHelpMem and FreeHelpMem); the PC's functions are PS2's in
// PS2's order, as far as they are there.

#include "pathweb.h"

#include "stub.h"

// FUNCTION: GAME 0x549f90
C_web_node::~C_web_node()
{
    delete[] conns;
}

// PS2's calls ClearNode, which frees what the path's help memory holds; the PC's has none
// FUNCTION: GAME 0x54a1e0
C_web_path::~C_web_path()
{
    delete[] nodes;
    delete[] unknown_14;
}

#pragma auto_inline(off)

// STUB: GAME 0x54a3b0
void C_web_path::AddNode(float x, float y, float z, ushort)
{
    StubLog("C_web_path::AddNode");
}

// STUB: GAME 0x54a410
void C_web_path::DeleteNode(uint)
{
    StubLog("C_web_path::DeleteNode");
}

#pragma auto_inline(on)

// Empty, as PS2's: the vtable's entry is the linker's folded empty function
// FUNCTION: GAME 0x4aabe0 FOLDED
void C_web_path::DrawConnections()
{
}

#pragma auto_inline(off)

// STUB: GAME 0x54a550
void C_web_path::CreateConnect(uint, uint)
{
    StubLog("C_web_path::CreateConnect");
}

// STUB: GAME 0x54a790
void C_web_path::ControlNodeConn(uint)
{
    StubLog("C_web_path::ControlNodeConn");
}

#pragma auto_inline(on)

// Returns 0, as PS2's: the vtable's entry is a folded copy
// FUNCTION: GAME 0x5e3fd0 FOLDED
int C_web_path::SelectLine(int, int)
{
    return 0;
}

#pragma auto_inline(off)

// STUB: GAME 0x54ab40
void C_web_path::SaveNodes(C_chunk *)
{
    StubLog("C_web_path::SaveNodes");
}

// STUB: GAME 0x54ade0
void C_web_path::MergeNodes(C_chunk *)
{
    StubLog("C_web_path::MergeNodes");
}

// STUB: GAME 0x54af10
void C_web_path::ReleaseSelect()
{
    StubLog("C_web_path::ReleaseSelect");
}

// STUB: GAME 0x54c5a0
void C_web_path::FindSelectedLine(uint &, uint &)
{
    StubLog("C_web_path::FindSelectedLine");
}

// STUB: GAME 0x54c620
void C_web_path::IncludeNode(uint, uint)
{
    StubLog("C_web_path::IncludeNode");
}

// STUB: GAME 0x54c8d0
void C_web_path::FlipOneWay(uint, uint, bool)
{
    StubLog("C_web_path::FlipOneWay");
}

// STUB: GAME 0x54cba0
void C_web_path::SortNodesByX()
{
    StubLog("C_web_path::SortNodesByX");
}

#pragma auto_inline(on)

// Empty, as PS2's: the vtable's entry is a folded copy
// FUNCTION: GAME 0x46ca70 FOLDED
void C_web_path::DebugFindPath(ushort, ushort)
{
}
