// The saved games and profiles (PS2 g_loadsave.cpp; the PC's file is the one at 0x604e60-0x60f5bc,
// docs/game-files.md), with the car database and the save files' G_Stream, as PS2's. Written so far: what InitSystem
// and CloseSystem reach (Init, Close, the constructors and destructors, AllDefault, ProfileEnumFiles, ProfileEnumRelease,
// ProfileLoad, ProfileSave, SaveGameNewGame, the PC's controls and options defaults, ApplyControls, FindBinding, the car
// database's Init and Close, G_Stream's reading and writing, RaceCarsEnable, RaceCarsDisable, GarageClear, Refresh,
// FreerideStoreA and FreerideStoreB) and LanguageSelect; the rest of what they and main call is stubs.

#include "ui/g_loadsave.h"
#include "ui/g_indicators.h"
#include "ui/g_m_start.h"
#include "ui/g_textdb.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "vehicles/C_car.h"
#include "vehicles/C_Vehicle.h"
#include "g_material.h"

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
                        unknown_1f0 = profile.unknown_44;
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
// before unknown_44's (three other orders give the same code)
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
        profile.unknown_44 = unknown_1f0;
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
