// The saved games and profiles (PS2 g_loadsave.cpp; the PC's file is the one at 0x604e60-0x60f5bc,
// docs/game-files.md), with the car database, as PS2's. Written so far: what InitSystem and CloseSystem reach (Init,
// Close, the constructors and destructors, AllDefault, ProfileEnumRelease, ApplyControls, the car database's Init and
// Close, RaceCarsEnable and RaceCarsDisable) and LanguageSelect; the rest of what they and main call is stubs.

#include "ui/g_loadsave.h"
#include "ui/g_indicators.h"
#include "ui/g_m_start.h"
#include "ui/g_textdb.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "g_material.h"

#include <string.h>

#include "IGraph/IGraph.h"
#include "rw_data.h"
#include "stub.h"

// GLOBAL: GAME 0x639320
bool profileIgnoreSave = true;

// The original constructs both in this order, its file's first functions
// GLOBAL: GAME 0x6d4628
G_LoadSaveClass mafiaLoadSave;
// GLOBAL: GAME 0x6d4560
G_CarDatabase mafiaCarDatabase;

// Calls its own object's AllDefault and ApplyControls through the global, as PS2's calls AllDefault, and runs the
// profiles' menu in the menu mission: quitting there is the only failure.
// FUNCTION: GAME 0x604ee0
bool G_LoadSaveClass::Init()
{
    unknown_90 = true;
    race_cars = 0;
    unknown_1f0 = 0;
    unknown_9c = 0;
    unknown_1e8 = 0;
    mafiaLoadSave.AllDefault();
    mafiaIndicators.Create();
    LoadMaterialsFromBin();
    mafiaLoadSave.ApplyControls();
    mafiaIndicators.ResolutionChanged();
    mafiaIndicators.unknown_40a4 = 0;
    ProfileEnumFiles();
    igraph->NullTimer();
    act_mission->Open("00menu", false, (uint)-1, false);
    int choice = GM_Menu::ExecuteMenu(new GM_ProfileSelect, false, true);
    ProfileEnumRelease();
    if (choice == PROFILESELECT_QUIT)
    {
        profileIgnoreSave = true;
        return false;
    }
    ProfileLoad();
    mafiaLoadSave.ApplyControls();
    profileIgnoreSave = false;
    return true;
}

// Everything through the globals, as PS2's
// FUNCTION: GAME 0x605030
void G_LoadSaveClass::Close()
{
    if (!profileIgnoreSave)
        mafiaLoadSave.ProfileSave();
    mafiaIndicators.Destroy();
    UnloadMaterials();
    textDatabase.Unload();
    mafiaCarDatabase.Close();
}

#pragma auto_inline(off)

// STUB: GAME 0x605070
bool G_LoadSaveClass::ProfileChange()
{
    StubLog("G_LoadSaveClass::ProfileChange");
    return false;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x605140
G_LoadSaveClass::G_LoadSaveClass()
{
    memset(bindings, 0, sizeof(bindings));
    memset(&settings, 0, sizeof(settings));
    memset(unknown_520, 0, sizeof(unknown_520));
    memset(unknown_5bc, 0, sizeof(unknown_5bc));
    unknown_8c = 0;
    unknown_5d4 = 0;
    unknown_91 = true;
}

// The options' camera values through the global (PS2's stores its one value there the same way)
// FUNCTION: GAME 0x6051e0
void G_LoadSaveClass::AllDefault()
{
    mafiaCarDatabase.Init();
    DefaultControls(DEFAULTS_ALL);
    DefaultSettings(DEFAULTS_ALL);
    memset(unknown_520, 0, sizeof(unknown_520));
    RaceCarsDisable((uint)-1);
    RaceCarsEnable(1);
    mafiaLoadSave.settings.camera = 1;
    mafiaLoadSave.settings.unknown_34 = 7;
    unknown_1f0 = 0;
    unknown_9c = 0;
    SaveGameNewGame();
}

#pragma auto_inline(off)

// STUB: GAME 0x605260
void G_LoadSaveClass::ProfileEnumFiles()
{
    StubLog("G_LoadSaveClass::ProfileEnumFiles");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x605530
void G_LoadSaveClass::ProfileEnumRelease()
{
    profiles.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x6057f0
void G_LoadSaveClass::ProfileLoad()
{
    StubLog("G_LoadSaveClass::ProfileLoad");
}

// STUB: GAME 0x6059f0
void G_LoadSaveClass::ProfileSave()
{
    StubLog("G_LoadSaveClass::ProfileSave");
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
    // Its two letters in place of the "xx"
    name[14] = language >> 8;
    name[15] = language;
    textDatabase.LoadDatabase(name);
    mafiaIndicators.LanguageSelect(language);
}

#pragma auto_inline(off)

// STUB: GAME 0x607600
void G_LoadSaveClass::DefaultControls(uint flags)
{
    StubLog("G_LoadSaveClass::DefaultControls");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x607a60
void G_LoadSaveClass::ApplyControls()
{
    controls.ClearControls();
    for (int i = 0; i < NUM_CONTROLS; i++)
        controls.SetControl(&bindings[i], i);
    controls.SetMouseScale(400, 400, 400);
}

#pragma auto_inline(off)

// STUB: GAME 0x607ab0
void G_LoadSaveClass::DefaultSettings(uint flags)
{
    StubLog("G_LoadSaveClass::DefaultSettings");
}

#pragma auto_inline(on)

// The default car's texts: "none" and "bad c/p-index.def" (Czech, the car and player indices)
// FUNCTION: GAME 0x60a2f0
G_CarDatabase::G_CarDatabase()
{
    num_cars = 0;
    cars = NULL;
    num_players = 0;
    players = NULL;
    none.unknown_88 = 0;
    // STRING: GAME 0x639670
    strcpy(none.id, "none");
    // STRING: GAME 0x639658
    strcpy(none.name, "Spatny c|p-index.def");
    unknown_bc = 0;
    unknown_c0 = 0;
    unknown_b8 = 0;
}

// FUNCTION: GAME 0x60a350
void G_CarDatabase::Init()
{
    // STRING: GAME 0x639690
    int file = dtaOpen("tables\\carindex.def", false);
    if (file >= 0)
    {
        if (cars)
            delete[] cars;
        int size = dtaSeek(file, 0, SEEK_END);
        dtaSeek(file, 0, SEEK_SET);
        cars = (S_car_index *)new char[size];
        dtaRead(file, cars, size);
        dtaClose(file);
        num_cars = size / sizeof(S_car_index);
    }
    // STRING: GAME 0x639678
    file = dtaOpen("tables\\playerindex.def", false);
    if (file >= 0)
    {
        if (players)
            delete[] players;
        int size = dtaSeek(file, 0, SEEK_END);
        dtaSeek(file, 0, SEEK_SET);
        players = (S_player_index *)new char[size];
        dtaRead(file, players, size);
        dtaClose(file);
        num_players = size / sizeof(S_player_index);
    }
}

// FUNCTION: GAME 0x60a420
void G_CarDatabase::Close()
{
    if (cars)
        delete[] cars;
    cars = NULL;
    num_cars = 0;
    if (players)
        delete[] players;
    players = NULL;
    num_players = 0;
}

// FUNCTION: GAME 0x60b610
void G_LoadSaveClass::RaceCarsEnable(uint cars)
{
    race_cars |= cars;
}

// FUNCTION: GAME 0x60b630
void G_LoadSaveClass::RaceCarsDisable(uint cars)
{
    race_cars &= ~cars;
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

// The vectors' destruction only, after FreerideSetupAfter as PS2's
// FUNCTION: GAME 0x60ef50
G_LoadSaveClass::~G_LoadSaveClass()
{
}
