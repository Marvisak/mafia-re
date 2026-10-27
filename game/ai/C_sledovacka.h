#pragma once

#include "Main.h"

#include <vector>

class C_actor;
class C_driver;

// The cars that chase, flee or follow (PS2 C_sledovacka, C_sledovacka.cpp), the game's 0x2b18. PS2's is 0x20 bytes, its
// offsets in the comments; the PC's size isn't known yet.
class C_sledovacka
{
public:
    // A bridge of the mission and the road node it is at (PS2 C_sledovacka::S_BRIDGE, whose members are the same; the
    // names are ours), 8 bytes
    struct S_BRIDGE
    {
        C_actor *actor;                 // 0x0
        ushort node;                    // 0x4, an index of C_roads::nodes
    };
    // A car the chases drive (PS2 C_sledovacka::S_ELEMENT, 0x50 bytes), 0x44 bytes: only its driver is known so far,
    // which Done deletes
    struct S_ELEMENT
    {
        int unknown_0;                  // 0x0
        C_driver *driver;               // 0x4
        uchar unknown_8[0x3c];          // 0x8
    };

    std::vector<S_BRIDGE> bridges;      // 0x0
    std::vector<S_ELEMENT> elements;    // 0x10 (PS2 0xc, a gif::vector)
    int unknown_20;                     // 0x20 (PS2 0x18)

    C_sledovacka();
    ~C_sledovacka();

    // PS2 names: C_game's Init and Done call them. Init finds the mission's bridges, Done deletes the drivers and
    // forgets the cars and the bridges.
    void Init();
    void Done();
};

// What Init's inlined push_back calls out of line: the bridges' _Ucopy and _Ufill, folded with other vectors' of 8-byte
// elements (the original kept g_indicators.cpp's copies)
// TEMPLATE: GAME 0x604dd0
// std::vector<C_sledovacka::S_BRIDGE,std::allocator<C_sledovacka::S_BRIDGE> >::_Ucopy

// TEMPLATE: GAME 0x604e00
// std::vector<C_sledovacka::S_BRIDGE,std::allocator<C_sledovacka::S_BRIDGE> >::_Ufill
