#pragma once

#include "actors/C_bottle.h"

// The stones (PC only; the names are ours), ACTOR_STONE, 0x138 bytes: a bottle's physics. C_game::Init creates the game's
// from 2stone.i3d. Only the constructor so far.
// VTABLE: GAME 0x624c00
class C_stone : public C_bottle
{
public:
    C_stone();
};
