// The stones (PC only; the file is the one at 0x4aa2b0-0x4aa48c, between C_sledovacka.cpp and C_trf_c.cpp, its name
// ours, docs/game-files.md), which starts with the constructor. Written so far: the constructor, which C_game::Init
// calls.

#include "actors/C_stone.h"

// FUNCTION: GAME 0x4aa2b0
C_stone::C_stone()
{
    unknown_fc = false;
    unknown_6d = false;
    type = ACTOR_STONE;
}
