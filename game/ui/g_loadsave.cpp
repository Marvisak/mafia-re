// The saved games and profiles (PS2 g_loadsave.cpp; the PC's file is the one at 0x604e60-0x60f5bc,
// docs/game-files.md). Written so far: LanguageSelect; the rest of what main, InitSystem and CloseSystem call is stubs.

#include "ui/g_loadsave.h"
#include "ui/g_indicators.h"
#include "ui/g_textdb.h"
#include "mission/Mission.h"
#include "mission/game.h"

#include <string.h>

#include "stub.h"

// GLOBAL: GAME 0x6d4628
G_LoadSaveClass mafiaLoadSave;

#pragma auto_inline(off)

// STUB: GAME 0x604ee0
bool G_LoadSaveClass::Init()
{
    StubLog("G_LoadSaveClass::Init");
    return true;
}

// The original saves the profile unless a flag (0x639320) is set, then frees the indicators, a list at 0x60f780, the
// text database (0x6d8714) and the car database (0x6d4560), as PS2's
// STUB: GAME 0x605030
void G_LoadSaveClass::Close()
{
    StubLog("G_LoadSaveClass::Close");
}

// STUB: GAME 0x605070
bool G_LoadSaveClass::ProfileChange()
{
    StubLog("G_LoadSaveClass::ProfileChange");
    return false;
}

// STUB: GAME 0x605ff0
void G_LoadSaveClass::SaveGameNewGame()
{
    StubLog("G_LoadSaveClass::SaveGameNewGame");
}

// STUB: GAME 0x606150
void G_LoadSaveClass::SaveGameLoad()
{
    StubLog("G_LoadSaveClass::SaveGameLoad");
}

#pragma auto_inline(on)

// Loads the language's texts and the HUD's tables. -1 keeps the texts loaded while the game's flag is set and takes the
// Czech ones otherwise; InitSystem, the only caller (PS2's too), passes the data's language or 'en'.
// FUNCTION: GAME 0x607360
void G_LoadSaveClass::LanguageSelect(uint language)
{
    if (language == (uint)-1)
    {
        if (act_mission->game->unknown_40)
            return;
        language = 'cz';
    }
    char name[32];
    // STRING: GAME 0x639640
    strcpy(name, "tables\\textdb_xx.def");
    name[14] = language >> 8;
    name[15] = language;
    textDatabase.LoadDatabase(name);
    mafiaIndicators.LanguageSelect(language);
}

#pragma auto_inline(off)

// STUB: GAME 0x60e710
void G_LoadSaveClass::ExecuteCredits()
{
    StubLog("G_LoadSaveClass::ExecuteCredits");
}

// STUB: GAME 0x60e9d0
void G_LoadSaveClass::FreerideSetup()
{
    StubLog("G_LoadSaveClass::FreerideSetup");
}

// STUB: GAME 0x60eef0
void G_LoadSaveClass::FreerideSetupAfter()
{
    StubLog("G_LoadSaveClass::FreerideSetupAfter");
}

#pragma auto_inline(on)
