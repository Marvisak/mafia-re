// The cartridge cases (PS2 C_cartridge.cpp; the PC's file is most likely the one at 0x4307d0-0x430b9c, after C_car_ex.cpp
// as the name sorts, docs/game-files.md), which starts with the constructor. Written so far: the constructor, which
// C_game::Init calls, and the sounds.

#include "actors/C_cartridge.h"

// GLOBAL: GAME 0x63d6c8
I3D_sound *C_cartridge::sounds[3];

// FUNCTION: GAME 0x4307d0
C_cartridge::C_cartridge()
{
    unknown_fc = false;
    unknown_6d = false;
    type = ACTOR_CARTRIDGE;
}
