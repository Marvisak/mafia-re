#pragma once

#include "actors/C_bottle.h"

// The cartridge cases the guns throw out (PS2 C_cartridge, C_cartridge.cpp, whose code the PS2 build doesn't have),
// ACTOR_CARTRIDGE, 0x13c bytes: a bottle's physics. C_game::Init creates the game's from 2nabojnice.i3d and
// 2nabojniceb.i3d ("nabojnice" is Czech for a cartridge case). Only the constructor so far.
// VTABLE: GAME 0x623880
class C_cartridge : public C_bottle
{
public:
    C_cartridge();

    uint unknown_138;                   // 0x138

    // The cases' sounds, which C_game::Init creates and DoneIntern releases (PS2 name)
    static I3D_sound *sounds[3];
};
