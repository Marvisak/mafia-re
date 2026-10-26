#pragma once

#include "Main.h"

#include <vector>

class C_chunk;

// The cars' road network (PS2 C_roads, road_web.cpp), 0x24 bytes. Only what C_mission's Init and UnInit use so far.

// A node of the roads (PS2 S_road_node), 0xec bytes, the size SaveData counts them by
struct S_road_node
{
    uchar unknown_0[0xec];
};

// What SaveData writes after the nodes, 0x18 bytes each (the name is ours)
struct S_road_link
{
    uchar unknown_0[0x18];
};

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
