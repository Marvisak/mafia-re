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

    // PS2's three virtual functions in PS2's order, and LoadData, which isn't virtual on PS2, between the last two (the
    // parameters each one's ret takes)
    virtual void SortNodesByX();
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void RecompileDistances();
};
