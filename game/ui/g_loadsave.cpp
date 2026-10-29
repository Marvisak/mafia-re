// The saved games and profiles (PS2 g_loadsave.cpp; the PC's file is the one at 0x604e60-0x60f5bc,
// docs/game-files.md), with the car database and the save files' G_Stream, as PS2's. Written so far: what InitSystem
// and CloseSystem reach (Init, Close, the constructors and destructors, AllDefault, ProfileEnumFiles, ProfileEnumRelease,
// ProfileGetList, ProfileLoad, ProfileSave, SaveGameNewGame, the PC's controls and options defaults, ApplyControls, FindBinding, the car
// database's Init and Close, G_Stream's reading and writing, RaceCarsEnable, RaceCarsDisable, GarageClear, Refresh,
// FreerideStoreA and FreerideStoreB), LanguageSelect, and what C_game's Init and Done reach: ObjectivesClear, the
// garage's GarageGameInit and GarageGameDone with the car database's GetCar, GetCarModel, GetCarParams,
// GetIdxByModelLong and GetIDByActor, the end of mission garage menu's names, and the profiles' menus' ProfileCreate and
// ProfileDelete; and of what the scripts call, the car database's GetPlayerModel and GetIdxByModel, the garage's
// GarageAddLastStolen, GarageShowResults, CarCantGetOut and EnableSteal, FreerideEnableCar and FreerideEnable; the rest
// of what they and main call is stubs.

#include "ui/g_loadsave.h"
#include "ui/g_indicators.h"
#include "ui/g_m_func.h"
#include "ui/g_m_start.h"
#include "ui/g_textdb.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "actors/C_human.h"
#include "vehicles/C_car.h"
#include "vehicles/C_Vehicle.h"
#include "g_material.h"
#include "I3D_cache.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "IGraph/IGraph.h"
#include "ISound/ISound.h"
#include "rw_data.h"
#include "stub.h"

// GLOBAL: GAME 0x639320
bool profileIgnoreSave = true;

// GLOBAL: GAME 0x6d4540
S_freeride freeride;

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
    freeride_enabled = FREERIDE_SMALL_CITY;
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
    progress = NULL;
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
    freeride_enabled = FREERIDE_SMALL_CITY;
    unknown_9c = 0;
    SaveGameNewGame();
}

// The PC's version has no version check (PS2's takes only version 10's files) and numbers each head after its file
// FUNCTION: GAME 0x605260
std::vector<G_Profile_FileHead> *G_LoadSaveClass::ProfileEnumFiles()
{
    profiles.clear();
    G_Stream stream;
    stream.SetParams(PROFILE_MAGIC, true, false);
    for (int i = 0; i < MAX_PROFILES; i++)
    {
        char name[128];
        // STRING: GAME 0x6395f4
        sprintf(name, "savegame\\mafia%03d.sav", i);
        if (stream.OpenRead(name))
        {
            G_Profile_FileHead head;
            if (stream.ReadAdr(&head, sizeof(head)) == sizeof(head))
            {
                head.number = i;
                profiles.push_back(head);
            }
            stream.Close();
        }
    }
    return &profiles;
}

// FUNCTION: GAME 0x605530
void G_LoadSaveClass::ProfileEnumRelease()
{
    profiles.clear();
}

// FUNCTION: GAME 0x605560
std::vector<G_Profile_FileHead> *G_LoadSaveClass::ProfileGetList()
{
    return &profiles;
}

// PS2's also keeps the index and selects the memory card's profile
// FUNCTION: GAME 0x605570
void G_LoadSaveClass::ProfileSelect(uint index)
{
    if (index < profiles.size())
        profile = profiles[index];
}

// The file's number is the lowest no profile has. PS2's creates the profile on the memory card, with the defaults.
// FUNCTION: GAME 0x6055c0
bool G_LoadSaveClass::ProfileCreate(uchar *name)
{
    G_Profile_FileHead head;
    memset(&head, 0, sizeof(head));
    strncpy((char *)head.name, (char *)name, PROFILE_NAME_LENGTH);
    head.name[PROFILE_NAME_LENGTH] = 0;
    uint number = 0;
    uint last;
    G_Profile_FileHead *p;
    for (p = profiles.begin(); p < profiles.end(); p++)
    {
        if (!strcmpi((char *)name, (char *)p->name))
            return false;
    }
    do
    {
        last = number;
        for (p = profiles.begin(); p < profiles.end(); p++)
        {
            if (number == p->number)
                number++;
        }
    } while (number != last);
    head.number = number;
    // STRING: GAME 0x63960c
    CreateDirectory("savegame", NULL);
    G_Stream stream;
    stream.SetParams(PROFILE_MAGIC, true, false);
    char file_name[128];
    sprintf(file_name, "savegame\\mafia%03d.sav", number);
    if (stream.OpenWrite(file_name))
    {
        stream.WriteAdr(&head, sizeof(head));
        stream.Close();
    }
    profiles.push_back(head);
    ProfileSelect(profiles.size() - 1);
    return true;
}

// PS2's deletes the profile on the memory card
// FUNCTION: GAME 0x605740
void G_LoadSaveClass::ProfileDelete(uint index)
{
    char name[128];
    uint number = profiles[index].number;
    sprintf(name, "savegame\\mafia%03d.sav", number);
    remove(name);
    for (int i = 0; i < PROFILE_SAVE_FILES; i++)
    {
        // STRING: GAME 0x639618
        sprintf(name, "savegame\\mafia%03d.%03d", number, i);
        remove(name);
    }
    profiles.erase(profiles.begin() + index);
}

// The file: the head (the PC's version 1), the controls, the options and unknown_520, and nothing after them; anything
// else leaves the defaults. Then the sniper mode's binding, the tests taking FindBinding's result as a bool, so that its
// -1 counts as found: unless the first control shares the binding, it is cleared, or put on R if the first control has
// R.
// FUNCTION: GAME 0x6057f0
void G_LoadSaveClass::ProfileLoad()
{
    G_Stream stream;
    uint number = profile.number;
    stream.SetParams(PROFILE_MAGIC, true, false);
    char name[128];
    sprintf(name, "savegame\\mafia%03d.sav", number);
    unknown_91 = true;
    if (stream.OpenRead(name))
    {
        if (stream.ReadAdr(&profile, sizeof(profile)) == sizeof(profile))
        {
            profile.number = number;
            if (profile.magic == PROFILE_MAGIC && profile.version == 1)
            {
                stream.ReadAdr(bindings, sizeof(bindings));
                stream.ReadAdr(&settings, sizeof(settings));
                if (stream.ReadAdr(unknown_520, sizeof(unknown_520)) == sizeof(unknown_520))
                {
                    uint end;
                    if (!stream.ReadAdr(&end, sizeof(end)))
                    {
                        race_cars = profile.race_cars;
                        freeride_enabled = profile.freeride_enabled;
                        unknown_9c = profile.unknown_48;
                        unknown_91 = false;
                        if (FindBinding(CTRL_SNIPER, &bindings[CTRL_SNIPER], false, NUM_CONTROLS))
                        {
                            S_control key;
                            key.device = CONTROL_KEY;
                            key.code = DIK_R;
                            key.joy = 0;
                            key.half = CONTROL_AXIS_FULL;
                            if (mafiaLoadSave.FindBinding(CTRL_SNIPER, &key, false, -1))
                                SetBinding(CTRL_SNIPER, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
                            else
                                SetBinding(CTRL_SNIPER, CONTROL_KEY, DIK_R, 0, CONTROL_AXIS_FULL);
                            ApplyControls();
                        }
                    }
                }
            }
        }
        stream.Close();
    }
    if (unknown_91)
        AllDefault();
    Refresh(REFRESH_ALL);
    FreerideStoreA(1, 0, 1.0f, 1.0f, 1.0f);
    FreerideStoreB(0);
    unknown_1d4 = false;
}

// PS2's sets the head before it opens the file, with the time it saves. The order of the head's stores: unknown_4c's
// before freeride_enabled's (three other orders give the same code)
// FUNCTION: GAME 0x6059f0
void G_LoadSaveClass::ProfileSave()
{
    G_Stream stream;
    stream.SetParams(PROFILE_MAGIC, true, false);
    char name[128];
    sprintf(name, "savegame\\mafia%03d.sav", profile.number);
    if (stream.OpenWrite(name))
    {
        profile.magic = PROFILE_MAGIC;
        profile.version = 1;
        profile.unknown_c = 0;
        profile.race_cars = race_cars;
        profile.unknown_4c = 0;
        profile.freeride_enabled = freeride_enabled;
        profile.unknown_48 = unknown_9c;
        stream.WriteAdr(&profile, sizeof(profile));
        stream.WriteAdr(bindings, sizeof(bindings));
        stream.WriteAdr(&settings, sizeof(settings));
        stream.WriteAdr(unknown_520, sizeof(unknown_520));
        stream.Close();
    }
}

// FUNCTION: GAME 0x605ff0
void G_LoadSaveClass::SaveGameNewGame()
{
    unknown_1e8 = 0;
    unknown_98 = 0;
    GarageClear();
    freeride.unknown_18 = false;
}

#pragma auto_inline(off)

// STUB: GAME 0x606150
void G_LoadSaveClass::SaveGameLoad()
{
    StubLog("G_LoadSaveClass::SaveGameLoad");
}

// STUB: GAME 0x6072f0
void G_LoadSaveClass::SaveGameFromScript(uint slot, bool full, bool message)
{
    StubLog("G_LoadSaveClass::SaveGameFromScript");
}

#pragma auto_inline(on)

// Loads the language's texts and the HUD's tables. -1 keeps the texts loaded while the game's flag is set and takes the
// Czech ones otherwise; InitSystem, the only caller (PS2's too), passes the data's language or 'en'.
// FUNCTION: GAME 0x607360
void G_LoadSaveClass::LanguageSelect(uint language)
{
    if (language == (uint)-1)
    {
        if (act_mission->game->initialized)
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

// FUNCTION: GAME 0x6073c0
void G_LoadSaveClass::SetBinding(int index, ushort device, ushort code, int joy, int half)
{
    bindings[index].device = device;
    bindings[index].code = code;
    bindings[index].joy = joy;
    bindings[index].half = half;
}

// FUNCTION: GAME 0x607460
int G_LoadSaveClass::FindBinding(int index, S_control *control, bool any_half, int other)
{
    if (control->device == CONTROL_NONE && !control->code)
        return -1;
    if (index < NUM_PLAYER_CONTROLS)
    {
        for (int i = 0; i < NUM_PLAYER_CONTROLS; i++)
        {
            if (i != index && i != other && bindings[i].device == control->device && bindings[i].code == control->code
                && bindings[i].joy == control->joy && (any_half || bindings[i].half == control->half))
                return i;
        }
    }
    if (index >= NUM_PLAYER_CONTROLS || other >= NUM_PLAYER_CONTROLS)
    {
        for (int i = NUM_PLAYER_CONTROLS; i < NUM_CONTROLS; i++)
        {
            if (i != index && i != other && bindings[i].device == control->device && bindings[i].code == control->code
                && bindings[i].joy == control->joy && (any_half || bindings[i].half == control->half))
                return i;
        }
    }
    return -1;
}

// The player's defaults are the arrows and the right-hand keys first, WSAD and the left-hand ones second; the car's
// keys first and the first joystick second
// FUNCTION: GAME 0x607600
void G_LoadSaveClass::DefaultControls(uint flags)
{
    if (flags & DEFAULTS_CLEAR)
        memset(bindings, 0, sizeof(bindings));
    if (flags & DEFAULTS_PLAYER)
    {
        SetBinding(CTRL_FORWARD, CONTROL_KEY, DIK_UP, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_FORWARD + CTRL_SECONDARY, CONTROL_KEY, DIK_W, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_BACKWARD, CONTROL_KEY, DIK_DOWN, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_BACKWARD + CTRL_SECONDARY, CONTROL_KEY, DIK_S, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_LEFT, CONTROL_KEY, DIK_LEFT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_LEFT + CTRL_SECONDARY, CONTROL_KEY, DIK_A, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_RIGHT, CONTROL_KEY, DIK_RIGHT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_RIGHT + CTRL_SECONDARY, CONTROL_KEY, DIK_D, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_WALK, CONTROL_KEY, DIK_RSHIFT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_WALK + CTRL_SECONDARY, CONTROL_KEY, DIK_LSHIFT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_ACTION, CONTROL_MOUSE, MBUT_RIGHT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_ACTION + CTRL_SECONDARY, CONTROL_KEY, DIK_U, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_CROUCH, CONTROL_KEY, DIK_RCONTROL, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_CROUCH + CTRL_SECONDARY, CONTROL_KEY, DIK_Q, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_FIRE, CONTROL_MOUSE, MBUT_LEFT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_FIRE + CTRL_SECONDARY, CONTROL_KEY, DIK_F, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_JUMP, CONTROL_KEY, DIK_NUMPAD0, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_JUMP + CTRL_SECONDARY, CONTROL_KEY, DIK_E, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_AIM, CONTROL_MOUSE, CONTROL_MOUSE_X, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_AIM + CTRL_SECONDARY, CONTROL_MOUSE, CONTROL_MOUSE_Y, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_CYCLE_UP, CONTROL_MOUSE, CONTROL_MOUSE_Z, 0, CONTROL_AXIS_POSITIVE);
        SetBinding(CTRL_CYCLE_UP + CTRL_SECONDARY, CONTROL_KEY, DIK_PRIOR, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_CYCLE_DOWN, CONTROL_MOUSE, CONTROL_MOUSE_Z, 0, CONTROL_AXIS_NEGATIVE);
        SetBinding(CTRL_CYCLE_DOWN + CTRL_SECONDARY, CONTROL_KEY, DIK_NEXT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_INVENTORY, CONTROL_KEY, DIK_I, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_HIDE_WEAPON, CONTROL_KEY, DIK_H, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_DROP_WEAPON, CONTROL_KEY, DIK_BACK, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_MAP, CONTROL_KEY, DIK_TAB, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_TOGGLE_RUN, CONTROL_KEY, DIK_CAPITAL, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_RELOAD, CONTROL_KEY, DIK_L, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_SNIPER, CONTROL_KEY, DIK_R, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_OBJECTIVES, CONTROL_KEY, DIK_F1, 0, CONTROL_AXIS_FULL);
    }
    if (flags & DEFAULTS_CAR)
    {
        SetBinding(CTRL_ACCELERATE, CONTROL_KEY, DIK_UP, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_ACCELERATE + CTRL_SECONDARY, CONTROL_JOY, JOY_Y, 0, CONTROL_AXIS_NEGATIVE);
        SetBinding(CTRL_BRAKE, CONTROL_KEY, DIK_DOWN, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_BRAKE + CTRL_SECONDARY, CONTROL_JOY, JOY_Y, 0, CONTROL_AXIS_POSITIVE);
        SetBinding(CTRL_STEER_LEFT, CONTROL_KEY, DIK_LEFT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_STEER_LEFT + CTRL_SECONDARY, CONTROL_JOY, JOY_X, 0, CONTROL_AXIS_NEGATIVE);
        SetBinding(CTRL_STEER_RIGHT, CONTROL_KEY, DIK_RIGHT, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_STEER_RIGHT + CTRL_SECONDARY, CONTROL_JOY, JOY_X, 0, CONTROL_AXIS_POSITIVE);
        SetBinding(CTRL_HANDBRAKE, CONTROL_KEY, DIK_SPACE, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_HANDBRAKE + CTRL_SECONDARY, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_SPEED_LIMITER, CONTROL_KEY, DIK_F5, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_SPEED_LIMITER + CTRL_SECONDARY, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_GEAR_UP, CONTROL_KEY, DIK_A, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_GEAR_UP + CTRL_SECONDARY, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_GEAR_DOWN, CONTROL_KEY, DIK_Z, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_GEAR_DOWN + CTRL_SECONDARY, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_HORN, CONTROL_KEY, DIK_K, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_HORN + CTRL_SECONDARY, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
        // The second POV's left and right
        SetBinding(CTRL_LOOK_LEFT, CONTROL_KEY, DIK_COMMA, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_LOOK_LEFT + CTRL_SECONDARY, CONTROL_JOY, CONTROL_JOY_POV + 0x10 + CONTROL_POV_LEFT, 0,
                   CONTROL_AXIS_FULL);
        SetBinding(CTRL_LOOK_RIGHT, CONTROL_KEY, DIK_PERIOD, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_LOOK_RIGHT + CTRL_SECONDARY, CONTROL_JOY, CONTROL_JOY_POV + 0x10 + CONTROL_POV_RIGHT, 0,
                   CONTROL_AXIS_FULL);
        SetBinding(CTRL_CLUTCH, CONTROL_KEY, DIK_X, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_CLUTCH + CTRL_SECONDARY, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_GEARING, CONTROL_KEY, DIK_M, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_CAMERA, CONTROL_KEY, DIK_C, 0, CONTROL_AXIS_FULL);
        SetBinding(CTRL_REPLACE_CAR, CONTROL_KEY, DIK_NUMPAD0, 0, CONTROL_AXIS_FULL);
    }
}

// FUNCTION: GAME 0x607a60
void G_LoadSaveClass::ApplyControls()
{
    controls.ClearControls();
    for (int i = 0; i < NUM_CONTROLS; i++)
        controls.SetControl(&bindings[i], i);
    controls.SetMouseScale(400, 400, 400);
}

// The blood level and the profile's other values aren't in either group
// FUNCTION: GAME 0x607ab0
void G_LoadSaveClass::DefaultSettings(uint flags)
{
    if (flags & DEFAULTS_CLEAR)
        memset(&settings, 0, sizeof(settings));
    if (flags & DEFAULTS_OTHER)
    {
        settings.aiming_sensitivity_x = .5f;
        settings.aiming_sensitivity_y = .5f;
        settings.aiming_speed_x = .5f;
        settings.cursor = CURSOR_CROSSHAIR;
        settings.speedometer_mph = true;
        settings.side_roll = true;
        settings.mouse_inverted = false;
        settings.subtitles = false;
        settings.steering_linearity = .2f;
        settings.ff_volume = 1.0f;
        settings.ff_suspension = .703f;
        settings.ff_collisions = 1.0f;
        settings.ff_revs = .117f;
        settings.ff_tire_grip = 1.0f;
        settings.ff_player_movement = 1.0f;
    }
    if (flags & DEFAULTS_AUDIO_VIDEO)
    {
        settings.gamma = 0.0f;
        settings.sound_volume = 1.0f;
        settings.music_volume = 1.0f;
        settings.car_volume = 1.0f;
        settings.speech_volume = 1.0f;
    }
}

// G_Inventory's functions (actors/g_inventory.h), which lie in this file's range on the PC
#pragma auto_inline(off)

// STUB: GAME 0x6081d0
bool G_Inventory::SelectByID(uint id, std::vector<S_GameItem> *dropped)
{
    StubLog("G_Inventory::SelectByID");
    return false;
}

// STUB: GAME 0x608520
bool G_Inventory::HaveSomeWeapon(bool in_hand, uint flags)
{
    StubLog("G_Inventory::HaveSomeWeapon");
    return false;
}

// STUB: GAME 0x608a20
bool G_Inventory::IsIn(uint id, bool unknown)
{
    StubLog("G_Inventory::IsIn");
    return false;
}

// STUB: GAME 0x608aa0
bool G_Inventory::HaveSpaceFor(uint id, bool unknown)
{
    StubLog("G_Inventory::HaveSpaceFor");
    return true;
}

// STUB: GAME 0x6092b0
bool G_Inventory::Insert(S_GameItem *item, bool unknown)
{
    StubLog("G_Inventory::Insert");
    return false;
}

// STUB: GAME 0x6095e0
bool G_Inventory::Remove(S_GameItem *item, std::vector<S_GameItem> *dropped)
{
    StubLog("G_Inventory::Remove");
    return false;
}

// STUB: GAME 0x609920
void G_Inventory::SetCanHaveBigWeapon(bool can, std::vector<S_GameItem> *dropped)
{
    StubLog("G_Inventory::SetCanHaveBigWeapon");
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

// PS2's returns none for an index out of range
// FUNCTION: GAME 0x60a460
S_car_index *G_CarDatabase::GetCar(uint car)
{
    if (!cars)
        return &none;
    uint index = car & CAR_INDEX_MASK;
    if (index >= num_cars)
        return cars;
    return &cars[index];
}

// FUNCTION: GAME 0x60a4e0
char *G_CarDatabase::GetPlayerModel(uint id)
{
    if (players)
    {
        uint index = id & CAR_INDEX_MASK;
        if (index < num_players)
            return players[index].model;
    }
    // STRING: GAME 0x638144
    return "-";
}

// FUNCTION: GAME 0x60a6a0
bool G_CarDatabase::GetCarModel(uint car, char *name, bool second)
{
    uint index = car & CAR_INDEX_MASK;
    if (index >= num_cars)
        return false;
    if (second)
        // STRING: GAME 0x6396b8
        sprintf(name, "%s%02d.i3d", cars[index].id, (car >> CAR_VARIANT_SHIFT) + 10);
    else
        sprintf(name, "%s%02d.i3d", cars[index].id, car >> CAR_VARIANT_SHIFT);
    return true;
}

// The level's bits are cleared before they are read, so every car has level 0 (PS2's stores 0 too)
// FUNCTION: GAME 0x60a730
bool G_CarDatabase::GetCarParams(uint car, uint *index, uint *level, uint *variant)
{
    if (index)
        *index = 0;
    if (level)
        *level = 0;
    if (variant)
        *variant = 0;
    if ((car & CAR_INDEX_MASK) >= num_cars)
        return false;
    if (index)
        *index = car & CAR_INDEX_MASK;
    car &= ~(0xff << CAR_LEVEL_SHIFT);
    if (level)
    {
        *level = (car >> CAR_LEVEL_SHIFT) & 0xff;
        if (*level > 2)
            *level = 0;
    }
    if (variant)
    {
        *variant = car >> CAR_VARIANT_SHIFT;
        if (*variant > 4)
            *variant = 0;
    }
    return true;
}

// The first car, index 0, is never found (as GetIdxByModelLong's)
// FUNCTION: GAME 0x60a800
uint G_CarDatabase::GetIdxByModel(const char *model)
{
    for (uint i = 1; i < num_cars; i++)
    {
        if (!strcmpi(cars[i].id, model))
            return i;
    }
    return 0;
}

// The file's name without its path is the car's id, the variant's two digits and ".i3d" (PS2's takes ids without the
// extension). The first car, index 0, is never found.
// FUNCTION: GAME 0x60a850
uint G_CarDatabase::GetIdxByModelLong(const char *model, uint &variant)
{
    const char *name = strrchr(model, '\\');
    if (name)
        name++;
    else
        name = model;
    int len = strlen(name);
    if (len < 7)
        return 0;
    if (len > 31)
        return 0;
    variant = (uchar)name[len - 5] - '0';
    if (variant > 4)
        variant = 0;
    char id[32];
    strncpy(id, name, sizeof(id));
    id[len - 6] = 0;
    for (uint i = 1; i < num_cars; i++)
    {
        if (!strcmpi(cars[i].id, id))
            return i;
    }
    return 0;
}

// The level's bits are cleared after they are set, as GetCarParams clears them (PS2's the same)
// FUNCTION: GAME 0x60a910
uint G_CarDatabase::GetIDByActor(C_car *car)
{
    if (!car)
        return 0;
    const char *file_name = ((I3D_model *)car->frame)->file_name;
    uint variant;
    uint id = GetIdxByModelLong(file_name ? file_name : "", variant);
    id |= variant << CAR_VARIANT_SHIFT;
    int level = car->GetActualLevel();
    if (level < 0)
        level = 0;
    id |= level << CAR_LEVEL_SHIFT;
    return id & ~(0xff << CAR_LEVEL_SHIFT);
}

// FUNCTION: GAME 0x60b050
G_Stream::G_Stream()
{
    unknown_2d = false;
    unknown_2c = false;
    unknown_34 = 0;
    size = 0;
    file = NULL;
    buffer = NULL;
    key1 = STREAM_KEY1;
    key2 = STREAM_KEY2;
}

// FUNCTION: GAME 0x60b080
G_Stream::~G_Stream()
{
    Close();
}

// FUNCTION: GAME 0x60b090
void G_Stream::SetParams(uint magic, bool crypt, bool unknown)
{
    this->magic = magic;
    this->crypt = crypt;
    unknown_1 = unknown;
}

// The head isn't checked (PS2's compares its first dword with the magic)
// FUNCTION: GAME 0x60b0b0
bool G_Stream::OpenRead(char *name)
{
    key1 = STREAM_KEY1;
    key2 = STREAM_KEY2;
    // STRING: GAME 0x6396e4
    file = fopen(name, "rb");
    if (!file)
        return false;
    fread(head, 1, sizeof(head), file);
    return true;
}

// PS2's also keeps the head for its memory card
// FUNCTION: GAME 0x60b100
bool G_Stream::OpenWrite(char *name)
{
    key1 = STREAM_KEY1;
    key2 = STREAM_KEY2;
    memset(head, 0, sizeof(head));
    head[0] = magic;
    head[2] = 1;
    // STRING: GAME 0x636180
    file = fopen(name, "wb");
    if (!file)
        return false;
    fwrite(head, 1, sizeof(head), file);
    return true;
}

// FUNCTION: GAME 0x60b170
bool G_Stream::Close()
{
    if (file)
        fclose(file);
    file = NULL;
    return unknown_2d || unknown_2c ? true : false;
}

// FUNCTION: GAME 0x60b260
void G_Stream::Crypt(void *data, uint size)
{
    uint *p = (uint *)data;
    uint count = size / 4;
    for (uint i = 0; i < count; i++)
    {
        key2 += p[i];
        p[i] ^= key1;
        key1 += key2;
    }
}

// FUNCTION: GAME 0x60b2b0
void G_Stream::Decrypt(void *data, uint size)
{
    uint *p = (uint *)data;
    uint count = size / 4;
    for (uint i = 0; i < count; i++)
    {
        p[i] ^= key1;
        key2 += p[i];
        key1 += key2;
    }
}

// FUNCTION: GAME 0x60b2f0
uint G_Stream::WriteAdr(void *adr, uint size)
{
    if (!size)
        return 0;
    if (!crypt)
        return fwrite(adr, 1, size, file);
    uchar *data = new uchar[size];
    memcpy(data, adr, size);
    Crypt(data, size);
    uint written = fwrite(data, 1, size, file);
    delete[] data;
    return written;
}

// FUNCTION: GAME 0x60b380
uint G_Stream::ReadAdr(void *adr, uint size)
{
    if (!size)
        return 0;
    if (!crypt)
        return fread(adr, 1, size, file);
    uchar *data = new uchar[size];
    uint read = fread(data, 1, size, file);
    Decrypt(data, size);
    memcpy(adr, data, size);
    delete[] data;
    return read;
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

// FUNCTION: GAME 0x60b690
void G_LoadSaveClass::ObjectivesClear()
{
    objectives.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x60b6c0
void G_LoadSaveClass::ObjectivesAdd(uint id)
{
    StubLog("G_LoadSaveClass::ObjectivesAdd");
}

// STUB: GAME 0x60b8a0
void G_LoadSaveClass::ObjectivesRemove(uint id)
{
    StubLog("G_LoadSaveClass::ObjectivesRemove");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x60b940
void G_LoadSaveClass::GarageClear()
{
    for (int i = 0; i < MAX_GARAGE_CARS; i++)
    {
        garage[i].car = 0;
        garage[i].actor = NULL;
        unknown_168[i] = 0;
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x60b970
void G_LoadSaveClass::AddCarByIdx(uint car, uint unknown1, uint unknown2)
{
    StubLog("G_LoadSaveClass::AddCarByIdx");
}

// STUB: GAME 0x60b9f0
void G_LoadSaveClass::AddCarByActor(C_car *car)
{
    StubLog("G_LoadSaveClass::AddCarByActor");
}

// STUB: GAME 0x60baa0
void G_LoadSaveClass::RemoveCar(char *model, int level)
{
    StubLog("G_LoadSaveClass::RemoveCar");
}

#pragma auto_inline(on)

// Empty on the PC (PS2's G_GarageClass has them), so without markers: the original's calls go to a folded copy of an
// empty function (0x4aabe0)
void G_LoadSaveClass::GenerateCars()
{
}

void G_LoadSaveClass::ReleaseCars()
{
}

// The garage's cars into the mission's garage (PS2 G_GarageClass::GameInit; the garage is in this class on PC), in a
// mission that has its region, oblast_garaze_a: each car on the next of the parking places park00 to park24, its back
// on the place, named g_car_<slot>. The car is the actor the mission kept of the car's model, else a new one; the
// model's frame is positioned and set up the same way in both cases (written twice, as PS2's).
// FUNCTION: GAME 0x60baf0
void G_LoadSaveClass::GarageGameInit()
{
    unknown_1cc = false;
    unknown_1cd = false;
    int park = 0;
    // STRING: GAME 0x639738
    if (!act_mission->scene->FindFrame("oblast_garaze_a", ENUMF_ALL) &&
        // STRING: GAME 0x63971c
        !act_mission->scene->FindFrame("watercity.oblast_garaze_a", ENUMF_ALL))
        return;
    for (int i = 0; i < MAX_GARAGE_CARS; i++)
    {
        if (!garage[i].car)
            continue;
        I3D_frame *park_frame = NULL;
        while (park < MAX_GARAGE_CARS)
        {
            char park_name[128];
            // STRING: GAME 0x639710
            sprintf(park_name, "park%02d", park);
            park_frame = act_mission->scene->FindFrame(park_name, ENUMF_ALL);
            park++;
            if (park_frame)
                break;
        }
        if (!park_frame)
            return;
        if (garage[i].actor)
            continue;
        uint level;
        mafiaCarDatabase.GetCarParams(garage[i].car, NULL, &level, NULL);
        char model_name[128];
        mafiaCarDatabase.GetCarModel(garage[i].car, model_name, false);
        C_car *car = (C_car *)act_mission->FrmActBuff_GetActor(model_name, ACTOR_CAR);
        if (!car)
        {
            I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            model_cache.Open(model, model_name, 0, NULL, NULL, NULL);
            model->LinkTo(act_mission->scene->primary_sector, 0);
            S_vector offset = park_frame->GetWorldDir() * model->GetHRBoundVolume().bbox.max.z;
            model->SetPos(park_frame->GetWorldPos() + offset);
            model->SetDir(park_frame->GetWorldDir(), 0.0f);
            char car_name[32];
            // STRING: GAME 0x639704
            sprintf(car_name, "g_car_%d", i);
            model->SetName(car_name);
            model->Update();
            I3D_scene *scene = act_mission->scene;
            scene->SetFrameSectorPos(model, model->GetWorldPos());
            car = (C_car *)act_mission->CreateActor(ACTOR_CAR);
            if (car->Init(model))
            {
                car->SetActualLevel(level);
                car->GameInit();
                car->CalmVehicle(40);
                act_mission->game->unknown_fc.push_back((C_actor *&)car);
                garage[i].actor = car;
                mafiaIndicators.RadarAddCar(car, 0xffc0c0c0);
                model->Release();
            }
            else
            {
                delete car;
                model->Release();
            }
        }
        else
        {
            car->Deactivate(0);
            I3D_frame *frame = car->frame;
            if (frame)
            {
                frame->LinkTo(act_mission->scene->primary_sector, 0);
                S_vector offset = park_frame->GetWorldDir() * frame->GetHRBoundVolume().bbox.max.z;
                frame->SetPos(park_frame->GetWorldPos() + offset);
                frame->SetDir(park_frame->GetWorldDir(), 0.0f);
                char car_name[32];
                sprintf(car_name, "g_car_%d", i);
                frame->SetName(car_name);
                frame->Update();
                I3D_scene *scene = act_mission->scene;
                scene->SetFrameSectorPos(frame, frame->GetWorldPos());
            }
            car->Reset(0.0f, true);
            car->CalmVehicle(40);
            act_mission->game->unknown_fc.push_back((C_actor *&)car);
            garage[i].actor = car;
            mafiaIndicators.RadarAddCar(car, 0xffc0c0c0);
        }
    }
}

// The smaller and the larger of two vectors' members (the names are ours; PS2's code calls Min and Max for each)
static inline S_vector MinVector(const S_vector &a, const S_vector &b)
{
    return S_vector(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z));
}

static inline S_vector MaxVector(const S_vector &a, const S_vector &b)
{
    return S_vector(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z));
}

// PS2 G_GarageClass::GameDone's, the PC's own: the cars kept are the garage's, the mission's (cars, which stay where
// they are) and the mission's cars standing in the garage's box (between the regions oblast_garaze_a and
// oblast_garaze_b), but a frame named "nesahat" ("don't touch") and the trucks, and with unknown_1cc or unknown_1cd
// set unknown_1e8. Past GARAGE_ROOM the menu chooses which go; then the garage's actors leave the mission, kept for the
// next one unless they are its own, the kept cars fill the free slots, and a frame with "please wait" (text 0x581) is
// drawn. Open: the loop over cars at the start has an empty body (PS2's forgets its last stolen car there), and the
// last search through cars reads cars->end() without a test of cars, as the original does.
// FUNCTION: GAME 0x60c640
void G_LoadSaveClass::GarageGameDone(std::vector<C_car *> *cars)
{
    std::vector<S_garage_item> items;
    C_car **it;
    S_garage_item *p;
    if (cars)
    {
        for (it = cars->begin(); it < cars->end(); it++)
            ;
    }
    I3D_frame *area_a = act_mission->scene->FindFrame("oblast_garaze_a", ENUMF_ALL);
    if (!area_a)
        area_a = act_mission->scene->FindFrame("watercity.oblast_garaze_a", ENUMF_ALL);
    // STRING: GAME 0x63976c
    I3D_frame *area_b = act_mission->scene->FindFrame("oblast_garaze_b", ENUMF_ALL);
    if (!area_b)
        // STRING: GAME 0x639750
        area_b = act_mission->scene->FindFrame("watercity.oblast_garaze_b", ENUMF_ALL);
    S_vector box_min, box_max;
    box_min = box_max = S_vector(0.0f, 0.0f, 0.0f);
    if (area_a && area_b)
    {
        box_min = MinVector(area_a->GetWorldPos(), area_b->GetWorldPos());
        box_max = MaxVector(area_a->GetWorldPos(), area_b->GetWorldPos());
    }

    // The garage's cars: those without an actor, the usable ones and the mission's
    for (int i = 0; i < MAX_GARAGE_CARS; i++)
    {
        C_car *actor = garage[i].actor;
        if (!actor)
        {
            if (garage[i].car)
            {
                S_garage_item item;
                item.car = garage[i].car & ~(0xff << CAR_LEVEL_SHIFT);
                item.actor = actor;
                item.slot = i;
                items.push_back(item);
            }
            continue;
        }
        bool mission_car = false;
        if (cars)
        {
            for (it = cars->begin(); it < cars->end(); it++)
            {
                if (*it == actor)
                    break;
            }
            mission_car = it < cars->end();
        }
        if (!mission_car && !actor->IsCarUsable(1))
            continue;
        S_garage_item item;
        item.car = garage[i].car & ~(0xff << CAR_LEVEL_SHIFT);
        item.actor = actor;
        item.slot = i;
        items.push_back(item);
        RaceCarsEnable(mafiaCarDatabase.GetCar(item.car & CAR_INDEX_MASK)->unknown_84);
    }

    // The mission's cars in the garage's box, of its actors and of the garage's cars
    C_game *game = act_mission->game;
    for (C_actor **a = game->unknown_ec.begin(); a < game->unknown_ec.end(); a++)
    {
        if ((*a)->type != ACTOR_CAR)
            continue;
        S_vector pos = (*a)->pos;
        if (pos.x < box_min.x || pos.z < box_min.z || pos.x > box_max.x || pos.z > box_max.z)
            continue;
        if (!(*a)->frame->IsOn())
            continue;
        if (((C_car *)*a)->unknown_20b2)
            continue;
        // STRING: GAME 0x639748
        if (strstr((*a)->frame->GetName(), "nesahat"))
            continue;
        if (!((C_car *)*a)->IsCarUsable(1))
            continue;
        for (p = items.begin(); p < items.end(); p++)
        {
            if (p->actor == *a)
                break;
        }
        if (p < items.end())
            continue;
        S_garage_item item;
        item.car = mafiaCarDatabase.GetIDByActor((C_car *)*a);
        uint index = item.car & CAR_INDEX_MASK;
        if (index == CAR_TRUCK_FLATBED || index == CAR_TRUCK_COVERED || index == CAR_TRUCK)
            continue;
        item.car &= ~(0xff << CAR_LEVEL_SHIFT);
        item.actor = (C_car *)*a;
        item.slot = -1;
        items.push_back(item);
        RaceCarsEnable(mafiaCarDatabase.GetCar(item.car & CAR_INDEX_MASK)->unknown_84);
    }
    game = act_mission->game;
    for (C_actor **b = game->unknown_fc.begin(); b < game->unknown_fc.end(); b++)
    {
        C_car *car = (C_car *)*b;
        if (car->type != ACTOR_CAR)
            continue;
        S_vector pos = car->pos;
        if (pos.x < box_min.x || pos.z < box_min.z || pos.x > box_max.x || pos.z > box_max.z)
            continue;
        if (!car->frame->IsOn())
            continue;
        if (car->unknown_20b2)
            continue;
        if (strstr(car->frame->GetName(), "nesahat"))
            continue;
        if (!car->IsCarUsable(1))
            continue;
        for (p = items.begin(); p < items.end(); p++)
        {
            if (p->actor == car)
                break;
        }
        if (p < items.end())
            continue;
        S_garage_item item;
        item.car = mafiaCarDatabase.GetIDByActor(car);
        uint index = item.car & CAR_INDEX_MASK;
        if (index == CAR_TRUCK_FLATBED || index == CAR_TRUCK_COVERED || index == CAR_TRUCK)
            continue;
        item.car &= ~(0xff << CAR_LEVEL_SHIFT);
        item.actor = car;
        item.slot = -1;
        items.push_back(item);
        RaceCarsEnable(mafiaCarDatabase.GetCar(item.car & CAR_INDEX_MASK)->unknown_84);
    }

    if ((unknown_1cc || unknown_1cd) && unknown_1e8 && !unknown_1e8->unknown_20b2 && unknown_1e8->IsCarUsable(1))
    {
        S_garage_item item;
        item.car = mafiaCarDatabase.GetIDByActor(unknown_1e8);
        uint index = item.car & CAR_INDEX_MASK;
        if (index != CAR_TRUCK_FLATBED && index != CAR_TRUCK_COVERED && index != CAR_TRUCK &&
            index != CAR_RACING_BUGATTI)
        {
            item.actor = unknown_1e8;
            item.car &= ~(0xff << CAR_LEVEL_SHIFT);
            item.slot = -1;
            for (p = items.begin(); p < items.end(); p++)
            {
                if (p->actor == item.actor)
                    break;
            }
            if (p >= items.end())
            {
                items.push_back(item);
                RaceCarsEnable(mafiaCarDatabase.GetCar(item.car & CAR_INDEX_MASK)->unknown_84);
            }
        }
    }

    // The player chooses which go, until the rest fits
    std::vector<uint> discard;
    discard.clear();
    if (act_mission->game->unknown_3665)
    {
        int extra = 0;
        if (unknown_1cc)
            extra = 1;
        while (items.size() > discard.size() - extra + GARAGE_ROOM)
        {
            GM_Menu *menu = new GM_GarageEndOfMission(&items, &discard, cars);
            GM_Menu::ExecuteMenu(menu, false, true);
        }
    }
    if (cars)
    {
        for (uint j = 0; j < items.size(); j++)
        {
            for (it = cars->begin(); it < cars->end(); it++)
            {
                if (*it == items[j].actor)
                    discard.push_back(j);
            }
        }
    }
    uint k;
    for (k = 0; k < items.size(); k++)
    {
        uint *d;
        for (d = discard.begin(); d < discard.end(); d++)
        {
            if (*d == k)
                break;
        }
        if (d >= discard.end())
            continue;
        if (cars)
        {
            for (it = cars->begin(); it < cars->end(); it++)
            {
                if (*it == items[k].actor)
                    break;
            }
            if (it < cars->end())
                continue;
        }
        if (items[k].slot != -1)
            garage[items[k].slot].car = 0;
        items[k].car = 0;
        items[k].actor = NULL;
        items[k].slot = 0;
    }
    for (p = items.begin(); p < items.end(); p++)
    {
        if (!p->car && !p->actor && !p->slot)
        {
            items.erase(p);
            p--;
        }
    }

    // The garage's actors leave the mission, but the mission's cars
    for (int n = 0; n < MAX_GARAGE_CARS; n++)
    {
        if (!garage[n].actor)
            continue;
        if (cars)
        {
            for (it = cars->begin(); it < cars->end(); it++)
            {
                if (*it == garage[n].actor)
                    break;
            }
            if (it < cars->end())
                continue;
        }
        for (int seat = 0; seat < garage[n].actor->GetNumUnknownCa0(); seat++)
        {
            C_human *owner = garage[n].actor->GetOwner(seat);
            if (owner)
            {
                if (owner->type == ACTOR_ENTITY)
                    owner->Intern_FromCar(garage[n].actor->frame);
                if (owner->type == ACTOR_PLAYER)
                    owner->Intern_FromCar(garage[n].actor->frame);
            }
        }
        game = act_mission->game;
        for (C_actor **a = game->unknown_ec.begin(); a < game->unknown_ec.end(); a++)
        {
            if (*a == garage[n].actor)
            {
                game->unknown_ec.erase(a);
                a--;
            }
        }
        game = act_mission->game;
        for (C_actor **b = game->unknown_fc.begin(); b < game->unknown_fc.end(); b++)
        {
            if (*b == garage[n].actor)
            {
                game->unknown_fc.erase(b);
                b--;
            }
        }
        mafiaIndicators.RadarRemoveCar(garage[n].actor);
        act_mission->game->InvalidateActor(garage[n].actor, false);
        if (!act_mission->IsOwnActor(garage[n].actor))
        {
            garage[n].actor->unknown_220c = true;
            act_mission->RemoveActor(garage[n].actor);
            act_mission->FrmActBuff_StoreActor(garage[n].actor, false);
        }
        garage[n].actor = NULL;
    }

    // The kept cars new to the garage take its free slots
    std::vector<int> free_slots;
    for (int s = 0; s < MAX_GARAGE_CARS; s++)
        free_slots.push_back(s);
    for (p = items.begin(); p < items.end(); p++)
    {
        if (p->slot == -1)
            continue;
        for (int *f = free_slots.begin(); f < free_slots.end(); f++)
        {
            if (*f == p->slot)
                free_slots.erase(f);
        }
    }
    for (p = items.begin(); p < items.end(); p++)
    {
        if (p->slot != -1)
            continue;
        int slot = *free_slots.begin();
        free_slots.erase(free_slots.begin());
        garage[slot].car = p->car;
        if (cars)
        {
            for (it = cars->begin(); it < cars->end(); it++)
            {
                if (*it == p->actor)
                    break;
            }
        }
        if (it < cars->end())
            garage[slot].actor = p->actor;
        else
            garage[slot].actor = NULL;
    }

    if (act_mission->game->unknown_3665 && unknown_1cc)
        GarageShowResultsProc();
    igraph->ResetRenderProps();
    igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    igraph->BeginScene();
    igraph->SetState(DXS_ALPHA, ALPHASTATE_BLEND);
    mafiaIndicators.OutText(textDatabase.GetText(0x581), mafiaIndicators.screen_scale.x * 780.0f + levyOkraj.x,
        mafiaIndicators.screen_scale.y * 545.0f, mafiaIndicators.screen_scale.x * 0.0f,
        mafiaIndicators.screen_scale.y * 40.0f, (uint)-1, OUTTEXT_RIGHT, 1, NULL);
    igraph->EndScene();
    igraph->Present();
    unknown_1cc = false;
    unknown_1cd = false;
    unknown_1e8 = NULL;
}

// FUNCTION: GAME 0x60d480
const char *GM_GarageEndOfMission::OnBeforeCreate()
{
    // STRING: GAME 0x63977c
    return "garageendofmission";
}

#pragma auto_inline(off)

// STUB: GAME 0x60d490
void G_LoadSaveClass::CarManager()
{
    StubLog("G_LoadSaveClass::CarManager");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x60da70
const char *GM_GarageEndOfMission::vfunc_12()
{
    // STRING: GAME 0x639790
    return "garagemanager";
}

// FUNCTION: GAME 0x60dc80
void G_LoadSaveClass::GarageAddLastStolen()
{
    unknown_1cd = true;
}

// FUNCTION: GAME 0x60dc90
void G_LoadSaveClass::GarageShowResults()
{
    unknown_1cc = true;
}

#pragma auto_inline(off)

// STUB: GAME 0x60dca0
void G_LoadSaveClass::GarageShowResultsProc()
{
    StubLog("G_LoadSaveClass::GarageShowResultsProc");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x60ded0
void G_LoadSaveClass::CarCantGetOut(C_car *car)
{
    cant_get_out = car;
}

// The letter's place in the alphabet, the bit 0 for another character
// FUNCTION: GAME 0x60dee0
void G_LoadSaveClass::EnableSteal(uchar c)
{
    uint bit = 0;
    if (c >= 'A' && c <= 'Z')
        bit = c - 'A';
    if (c >= 'a' && c <= 'z')
        bit = c - 'a';
    unknown_98 |= 1 << bit;
}

#pragma auto_inline(off)

// STUB: GAME 0x60df20
bool G_LoadSaveClass::CanSteal(uchar *model)
{
    StubLog("G_LoadSaveClass::CanSteal");
    return false;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x60df60
bool G_LoadSaveClass::IsCarIn(C_car *car)
{
    StubLog("G_LoadSaveClass::IsCarIn");
    return false;
}

#pragma auto_inline(on)

// EnableSteal's for the free ride's cars
// FUNCTION: GAME 0x60df90
void G_LoadSaveClass::FreerideEnableCar(uchar c)
{
    uint bit = 0;
    if (c >= 'A' && c <= 'Z')
        bit = c - 'A';
    if (c >= 'a' && c <= 'z')
        bit = c - 'a';
    unknown_9c |= 1 << bit;
}

// PS2's sets the sound driver's volume, the videos' and the screen's offset. The cars' volume is relative to the sounds',
// the gamma ramp a power curve, steeper with more gamma.
// FUNCTION: GAME 0x60dfe0
void G_LoadSaveClass::Refresh(uint flags)
{
    if (flags & REFRESH_SOUND && isound)
        isound->SetVolume(settings.sound_volume);
    if (flags & REFRESH_MUSIC)
        act_mission->game->IShowSetVolume();
    if (flags & (REFRESH_SOUND | REFRESH_CARS))
        C_Vehicle::SetSoundVolume(settings.car_volume / (settings.sound_volume + .01f));
    if (flags & REFRESH_GAMMA)
    {
        LS3DGAMMARAMP ramp;
        float exponent = 1.0f / (settings.gamma + settings.gamma + 1.0f);
        for (int i = 0; i < 0x100; i++)
        {
            uint value = (uint)(pow(i * (1.0 / 255), exponent) * 255.0 + .5);
            if (value > 0xff)
                value = 0xff;
            ramp.red[i] = ramp.green[i] = ramp.blue[i] = value << 8;
        }
        igraph->SetGammaRamp(&ramp, 0);
    }
    if (flags & REFRESH_FORCE_FEEDBACK)
        C_car::SetForceFeedback(settings.ff_volume, settings.ff_tire_grip, settings.ff_collisions, settings.ff_suspension,
            settings.ff_revs);
    if (settings.camera < 1 || settings.camera >= 4)
        settings.camera = 1;
    if (settings.unknown_34 < 7 || settings.unknown_34 >= 0x10)
        settings.unknown_34 = 7;
}

#pragma auto_inline(off)

// STUB: GAME 0x60e710
void G_LoadSaveClass::ExecuteCredits()
{
    StubLog("G_LoadSaveClass::ExecuteCredits");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x60e910
void G_LoadSaveClass::FreerideEnable(uint level)
{
    freeride_enabled = level;
}

// FUNCTION: GAME 0x60e920
void G_LoadSaveClass::FreerideStoreA(uint a, uint b, float c, float d, float e)
{
    freeride.unknown_0 = a;
    freeride.unknown_4 = b;
    freeride.unknown_c[0] = c;
    freeride.unknown_c[1] = d;
    freeride.unknown_c[2] = e;
}

// FUNCTION: GAME 0x60e960
void G_LoadSaveClass::FreerideStoreB(uint state)
{
    freeride.state = state;
}

#pragma auto_inline(off)

// STUB: GAME 0x60e980
void G_LoadSaveClass::FreerideSetupFromScript()
{
    StubLog("G_LoadSaveClass::FreerideSetupFromScript");
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
