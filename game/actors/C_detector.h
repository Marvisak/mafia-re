#pragma once

#include "actors/C_actor.h"

// The script objects (PS2 C_detector, C_detector.cpp), whose constructor constructs a C_program of their own, 0x128
// bytes (PS2's 0x150). Only the constructor so far, which C_mission::CreateActor calls for ACTOR_DETECTOR.
class C_detector : public C_actor
{
public:
    C_detector();

    uchar unknown_70[0xb8];             // 0x70
};
