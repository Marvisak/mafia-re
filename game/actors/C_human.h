#pragma once

#include "actors/C_actor.h"

// The humans (PS2 C_human, C_human.cpp), the base of the player and of the AI's C_entity, 0xa9c bytes. Only the
// constructor so far, which the derived classes' constructors call.
class C_human : public C_actor
{
public:
    C_human();

    uchar unknown_70[0xa2c];            // 0x70
};
