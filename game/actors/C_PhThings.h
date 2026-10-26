#pragma once

#include "actors/C_actor.h"

// The physical things (PS2 C_PhThing, C_PhThings.cpp), the base of the public physics and of C_box_new, 0x38c bytes.
// Only the constructor so far, which the derived classes' constructors call.
class C_PhThing : public C_actor
{
public:
    C_PhThing();

    uchar unknown_70[0x31c];            // 0x70
};
