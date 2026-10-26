// The chases (PS2 C_sledovacka.cpp; the PC's file is the one at 0x4a0320-0x4aa2ac, docs/game-files.md), which starts
// with the constructor. Written so far: the constructor and the destructor, which the game's constructor and destructor
// call.

#include "ai/C_sledovacka.h"

// FUNCTION: GAME 0x4a0320
C_sledovacka::C_sledovacka() :
    unknown_20(0)
{
}

// The linker kept another file's copy of the same code (two vectors' destructors), in the force feedback's range
// FUNCTION: GAME 0x444670 FOLDED
C_sledovacka::~C_sledovacka()
{
}
