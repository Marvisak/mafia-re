#pragma once

#include "Main.h"

// The startup's Windows helpers (docs/game-files.md, file 77; the name is a guess, PS2's Winstuff.cpp)

// Looks for \MafiaGame\a1.dta on the drives C: to Z: and asks for the first CD until it is found or the player cancels;
// false if they cancel. The name is ours.
bool WaitForCD();
