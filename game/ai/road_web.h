#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_chunk;

// The cars' road network (PS2 C_roads, road_web.cpp), 0x24 bytes. Only what C_mission's Init and UnInit use so far.

// A node of the roads (PS2 S_road_node), 0xec bytes, the size SaveData counts them by
struct S_road_node
{
    S_vector pos;                       // 0x0 (the name is ours)
    uchar unknown_c[0xe0];              // 0xc
};

// What SaveData writes after the nodes, 0x18 bytes each (the name is ours). GetNearNode returns one of them, the nodes
// at its two ends most likely: C_sledovacka::Init takes the nearer one.
struct S_road_link
{
    uchar unknown_0[0x14];              // 0x0
    ushort unknown_14;                  // 0x14, a node's index
    ushort unknown_16;                  // 0x16, a node's index
};

// GetNearNode's index without its flag, the top bit (the name is ours)
#define NEARNODE_INDEX 0x7fff

// VTABLE: GAME 0x625370
class C_roads
{
public:
    std::vector<S_road_node> nodes;     // 0x4
    std::vector<S_road_link> links;     // 0x14

    C_roads();
    // Not virtual: UnInit calls it directly
    ~C_roads();

    // Empties the network (the name is ours). Inline: C_mission::Close has it inlined (PS2's too), the object kept in a
    // register across the first clear, the second clear called out of line as the budget's nested call.
    void Clear()
    {
        nodes.clear();
        links.clear();
    }

    // Reads the mission's road.bin (PS2 name): not virtual, as PS2's, C_mission::Open calls it directly. It ends with
    // the vtable's RecompileDistances.
    void LoadData(C_chunk *chunk);
    // The node nearest to a position, 0xffff for none (PS2 name; PS2's takes a reference). Its top bit is a flag:
    // C_sledovacka::Init masks it off (NEARNODE_INDEX)
    ushort GetNearNode(S_vector pos);

    // PS2's three virtual functions in PS2's order, and a second reader between the last two, PC only (the parameters
    // each one's ret takes). It reads what LoadData reads; the name is ours, after C_web_path's MergeNodes.
    virtual void SortNodesByX();
    virtual void SaveData(C_chunk *chunk);
    virtual void MergeData(C_chunk *chunk);
    virtual void RecompileDistances();
};

// What C_mission::Close's inlined Clear calls (Mission.cpp's copies): the nodes' erase, and the links' clear, with its
// erase inlined
// TEMPLATE: GAME 0x5472a0
// std::vector<S_road_node,std::allocator<S_road_node> >::erase

// TEMPLATE: GAME 0x5470f0
// std::vector<S_road_link,std::allocator<S_road_link> >::clear
