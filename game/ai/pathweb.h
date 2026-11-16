#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_chunk;

// The walking paths' web (PS2 C_web_path, C_web_node and C_web_connection, pathweb.cpp). The classes are packed, as
// PS2's are (its C_web_path is 0x31 bytes, with its vtable at 0x2d): the PC's node is 0x1e bytes, its connection 8 and
// its path 0x1d, the sizes its new[] and new take. PS2's path also has a help memory the pathfinding allocates from
// (AllocHelpMem, FreeHelpMem, GetNextHlpMemBlock), which the PC's hasn't.

#pragma pack(push, 1)

// The magic of the nodes' file (the name is ours)
#define WEB_NODES_MAGIC 0x1abcedf

// The flags of the nodes and their connections (C_web_node::flags, C_web_connection::flags)
// The connection is selected in the editor: ReleaseSelect clears it, FindSelectedLine looks for it (the name is ours)
#define WEBF_SELECTED 0x40
// The connection is the closed way of a one-way pair: FlipOneWay moves it to the connection back, the paths don't
// take it (the name is ours)
#define WEBF_ONEWAY 0x80
// Cleared by DeRiciConns (the names are ours, by value); the paths don't take a connection with WEBF_UNKNOWN_4000
#define WEBF_UNKNOWN_1000 0x1000
#define WEBF_UNKNOWN_4000 0x4000
// The nodes of the seagulls' rings: C_seagull::GameInit asks GetNearNode for one (the name is ours, by value)
#define WEBF_UNKNOWN_8 0x8
// The nodes' flags LegalConnect tests (8 and 4 make a connection's flags 4; the names are ours, by value)
#define WEBF_UNKNOWN_1 0x1
#define WEBF_UNKNOWN_2 0x2
#define WEBF_UNKNOWN_4 0x4
#define WEBF_UNKNOWN_10 0x10

// A node's connection to another (PS2 name), 8 bytes, the size the node's Resize allocates them by. Its destructor is
// declared and empty: the node's delete[] destroys them through the empty function 0x404990.
class C_web_connection
{
public:
    // The names are ours
    ushort node;                        // 0x0, the other node
    ushort flags;                       // 0x2, WEBF_*
    float distance;                     // 0x4, AddConn's

    // The node's Resize constructs them through the copy at the file's end
    // FUNCTION: GAME 0x54d2b0
    C_web_connection()
    {
        node = 0;
        flags = 0;
    }
    ~C_web_connection() {}
};

class C_web_node
{
public:
    S_vector pos;                       // 0x0 (PS2 GetPos's), the radar's edges are drawn through it
    // The names are ours
    ushort flags;                       // 0xc, WEBF_*, PS2 C_web_path::AddNode's
    ushort id;                          // 0xe (PS2 0xe), what FindNodeID finds, C_wagon::GetLastNode's
    // 0x10, the width around the node in centimetres in the low 16 bits (ai_loc::Update tests the connections with it;
    // the name is a guess): a dword, which the constructor sets and the path's Resize copies whole, read by both builds
    // as its low 16 bits, converted as unsigned
    uint width;
    uint unknown_14;                    // 0x14
    C_web_connection *conns;            // 0x18
    uchar max_conns;                    // 0x1c
    uchar num_conns;                    // 0x1d

    // The path's Resize constructs them through the copy at the file's end
    // FUNCTION: GAME 0x54d2c0
    C_web_node()
    {
        pos.x = 0.0f;
        pos.y = 0.0f;
        pos.z = 0.0f;
        flags = 0;
        id = 0;
        width = 50;
        unknown_14 = 0;
        conns = NULL;
        max_conns = 0;
        num_conns = 0;
    }
    ~C_web_node();

    // PS2 names. PS2's Resize also takes the path, whose help memory it allocates from.
    void Resize(ushort num);
    void AddConn(ushort node, S_vector from, S_vector to, ushort flags);
    void DeleteConn(ushort i);
    ushort FindConnIdx(ushort node);

    // PS2 name (PS2's is out of line, returning the vector by value)
    S_vector GetPos() const
    {
        return S_vector(pos.x, pos.y, pos.z);
    }
    // The connection, NULL past the last (the name is ours; PS2's ai_loc::Update has the same test inline)
    C_web_connection *GetConn(ushort i) const
    {
        return i < num_conns ? &conns[i] : NULL;
    }
};

// The pathfinding's callbacks (the names are ours): the cost of the connection from one node to another, and the
// test of a node as the path's end
typedef float (*t_web_cost_func)(ushort from, ushort to, uchar conn);
typedef int (*t_web_end_func)(ushort node);

// VTABLE: GAME 0x6251f8
class C_web_path
{
public:
    // PS2's FindPath's work array, one for each node (PS2 name; 8 bytes on PS2): the node the path came from and its
    // distance (the names are ours)
    struct S_pathfind
    {
        ushort from;                    // 0x0
        float distance;                 // 0x2

        S_pathfind()
        {
            from = 0xffff;
            distance = 1e10f;
        }
    };

    // The names are ours
    C_web_node *nodes;                  // 0x4
    uint max_nodes;                     // 0x8 (PS2 0x4), the array's size, which Resize grows
    uint num_nodes;                     // 0xc, the nodes' count, by PS2's ClearNode (PS2 0x8)
    uint grow_size;                     // 0x10 (PS2 0xc), what Resize grows the array by (50 for the mission's)
    S_pathfind *pathfind;               // 0x14 (PS2 0x24), num_pathfind of them
    uint num_pathfind;                  // 0x18 (PS2 0x28)
    bool finding_by_roads;              // 0x1c (PS2 0x2c), set by FindPath while its FindPath_FastByRoads runs

    // Inline: C_mission::Init has it inlined. VC6 stores the vtable after the initializers and before the body, and the
    // original's comes after the last four members (PS2's, which stores its vtable first, has the same split).
    C_web_path(uint grow) :
        grow_size(grow),
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

    // PS2 names, in the PC's order
    void Resize(uint num);
    bool LegalConnect(uint node1, uint node2, ushort &conn);
    // Reads the nodes of the mission's check.bin, which C_mission::Open calls directly, as PS2's
    void LoadNodes(C_chunk *chunk);
    // The index of the node with the id, -1 for none
    int FindNodeID(ushort id);
    bool FindPath_FastByRoads(uint from, uint to, std::vector<uint> *path, ushort flags);
    bool FindPath(uint from, uint to, std::vector<uint> *path, ushort flags);
    bool FindPathCompFunc(uint from, uint to, std::vector<uint> *path, t_web_cost_func cost, ushort flags);
    bool FindPathCompFunc(uint from, t_web_end_func end, std::vector<uint> *path, t_web_cost_func cost,
        ushort flags);
    // The node nearest pos with one of the flags (and without WEBF_UNKNOWN_4000), -1 for none; where radius is above
    // 0, nodes closer than it are skipped (a minimum, not a reach). A binary search: the nodes are sorted by x
    // (SortNodesByX). The parameters' names are ours.
    int GetNearNode(const S_vector &pos, float radius, ushort flags);
    bool IsSorted();
    // PS2 name (Czech, roughly "of the connections"), which C_game::Init calls
    void DeRiciConns();
    bool FindPathCompFuncJIPO(uint from, t_web_end_func end, std::vector<uint> *path, t_web_cost_func cost,
        ushort flags);

    // PS2's virtual functions in PS2's order, but for SaveNodes, which the PC hasn't (its vtable has 14 entries, PS2's
    // 15; the parameters each one's ret takes)
    virtual void AddNode(float x, float y, float z, ushort flags);
    virtual void DeleteNode(uint node);
    virtual void IncludeNode(uint node1, uint node2);
    virtual void DrawConnections();
    virtual void DebugFindPath(ushort, ushort);
    virtual void CreateConnect(uint node1, uint node2);
    virtual void ControlNodeConn(uint node);
    virtual int SelectLine(int, int);
    virtual void ReleaseSelect();
    virtual void FindSelectedLine(uint &node1, uint &node2);
    virtual void SortNodesByX();
    virtual void MergeNodes(C_chunk *chunk);
    virtual void RecompileDistance(uint node);
    virtual void FlipOneWay(uint node1, uint node2, bool both);
};

#pragma pack(pop)

// PS2 name: a free function of the pathfinding (__fastcall under /Gr), which FindPathCompFuncJIPO calls
void _STDVECTCOPY(std::vector<uint> *dst, std::vector<uint> *src);
