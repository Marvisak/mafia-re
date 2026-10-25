// PS2's tshift.cpp (the PC's file is the one at 0x560410-0x5605bc, docs/game-files.md): S_tshift_elem::Tick
// (0x560410), C_tshift's constructor and C_tshift::Tick (0x560510), PS2's order with its destructor between the last two.
// Written so far: the constructor and the destructor, which C_mission's Init and UnInit call.

#include "mission/tshift.h"

// FUNCTION: GAME 0x5604f0
C_tshift::C_tshift()
{
}

// The linker kept another file's copy of this code (a vector's destructor)
// FUNCTION: GAME 0x412000 FOLDED
C_tshift::~C_tshift()
{
}
