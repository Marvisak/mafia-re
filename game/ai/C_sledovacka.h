#pragma once

#include "Main.h"

#include <vector>

// The cars that chase, flee or follow (PS2 C_sledovacka, C_sledovacka.cpp), the game's 0x2b18. PS2's is 0x20 bytes, its
// offsets in the comments; the PC's size isn't known yet. Only the constructor and the destructor are written so far.
class C_sledovacka
{
public:
    // A bridge and an element of a path (PS2 C_sledovacka::S_BRIDGE and S_ELEMENT): what they hold isn't known yet. The
    // file's copy loops move elements of 0x44 and 0x70 bytes.
    struct S_BRIDGE
    {
    };
    struct S_ELEMENT
    {
    };

    std::vector<S_BRIDGE> bridges;      // 0x0
    // PS2's is a gif::vector, most likely of its S_ELEMENT, the only one PS2's file has
    std::vector<S_ELEMENT> elements;    // 0x10 (PS2 0xc)
    int unknown_20;                     // 0x20 (PS2 0x18)

    C_sledovacka();
    ~C_sledovacka();
};
