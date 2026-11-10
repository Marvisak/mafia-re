#pragma once

#include "Main.h"

#include "I3D/I3d_math.h"

class C_chunk;

// The walking paths' web (PS2 C_web_path and C_web_node, pathweb.cpp). Only what C_mission's Init and UnInit use so
// far. The classes are packed, as PS2's are (its C_web_path is 0x31 bytes, with its vtable at 0x2d): the PC's node is
// 0x1e bytes and the path 0x1d, the sizes its new[] and new take.

#pragma pack(push, 1)

// The flags of the nodes and their connections (C_web_node::flags, S_web_conn::flags) that DeRiciConns clears (the
// names are ours, by value)
#define WEBF_UNKNOWN_1000 0x1000
#define WEBF_UNKNOWN_4000 0x4000
// The nodes of the seagulls' rings: C_seagull::GameInit asks GetNearNode for one (the name is ours, by value)
#define WEBF_UNKNOWN_8 0x8

// A node's connection to another (8 bytes, the size the node's Resize allocates them by; the names are ours). Its
// destructor is declared and empty: the node's delete[] destroys them through the empty function 0x404990.
struct S_web_conn
{
    ushort node;                        // 0x0, the other node
    ushort flags;                       // 0x2, WEBF_*
    float distance;                     // 0x4

    // Resize constructs them through 0x54d2b0
    S_web_conn();
    ~S_web_conn() {}
};

class C_web_node
{
public:
    S_vector pos;                       // 0x0 (PS2 GetPos's), the radar's edges are drawn through it
    // The names are ours
    ushort flags;                       // 0xc, WEBF_*, PS2 C_web_path::AddNode's
    ushort id;                          // 0xe (PS2 0xe), what PS2's C_web_path::FindNodeID finds, C_wagon::GetLastNode's
    // 0x10, the width around the node in centimetres (ai_loc::Update tests the connections with it; the name is a guess):
    // both builds read it as the low 16 bits of a dword, converted as unsigned
    uint width : 16;
    uchar unknown_14[4];                // 0x14
    S_web_conn *conns;                  // 0x18
    uchar max_conns;                    // 0x1c
    uchar num_conns;                    // 0x1d

    ~C_web_node();

    // PS2 name (PS2's is out of line, returning the vector by value)
    S_vector GetPos() const
    {
        return S_vector(pos.x, pos.y, pos.z);
    }
    // The connection, NULL past the last (the name is ours; PS2's ai_loc::Update has the same test inline)
    S_web_conn *GetConn(ushort i) const
    {
        return i < num_conns ? &conns[i] : NULL;
    }
};

// VTABLE: GAME 0x6251f8
class C_web_path
{
public:
    // The names are ours
    C_web_node *nodes;                  // 0x4
    int max_nodes;                      // 0x8 (PS2 0x4), the array's size, which PS2's Resize grows
    int num_nodes;                      // 0xc, the nodes' count, by PS2's ClearNode (PS2 0x8)
    uint unknown_10;                    // 0x10, the constructor's (50 for the mission's, PS2's too)
    // 0x14 (PS2 0x24), PS2's FindPath's work array, an 8-byte C_web_path::S_pathfind for each node (PS2's new[] for
    // num_nodes of them where num_pathfind differs)
    uchar *pathfind;
    int num_pathfind;                   // 0x18 (PS2 0x28)
    bool finding_by_roads;              // 0x1c (PS2 0x2c), set by PS2's FindPath while its FindPath_FastByRoads runs

    // Inline: C_mission::Init has it inlined. VC6 stores the vtable after the initializers and before the body, and the
    // original's comes after the last four members (PS2's, which stores its vtable first, has the same split).
    C_web_path(uint unknown) :
        unknown_10(unknown),
        pathfind(NULL),
        num_pathfind(0),
        finding_by_roads(false)
    {
        nodes = NULL;
        max_nodes = 0;
        num_nodes = 0;
    }
    // Not virtual: UnInit calls it directly
    ~C_web_path();

    // PS2 name. Inline: C_mission::Close has it inlined. PS2's also clears each node's connections and frees the nodes
    // unless its help memory holds them; the PC's keeps the nodes and frees only the array PS2 frees with them.
    void ClearNode()
    {
        num_nodes = 0;
        delete[] pathfind;
        pathfind = NULL;
        num_pathfind = 0;
        finding_by_roads = false;
    }

    // Reads the nodes of the mission's check.bin (PS2 name), which C_mission::Open calls directly, as PS2's
    void LoadNodes(C_chunk *chunk);
    // PS2 name (Czech, roughly "of the connections"), which C_game::Init calls
    void DeRiciConns();
    // The node nearest pos with one of the flags (and without WEBF_UNKNOWN_4000), within radius where it is above 0;
    // -1 for none (PS2 name; the parameters' names are ours), which C_seagull::GameInit calls
    int GetNearNode(const S_vector &pos, float radius, ushort flags);

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
