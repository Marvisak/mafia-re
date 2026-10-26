#pragma once

#include "Main.h"

// The in-engine cutscenes (PC only, the names are ours: PS2 plays its cutscenes as videos, C_vid, which its
// GameLoopSingle updates where the PC's ticks this object). Its Tick (0x599610) plays two tracks of keys, 0x40 and
// 0x38 bytes, until a key whose time is 666.0; GameLoopSingle stops it on Space or Enter, and on Escape in the intro
// mission. Vtable 0x625ad8, whose first function takes a state (0 or 4, 4 when it is stopped). Only what
// C_mission::Close calls so far.
class C_cutscene
{
public:
    // Unloads the cutscene and gives the HUD and the player's controls back (C_mission::Close, C_game::Done, the racing
    // menu and GameLoopSingle call it)
    void Close();
};

// The one object
extern C_cutscene cutscene;
