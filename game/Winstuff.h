#pragma once

#include "Main.h"

// The startup's Windows helpers (docs/game-files.md, file 77; the name is a guess, PS2's Winstuff.cpp). The names are
// ours.

// Asks for the first CD until CheckCD finds it; false if the player cancels. main calls it before the engine starts,
// C_mission::Open with it running, which presents its frame before each message.
bool WaitForCD();
// Whether the file exists (FindFirstFile)
bool FileExists(const char *name);
// Looks for a1.dta in \MafiaDE, \MafiaEN, \MafiaSP, \MafiaFR, \MafiaIT and the root's \MafiaGame on each drive C: to Z:
// that has a volume
bool CheckCD();
