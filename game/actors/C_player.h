#pragma once

#include "actors/C_human.h"

// The player (PS2 C_player, C_player.cpp), a human, 0xb00 bytes (PS2's 0xc90). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_PLAYER.
class C_player : public C_human
{
public:
    C_player();

    uchar unknown_a9c[0x64];            // 0xa9c
};
