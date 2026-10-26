// The humans' blood (PS2 C_blood.cpp; the PC's file is the second one in the range 0x411be0-0x41234c, after C_chunk.cpp,
// docs/game-files.md): C_blood's Tick (0x411e30) and Init (0x411fb0), then C_blood_manager's destructor, GameInit
// (0x412020), GameDone (0x412050), Tick (0x4120a0) and AddBlood (0x412140), PS2's order. Written so far: the manager's
// constructor and destructor, which the game's constructor and destructor call.

#include "actors/C_blood.h"

// The same code as C_tshift's constructor, whose copy the linker kept (0x5604f0): the marker is there
C_blood_manager::C_blood_manager()
{
}

// The original's linker kept this copy (0x412000) of the code C_tshift's destructor shares, ours C_tshift's: the marker
// is there
C_blood_manager::~C_blood_manager()
{
}
