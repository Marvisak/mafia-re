// The bottles (PS2 C_bottle.cpp; the PC's file is the one at 0x412350-0x413d8c, docs/game-files.md), which starts
// with the constructor. Written so far: the constructor, which C_mission::CreateActor calls.

#include "actors/C_bottle.h"

// PS2's, its stores in this order
// FUNCTION: GAME 0x412350
C_bottle::C_bottle()
{
    unknown_fc = true;
    type = ACTOR_BOTTLE;
    unknown_6d = true;
    unknown_128 = true;
}
