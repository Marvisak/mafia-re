#pragma once

#include "actors/C_PhThings.h"

// The public physics (PS2 C_public_physics, C_public_physics.cpp), a physical thing without members of its own,
// 0x38c bytes (PS2's 0x650). Only the constructor so far, which C_mission::CreateActor calls for
// ACTOR_PUBLIC_PHYSICS.
class C_public_physics : public C_PhThing
{
public:
    C_public_physics();
};
