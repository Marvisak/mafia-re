#pragma once

#include "actors/C_actor.h"

// The humans (PS2 C_human, C_human.cpp), the base of the player and of the AI's C_entity, 0xa9c bytes. Only the
// constructor so far, which the derived classes' constructors call, and the members the camera reads.
class C_human : public C_actor
{
public:
    C_human();

    uchar unknown_70[0x174];            // 0x70
    bool crouching;                     // 0x1e4 (PS2 0x390), the camera drops while it is set (the name is ours)
    uchar unknown_1e5[0x88f];           // 0x1e5
    C_actor *killer;                    // 0xa74 (PS2 0xba8), whom the death camera looks at (the name is ours)
    uchar unknown_a78[0x24];            // 0xa78
};
