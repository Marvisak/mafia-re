#pragma once

#include "Main.h"

// Loads one of the patch's script files, patch\a1.bin to a4.bin (PC only, the name is ours)
void LoadPatchFile(const char *name);
// Frees what LoadPatchFile loaded, at CloseSystem (PC only, the name is ours)
void UnloadPatchFiles();
