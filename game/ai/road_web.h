#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_chunk;
struct S_car_node;

// The cars' road network (PS2 C_roads, road_web.cpp), 0x24 bytes: the crossings, and the nodes along the roads between
// them. PS2 keeps both in arrays of its own compact types (S_road_crossing, S_road_node) and converts them from and to
// the file's, which are the PC's (its S_road_crossing_1, 0xec bytes, and S_road_node_1, 0x18 bytes).

// The road from a crossing through one of its connections (the name is ours), 0x34 bytes: the chases' path nodes point
// at their road's (S_car_node::lanes)
struct S_road_lanes
{
    ushort cross;                       // 0x0, the crossing at its other end, 0xffff for none (the name is ours)
    uchar unknown_2[2];                 // 0x2
    float distance;                     // 0x4, the road's length to that crossing (the name is ours)
    // 0x8, the angle round the crossing from the road before to this one (RecompileDistances orders the roads by it;
    // the name is ours)
    float angle;
    bool unknown_c;                     // 0xc, which the chases' path nodes take (S_car_node::unknown_54)
    uchar unknown_d[7];                 // 0xd
    // 0x14, the lanes, from the road's middle out: a type (0 none: DestroyAllOffPaths drops a road without any; the
    // chases drive on 2 and 3, else on 1) and a width
    struct S_lane
    {
        ushort unknown_0;               // 0x0
        uchar type;                     // 0x2
        float width;                    // 0x4
    } lane[4];
};

// A crossing (PS2 S_road_crossing, the file's S_road_crossing_1), 0xec bytes, the size SaveData counts them by
struct S_road_crossing
{
    S_vector pos;                       // 0x0 (the name is ours)
    // 0xc, whether the crossing has a semaphore: C_shvestky::TestOverRed tests the red light only at such a crossing
    // (PS2's tests 0x10 of its S_road_crossing; the name is a guess)
    bool semaphore;
    uchar unknown_d[3];                 // 0xd
    // 0x10, the speed to drive at, in m/s, which the chases' path nodes take (S_car_node::speed): LoadData turns the
    // files' 16.666 (60 km/h) into 13.3 (the name is ours)
    float speed;
    // 0x14, the crossing's connections, the first index of the road through each: a node with NEARNODE_FLAG, or the
    // crossing at its other end for a road without nodes, as C_roads::GetPos takes them, unused ones none of those
    // (the name is ours)
    ushort conns[4];
    S_road_lanes lanes[4];              // 0x1c, the road through each connection (the name is ours)
};

// A node along a road (PS2 S_road_node, the file's S_road_node_1), 0x18 bytes, kept sorted by x (SortNodesByX):
// GetNearNode returns one of them
struct S_road_node
{
    S_vector pos;                       // 0x0 (the name is ours)
    float speed;                        // 0xc, as the crossing's (S_road_crossing::speed; the name is ours)
    // 0x10, its neighbours along the road as indices for C_roads::GetPos, a crossing's or with NEARNODE_FLAG a node's,
    // none at a dead end: an array, which MergeData, SortNodesByX and FindPath index (the name is ours)
    ushort conns[2];
    // 0x14, the crossings at the road's two ends, as RecompileDistances walks it (the crossing it starts from, the one
    // it reaches), of which C_sledovacka::Init takes the nearer one (the names are ours)
    ushort cross0;
    ushort cross1;
};

// Where the line through p1 along d1 meets the line through p2 along d2 (S_vector2's, the game's; the name is ours),
// false (the vector unchanged) for parallel lines. The copy the linker kept is the traffic's (0x4af150), which
// C_trf_c.cpp calls out of line; C_sledovacka::RecompilePath inlines it.
inline bool S_vector2::Intersection(const S_vector2 &p1, const S_vector2 &d1, const S_vector2 &p2, const S_vector2 &d2)
{
    float cross = d1.x * d2.y - d1.y * d2.x;
    if (IsAbsMrgZero(cross))
        return false;
    float t = ((p1.y - p2.y) * d2.x - (p1.x - p2.x) * d2.y) / cross;
    x = p1.x + d1.x * t;
    y = p1.y + d1.y * t;
    return true;
}

// An index of the network: a crossing's, or with the top bit a node's (GetNearNode returns a node's without it; the
// names are ours)
#define NEARNODE_INDEX 0x7fff
// The flag itself, which C_sledovacka::Tick sets on the nodes it passes to FindPath, and none
#define NEARNODE_FLAG 0x8000
#define NEARNODE_NONE 0xffff

// road.bin's version (the name is ours)
#define ROAD_VERSION 2

// VTABLE: GAME 0x625370
class C_roads
{
public:
    std::vector<S_road_crossing> crossings; // 0x4 (the name is ours)
    std::vector<S_road_node> nodes;     // 0x14 (the name is ours)

    C_roads();
    // Not virtual: UnInit calls it directly
    ~C_roads();

    // Empties the network (the name is ours). Inline: C_mission::Close has it inlined (PS2's too), the object kept in a
    // register across the first clear, the second clear called out of line as the budget's nested call.
    void Clear()
    {
        crossings.clear();
        nodes.clear();
    }

    // Reads the mission's road.bin (PS2 name): not virtual, as PS2's, C_mission::Open calls it directly. It ends with
    // the vtable's RecompileDistances.
    bool LoadData(C_chunk *chunk);
    // The node nearest to a position, 0xffff for none (PS2 name; PS2's takes a reference): an index of nodes without
    // NEARNODE_FLAG
    ushort GetNearNode(S_vector pos);
    // The crossing a node's road leads to, walking away from the index from (PS2 name and parameters)
    ushort FindCrossing(ushort node, ushort from);
    // Which of a crossing's roads leads to another crossing (PS2 name and parameters)
    int NPC_FindCrossIdxToCross(ushort cross, ushort to, ushort node);
    // A traffic car's path from a node to the crossing at the end its heading points to (PS2 name and parameters;
    // PS2's takes the heading by reference and a gif::vector)
    bool NPC_FindPathFromNodeToCross(ushort node, S_vector heading, std::vector<S_car_node> *path);
    // A traffic car's path from a crossing to the next one (PS2 name and parameters)
    bool NPC_FindPathToNextCross1(ushort cross, int dir, std::vector<S_car_node> *path);
    // Appends a node and the way along its road to a crossing to path (PS2 name and parameters)
    void FillPathFromNodeToCross(ushort node, ushort next, std::vector<ushort> &path, ushort start, ushort end);
    // The crossing a node's road leads to and the distance to it (PS2 name and parameters)
    ushort FindCrossingFromNode(ushort node, ushort from, float &distance);
    // The crossing nearest to a node (PS2 name and parameters)
    ushort FindNearCrossingFromNode(ushort node, int &dir, bool back);
    // The crossings from one crossing to another (PS2 name, typo included)
    bool FindPathBeweenCross(ushort from, ushort to, std::vector<ushort> &path);
    // The nodes from one crossing to another into path (PS2 name and parameters)
    bool FillPathFromCrossToCross(ushort from, ushort to, std::vector<ushort> &path, ushort from_node, ushort to_node);
    // The nodes from one node to another into path (PS2 name and parameters)
    bool FindPath(ushort from, ushort to, std::vector<ushort> &path);
    // Whether nodes is sorted by x (PS2 name)
    bool IsSorted();
    // Drops the roads that have no lanes (PS2 name)
    void DestroyAllOffPaths();
    // A crossing's position, or a node's with NEARNODE_FLAG (the name is ours; the copy the linker kept is
    // C_shvestky.cpp's). Returned from each branch: a conditional expression selects the pointer in another register.
    // FUNCTION: GAME 0x4a02a0
    S_vector GetPos(ushort index)
    {
        if (index & NEARNODE_FLAG)
            return nodes[index & NEARNODE_INDEX].pos;
        return crossings[index].pos;
    }

    // PS2's three virtual functions in PS2's order, and a second reader between the last two, PC only (the parameters
    // each one's ret takes). It reads what LoadData reads; the name is ours, after C_web_path's MergeNodes.
    virtual void SortNodesByX();
    virtual bool SaveData(C_chunk *chunk);
    virtual bool MergeData(C_chunk *chunk);
    virtual void RecompileDistances();
};

// What C_mission::Close's inlined Clear calls (Mission.cpp's copies): the crossings' erase, and the nodes' clear, with
// its erase inlined
// TEMPLATE: GAME 0x5472a0
// std::vector<S_road_crossing,std::allocator<S_road_crossing> >::erase

// TEMPLATE: GAME 0x5470f0
// std::vector<S_road_node,std::allocator<S_road_node> >::clear

// road_web.cpp's own: FindPath's clear, and LoadData's and MergeData's resize of the crossings
// TEMPLATE: GAME 0x55cb90
// ?erase@?$vector@GV?$allocator@G@std@@@std@@QAEPAGPAG0@Z

// TEMPLATE: GAME 0x55cbc0
// ?insert@?$vector@US_road_crossing@@V?$allocator@US_road_crossing@@@std@@@std@@QAEXPAUS_road_crossing@@IABU3@@Z
