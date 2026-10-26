#pragma once

#include "actors/C_PhThings.h"

// The physical objects (PS2 C_box_new, C_box_new.cpp), a physical thing, 0x3a8 bytes (PS2's 0x660). Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_BOX_NEW.
class C_box_new : public C_PhThing
{
public:
    C_box_new();

    uchar unknown_38c[0x1c];            // 0x38c
};
