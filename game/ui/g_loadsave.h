#pragma once

#include "Main.h"

#include <vector>

#include "input/controls.h"

// The game's controls, each an entry of the controls' tables (C_controls::SetControl's index; the name is ours)
#define NUM_CONTROLS 60

// The flags of G_LoadSaveClass::DefaultControls and DefaultSettings (the names are ours): 0x80 clears the block first,
// the other bits choose groups of defaults (DefaultControls' 1 and 2, DefaultSettings' 4 and 8)
#define DEFAULTS_CLEAR 0x80
#define DEFAULTS_ALL 0xffff

// G_Profile_FileHead::magic (the name is ours)
#define PROFILE_MAGIC 'Prof'

// A profile's head (the name is ours, after PS2's G_SaveGame_FileHead), 0x54 bytes: the first record of its file,
// savegame\mafia<number>.sav, which ProfileSave writes and ProfileEnumFiles reads from each file into the list
struct G_Profile_FileHead
{
    uint magic;                         // 0x0, PROFILE_MAGIC
    uint version;                       // 0x4, 1
    uint number;                        // 0x8, the file's
    uint unknown_c;                     // 0xc, 0 when saved
    uchar unknown_10[0x34];
    uint unknown_44;                    // 0x44, G_LoadSaveClass::unknown_1f0 when saved
    uint unknown_48;                    // 0x48, G_LoadSaveClass::unknown_9c
    uint unknown_4c;                    // 0x4c, 0 when saved
    uint race_cars;                     // 0x50, G_LoadSaveClass::race_cars
};

// A saved game's head (PS2's name), 0x18 bytes: the first record of its file, savegame\mafia<profile>.<number>
struct G_SaveGame_FileHead
{
    uint unknown_0;
    uint version;                       // 0x4, 0x10
    uint number;                        // 0x8, the file's
    uchar unknown_c[0xc];
};

// The profile's options (the name is ours), 0x5c bytes, which ProfileSave writes after the controls; DefaultSettings
// sets their defaults
struct S_profile_settings
{
    uchar unknown_0[0x30];
    // 1 to 3 (the profile's load at 0x60e0fe resets others to 1): the camera G_Camera::Reset takes, the number of an
    // entry of the camera's cameraParams (where PS2's Reset takes 1)
    uint camera;                        // 0x30
    // The same for 7 to 15, 7 otherwise
    uint unknown_34;                    // 0x34
    uchar unknown_38[0x24];
};

// The saved games, profiles and the game's progress (PS2 G_LoadSaveClass, g_loadsave.cpp), at least 0x5d8 bytes (the
// constructor clears 0x5d4). Only what main, the camera and the start use so far.
class G_LoadSaveClass
{
public:
    std::vector<G_Profile_FileHead> profiles;   // 0x0, what ProfileEnumFiles found
    std::vector<G_SaveGame_FileHead> savegames; // 0x10, the profile's saved games
    G_Profile_FileHead profile;         // 0x20, the profile's head
    uchar unknown_74[0x18];
    uint unknown_8c;                    // 0x8c
    bool unknown_90;                    // 0x90, set by Init (PS2's 0x2b0 the same)
    bool unknown_91;                    // 0x91, set while ProfileLoad reads the profile: a failed load leaves the defaults
    uint race_cars;                     // 0x94, RaceCarsEnable's bits
    uint unknown_98;
    uint unknown_9c;                    // 0x9c
    uchar unknown_a0[0x1d4 - 0xa0];
    bool unknown_1d4;                   // 0x1d4, cleared by ProfileLoad
    std::vector<uint> objectives;       // 0x1d8, ObjectivesClear and ObjectivesRemove's
    uint unknown_1e8;                   // 0x1e8
    uint unknown_1ec;
    uint unknown_1f0;                   // 0x1f0
    S_control bindings[NUM_CONTROLS];   // 0x1f4, the profile's controls, which ApplyControls hands to the controls
    S_profile_settings settings;        // 0x4c4
    uchar unknown_520[0x9c];            // 0x520, which ProfileSave writes last (PS2's 0x2fc, as AllDefault clears it)
    uchar unknown_5bc[0x18];
    uint unknown_5d4;                   // 0x5d4

    G_LoadSaveClass();
    ~G_LoadSaveClass();

    // Sets up the profiles at the start, the player choosing one in the profiles' menu; false is fatal, the player
    // quitting there
    bool Init();
    // Saves the profile and frees the game's tables, at CloseSystem
    void Close();
    // The player chose another profile; true to restart the menu with it
    bool ProfileChange();
    // The defaults: the car database, the controls, the options, the race cars and a new game
    void AllDefault();
    // Reads the head of each profile file into profiles
    void ProfileEnumFiles();
    // Forgets them
    void ProfileEnumRelease();
    // Reads the profile's file, which profile.number names, or takes the defaults
    void ProfileLoad();
    // Writes the profile's file
    void ProfileSave();
    void SaveGameNewGame();
    // Loads the saved game the player chose and opens its mission
    void SaveGameLoad();
    // The game's language, two lowercase letters as a multicharacter constant ('en')
    void LanguageSelect(uint language);
    // The controls' defaults (PC only, the name is ours), which flags choose as DefaultSettings's
    void DefaultControls(uint flags);
    // Hands the profile's controls to the controls' object (PC only, the name is ours)
    void ApplyControls();
    // The options' defaults: flags 0x80 clears them first, 4 and 8 choose groups
    void DefaultSettings(uint flags);
    void RaceCarsEnable(uint cars);
    void RaceCarsDisable(uint cars);
    // Plays tables\credits.def
    void ExecuteCredits();
    void FreerideSetup();
    // Finds Tommy and his car in the free ride's mission
    void FreerideSetupAfter();
};

// A car of tables\carindex.def (the name is ours), 0xa8 bytes
struct S_car_index
{
    char id[0x20];                      // 0x0, "fordttud"
    char model[0x20];                   // 0x20, "fordTrun.I3D"
    char shadow_model[0x20];            // 0x40, "fordTrunS.i3d"
    char name[0x20];                    // 0x60, "Bolt Ace Tudor"
    uint unknown_80;
    uint unknown_84;                    // 0x84, PS2's AllDefault enables these race cars
    uint unknown_88;                    // 0x88
    uchar unknown_8c[0x1c];
};

// A player's model of tables\playerindex.def (the name is ours), 0x24 bytes
struct S_player_index
{
    char model[0x20];                   // "civil02.i3d"
    uint flags;
};

// The cars and the players' models (PS2 G_CarDatabase, in g_loadsave.cpp as on PC), 0xc4 bytes
class G_CarDatabase
{
public:
    uint num_cars;                      // 0x0
    S_car_index *cars;                  // 0x4, tables\carindex.def
    uint num_players;                   // 0x8
    S_player_index *players;            // 0xc, tables\playerindex.def
    S_car_index none;                   // 0x10, what PS2's GetCar returns for an index out of range
    uint unknown_b8;                    // 0xb8
    uint unknown_bc;                    // 0xbc
    uint unknown_c0;                    // 0xc0

    G_CarDatabase();
    ~G_CarDatabase() { Close(); }

    // Reads both tables
    void Init();
    // Frees them
    void Close();
};

// Set while the profile mustn't be saved: from the start until Init has loaded one, and when the player quit in the
// profiles' menu (PS2's name)
extern bool profileIgnoreSave;
extern G_LoadSaveClass mafiaLoadSave;
extern G_CarDatabase mafiaCarDatabase;
