#pragma once

#include "actors/C_actor.h"

// PS2's C_irenka (C_irenka.cpp, a Czech name; what it is isn't known yet), 0x10c bytes (PS2's 0x140). Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_IRENKA.
class C_irenka : public C_actor
{
public:
    C_irenka();

    uchar unknown_70[0x9c];             // 0x70
};
