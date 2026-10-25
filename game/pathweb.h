#pragma once

#include "Main.h"

class C_chunk;

// The walking paths' web (PS2 C_web_path and C_web_node, pathweb.cpp). Only what C_mission's Init and UnInit use so
// far. The classes are packed, as PS2's are (its C_web_path is 0x31 bytes, with its vtable at 0x2d): the PC's node is
// 0x1e bytes and the path 0x1d, the sizes its new[] and new take.

#pragma pack(push, 1)

// A node's connection to another (8 bytes, the size the node's Resize allocates them by; the names are ours). Its
// destructor is declared and empty: the node's delete[] destroys them through the empty function 0x404990.
struct S_web_conn
{
    ushort node;                        // 0x0, the other node
    ushort unknown_2;                   // 0x2
    float distance;                     // 0x4

    // Resize constructs them through 0x54d2b0
    S_web_conn();
    ~S_web_conn() {}
};

class C_web_node
{
public:
    uchar unknown_0[0x18];              // 0x0
    S_web_conn *conns;                  // 0x18
    uchar max_conns;                    // 0x1c
    uchar num_conns;                    // 0x1d

    ~C_web_node();
};

// VTABLE: GAME 0x6251f8
class C_web_path
{
public:
    C_web_node *nodes;                  // 0x4
    int unknown_8;                      // 0x8
    int unknown_c;                      // 0xc
    uint unknown_10;                    // 0x10, the constructor's (50 for the mission's, PS2's too)
    uchar *unknown_14;                  // 0x14
    int unknown_18;                     // 0x18
    bool unknown_1c;                    // 0x1c

    // Inline: C_mission::Init has it inlined. VC6 stores the vtable after the initializers and before the body, and the
    // original's comes after the last four members (PS2's, which stores its vtable first, has the same split).
    C_web_path(uint unknown) :
        unknown_10(unknown),
        unknown_14(NULL),
        unknown_18(0),
        unknown_1c(false)
    {
        nodes = NULL;
        unknown_8 = 0;
        unknown_c = 0;
    }
    // Not virtual: UnInit calls it directly
    ~C_web_path();

    // PS2's virtual functions in PS2's order, but for RecompileDistance, which isn't virtual on the PC (its vtable has
    // 14 entries, the parameters each one's ret takes)
    virtual void AddNode(float x, float y, float z, ushort);
    virtual void DeleteNode(uint);
    virtual void IncludeNode(uint, uint);
    virtual void DrawConnections();
    virtual void DebugFindPath(ushort, ushort);
    virtual void CreateConnect(uint, uint);
    virtual void ControlNodeConn(uint);
    virtual int SelectLine(int, int);
    virtual void ReleaseSelect();
    virtual void FindSelectedLine(uint &, uint &);
    virtual void SortNodesByX();
    virtual void SaveNodes(C_chunk *);
    virtual void MergeNodes(C_chunk *);
    virtual void FlipOneWay(uint, uint, bool);
};

#pragma pack(pop)
