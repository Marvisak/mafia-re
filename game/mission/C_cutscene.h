#pragma once

#include "actors/C_actor.h"

// The in-engine cutscenes (PC only, the names are ours: PS2 plays its cutscenes as videos, C_vid, which its
// GameLoopSingle updates where the PC's ticks this object). Its Tick (0x599610) plays two tracks of keys, 0x40 and
// 0x38 bytes, until a key whose time is 666.0; GameLoopSingle stops it on Space or Enter, and on Escape in the intro
// mission. Vtable 0x625ad8, whose first function takes a state (0 or 4, 4 when it is stopped). Only what
// C_mission::Close and C_actor's destructor call so far.
class C_cutscene
{
public:
    // Takes the actor from the cutscene's lists (+0x90's, which takes the flag too, +0x8's and +0x18's): true if one held
    // it (the name is ours; C_actor's destructor calls it)
    bool RemoveActor(C_actor *actor, bool unknown);
    // Unloads the cutscene and gives the HUD and the player's controls back (C_mission::Close, C_game::Done, the racing
    // menu and GameLoopSingle call it)
    void Close();
};

// The one object
extern C_cutscene cutscene;

// The seagulls (PC only, the name is ours: they load racek.i3d, Czech for a seagull), 0x244 bytes. Their file is the
// cutscenes': the constructor (0x59e7e0) is in its range, after Close. Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_SEAGULL.
class C_seagull : public C_actor
{
public:
    C_seagull();

    uchar unknown_70[0x1d4];            // 0x70
};
