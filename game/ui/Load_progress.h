#pragma once

#include "Main.h"

#include "I3D/I3D.h"

class GM_LoadProgress;

// The game's own message to the load callback (the name is ours): the mission's part of the progress, after the scene's
// half, which C_mission::Open ends with LOAD_PROGRESS_END
#define CBM_MISSION_PROGRESS ((LS3D_CALLBACK_MESSAGE)10000)

// The loading screen the callback creates when mafiaLoadSave has none (PS2 name)
extern GM_LoadProgress *menuProgress;

// The callback of a mission's loading (PS2 name): CBM_PROGRESS from the scene's loader fills the bar's first half and
// CBM_MISSION_PROGRESS the second, on mafiaLoadSave's loading screen or one of its own, which LOAD_PROGRESS_BEGIN
// creates and LOAD_PROGRESS_END destroys
void __stdcall cbLoadScene(LS3D_CALLBACK_MESSAGE msg, uint prm1, uint prm2, void *context);

// cbLoadScene's last progress shown (PS2 name). This compiler puts a function's static into the PDB as a module-level
// symbol with its plain name, where reccmp doesn't look for one, so a by-name marker matches it.
// GLOBAL: GAME 0x647b1c
// last_bit
