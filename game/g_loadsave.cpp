// The saved games and profiles (PS2 g_loadsave.cpp; the PC's file is the one at 0x604e60-0x60f5bc,
// docs/game-files.md). Not written yet: only what main needs to link.

#include "g_loadsave.h"

#include "ls3df.h"

// GLOBAL: GAME 0x6d4628
G_LoadSaveClass mafiaLoadSave;

#pragma auto_inline(off)

// STUB: GAME 0x605070
bool G_LoadSaveClass::ProfileChange()
{
    dbgPrintf("STUB: G_LoadSaveClass::ProfileChange");
    return false;
}

// STUB: GAME 0x605ff0
void G_LoadSaveClass::SaveGameNewGame()
{
    dbgPrintf("STUB: G_LoadSaveClass::SaveGameNewGame");
}

// STUB: GAME 0x606150
void G_LoadSaveClass::SaveGameLoad()
{
    dbgPrintf("STUB: G_LoadSaveClass::SaveGameLoad");
}

// STUB: GAME 0x60e710
void G_LoadSaveClass::ExecuteCredits()
{
    dbgPrintf("STUB: G_LoadSaveClass::ExecuteCredits");
}

// STUB: GAME 0x60e9d0
void G_LoadSaveClass::FreerideSetup()
{
    dbgPrintf("STUB: G_LoadSaveClass::FreerideSetup");
}

// STUB: GAME 0x60eef0
void G_LoadSaveClass::FreerideSetupAfter()
{
    dbgPrintf("STUB: G_LoadSaveClass::FreerideSetupAfter");
}

#pragma auto_inline(on)
