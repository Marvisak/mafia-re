#pragma once

#include "actors/C_actor.h"

// The dogs (PS2 C_dog, C_dog.cpp), 0x310 bytes (PS2's 0x350). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_DOG.
class C_dog : public C_actor
{
public:
    C_dog();

    // The dogs' animations' tracks (PS2 name)
    static void LoadTCK();

    uchar unknown_70[0x2a0];            // 0x70
};
