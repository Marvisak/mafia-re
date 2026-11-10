// The saved games and profiles (PS2 g_loadsave.cpp; the PC's file is the one at 0x604e60-0x60f5bc,
// docs/game-files.md), with the car database and the save files' G_Stream, as PS2's, and on the PC also the PC's
// controls and options, G_Inventory's functions (PS2 g_inventory.cpp), the garage (PS2 g_garage.cpp's
// G_GarageClass, in G_LoadSaveClass here), the replay, the credits and the free ride's setup. Written whole.

#include "ui/g_loadsave.h"
#include "ui/g_indicators.h"
#include "ui/g_m_func.h"
#include "ui/g_m_start.h"
#include "ui/g_textdb.h"
#include "ui/racing.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "actors/C_human.h"
#include "ai/C_person_traffic.h"
#include "ai/jp_ai_main.h"
#include "vehicles/C_car.h"
#include "vehicles/C_trf_c.h"
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

// Data nothing in the code reads (the name is a placeholder): 0, 0, 1, then a list of small numbers ending with
// 0xffff
// GLOBAL: GAME 0x639324
uint unknown_639324[] = {
    0x0, 0x0, 0x1, 0x6c, 0x6d, 0x6e, 0x6f, 0x2, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb,
    0xc, 0xd, 0x7a, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
    0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
    0x70, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x6b, 0xffff,
};

// The text database's ids of the controls' labels on the controls menu's pages, by CTRL_* (the name is ours): the
// secondary bindings have their primary's, 0x20 none
// GLOBAL: GAME 0x6394f0
uint controlTextIDs[NUM_CONTROLS] = {
    0x12d, 0x12d, 0x12e, 0x12e, 0x12f, 0x12f, 0x130, 0x130, 0x131, 0x131,
    0x133, 0x133, 0x134, 0x134, 0x135, 0x135, 0x137, 0x137, 0x138, 0x138,
    0x13d, 0x13d, 0x13d, 0x13d, 0x13f, 0x140, 0x141, 0x142, 0x143, 0x144,
    0x145, 0x157, 0, 0x14a, 0x14a, 0x14b, 0x14b, 0x14c, 0x14c, 0x14d,
    0x14d, 0x14e, 0x14e, 0x14f, 0x14f, 0x151, 0x151, 0x152, 0x152, 0x153,
    0x153, 0x154, 0x154, 0x155, 0x155, 0x156, 0x156, 0x150, 0x136, 0x158,
};

// GetControlTextID's for an index outside the table (the name is ours)
#define TEXT_MAFIA 0x64

// G_CreditsStruct::type, how a line of the credits is laid out (the names are ours): a title is the text database's
// text, the others the line's own. ParseCredits gives each its height.
#define CREDIT_HEADING 'A'              // a title, 30 high, centered
#define CREDIT_ROLE 'B'                 // a role and its actor beside each other, 25 high; any other letter too
#define CREDIT_TITLE_NAME 'C'           // a title and a name under it, centered, 25 high
#define CREDIT_TITLE_BESIDE 'D'         // a title and a name beside each other, the name 20 high
#define CREDIT_TWO_NAMES 'E'            // two names, each centered in its half of the screen, 20 high
#define CREDIT_TITLE 'F'                // a title, 20 high, centered
#define CREDIT_NAME 'G'                 // a name, 20 high, centered
#define CREDIT_HEADING_LINE 'H'         // a title on the line, 30 high, centered (PC only)

// A line of the credits (PS2's name; the members' names are ours), 0x8c bytes: tables\credits.def is an array of them
struct G_CreditsStruct
{
    uint type;                          // CREDIT_*
    float y;                            // 0x4, the line's place in the credits, which ParseCredits sets
    // 0x8, in the file the text database's id, which ParseCredits replaces with its text (NULL for none)
    const char *title;
    char text[0x40];                    // 0xc
    char text2[0x40];                   // 0x4c
};

// The credits ExecuteCredits read from tables\credits.def and their number (PS2's names)
// GLOBAL: GAME 0x6c4534
G_CreditsStruct *g_credits;
// GLOBAL: GAME 0x6c4530
uint g_numOfCredits;

// The credits' helpers (PS2 names), which ExecuteCredits calls
void RenderOneCredit(G_CreditsStruct *credit, float time);
void ParseCredits();

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
    freeride_cars = 0;
    last_stolen = 0;
    mafiaLoadSave.AllDefault();
    mafiaIndicators.Create();
    LoadMaterialsFromBin();
    mafiaLoadSave.ApplyControls();
    mafiaIndicators.ResolutionChanged();
    mafiaIndicators.show_flags = 0;
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

// Init's choice again, the profile saved first unless it mustn't be
// FUNCTION: GAME 0x605070
bool G_LoadSaveClass::ProfileChange()
{
    if (!profileIgnoreSave)
        mafiaLoadSave.ProfileSave();
    ProfileEnumFiles();
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

// FUNCTION: GAME 0x605140
G_LoadSaveClass::G_LoadSaveClass()
{
    memset(bindings, 0, sizeof(bindings));
    memset(&settings, 0, sizeof(settings));
    memset(unknown_520, 0, sizeof(unknown_520));
    memset(unknown_5bc, 0, sizeof(unknown_5bc));
    replay_mode = REPLAY_OFF;
    progress = NULL;
    load_failed = true;
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
    mafiaLoadSave.settings.car_camera = 7;
    freeride_enabled = FREERIDE_SMALL_CITY;
    freeride_cars = 0;
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
    load_failed = true;
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
                        freeride_cars = profile.freeride_cars;
                        load_failed = false;
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
    if (load_failed)
        AllDefault();
    Refresh(REFRESH_ALL);
    FreerideStoreA(1, 0, 1.0f, 1.0f, 1.0f);
    FreerideStoreB(0);
    skip_save = false;
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
        profile.freeride_cars = freeride_cars;
        stream.WriteAdr(&profile, sizeof(profile));
        stream.WriteAdr(bindings, sizeof(bindings));
        stream.WriteAdr(&settings, sizeof(settings));
        stream.WriteAdr(unknown_520, sizeof(unknown_520));
        stream.Close();
    }
}

// The files of the numbers the text database names (TEXT_SAVEGAME_NAMES), of version SAVEGAME_VERSION, each head
// numbered after its file. PS2's takes a bool.
// FUNCTION: GAME 0x605b20
std::vector<G_SaveGame_FileHead> *G_LoadSaveClass::SaveGameEnumFiles()
{
    G_Stream stream;
    stream.SetParams(SAVEGAME_MAGIC, true, false);
    savegames.clear();
    for (int i = 0; i < PROFILE_SAVE_FILES; i++)
    {
        if (!textDatabase.GetTextOrNULL(TEXT_SAVEGAME_NAMES + i))
            continue;
        char name[128];
        sprintf(name, "savegame\\mafia%03d.%03d", profile.number, i);
        if (stream.OpenRead(name))
        {
            G_SaveGame_FileHead head;
            if (stream.ReadAdr(&head, sizeof(head)) == sizeof(head) && head.version == SAVEGAME_VERSION)
            {
                head.number = i;
                savegames.push_back(head);
            }
            stream.Close();
        }
    }
    return &savegames;
}

// FUNCTION: GAME 0x605e10
void G_LoadSaveClass::SaveGameEnumRelease()
{
    savegames.clear();
}

// A memcpy, as PS2's (whose returns the list's head, or NULL)
// FUNCTION: GAME 0x605e40
void G_LoadSaveClass::SaveGameSelect(uint index)
{
    if (index < savegames.size())
        memcpy(&savegame, &savegames[index], sizeof(savegame));
}

// SAVEGAME_NEW or more takes the lowest number no saved game has, from 1. PS2's takes a second parameter, which it keeps.
// FUNCTION: GAME 0x605e90
void G_LoadSaveClass::SaveGameCreate(uint number)
{
    G_SaveGame_FileHead head;
    memset(&head, 0, sizeof(head));
    uint i;
    if (number < SAVEGAME_NEW)
    {
        head.number = number;
        for (i = 0; i < savegames.size(); i++)
        {
            if (savegames[i].number == number)
            {
                SaveGameSelect(i);
                break;
            }
        }
        if (i >= savegames.size())
        {
            savegames.push_back(head);
            SaveGameSelect(savegames.size() - 1);
        }
    }
    else
    {
        uint n = 1;
        for (i = 0; i < savegames.size(); i++)
        {
            if (savegames[i].number == n)
            {
                n++;
                i = (uint)-1;
            }
        }
        head.number = n;
        savegames.push_back(head);
        SaveGameSelect(savegames.size() - 1);
    }
}

// FUNCTION: GAME 0x605ff0
void G_LoadSaveClass::SaveGameNewGame()
{
    last_stolen = 0;
    steal_cars = 0;
    GarageClear();
    freeride.active = false;
}

// The PC's reads the file's head and the info after it, both cleared first, and closes the stream whether it opened or
// not (PS2's copies the list's head, which is all it keeps)
// FUNCTION: GAME 0x606020
void G_LoadSaveClass::SaveGameLoadHeader(uint index, G_SaveGame_FileHead *head, G_SaveGame_Info *info)
{
    G_Stream stream;
    stream.SetParams(SAVEGAME_MAGIC, true, false);
    if (index < savegames.size())
    {
        memset(head, 0, sizeof(*head));
        memset(info, 0, sizeof(*info));
        char name[128];
        sprintf(name, "savegame\\mafia%03d.%03d", profile.number, savegames[index].number);
        if (stream.OpenRead(name))
        {
            if (stream.ReadAdr(head, sizeof(*head)) == sizeof(*head) && head->version == SAVEGAME_VERSION)
                stream.ReadAdr(info, sizeof(*info));
        }
        stream.Close();
    }
}

// Reads the file three times: first the game's head and the actors' records, opening the saved game's mission where
// another one is open and creating the actors the mission doesn't have; then, after the game's Init, each actor's
// number; then the game's data, the actors' data, the AI's groups and followers. PS2's changes the mission through its
// InterCity functions and leaves the garage to G_GarageClass::SaveGameLoad; the PC's does both here. The members after
// the mission's Open are read through the accessors: as plain reads, the inlined model cache's Open inlines more.
// FUNCTION: GAME 0x606150
void G_LoadSaveClass::SaveGameLoad()
{
    G_Stream stream;
    void *data = NULL;
    FreerideStoreB(0);
    stream.SetParams(SAVEGAME_MAGIC, true, false);
    char name[128];
    sprintf(name, "savegame\\mafia%03d.%03d", profile.number, savegame.number);
    if (stream.OpenRead(name))
    {
        G_SaveGame_GameHead head;
        G_SaveGame_ActorHead actor_head;
        void *buf;
        if (stream.ReadAdr(&savegame, sizeof(savegame)) == sizeof(savegame))
        {
            act_mission->GetGame()->Done();
            if (savegame.version == SAVEGAME_VERSION)
            {
                G_SaveGame_Info *info = (G_SaveGame_Info *)stream.Read(sizeof(G_SaveGame_Info));
                steal_cars = info->steal_cars;
                delete info;
                stream.ReadAdr(&head, sizeof(head));
                if (strcmp(head.mission, act_mission->mission_name))
                {
                    progress = new GM_LoadProgress(0, .7f, (uchar *)head.mission);
                    act_mission->Close();
                    act_mission->Open(head.mission, false, (uint)-1, true);
                }
                else
                    progress = new GM_LoadProgress(0, 0.0f, (uchar *)head.mission);
                char klz_name[256];
                // STRING: GAME 0x6362b4
                sprintf(klz_name, "missions\\%s\\tree.klz", head.mission);
                gifCollision.LoadCollision(klz_name, true);
                mafiaCarDatabase.LoadDetails();
                for (int i = 0; i < MAX_GARAGE_CARS; i++)
                {
                    shown_cars[i] = head.unknown_8c[i];
                    shown_cars[i] &= ~(0xff << CAR_LEVEL_SHIFT);
                    garage[i].actor = NULL;
                    garage[i].car = head.garage_cars[i];
                    garage[i].car &= ~(0xff << CAR_LEVEL_SHIFT);
                    mafiaCarDatabase.RepairIndex(shown_cars[i]);
                    mafiaCarDatabase.RepairIndex(garage[i].car);
                }
                mafiaCarDatabase.UnloadDetails();
                freeride.active = head.freeride_active != 0;
                freeride.police_density = head.freeride_police_density;
                freeride.traffic_density = head.freeride_traffic_density;
                freeride.pedestrian_density = head.freeride_pedestrian_density;
                if (head.size)
                {
                    buf = data = stream.Read(head.size);
                    act_mission->GetGame()->SaveGameLoadBefore(&data);
                    delete buf;
                }
                if (head.groups_size)
                {
                    data = stream.Read(head.groups_size);
                    delete data;
                }
                if (head.follow_size)
                {
                    data = stream.Read(head.follow_size);
                    delete data;
                }
                std::vector<C_actor *> &actors = act_mission->GetActors();
                while (stream.ReadAdr(&actor_head, sizeof(actor_head)) == sizeof(actor_head))
                {
                    buf = data = stream.Read(actor_head.size);
                    C_actor **it;
                    for (it = actors.begin(); it < actors.end(); it++)
                    {
                        I3D_frame *frm = (*it)->GetFrame();
                        if (frm && !strcmp(actor_head.name, frm->GetName()))
                        {
                            if (actor_head.garage_slot < MAX_GARAGE_CARS)
                                garage[actor_head.garage_slot].actor = (C_car *)*it;
                            break;
                        }
                    }
                    if (it >= actors.end())
                    {
                        I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
                        model_cache.Open(model, actor_head.model, 0, NULL, NULL, NULL);
                        model->LinkTo(act_mission->GetScene()->primary_sector, 0);
                        act_mission->GetScene()->AddFrame(model);
                        model->Release();
                        model->SetName(actor_head.name);
                        C_actor *act = act_mission->CreateActor(actor_head.type);
                        if (act)
                        {
                            if (act->Init(model))
                            {
                                if (actor_head.garage_slot < MAX_GARAGE_CARS)
                                {
                                    act_mission->AddActor(act, true);
                                    garage[actor_head.garage_slot].actor = (C_car *)act;
                                }
                                else
                                    act_mission->AddActor(act, false);
                            }
                            else
                                delete act;
                        }
                    }
                    delete buf;
                }
            }
        }
        stream.Close();
        act_mission->GetGame()->Init();
        if (progress)
        {
            delete progress;
            progress = NULL;
        }
        if (stream.OpenRead(name))
        {
            if (stream.ReadAdr(&savegame, sizeof(savegame)) == sizeof(savegame) &&
                savegame.version == SAVEGAME_VERSION)
            {
                delete stream.Read(sizeof(G_SaveGame_Info));
                stream.ReadAdr(&head, sizeof(head));
                if (head.size)
                {
                    data = stream.Read(head.size);
                    delete data;
                }
                if (head.groups_size)
                {
                    data = stream.Read(head.groups_size);
                    delete data;
                }
                if (head.follow_size)
                {
                    data = stream.Read(head.follow_size);
                    delete data;
                }
                std::vector<C_actor *> &actors = act_mission->GetActors();
                while (stream.ReadAdr(&actor_head, sizeof(actor_head)) == sizeof(actor_head))
                {
                    buf = data = stream.Read(actor_head.size);
                    for (C_actor **it = actors.begin(); it < actors.end(); it++)
                    {
                        I3D_frame *frm = (*it)->GetFrame();
                        if (frm && !strcmp(actor_head.name, frm->GetName()))
                        {
                            (*it)->SaveGameLoadGuid(&data);
                            break;
                        }
                    }
                    delete buf;
                }
            }
            stream.Close();
        }
        if (stream.OpenRead(name))
        {
            if (stream.ReadAdr(&savegame, sizeof(savegame)) == sizeof(savegame) &&
                savegame.version == SAVEGAME_VERSION)
            {
                uchar *groups = NULL;
                uchar *follow = NULL;
                delete stream.Read(sizeof(G_SaveGame_Info));
                stream.ReadAdr(&head, sizeof(head));
                if (head.size)
                {
                    buf = data = stream.Read(head.size);
                    act_mission->GetGame()->SaveGameLoad(&data);
                    delete buf;
                }
                if (head.groups_size)
                {
                    data = stream.Read(head.groups_size);
                    groups = new uchar[head.groups_size];
                    memcpy(groups, data, head.groups_size);
                    delete data;
                }
                if (head.follow_size)
                {
                    data = stream.Read(head.follow_size);
                    follow = new uchar[head.follow_size];
                    memcpy(follow, data, head.follow_size);
                    delete data;
                }
                std::vector<C_actor *> &actors = act_mission->GetActors();
                while (stream.ReadAdr(&actor_head, sizeof(actor_head)) == sizeof(actor_head))
                {
                    buf = data = stream.Read(actor_head.size);
                    for (C_actor **it = actors.begin(); it < actors.end(); it++)
                    {
                        I3D_frame *frm = (*it)->GetFrame();
                        if (frm && !strcmp(actor_head.name, frm->GetName()))
                        {
                            if ((*it)->GetType() == ACTOR_PLAYER && !act_mission->GetGame()->GetPlayer())
                            {
                                C_human *player = (C_human *)*it;
                                C_game *game = act_mission->GetGame();
                                game->camera_human = player;
                                game->player = player;
                                game->GetCamera().SetPlayer(player);
                            }
                            (*it)->SaveGameLoad(&data);
                            break;
                        }
                    }
                    delete buf;
                }
                if (head.groups_size && groups)
                {
                    data = groups;
                    AIManager.groups.Load(&data);
                    delete groups;
                }
                if (head.follow_size && follow)
                {
                    data = follow;
                    AIManager.follow.Load(&data);
                    delete follow;
                }
            }
            stream.Close();
        }
    }
    skip_save = true;
    act_mission->GetGame()->ChangeActState();
    // STRING: GAME 0x639630
    act_mission->GlobalProgramRun("GameAfterLoad");
}

// The actor's head, then its record. PS2's counts the records through a fifth parameter; the PC's garage slot is PC only.
// FUNCTION: GAME 0x606d80
void G_LoadSaveClass::_SaveProc(G_Stream *stream, C_actor *actor, G_SaveGame_ActorHead *head, uint size)
{
    void *buf = NULL;
    I3D_frame *frame = actor->frame;
    if (!frame)
        return;
    strncpy(head->name, frame->GetName(), sizeof(head->name));
    head->name[sizeof(head->name) - 1] = 0;
    head->size = size;
    head->type = actor->type;
    if (frame->GetType() == FRAME_MODEL)
    {
        const char *file_name = ((I3D_model *)frame)->GetFileName();
        const char *s = strrchr(file_name, '\\');
        if (!s)
            s = strrchr(file_name, '/');
        if (!s)
            s = file_name;
        else
            s++;
        strncpy(head->model, s, sizeof(head->model));
        head->model[sizeof(head->model) - 1] = 0;
    }
    else
        head->model[0] = 0;
    head->garage_slot = (uint)-1;
    if (head->type == ACTOR_CAR)
    {
        for (int i = 0; i < MAX_GARAGE_CARS; i++)
        {
            if (garage[i].actor == actor)
            {
                head->garage_slot = i;
                break;
            }
        }
    }
    stream->WriteAdr(head, sizeof(*head));
    buf = stream->WantWrite(size);
    actor->SaveGameSave(&buf);
    stream->Write();
}

// The file: the head (savegame's, which it completes), the info, the game's head and record, the AI's groups' and
// following's records, then the actors' heads and records, the cars and the trains first. full saves every actor
// (C_actor::global_actors), else the game's carried_actors, nothing where it is empty. PS2's keeps the info in its head and
// writes the number of actors before theirs.
// FUNCTION: GAME 0x606ea0
void G_LoadSaveClass::SaveGameSave(bool full)
{
    G_Stream stream;
    void *buf = NULL;
    stream.SetParams(SAVEGAME_MAGIC, true, false);
    G_SaveGame_Info info;
    memset(&info, 0, sizeof(info));
    G_SaveGame_GameHead game_head;
    memset(&game_head, 0, sizeof(game_head));
    char name[128];
    sprintf(name, "savegame\\mafia%03d.%03d", profile.number, savegame.number);
    if (stream.OpenWrite(name))
    {
        savegame.magic = SAVEGAME_HEAD_MAGIC;
        savegame.version = SAVEGAME_VERSION;
        savegame.mission_name_id = mission_name_id;
        stream.WriteAdr(&savegame, sizeof(savegame));
        info.number = savegame.number;
        if (act_mission->game->player && act_mission->game->initialized)
        {
            info.health = (int)(act_mission->game->player->properties[HUMAN_PROPERTY_ENERGY] /
                act_mission->game->player->max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
            if (!act_mission->game->player->dead && info.health < 1)
                info.health = 1;
        }
        else
            info.health = 100;
        info.play_time = mafiaIndicators.play_time;
        SYSTEMTIME st;
        GetLocalTime(&st);
        info.time = st.wHour << 16 | st.wMinute << 8 | st.wSecond;
        info.date = st.wYear << 16 | st.wMonth << 8 | st.wDay;
        info.steal_cars = steal_cars;
        stream.WriteAdr(&info, sizeof(info));
        uint size = act_mission->game->SaveGameGetSize(full);
        strncpy(game_head.mission, act_mission->mission_name, sizeof(game_head.mission));
        game_head.mission[sizeof(game_head.mission) - 1] = 0;
        for (int i = 0; i < MAX_GARAGE_CARS; i++)
        {
            game_head.garage_cars[i] = garage[i].car;
            game_head.garage_cars[i] &= ~(0xff << CAR_LEVEL_SHIFT);
            game_head.unknown_8c[i] = shown_cars[i];
            game_head.unknown_8c[i] &= ~(0xff << CAR_LEVEL_SHIFT);
        }
        game_head.size = size;
        game_head.groups_size = AIManager.groups.GetSize();
        game_head.follow_size = AIManager.follow.GetSize();
        game_head.freeride_active = freeride.active;
        game_head.freeride_police_density = freeride.police_density;
        game_head.freeride_traffic_density = freeride.traffic_density;
        game_head.freeride_pedestrian_density = freeride.pedestrian_density;
        stream.WriteAdr(&game_head, sizeof(game_head));
        buf = stream.WantWrite(size);
        act_mission->game->SaveGameSave(&buf, full);
        stream.Write();
        if (game_head.groups_size)
        {
            buf = stream.WantWrite(game_head.groups_size);
            AIManager.groups.Save(&buf);
            stream.Write();
        }
        if (game_head.follow_size)
        {
            buf = stream.WantWrite(game_head.follow_size);
            AIManager.follow.Save(&buf);
            stream.Write();
        }
        C_game *game = act_mission->game;
        if (full || game->carried_actors.size())
        {
            std::vector<C_actor *> *actors = full ? &C_actor::global_actors : &game->carried_actors;
            G_SaveGame_ActorHead head;
            C_actor **it;
            for (it = actors->begin(); it < actors->end(); it++)
            {
                if (((*it)->type == ACTOR_CAR || (*it)->type == ACTOR_RAILWAY) && !(*it)->unknown_65)
                {
                    uint actor_size = (*it)->SaveGameGetSize();
                    if (actor_size)
                        _SaveProc(&stream, *it, &head, actor_size);
                }
            }
            for (it = actors->begin(); it < actors->end(); it++)
            {
                if ((*it)->type != ACTOR_CAR && (*it)->type != ACTOR_RAILWAY && !(*it)->unknown_65)
                {
                    uint actor_size = (*it)->SaveGameGetSize();
                    if (actor_size)
                        _SaveProc(&stream, *it, &head, actor_size);
                }
            }
        }
        stream.Close();
    }
}

// FUNCTION: GAME 0x6072e0
uint G_LoadSaveClass::GetScreenShotIdx(uint index)
{
    return index;
}

// The slot is a text of the text database, the saved game's name, numbered from TEXT_SAVEGAME_NAMES. Right after
// SaveGameLoad, which runs the mission's script again from its save, the first save is skipped. PS2's saves to the memory
// card, its second bool a choice of its.
// FUNCTION: GAME 0x6072f0
void G_LoadSaveClass::SaveGameFromScript(uint slot, bool full, bool message)
{
    if (skip_save)
    {
        skip_save = false;
        return;
    }
    mission_name_id = slot;
    SaveGameCreate(slot - TEXT_SAVEGAME_NAMES);
    SaveGameSave(full);
    SaveGameEnumRelease();
    ProfileSave();
    if (message)
        mafiaIndicators.ConsoleAddTextByID(TEXT_GAME_SAVED, 0xffffff);
}

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

// FUNCTION: GAME 0x607400
void G_LoadSaveClass::SetOppositeBinding(int index, S_control *control)
{
    if (control->device == CONTROL_JOY)
    {
        bindings[index].device = CONTROL_JOY;
        bindings[index].code = control->code;
        bindings[index].joy = control->joy;
        bindings[index].half = CONTROL_AXIS_FULL;
        if (control->half == CONTROL_AXIS_POSITIVE)
            bindings[index].half = CONTROL_AXIS_NEGATIVE;
        if (control->half == CONTROL_AXIS_NEGATIVE)
            bindings[index].half = CONTROL_AXIS_POSITIVE;
    }
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

// With any_half each binding of the key, button or axis goes at once; otherwise a binding of the opposite half stays.
// The car's loop lacks a break: a positive half keeps the bindings of the positive half too.
// FUNCTION: GAME 0x607520
void G_LoadSaveClass::RemoveBinding(int index, S_control *control, bool any_half)
{
    int i;
    if (index < NUM_PLAYER_CONTROLS)
    {
        for (i = 0; i < NUM_PLAYER_CONTROLS; i++)
        {
            if (bindings[i].device == control->device && bindings[i].code == control->code
                && bindings[i].joy == control->joy)
            {
                if (any_half)
                {
                    SetBinding(i, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
                    continue;
                }
                switch (control->half)
                {
                case CONTROL_AXIS_POSITIVE:
                    if (bindings[i].half == CONTROL_AXIS_NEGATIVE)
                        continue;
                    break;
                case CONTROL_AXIS_NEGATIVE:
                    if (bindings[i].half == CONTROL_AXIS_POSITIVE)
                        continue;
                    break;
                }
                SetBinding(i, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
            }
        }
    }
    else
    {
        for (i = NUM_PLAYER_CONTROLS; i < NUM_CONTROLS; i++)
        {
            if (bindings[i].device == control->device && bindings[i].code == control->code
                && bindings[i].joy == control->joy)
            {
                if (any_half)
                {
                    SetBinding(i, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
                    continue;
                }
                switch (control->half)
                {
                case CONTROL_AXIS_POSITIVE:
                    if (bindings[i].half == CONTROL_AXIS_NEGATIVE)
                        continue;
                case CONTROL_AXIS_NEGATIVE:
                    if (bindings[i].half == CONTROL_AXIS_POSITIVE)
                        continue;
                    break;
                }
                SetBinding(i, CONTROL_NONE, 0, 0, CONTROL_AXIS_FULL);
            }
        }
    }
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

// G_Inventory's functions (actors/g_inventory.h), which lie in this file's range on the PC, and PS2's
// G_EnabledCars::HaveCar
// The item's gameItemTable flags of mask, none for an id past the table's end (the names are ours; PS2's inventory
// tests the id at each flag test too). Written with two returns: the original keeps each result in a temporary, which
// a ?: doesn't
static inline uint GetItemFlags(const S_GameItem *item, uint mask)
{
    if (item->id < numOfGameItems)
        return gameItemTable[item->id].flags & mask;
    return 0;
}

// Whether the item may be held in the hand, not for an id past the table's end
static inline uint CanBeInHand(const S_GameItem *item)
{
    if (item->id < numOfGameItems)
        return !(gameItemTable[item->id].flags & ITEMF_NOT_IN_HAND);
    return 0;
}

// The tests of an item's kind by gameItemTable's flags, of an item or of an id, 0 for an id out of the table (the
// names are ours): an item that isn't a weapon, a weapon, a big item, a short big weapon, an item that isn't big, a
// weapon whose ammunition merges with an item of its id
static inline int IsItem(const S_GameItem &item)
{
    int r;
    if (item.id < numOfGameItems)
        r = !(gameItemTable[item.id].flags & (ITEMF_WEAPON | ITEMF_MELEE));
    else
        r = 0;
    return r;
}
static inline int IsWeapon(const S_GameItem &item)
{
    int r;
    if (item.id < numOfGameItems)
        r = gameItemTable[item.id].flags & (ITEMF_WEAPON | ITEMF_MELEE);
    else
        r = 0;
    return r;
}
static inline int IsBig(const S_GameItem &item)
{
    int r;
    if (item.id < numOfGameItems)
        r = gameItemTable[item.id].flags & ITEMF_BIG;
    else
        r = 0;
    return r;
}
static inline int IsShort(const S_GameItem &item)
{
    int r;
    if (item.id < numOfGameItems)
        r = gameItemTable[item.id].flags & ITEMF_SHORT;
    else
        r = 0;
    return r;
}
static inline int IsSmall(const S_GameItem &item)
{
    int r;
    if (item.id < numOfGameItems)
        r = !(gameItemTable[item.id].flags & ITEMF_BIG);
    else
        r = 0;
    return r;
}
static inline int IsItem(uint id)
{
    int r;
    if (id < numOfGameItems)
        r = !(gameItemTable[id].flags & (ITEMF_WEAPON | ITEMF_MELEE));
    else
        r = 0;
    return r;
}
static inline int IsBig(uint id)
{
    int r;
    if (id < numOfGameItems)
        r = gameItemTable[id].flags & ITEMF_BIG;
    else
        r = 0;
    return r;
}
static inline int IsShort(uint id)
{
    int r;
    if (id < numOfGameItems)
        r = gameItemTable[id].flags & ITEMF_SHORT;
    else
        r = 0;
    return r;
}
static inline int CanMerge(uint id)
{
    int r;
    if (id < numOfGameItems)
        r = !(gameItemTable[id].flags & ITEMF_NO_MERGE);
    else
        r = 0;
    return r;
}

// FUNCTION: GAME 0x607b80
S_item_def *G_Inventory::GetWeaponInRightHand()
{
    if (right_hand.id < numOfGameItems)
        return &gameItemTable[right_hand.id];
    return gameItemTable;
}

// FUNCTION: GAME 0x607bb0
S_item_def *G_Inventory::GetWeaponInLeftHand()
{
    return gameItemTable;
}

// The kinds' code in the source order of the jump table's targets (the car's before the items'). The items' drop is
// spelled insert(end(), x), the others' push_back(x): the inline budget left at the end inlines only the items'
// vector insert, as the original's
// FUNCTION: GAME 0x607bc0
bool G_Inventory::Select(uint index, uint kind, std::vector<S_GameItem> *dropped)
{
    switch (kind) {
    case INVENTORY_HAND:
        return true;
    case INVENTORY_POCKET:
        if (right_hand.id && !Insert(&right_hand, true)) {
            if (IsItem(right_hand) || IsBig(right_hand)) {
                if (in_car && !car_item.id)
                    car_item = right_hand;
                else if (dropped)
                    dropped->push_back(right_hand);
            } else {
                std::swap(right_hand, pockets[index]);
                return true;
            }
        }
        right_hand = pockets[index];
        pockets[index].id = 0;
        _Sesyp();
        return true;
    case INVENTORY_BIG_WEAPON:
        if (right_hand.id) {
            if (IsBig(right_hand) && (can_have_big_weapon || IsShort(right_hand))) {
                std::swap(right_hand, big_weapon);
                return true;
            }
            if (!Insert(&right_hand, true)) {
                if (in_car && !car_item.id)
                    car_item = right_hand;
                else if (dropped)
                    dropped->push_back(right_hand);
            }
        }
        right_hand = big_weapon;
        big_weapon.id = 0;
        return true;
    case INVENTORY_CAR:
        if (right_hand.id && !Insert(&right_hand, true)) {
            std::swap(right_hand, car_item);
            return true;
        }
        right_hand = car_item;
        car_item.id = 0;
        return true;
    case INVENTORY_ITEM:
        if (right_hand.id && !Insert(&right_hand, true)) {
            if (IsWeapon(right_hand)) {
                if (in_car && !car_item.id)
                    car_item = right_hand;
                else if (dropped)
                    dropped->insert(dropped->end(), right_hand);
            } else {
                S_GameItem tmp = right_hand;
                right_hand = items[index];
                items[index] = tmp;
                return true;
            }
        }
        right_hand = items[index];
        items[index].id = 0;
        _Sesyp();
        return true;
    }
    return false;
}

// FUNCTION: GAME 0x6081d0
bool G_Inventory::SelectByID(uint id, std::vector<S_GameItem> *dropped)
{
    if (right_hand.id == id)
        return true;
    int i;
    for (i = 0; i < INVENTORY_POCKETS; i++)
        if (pockets[i].id == id)
            return Select(i, INVENTORY_POCKET, dropped);
    if (big_weapon.id == id)
        return Select(0, INVENTORY_BIG_WEAPON, dropped);
    for (uint j = 0; j < num_items; j++)
        if (items[j].id == id)
            return Select(j, INVENTORY_ITEM, dropped);
    if (in_car && car_item.id == id)
        return Select(0, INVENTORY_CAR, dropped);
    return false;
}

// The player keeps a weapon that has no place in the hand; the others drop it, into a dropped that has to be there
// FUNCTION: GAME 0x6082a0
void G_Inventory::Holster(std::vector<S_GameItem> *dropped, bool keep, bool player)
{
    if (Insert(&right_hand, true)) {
        right_hand.id = 0;
        return;
    }
    if (in_car && !car_item.id) {
        car_item = right_hand;
        right_hand.id = 0;
        return;
    }
    if (player) {
        if (messages)
            // "You don't have space to hide this weapon!"
            mafiaIndicators.ConsoleAddText(textDatabase.GetText(0xd70), 0xffffff);
        return;
    }
    if (!keep) {
        dropped->push_back(right_hand);
        right_hand.id = 0;
    }
}

// The car's place doesn't count, here and in IsIn
// FUNCTION: GAME 0x608520
bool G_Inventory::HaveSomeWeapon(bool in_hand, uint flags)
{
    if (right_hand.id < numOfGameItems && gameItemTable[right_hand.id].flags & flags)
        return true;
    if (!in_hand) {
        int i;
        for (i = 0; i < INVENTORY_POCKETS; i++)
            if (pockets[i].id < numOfGameItems && gameItemTable[pockets[i].id].flags & flags)
                return true;
        if (big_weapon.id < numOfGameItems && gameItemTable[big_weapon.id].flags & flags)
            return true;
        for (uint j = 0; j < num_items; j++)
            if (items[j].id < numOfGameItems && gameItemTable[items[j].id].flags & flags)
                return true;
    }
    return false;
}

// Unlike the others it doesn't test the ids against gameItemTable's size
// FUNCTION: GAME 0x6085f0
uint G_Inventory::HaveItems(std::vector<S_GameItem> *list, uint flags)
{
    list->clear();
    if (right_hand.id && gameItemTable[right_hand.id].flags & flags)
        list->push_back(right_hand);
    int i;
    for (i = 0; i < INVENTORY_POCKETS; i++)
        if (pockets[i].id && gameItemTable[pockets[i].id].flags & flags)
            list->push_back(pockets[i]);
    if (big_weapon.id && gameItemTable[big_weapon.id].flags & flags)
        list->push_back(big_weapon);
    for (uint j = 0; j < num_items; j++)
        if (items[j].id && gameItemTable[items[j].id].flags & flags)
            list->push_back(items[j]);
    return list->size();
}

// The ids of the hand's weapon and of the places' items but the items' (the hand's, the pockets', the big weapon's
// and the car's, at most 8): cycle loses the others and gains the new ones at its end. The last search's result isn't
// used, as in the original.
// FUNCTION: GAME 0x608770
void G_Inventory::CyclePrepare()
{
    uint num = 0;
    uint ids[1 + INVENTORY_POCKETS + 1 + 1];
    if (right_hand.id && IsWeapon(right_hand))
        ids[num++] = right_hand.id;
    for (int i = 0; i < INVENTORY_POCKETS; i++)
        if (pockets[i].id)
            ids[num++] = pockets[i].id;
    if (big_weapon.id)
        ids[num++] = big_weapon.id;
    if (in_car && car_item.id)
        ids[num++] = car_item.id;
    uint j;
    uint *it;
    for (it = cycle.begin(); it < cycle.end(); it++) {
        for (j = 0; j < num; j++)
            if (*it == ids[j])
                break;
        if (j >= num) {
            cycle.erase(it);
            it--;
        }
    }
    for (j = 0; j < num; j++) {
        for (it = cycle.begin(); it < cycle.end(); it++)
            if (*it == ids[j])
                break;
        if (it >= cycle.end())
            cycle.push_back(ids[j]);
    }
    for (j = 0; j < cycle.size() && cycle[j] != cycle_id; j++)
        ;
}

// FUNCTION: GAME 0x6088f0
bool G_Inventory::CycleUp(std::vector<S_GameItem> *dropped)
{
    if (right_hand.id)
        cycle_id = right_hand.id;
    CyclePrepare();
    if (!cycle.size())
        return false;
    int i;
    for (i = 0; i < cycle.size() && cycle[i] != cycle_id; i++)
        ;
    i--;
    if (i < 0)
        i = cycle.size() - 1;
    cycle_id = cycle[i];
    return SelectByID(cycle_id, dropped);
}

// FUNCTION: GAME 0x608990
bool G_Inventory::CycleDown(std::vector<S_GameItem> *dropped)
{
    if (right_hand.id)
        cycle_id = right_hand.id;
    CyclePrepare();
    if (!cycle.size())
        return false;
    int i;
    for (i = 0; i < cycle.size() && cycle[i] != cycle_id; i++)
        ;
    i++;
    if (i >= cycle.size())
        i = 0;
    cycle_id = cycle[i];
    return SelectByID(cycle_id, dropped);
}

// FUNCTION: GAME 0x608a20
bool G_Inventory::IsIn(uint id, bool in_hand)
{
    if (right_hand.id == id)
        return true;
    if (!in_hand) {
        int i;
        for (i = 0; i < INVENTORY_POCKETS; i++)
            if (pockets[i].id == id)
                return true;
        if (big_weapon.id == id)
            return true;
        for (uint j = 0; j < num_items; j++)
            if (items[j].id == id)
                return true;
    }
    return false;
}

// FUNCTION: GAME 0x608aa0
bool G_Inventory::HaveSpaceFor(uint id, bool with_hand)
{
    if (id <= GAMEITEM_SPECIAL_ACTION || id >= numOfGameItems)
        return false;
    if (IsItem(id)) {
        if (with_hand && !right_hand.id)
            return true;
        if (IsBig(id))
            return false;
        return num_items < INVENTORY_ITEMS ? true : false;
    }
    if (IsBig(id)) {
        if (CanMerge(id)) {
            if (big_weapon.id == id)
                return true;
            if (with_hand && right_hand.id == id)
                return true;
        }
        if ((can_have_big_weapon || IsShort(id)) && !big_weapon.id)
            return true;
        if (!with_hand)
            return false;
        if (!right_hand.id)
            return true;
        if (IsSmall(right_hand))
            return true;
        return false;
    }
    if (CanMerge(id)) {
        for (int i = 0; i < INVENTORY_POCKETS; i++)
            if (pockets[i].id == id)
                return true;
        if (with_hand && right_hand.id == id)
            return true;
    }
    if (!pockets[INVENTORY_POCKETS - 1].id)
        return true;
    if (with_hand && !right_hand.id)
        return true;
    return false;
}

// FUNCTION: GAME 0x608c40
bool G_Inventory::DobijNaboje(S_GameItem *item, S_GameItem *from)
{
    uint total = from->ammo_loaded + from->ammo_spare + item->ammo_spare;
    uint max = gameItemTable[item->id].spare_max;
    if (messages) {
        if (item->ammo_spare >= max)
            // "You already have this weapon"
            mafiaIndicators.ConsoleAddText(textDatabase.GetText(0xd6e), 0xffffff);
        else
            // "The weapon is fully loaded"
            mafiaIndicators.ConsoleAddText(textDatabase.GetText(0xd6f), 0xffffff);
    }
    if (total > max) {
        item->ammo_spare = max;
        from->ammo_loaded = 0;
        from->ammo_spare = total - max;
    } else {
        item->ammo_spare = total;
        from->ammo_loaded = 0;
        from->ammo_spare = 0;
    }
    return from->ammo_spare ? false : true;
}

// FUNCTION: GAME 0x608ce0
bool G_Inventory::InsertFree(S_GameItem *item)
{
    if (IsWeapon(*item)) {
        if (IsBig(*item)) {
            if ((IsShort(*item) || can_have_big_weapon) && !big_weapon.id) {
                big_weapon = *item;
                return true;
            }
            return false;
        }
        if (pockets[INVENTORY_POCKETS - 1].id)
            return false;
        for (int i = 0; i < INVENTORY_POCKETS; i++)
            if (!pockets[i].id) {
                pockets[i] = *item;
                return true;
            }
        return false;
    }
    if (IsBig(*item))
        return false;
    if (num_items >= INVENTORY_ITEMS)
        return false;
    items[num_items] = *item;
    num_items++;
    return true;
}

// A weapon of the same kind already there takes the new one's ammunition, the big weapon going into the hand for it
// first; what finds no place goes into dropped, the hand's item where the new one takes the hand
// FUNCTION: GAME 0x608e20
bool G_Inventory::InsertNew(S_GameItem *item, std::vector<S_GameItem> *dropped)
{
    if (item->id) {
        if (GetItemFlags(item, ITEMF_MELEE | ITEMF_WEAPON)) {
            if (GetItemFlags(item, ITEMF_NO_MERGE)) {
                if (!right_hand.id) {
                    right_hand = *item;
                    return true;
                }
                if (!InsertFree(&right_hand))
                    dropped->push_back(right_hand);
                right_hand = *item;
                return true;
            }
            // Where the same weapon is: -2 nowhere, -1 the hand, 0 to 4 weapons, 5 the big weapon
            int slot = -2;
            for (int i = 0; i < INVENTORY_POCKETS; i++)
                if (pockets[i].id == item->id) {
                    slot = i;
                    break;
                }
            if (big_weapon.id == item->id)
                slot = 5;
            if (right_hand.id == item->id)
                slot = -1;
            if (slot > -2) {
                if (slot > -1) {
                    if (slot == 5) {
                        S_GameItem big = big_weapon;
                        big_weapon.id = 0;
                        if (!InsertFree(&right_hand))
                            dropped->push_back(right_hand);
                        right_hand = big;
                    } else
                        return DobijNaboje(&pockets[slot], item);
                }
                return DobijNaboje(&right_hand, item);
            }
            if (!right_hand.id) {
                right_hand = *item;
                return true;
            }
            if (GetItemFlags(item, ITEMF_BIG)) {
                if (!InsertFree(&right_hand))
                    dropped->push_back(right_hand);
                right_hand = *item;
                return true;
            }
            if (!InsertFree(item))
                return false;
        } else {
            if (right_hand.id || !CanBeInHand(item)) {
                if (InsertFree(item))
                    return true;
                if (!GetItemFlags(item, ITEMF_NOT_IN_HAND)) {
                    dropped->push_back(*item);
                    return true;
                }
                if (!InsertFree(&right_hand))
                    dropped->push_back(right_hand);
            }
            right_hand = *item;
        }
    }
    return true;
}

// FUNCTION: GAME 0x6092b0
bool G_Inventory::Insert(S_GameItem *item, bool no_merge)
{
    if (!item->id)
        return true;
    if (item->id >= numOfGameItems)
        return false;
    if (GetItemFlags(item, ITEMF_MELEE | ITEMF_WEAPON)) {
        if (GetItemFlags(item, ITEMF_NO_MERGE)) {
            if (!right_hand.id) {
                right_hand = *item;
                return true;
            }
            if (GetItemFlags(item, ITEMF_BIG)) {
                if ((GetItemFlags(item, ITEMF_SHORT) || can_have_big_weapon) && !big_weapon.id) {
                    big_weapon = *item;
                    return true;
                }
                if (!InsertFree(&right_hand))
                    return false;
                right_hand = *item;
                return true;
            }
            if (pockets[INVENTORY_POCKETS - 1].id)
                return false;
            for (int i = 0; i < INVENTORY_POCKETS; i++)
                if (!pockets[i].id) {
                    pockets[i] = *item;
                    return true;
                }
            return false;
        }
        if (!no_merge) {
            if (right_hand.id == item->id)
                return DobijNaboje(&right_hand, item);
            if (GetItemFlags(item, ITEMF_BIG)) {
                if (big_weapon.id == item->id)
                    return DobijNaboje(&big_weapon, item);
            } else {
                for (int i = 0; i < INVENTORY_POCKETS; i++)
                    if (pockets[i].id == item->id)
                        return DobijNaboje(&pockets[i], item);
            }
        }
        if (!right_hand.id) {
            right_hand = *item;
            return true;
        }
        if (GetItemFlags(item, ITEMF_BIG)) {
            if ((GetItemFlags(item, ITEMF_SHORT) || can_have_big_weapon) && !big_weapon.id) {
                big_weapon = *item;
                return true;
            }
            if (!InsertFree(&right_hand))
                return false;
            right_hand = *item;
            return true;
        }
        if (pockets[INVENTORY_POCKETS - 1].id)
            return false;
        for (int i = 0; i < INVENTORY_POCKETS; i++)
            if (!pockets[i].id) {
                pockets[i] = *item;
                return true;
            }
        return false;
    }
    if (!right_hand.id && CanBeInHand(item)) {
        right_hand = *item;
        return true;
    }
    if (GetItemFlags(item, ITEMF_BIG)) {
        if (!InsertFree(&right_hand))
            return false;
        right_hand = *item;
        return true;
    }
    if (num_items >= INVENTORY_ITEMS)
        return false;
    items[num_items] = *item;
    num_items++;
    return true;
}

// PS2's RemoveByIdx(uint, uint): its RemoveInHand (PS2's selected item, not inlined there) is RemoveByIdx(0,
// INVENTORY_HAND) on the PC
// FUNCTION: GAME 0x609560
void G_Inventory::RemoveByIdx(uint idx, uint where)
{
    switch (where) {
    case INVENTORY_HAND:
        right_hand.id = 0;
        break;
    case INVENTORY_POCKET:
        if (idx < INVENTORY_POCKETS)
            pockets[idx].id = 0;
        break;
    case INVENTORY_BIG_WEAPON:
        big_weapon.id = 0;
        break;
    case INVENTORY_ITEM:
        if (idx < num_items)
            items[idx].id = 0;
        break;
    }
    _Sesyp();
}

// FUNCTION: GAME 0x6095e0
bool G_Inventory::Remove(S_GameItem *item, std::vector<S_GameItem> *dropped)
{
    if (right_hand.id == item->id) {
        if (dropped)
            dropped->push_back(right_hand);
        right_hand.id = 0;
        return true;
    }
    int i;
    for (i = 0; i < INVENTORY_POCKETS; i++)
        if (pockets[i].id == item->id) {
            if (dropped)
                dropped->push_back(pockets[i]);
            for (; i < INVENTORY_POCKETS - 1; i++)
                pockets[i] = pockets[i + 1];
            pockets[i].id = 0;
            return true;
        }
    if (big_weapon.id == item->id) {
        if (dropped)
            dropped->push_back(big_weapon);
        big_weapon.id = 0;
        return true;
    }
    uint j;
    for (j = 0; j < num_items; j++)
        if (items[j].id == item->id) {
            if (dropped)
                dropped->push_back(items[j]);
            for (; j < num_items - 1; j++)
                items[j] = items[j + 1];
            num_items--;
            return true;
        }
    return false;
}

// FUNCTION: GAME 0x609920
void G_Inventory::SetCanHaveBigWeapon(bool can, std::vector<S_GameItem> *dropped)
{
    can_have_big_weapon = can;
    if (!can && big_weapon.id) {
        if (!HaveSpaceFor(big_weapon.id, true) || !Insert(&big_weapon, false)) {
            dropped->push_back(big_weapon);
            big_weapon.id = 0;
        }
    }
}

// FUNCTION: GAME 0x609b60
void G_Inventory::SetInCar(bool in, std::vector<S_GameItem> *dropped)
{
    if (in_car && !in)
        InsertNew(&car_item, dropped);
    if (!in_car && in)
        car_item.id = 0;
    in_car = in;
}

// The items of a body to take (PS2 name): the PC's tells the console "Nothing found" where it has none
// FUNCTION: GAME 0x609bb0
void G_Inventory::EnemyToList(std::vector<S_GameItem *> *list)
{
    list->clear();
    if (right_hand.id)
        list->push_back(&right_hand);
    for (int i = 0; i < INVENTORY_POCKETS; i++)
        if (pockets[i].id)
            list->push_back(&pockets[i]);
    if (big_weapon.id)
        list->push_back(&big_weapon);
    for (uint j = 0; j < num_items; j++)
        list->push_back(&items[j]);
    if (!list->size())
        mafiaIndicators.ConsoleAddTextByID(3400, 0xffffff);     // "Nothing found"
}

// FUNCTION: GAME 0x609cb0
void G_Inventory::RemoveByPtr(S_GameItem *item)
{
    if (&right_hand == item)
        right_hand.id = 0;
    for (int i = 0; i < INVENTORY_POCKETS; i++)
        if (&pockets[i] == item)
            pockets[i].id = 0;
    if (&big_weapon == item)
        big_weapon.id = 0;
    for (uint j = 0; j < num_items; j++)
        if (&items[j] == item)
            items[j].id = 0;
    _Sesyp();
}

// FUNCTION: GAME 0x609d20
uint G_Inventory::GetAmmo(int *spare)
{
    if (spare) {
        if (!right_hand.id) {
            *spare = 0;
            return 0;
        }
        *spare = right_hand.ammo_spare;
        return right_hand.ammo_loaded;
    }
    if (!right_hand.id)
        return 0;
    return right_hand.ammo_loaded;
}

// FUNCTION: GAME 0x609d60
uint G_Inventory::GetAmmoByID(uint id, int *spare)
{
    if (id) {
        if (right_hand.id == id) {
            if (spare)
                *spare = right_hand.ammo_spare;
            return right_hand.ammo_loaded;
        }
        for (int i = 0; i < INVENTORY_POCKETS; i++)
            if (pockets[i].id == id) {
                if (spare)
                    *spare = pockets[i].ammo_spare;
                return pockets[i].ammo_loaded;
            }
        if (big_weapon.id == id) {
            if (spare)
                *spare = big_weapon.ammo_spare;
            return big_weapon.ammo_loaded;
        }
        for (uint j = 0; j < num_items; j++)
            if (items[j].id == id) {
                if (spare)
                    *spare = items[j].ammo_spare;
                return items[j].ammo_loaded;
            }
    }
    if (spare)
        *spare = 0;
    return 0;
}

// FUNCTION: GAME 0x609e50
bool G_Inventory::Vystrel()
{
    if (!right_hand.id)
        return false;
    if (!right_hand.ammo_loaded)
        return false;
    right_hand.ammo_loaded--;
    return true;
}

// A weapon of ITEMF_MAGAZINE loses what was left in it
// FUNCTION: GAME 0x609e70
bool G_Inventory::Nabij()
{
    if (!right_hand.id)
        return false;
    uint total = right_hand.ammo_loaded + right_hand.ammo_spare;
    S_item_def *def = &gameItemTable[right_hand.id];
    uint max = def->ammo_max;
    if (def->flags & ITEMF_MAGAZINE) {
        if (!right_hand.ammo_spare)
            return true;
        if ((uint)right_hand.ammo_spare > max) {
            right_hand.ammo_loaded = max;
            right_hand.ammo_spare -= max;
        } else {
            right_hand.ammo_loaded = right_hand.ammo_spare;
            right_hand.ammo_spare = 0;
        }
    } else {
        if (total > max) {
            right_hand.ammo_loaded = max;
            right_hand.ammo_spare = total - max;
        } else {
            right_hand.ammo_loaded = total;
            right_hand.ammo_spare = 0;
        }
    }
    return true;
}

// FUNCTION: GAME 0x609f00
bool G_Inventory::NabijJeden()
{
    if (!right_hand.id)
        return false;
    if ((uint)right_hand.ammo_loaded < gameItemTable[right_hand.id].ammo_max && right_hand.ammo_spare) {
        right_hand.ammo_loaded++;
        right_hand.ammo_spare--;
        return true;
    }
    return false;
}

// FUNCTION: GAME 0x609f40
bool G_Inventory::MuzeNabitJeden()
{
    if (!right_hand.id)
        return false;
    if ((uint)right_hand.ammo_loaded < gameItemTable[right_hand.id].ammo_max && right_hand.ammo_spare)
        return true;
    return false;
}

// The empty places closed up (Czech "pour together"; PS2's is empty): the weapons' fifth place cleared after each
// move, an empty other item taking the next one's place without the next being tested again
// FUNCTION: GAME 0x609f80
void G_Inventory::_Sesyp()
{
    for (int i = 0; i < INVENTORY_POCKETS - 1; i++)
        if (!pockets[i].id) {
            memmove(&pockets[i], &pockets[i + 1], (INVENTORY_POCKETS - 1 - i) * sizeof(S_GameItem));
            pockets[INVENTORY_POCKETS - 1].id = 0;
        }
    for (uint j = 0; j < num_items; j++)
        if (!items[j].id) {
            num_items--;
            memmove(&items[j], &items[j + 1], (num_items - j) * sizeof(S_GameItem));
        }
}

// The flags' dword and the items (PS2's is 0x100 bytes)
// FUNCTION: GAME 0x60a000
int G_Inventory::SaveGameGetSize()
{
    return sizeof(uint) + sizeof(S_inventory_save);
}

// FUNCTION: GAME 0x60a010
bool G_Inventory::SaveGameSave(void **buf)
{
    *(uint *)*buf = can_have_big_weapon;
    if (in_car)
        *(uint *)*buf |= SAVE_INVENTORY_IN_CAR;
    *buf = (uint *)*buf + 1;
    S_inventory_save *save = (S_inventory_save *)*buf;
    memset(save, 0, sizeof(*save));
    save->right_hand = right_hand;
    save->car_item = car_item;
    for (int i = 0; i < INVENTORY_POCKETS; i++)
        save->pockets[i] = pockets[i];
    save->big_weapon = big_weapon;
    for (uint j = 0; j < num_items; j++)
        save->items[j] = items[j];
    *buf = (S_inventory_save *)*buf + 1;
    return true;
}

// FUNCTION: GAME 0x60a130
bool G_Inventory::SaveGameLoad(void **buf)
{
    can_have_big_weapon = *(uint *)*buf & (SAVE_INVENTORY_IN_CAR - 1);
    in_car = false;
    if (*(uchar *)*buf & SAVE_INVENTORY_IN_CAR)
        in_car = true;
    *buf = (uint *)*buf + 1;
    S_inventory_save *save = (S_inventory_save *)*buf;
    right_hand = save->right_hand;
    car_item = save->car_item;
    for (int i = 0; i < INVENTORY_POCKETS; i++)
        pockets[i] = save->pockets[i];
    big_weapon = save->big_weapon;
    num_items = 0;
    for (int j = 0; j < INVENTORY_ITEMS; j++) {
        items[j] = save->items[j];
        if (items[j].id)
            num_items++;
    }
    *buf = (S_inventory_save *)*buf + 1;
    return true;
}

// FUNCTION: GAME 0x60a250
bool G_EnabledCars::HaveCar(uint car, bool by_level)
{
    if (by_level)
    {
        uint index = car & CAR_INDEX_MASK;
        switch ((car >> CAR_LEVEL_SHIFT) & 0xff)
        {
        case 0:
            return level_cars[0][index >> 5] & 1 << (index & 31) ? true : false;
        case 1:
            return level_cars[1][index >> 5] & 1 << (index & 31) ? true : false;
        case 2:
            return level_cars[2][index >> 5] & 1 << (index & 31) ? true : false;
        }
        return false;
    }
    uint index = car & CAR_INDEX_MASK;
    return cars[index >> 5] & 1 << (index & 31) ? true : false;
}

// The default car's texts: "none" and "bad c/p-index.def" (Czech, the car and player indices)
// FUNCTION: GAME 0x60a2f0
G_CarDatabase::G_CarDatabase()
{
    num_cars = 0;
    cars = NULL;
    num_players = 0;
    players = NULL;
    none.car_class = 0;
    // STRING: GAME 0x639670
    strcpy(none.id, "none");
    // STRING: GAME 0x639658
    strcpy(none.name, "Spatny c|p-index.def");
    num_details = 0;
    details = NULL;
    details_ref = 0;
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

// FUNCTION: GAME 0x60a4a0
S_car_details *G_CarDatabase::GetCarDetails(uint car)
{
    if (details)
    {
        uint index = car & CAR_INDEX_MASK;
        if (index < num_details)
            return &details[index];
    }
    return NULL;
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

// FUNCTION: GAME 0x60a510
const char *G_CarDatabase::GetCarName(uint car)
{
    uint index = car & CAR_INDEX_MASK;
    if (index >= num_cars)
        // STRING: GAME 0x6396a4
        return "Car out of range";
    return cars[index].name;
}

// The details are by the car's index as the cars are, but the index is tested against the cars' number
// FUNCTION: GAME 0x60a540
uint G_CarDatabase::GetCarSecondName(uint car, uint color)
{
    uint index = car & CAR_INDEX_MASK;
    if (index >= num_cars)
        return 0;
    if (!details)
        return 0;
    if (!color)
        color = car >> CAR_VARIANT_SHIFT;
    switch (color)
    {
    case 1:
        return details[index].color_names[1];
    case 2:
        return details[index].color_names[2];
    case 3:
        return details[index].color_names[3];
    case 4:
        return details[index].color_names[4];
    default:
        return details[index].color_names[0];
    }
}

// FUNCTION: GAME 0x60a600
void G_CarDatabase::RepairIndex(uint &car)
{
    if (!car)
        return;
    if ((car & CAR_INDEX_MASK) >= num_cars)
        car = 0;
    else if (details && car >> CAR_VARIANT_SHIFT >= details[car & CAR_INDEX_MASK].num_colors)
        car &= CAR_INDEX_MASK;
}

// FUNCTION: GAME 0x60a660
uint G_CarDatabase::GetCarMaxColor(uint car)
{
    uint index = car & CAR_INDEX_MASK;
    if (index < num_cars && details && details[index].num_colors)
        return details[index].num_colors - 1;
    return 0;
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

// FUNCTION: GAME 0x60a7c0
uint G_CarDatabase::GetIDByParams(uint car, uint level, uint color)
{
    return (car & CAR_INDEX_MASK) | color << CAR_VARIANT_SHIFT;
}

// GetIDByParams' code, written out
// FUNCTION: GAME 0x60a7e0
uint G_CarDatabase::GetIDByModel(const char *model, uint level, uint color)
{
    return (GetIdxByModel(model) & CAR_INDEX_MASK) | color << CAR_VARIANT_SHIFT;
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

// A model whose name starts with "FMV" is looked up without it. The first car, index 0, is never found (as
// GetIdxByModel's).
// FUNCTION: GAME 0x60a970
const char *G_CarDatabase::GetWreckageByModel(const char *model)
{
    // STRING: GAME 0x6396c4
    if (strstr(model, "FMV") == model)
    {
        model += 3;
        for (uint i = 1; i < num_cars; i++)
        {
            if (!strcmpi(cars[i].id, model))
                return cars[i].wreck_model;
        }
    }
    else
    {
        for (uint i = 1; i < num_cars; i++)
        {
            if (!strcmpi(cars[i].id, model))
                return cars[i].wreck_model;
        }
    }
    return NULL;
}

// As GetWreckageByModel
// FUNCTION: GAME 0x60aa20
const char *G_CarDatabase::GetShadowByModel(const char *model)
{
    if (strstr(model, "FMV") == model)
    {
        model += 3;
        for (uint i = 1; i < num_cars; i++)
        {
            if (!strcmpi(cars[i].id, model))
                return cars[i].shadow_model;
        }
    }
    else
    {
        for (uint i = 1; i < num_cars; i++)
        {
            if (!strcmpi(cars[i].id, model))
                return cars[i].shadow_model;
        }
    }
    return NULL;
}

// The first car, index 0, is never found (as GetIdxByModel's)
// FUNCTION: GAME 0x60aad0
uint G_CarDatabase::GetCivilMaskByModel(const char *model)
{
    for (uint i = 1; i < num_cars; i++)
    {
        if (!strcmpi(cars[i].id, model))
            return cars[i].civil_mask;
    }
    return 0;
}

// FUNCTION: GAME 0x60ab30
uint G_CarDatabase::GetCarsByClass(uint car_class, std::vector<uint> *list, G_EnabledCars *enabled)
{
    list->clear();
    for (int i = 0; i < num_cars; i++)
    {
        if (cars[i].car_class & car_class)
        {
            if (!enabled || enabled->HaveCar(i, false))
                list->push_back(i);
        }
    }
    return list->size();
}

// FUNCTION: GAME 0x60abe0
uint G_CarDatabase::GetCarsByManufacturer(uint manufacturer, std::vector<uint> *list, G_EnabledCars *enabled)
{
    uint mask = 1 << manufacturer;
    list->clear();
    for (int i = 0; i < num_cars; i++)
    {
        if (cars[i].manufacturer & mask)
        {
            if (!enabled || enabled->HaveCar(i, false))
                list->push_back(i);
        }
    }
    return list->size();
}

// FUNCTION: GAME 0x60aca0
uint G_CarDatabase::GetCarsByCarcyclopediaMask(uint mask, std::vector<uint> *list)
{
    list->clear();
    if (!details)
        return 0;
    for (int i = 0; i < num_details; i++)
    {
        if (details[i].carcyclopedia_mask & mask)
            list->push_back(i);
    }
    return list->size();
}

// FUNCTION: GAME 0x60ad40
bool G_CarDatabase::IsEnabledForRace(uint car, uint mask)
{
    uint index = car & CAR_INDEX_MASK;
    if (index >= num_cars)
        return false;
    return cars[index].race_cars & mask ? true : false;
}

// FUNCTION: GAME 0x60ad80
bool G_CarDatabase::IsEnabledForChampionship(uint car, uint mask)
{
    uint index = car & CAR_INDEX_MASK;
    if (index >= num_cars)
        return false;
    return cars[index].car_class & mask ? true : false;
}

// FUNCTION: GAME 0x60adc0
bool G_CarDatabase::IsEnabledForFreeride(uint car, uint mask)
{
    uint index = car & CAR_INDEX_MASK;
    if (index >= num_cars)
        return false;
    return cars[index].freeride_mask & mask ? true : false;
}

// FUNCTION: GAME 0x60ae00
uint G_CarDatabase::GetCarClass(uint car)
{
    uint index = car & CAR_INDEX_MASK;
    if (index >= num_cars)
        return 0;
    return cars[index].car_class;
}

// The first car, index 0, is never found (as GetIdxByModel's)
// FUNCTION: GAME 0x60ae30
uint G_CarDatabase::GetStealTime(const char *model)
{
    for (uint i = 1; i < num_cars; i++)
    {
        if (!strcmpi(cars[i].id, model))
            return cars[i].steal_time;
    }
    return STEAL_TIME_DEFAULT;
}

// FUNCTION: GAME 0x60aea0
uint G_CarDatabase::GetPlayerCivilMaskByModel(const char *model)
{
    for (uint i = 0; i < num_players; i++)
    {
        if (!strcmpi(players[i].model, model))
            return players[i].flags;
    }
    return 0;
}

// PC only (PS2's GetCarsByRaceMask, which isn't on the PC, is the cars' by their race_cars): the models are pointers
// into the table
// FUNCTION: GAME 0x60aef0
uint G_CarDatabase::GetPlayersByCivilMask(uint mask, std::vector<char *> *list)
{
    list->clear();
    for (int i = 0; i < num_players; i++)
    {
        if (players[i].flags & mask)
            list->push_back(players[i].model);
    }
    return list->size();
}

// FUNCTION: GAME 0x60af70
void G_CarDatabase::LoadDetails()
{
    if (++details_ref > 1)
        return;
    details_ref = 0;
    // STRING: GAME 0x6396c8
    int file = dtaOpen("tables\\carcyclopedia.def", false);
    num_details = 0;
    if (file < 0)
        return;
    uint size = dtaSeek(file, 0, SEEK_END);
    dtaSeek(file, 0, SEEK_SET);
    details = (S_car_details *)new char[size];
    dtaRead(file, details, size);
    dtaClose(file);
    details_ref = 1;
    num_details = size / sizeof(S_car_details);
}

// FUNCTION: GAME 0x60b000
void G_CarDatabase::UnloadDetails()
{
    if (--details_ref <= 0 && details)
    {
        num_details = 0;
        delete[] details;
        details = NULL;
    }
    details_ref = 0;
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

// PS2's also places the buffer in its memory card's image
// FUNCTION: GAME 0x60b1a0
void *G_Stream::WantWrite(uint size)
{
    if (!size)
        return NULL;
    buffer = new uchar[size];
    this->size = size;
    return buffer;
}

// FUNCTION: GAME 0x60b1d0
void G_Stream::Write()
{
    if (crypt)
        Crypt(buffer, size);
    fwrite(buffer, 1, size, file);
    delete[] buffer;
    size = 0;
}

// FUNCTION: GAME 0x60b210
void *G_Stream::Read(uint size)
{
    if (!size)
        return NULL;
    buffer = new uchar[size];
    fread(buffer, 1, size, file);
    if (crypt)
        Decrypt(buffer, size);
    return buffer;
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

// The replay's buffer, its position and length, and the tick's dword (PS2 names)
// GLOBAL: GAME 0x6c4540
uint replayBuffer[REPLAY_LENGTH];
// GLOBAL: GAME 0x6d4c04
uint replayPos;
// GLOBAL: GAME 0x6d4c08
uint replayLen;
// GLOBAL: GAME 0x6c453c
uint replayDword;

// A recording keeps each tick's dword when the next tick starts, the first tick only starting it
// FUNCTION: GAME 0x60b400
void G_LoadSaveClass::ReplayTick(int &keys)
{
    if (replay_mode == REPLAY_OFF)
        return;
    if (replay_mode < REPLAY_OFF)
    {
        if (replay_mode < REPLAY_RECORD)
            replay_mode = REPLAY_RECORD;
        else
            _replayPutNext(replayDword);
        replayDword = 0;
        if (keys > REPLAY_KEYS)
            keys = REPLAY_KEYS;
        replayDword |= keys;
    }
    else
    {
        replayDword = _replayGetNext();
        keys = replayDword & REPLAY_KEYS;
    }
}

// FUNCTION: GAME 0x60b470
void G_LoadSaveClass::ReplaySet(bool record, bool play)
{
    replayPos = 0;
    replayDword = 0;
    if (record)
    {
        replayLen = 0;
        replay_mode = REPLAY_RECORD_START;
    }
    else if (play)
        replay_mode = REPLAY_PLAY;
    else
        replay_mode = REPLAY_OFF;
    unknown_90 = true;
}

// FUNCTION: GAME 0x60b4d0
bool G_LoadSaveClass::ReplayLoad(uint number)
{
    char name[256];
    // STRING: GAME 0x6396e8
    sprintf(name, "savegame\\reply%03d.r%02d", profile.number, number);
    FILE *f = fopen(name, "rb");
    if (!f)
        return false;
    uint size;
    fread(&size, 1, sizeof(size), f);
    if (size)
    {
        uchar *data = new uchar[size];
        fread(data, 1, size, f);
        if (CRacing)
            CRacing->LoadReplay(size, data);
        delete[] data;
    }
    fread(replayBuffer, sizeof(uint), REPLAY_LENGTH, f);
    fclose(f);
    replayLen = REPLAY_LENGTH;
    return true;
}

// FUNCTION: GAME 0x60b5a0
uint G_LoadSaveClass::_replayGetNext()
{
    uint value = replayBuffer[replayPos];
    replayPos++;
    if (replayPos >= replayLen)
        replayPos = replayLen - 1;
    if (replayPos >= REPLAY_LENGTH)
        replayPos = REPLAY_LENGTH - 1;
    return value;
}

// FUNCTION: GAME 0x60b5e0
void G_LoadSaveClass::_replayPutNext(uint value)
{
    replayBuffer[replayPos] = value;
    replayPos++;
    if (replayPos >= REPLAY_LENGTH)
        replayPos = REPLAY_LENGTH - 1;
    replayLen = replayPos;
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

// FUNCTION: GAME 0x60b650
uint G_LoadSaveClass::RaceCarsGetEnabled()
{
    return race_cars;
}

// PS2's also enables the car's free ride bits (its 0xa0)
// FUNCTION: GAME 0x60b660
void G_LoadSaveClass::RaceCarsAddWin(uint car)
{
    race_cars |= mafiaCarDatabase.GetCar(car)->race_cars;
}

// FUNCTION: GAME 0x60b690
void G_LoadSaveClass::ObjectivesClear()
{
    objectives.clear();
}

// An objective the list has already isn't added again
// FUNCTION: GAME 0x60b6c0
void G_LoadSaveClass::ObjectivesAdd(uint id)
{
    for (uint *p = objectives.begin(); p < objectives.end(); p++)
    {
        if (*p == id)
            return;
    }
    objectives.push_back(id);
    ObjectivesDisplay();
    mafiaIndicators.MissionHelpSetTime(5000);
}

// PS2's shows the objectives left again
// FUNCTION: GAME 0x60b8a0
void G_LoadSaveClass::ObjectivesRemove(uint id)
{
    for (uint *p = objectives.begin(); p < objectives.end(); p++)
    {
        if (*p == id)
        {
            objectives.erase(p);
            return;
        }
    }
}

// FUNCTION: GAME 0x60b8f0
void G_LoadSaveClass::ObjectivesDisplay()
{
    mafiaIndicators.MissionHelpClear();
    for (uint *p = objectives.begin(); p < objectives.end(); p++)
        mafiaIndicators.MissionHelpAdd(*p);
    mafiaIndicators.MissionHelpSetTime(3000);
}

// FUNCTION: GAME 0x60b940
void G_LoadSaveClass::GarageClear()
{
    for (int i = 0; i < MAX_GARAGE_CARS; i++)
    {
        garage[i].car = 0;
        garage[i].actor = NULL;
        shown_cars[i] = 0;
    }
}

// The car into the first free slot, its level cleared (GetIDByParams sets none); PS2's takes the color only. The race
// cars it enables are enabled also where the garage is full.
// FUNCTION: GAME 0x60b970
void G_LoadSaveClass::AddCarByIdx(uint car, uint color, uint level)
{
    for (int i = 0; i < MAX_GARAGE_CARS; i++)
    {
        if (!garage[i].car && !garage[i].actor)
        {
            garage[i].car = mafiaCarDatabase.GetIDByParams(car, level, color);
            garage[i].car &= ~(0xff << CAR_LEVEL_SHIFT);
            garage[i].actor = NULL;
            break;
        }
    }
    RaceCarsEnable(mafiaCarDatabase.GetCar(car & CAR_INDEX_MASK)->race_cars);
}

// A car the garage has already isn't added again (PS2's takes its descriptor from GetIDByActor and leaves out the
// police's cars and the trucks)
// FUNCTION: GAME 0x60b9f0
void G_LoadSaveClass::AddCarByActor(C_car *car)
{
    if (!car)
        return;
    uint id = mafiaCarDatabase.GetIDByModel(car->model_name, car->GetActualLevel(), car->GetColor());
    int i;
    for (i = 0; i < MAX_GARAGE_CARS; i++)
    {
        if (garage[i].car && garage[i].actor == car)
            return;
    }
    for (i = 0; i < MAX_GARAGE_CARS; i++)
    {
        if (!garage[i].car && !garage[i].actor)
        {
            garage[i].car = id;
            garage[i].car &= ~(0xff << CAR_LEVEL_SHIFT);
            garage[i].actor = car;
            break;
        }
    }
    RaceCarsEnable(mafiaCarDatabase.GetCar(id & CAR_INDEX_MASK)->race_cars);
}

// The first car of the model's index, whatever its level and variant
// FUNCTION: GAME 0x60baa0
void G_LoadSaveClass::RemoveCar(char *model, int level)
{
    uint index = mafiaCarDatabase.GetIdxByModel(model);
    for (int i = 0; i < MAX_GARAGE_CARS; i++)
    {
        if ((garage[i].car & CAR_INDEX_MASK) == index)
        {
            garage[i].car = 0;
            garage[i].actor = NULL;
            return;
        }
    }
}

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
    show_results = false;
    add_last_stolen = false;
    int park = 0;
    // STRING: GAME 0x639738
    if (!act_mission->GetScene()->FindFrame("oblast_garaze_a", ENUMF_ALL) &&
        // STRING: GAME 0x63971c
        !act_mission->GetScene()->FindFrame("watercity.oblast_garaze_a", ENUMF_ALL))
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
            park_frame = act_mission->GetScene()->FindFrame(park_name, ENUMF_ALL);
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
            model->LinkTo(act_mission->GetScene()->primary_sector, 0);
            S_vector offset = park_frame->GetWorldDir() * model->GetHRBoundVolume().bbox.max.z;
            model->SetPos(park_frame->GetWorldPos() + offset);
            model->SetDir(park_frame->GetWorldDir(), 0.0f);
            char car_name[32];
            // STRING: GAME 0x639704
            sprintf(car_name, "g_car_%d", i);
            model->SetName(car_name);
            model->Update();
            I3D_scene *scene = act_mission->GetScene();
            scene->SetFrameSectorPos(model, model->GetWorldPos());
            car = (C_car *)act_mission->CreateActor(ACTOR_CAR);
            if (car->Init(model))
            {
                car->SetActualLevel(level);
                car->GameInit();
                car->CalmVehicle(40);
                act_mission->game->inactive_actors.push_back((C_actor *&)car);
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
                frame->LinkTo(act_mission->GetScene()->primary_sector, 0);
                S_vector offset = park_frame->GetWorldDir() * frame->GetHRBoundVolume().bbox.max.z;
                frame->SetPos(park_frame->GetWorldPos() + offset);
                frame->SetDir(park_frame->GetWorldDir(), 0.0f);
                char car_name[32];
                sprintf(car_name, "g_car_%d", i);
                frame->SetName(car_name);
                frame->Update();
                I3D_scene *scene = act_mission->GetScene();
                scene->SetFrameSectorPos(frame, frame->GetWorldPos());
            }
            car->Reset(0.0f, true);
            car->CalmVehicle(40);
            act_mission->game->inactive_actors.push_back((C_actor *&)car);
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

// A frame with "please wait" (text 0x581) at the screen's bottom right (the name is ours, after PS2's
// G_IndicatorsClass::PleaseWait, which draws more; C_game::Done draws the same)
static inline void PleaseWait()
{
    igraph->ResetRenderProps();
    igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    igraph->BeginScene();
    igraph->SetState(DXS_ALPHA, ALPHASTATE_BLEND);
    mafiaIndicators.OutText(textDatabase.GetText(0x581), mafiaIndicators.screen_scale.x * 780.0f + levyOkraj.x,
        mafiaIndicators.screen_scale.y * 545.0f, mafiaIndicators.screen_scale.x * 0.0f,
        mafiaIndicators.screen_scale.y * 40.0f, (uint)-1, OUTTEXT_RIGHT, 1, NULL);
    igraph->EndScene();
    igraph->Present();
}

// PS2 G_GarageClass::GameDone's, the PC's own: the cars kept are the garage's, the mission's (cars, which stay where
// they are) and the mission's cars standing in the garage's box (between the regions oblast_garaze_a and
// oblast_garaze_b), but a frame named "nesahat" ("don't touch") and the trucks, and with show_results or add_last_stolen
// set last_stolen. Past GARAGE_ROOM the menu chooses which go; then the garage's actors leave the mission, kept for the
// next one unless they are its own, the kept cars fill the free slots, and a frame with "please wait" (text 0x581) is
// drawn. Open: the loop over cars at the start has an empty body (PS2's forgets its last stolen car there), and the
// last search through cars reads cars->end() without a test of cars, as the original does.
// FUNCTION: GAME 0x60c640
void G_LoadSaveClass::GarageGameDone(std::vector<C_car *> *cars)
{
    std::vector<S_garage_item> items;
    S_garage_item *p;
    S_garage_item item;
    if (cars)
    {
        for (C_car **c = cars->begin(); c < cars->end(); c++)
            ;
    }
    I3D_frame *area_a = act_mission->GetScene()->FindFrame("oblast_garaze_a", ENUMF_ALL);
    if (!area_a)
        area_a = act_mission->GetScene()->FindFrame("watercity.oblast_garaze_a", ENUMF_ALL);
    // STRING: GAME 0x63976c
    I3D_frame *area_b = act_mission->GetScene()->FindFrame("oblast_garaze_b", ENUMF_ALL);
    if (!area_b)
        // STRING: GAME 0x639750
        area_b = act_mission->GetScene()->FindFrame("watercity.oblast_garaze_b", ENUMF_ALL);
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
        if (!garage[i].actor)
        {
            if (garage[i].car)
            {
                item.car = garage[i].car & ~(0xff << CAR_LEVEL_SHIFT);
                item.actor = garage[i].actor;
                item.slot = i;
                items.push_back(item);
            }
            continue;
        }
        bool mission_car = false;
        if (cars)
        {
            C_car **it;
            for (it = cars->begin(); it < cars->end(); it++)
            {
                if (*it == garage[i].actor)
                    break;
            }
            if (it < cars->end())
                mission_car = true;
        }
        if (!mission_car && !garage[i].actor->IsCarUsable(1))
            continue;
        item.actor = garage[i].actor;
        item.car = garage[i].car & ~(0xff << CAR_LEVEL_SHIFT);
        item.slot = i;
        items.push_back(item);
        RaceCarsEnable(mafiaCarDatabase.GetCar(item.car & CAR_INDEX_MASK)->race_cars);
    }

    // The mission's cars in the garage's box, of its actors and of the garage's cars
    C_game *game = act_mission->game;
    for (C_actor **a = game->active_actors.begin(); a < game->active_actors.end(); a++)
    {
        if ((*a)->GetType() != ACTOR_CAR)
            continue;
        S_vector pos = (*a)->GetPos();
        if (pos.x < box_min.x || pos.z < box_min.z || pos.x > box_max.x || pos.z > box_max.z)
            continue;
        if (!(*a)->GetFrame()->IsOn())
            continue;
        if (((C_car *)*a)->police_model)
            continue;
        // STRING: GAME 0x639748
        if (strstr((*a)->GetFrame()->GetName(), "nesahat"))
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
        item.car = mafiaCarDatabase.GetIDByActor((C_car *)*a);
        uint index = item.car & CAR_INDEX_MASK;
        if (index == CAR_TRUCK_FLATBED || index == CAR_TRUCK_COVERED || index == CAR_TRUCK)
            continue;
        item.car &= ~(0xff << CAR_LEVEL_SHIFT);
        item.actor = (C_car *)*a;
        item.slot = -1;
        items.push_back(item);
        RaceCarsEnable(mafiaCarDatabase.GetCar(item.car & CAR_INDEX_MASK)->race_cars);
    }
    game = act_mission->game;
    for (C_actor **b = game->inactive_actors.begin(); b < game->inactive_actors.end(); b++)
    {
        if ((*b)->GetType() != ACTOR_CAR)
            continue;
        S_vector pos = (*b)->GetPos();
        if (pos.x < box_min.x || pos.z < box_min.z || pos.x > box_max.x || pos.z > box_max.z)
            continue;
        if (!(*b)->GetFrame()->IsOn())
            continue;
        if (((C_car *)*b)->police_model)
            continue;
        if (strstr((*b)->GetFrame()->GetName(), "nesahat"))
            continue;
        if (!((C_car *)*b)->IsCarUsable(1))
            continue;
        for (p = items.begin(); p < items.end(); p++)
        {
            if (p->actor == *b)
                break;
        }
        if (p < items.end())
            continue;
        item.car = mafiaCarDatabase.GetIDByActor((C_car *)*b);
        uint index = item.car & CAR_INDEX_MASK;
        if (index == CAR_TRUCK_FLATBED || index == CAR_TRUCK_COVERED || index == CAR_TRUCK)
            continue;
        item.car &= ~(0xff << CAR_LEVEL_SHIFT);
        item.actor = (C_car *)*b;
        item.slot = -1;
        items.push_back(item);
        RaceCarsEnable(mafiaCarDatabase.GetCar(item.car & CAR_INDEX_MASK)->race_cars);
    }

    if ((show_results || add_last_stolen) && last_stolen && !last_stolen->police_model && last_stolen->IsCarUsable(1))
    {
        item.car = mafiaCarDatabase.GetIDByActor(last_stolen);
        uint index = item.car & CAR_INDEX_MASK;
        if (index != CAR_TRUCK_FLATBED && index != CAR_TRUCK_COVERED && index != CAR_TRUCK &&
            index != CAR_RACING_BUGATTI)
        {
            item.actor = last_stolen;
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
                RaceCarsEnable(mafiaCarDatabase.GetCar(item.car & CAR_INDEX_MASK)->race_cars);
            }
        }
    }

    // The player chooses which go, until the rest fits
    std::vector<uint> discard;
    discard.clear();
    if (act_mission->game->mission_passed)
    {
        int extra = 0;
        if (show_results)
            extra = 1;
        while (items.size() > discard.size() - extra + GARAGE_ROOM)
        {
            GM_Menu *menu = new GM_GarageEndOfMission(&items, &discard, cars);
            GM_Menu::ExecuteMenu(menu, false, true);
        }
    }
    if (cars)
    {
        for (int j = 0; j < items.size(); j++)
        {
            for (C_car **it = cars->begin(); it < cars->end(); it++)
            {
                if (*it == items[j].actor)
                    discard.push_back(j);
            }
        }
    }
    for (uint k = 0; k < items.size(); k++)
    {
        for (uint *d = discard.begin(); d < discard.end(); d++)
        {
            if (*d == k)
            {
                if (cars)
                {
                    C_car **it;
                    for (it = cars->begin(); it < cars->end(); it++)
                    {
                        if (*it == items[k].actor)
                            break;
                    }
                    if (it < cars->end())
                        break;
                }
                if (items[k].slot != -1)
                    garage[items[k].slot].car = 0;
                items[k].car = 0;
                items[k].actor = NULL;
                items[k].slot = 0;
                break;
            }
        }
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
            C_car **it;
            for (it = cars->begin(); it < cars->end(); it++)
            {
                if (*it == garage[n].actor)
                    break;
            }
            if (it < cars->end())
                continue;
        }
        for (int seat = 0; seat < garage[n].actor->GetNumSeats(); seat++)
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
        for (C_actor **a = game->active_actors.begin(); a < game->active_actors.end(); a++)
        {
            if (*a == garage[n].actor)
            {
                game->active_actors.erase(a);
                a--;
            }
        }
        game = act_mission->game;
        for (C_actor **b = game->inactive_actors.begin(); b < game->inactive_actors.end(); b++)
        {
            if (*b == garage[n].actor)
            {
                game->inactive_actors.erase(b);
                b--;
            }
        }
        mafiaIndicators.RadarRemoveCar(garage[n].actor);
        act_mission->game->InvalidateActor(garage[n].actor, false);
        if (!act_mission->IsOwnActor(garage[n].actor))
        {
            garage[n].actor->temporary = true;
            act_mission->UnloadActor(garage[n].actor);
            act_mission->FrmActBuff_StoreActor(garage[n].actor, false);
        }
        garage[n].actor = NULL;
    }

    // The kept cars new to the garage take its free slots
    int s = 0;
    std::vector<uint> free_slots;
    for (; s < MAX_GARAGE_CARS; s++)
        free_slots.push_back(s);
    for (p = items.begin(); p < items.end(); p++)
    {
        if (p->slot == -1)
            continue;
        for (uint *f = free_slots.begin(); f < free_slots.end(); f++)
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
        C_car **it;
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

    if (act_mission->game->mission_passed && show_results)
        GarageShowResultsProc();
    PleaseWait();
    show_results = false;
    add_last_stolen = false;
    last_stolen = NULL;
}

// FUNCTION: GAME 0x60d480
const char *GM_GarageEndOfMission::OnBeforeCreate()
{
    // STRING: GAME 0x63977c
    return "garageendofmission";
}

// The garage manager (PS2's G_GarageClass::CarManager, the PC's own): the garage's cars, but those whose actor's frame
// is off, and the mission's cars in the garage's box (between the regions oblast_garaze_a and oblast_garaze_b, all of
// them where the scene has no oblast_garaze_a), but the dead, those switched off and a frame named "nesahat" ("don't
// touch"). The menu runs again after each click (GARAGEMANAGER_AGAIN), with the item clicked active again; then the
// cars chosen are destroyed behind a fade.
// FUNCTION: GAME 0x60d490
void G_LoadSaveClass::CarManager()
{
    std::vector<uint> destroy;
    std::vector<S_garage_item> items;
    I3D_frame *area_a = act_mission->GetScene()->FindFrame("oblast_garaze_a", ENUMF_ALL);
    if (!area_a)
        area_a = act_mission->GetScene()->FindFrame("watercity.oblast_garaze_a", ENUMF_ALL);
    I3D_frame *area_b = act_mission->GetScene()->FindFrame("oblast_garaze_b", ENUMF_ALL);
    if (!area_b)
        area_b = act_mission->GetScene()->FindFrame("watercity.oblast_garaze_b", ENUMF_ALL);
    S_vector box_min, box_max;
    box_min = box_max = S_vector(0.0f, 0.0f, 0.0f);
    if (area_a && area_b)
    {
        box_min = MinVector(area_a->GetWorldPos(), area_b->GetWorldPos());
        box_max = MaxVector(area_a->GetWorldPos(), area_b->GetWorldPos());
    }
    items.clear();
    S_garage_item item;

    // The garage's cars: those without an actor, and those whose actor's frame isn't off
    for (int i = 0; i < MAX_GARAGE_CARS; i++)
    {
        if (!garage[i].actor)
        {
            if (!garage[i].car)
                continue;
        }
        else
        {
            if (garage[i].actor->frame && !garage[i].actor->frame->IsOn())
                continue;
        }
        item.car = garage[i].car;
        item.actor = garage[i].actor;
        item.slot = i;
        items.push_back(item);
    }

    // The mission's cars in the garage's box, of its actors and of the garage's cars
    S_garage_item *p;
    C_game *game = act_mission->game;
    for (C_actor **a = game->active_actors.begin(); a < game->active_actors.end(); a++)
    {
        if ((*a)->GetType() != ACTOR_CAR)
            continue;
        if ((*a)->GetDead())
            continue;
        if ((*a)->GetFrame() && !(*a)->GetFrame()->IsOn())
            continue;
        S_vector pos = (*a)->GetPos();
        if (area_a && (pos.x < box_min.x || pos.z < box_min.z || pos.x > box_max.x || pos.z > box_max.z))
            continue;
        if (strstr((*a)->GetFrame()->GetName(), "nesahat"))
            continue;
        for (p = items.begin(); p < items.end(); p++)
        {
            if (p->actor == *a)
                break;
        }
        if (p < items.end())
            continue;
        item.car = 0;
        item.actor = (C_car *)*a;
        item.slot = -1;
        items.push_back(item);
    }
    game = act_mission->game;
    for (C_actor **b = game->inactive_actors.begin(); b < game->inactive_actors.end(); b++)
    {
        if ((*b)->GetType() != ACTOR_CAR)
            continue;
        if ((*b)->GetDead())
            continue;
        if ((*b)->GetFrame() && !(*b)->GetFrame()->IsOn())
            continue;
        S_vector pos = (*b)->GetPos();
        if (area_a && (pos.x < box_min.x || pos.z < box_min.z || pos.x > box_max.x || pos.z > box_max.z))
            continue;
        if (strstr((*b)->GetFrame()->GetName(), "nesahat"))
            continue;
        for (p = items.begin(); p < items.end(); p++)
        {
            if (p->actor == *b)
                break;
        }
        if (p < items.end())
            continue;
        item.car = 0;
        item.actor = (C_car *)*b;
        item.slot = -1;
        items.push_back(item);
    }

    uint active = 0;
    GM_Menu *menu;
    do
        menu = new GM_GarageManager(&items, &destroy, &active, cant_get_out);
    while (GM_Menu::ExecuteMenu(menu, false, true) == GARAGEMANAGER_AGAIN);
    if (destroy.size())
    {
        mafiaIndicators.FadeInOut(false, 300, 0xffffffff);
        for (uint *d = destroy.begin(); d < destroy.end(); d++)
            DestroyCar(&items[*d]);
    }
}

// FUNCTION: GAME 0x60da70
const char *GM_GarageManager::OnBeforeCreate()
{
    // STRING: GAME 0x639790
    return "garagemanager";
}

// A car the garage manager's player chose to destroy (PS2's DestroyCar takes the car): not one with somebody in it. A
// mission's car is only taken out of the game; a garage car's actor leaves the mission and is released, and its slot
// is emptied.
// FUNCTION: GAME 0x60da80
void G_LoadSaveClass::DestroyCar(S_garage_item *item)
{
    if (item->actor)
    {
        for (int seat = 0; seat < item->actor->GetNumSeats(); seat++)
        {
            if (item->actor->GetOwner(seat))
                return;
        }
    }
    if (item->slot != -1)
    {
        if (item->actor)
        {
            item->actor->DeactivateCar(0);
            item->actor->SetActState(ACTSTATE_NONE);
            item->actor->frame->SetOn(false);
            act_mission->game->InvalidateActor(item->actor, false);
            std::vector<C_actor *> &active = act_mission->game->active_actors;
            for (C_actor **a = active.begin(); a < active.end(); a++)
            {
                if (*a == item->actor)
                {
                    active.erase(a);
                    a--;
                }
            }
            std::vector<C_actor *> &inactive = act_mission->game->inactive_actors;
            for (C_actor **b = inactive.begin(); b < inactive.end(); b++)
            {
                if (*b == item->actor)
                {
                    inactive.erase(b);
                    b--;
                }
            }
            mafiaIndicators.RadarRemoveCar(item->actor);
            act_mission->game->InvalidateActor(item->actor, false);
            item->actor->GameDone();
            act_mission->UnloadActor(item->actor);
            item->actor->Release();
        }
        item->actor = NULL;
        garage[item->slot].actor = NULL;
        garage[item->slot].car = 0;
    }
    else
    {
        item->actor->DeactivateCar(0);
        item->actor->SetActState(ACTSTATE_NONE);
        item->actor->frame->SetOn(false);
        act_mission->game->InvalidateActor(item->actor, false);
    }
}

// FUNCTION: GAME 0x60dc80
void G_LoadSaveClass::GarageAddLastStolen()
{
    add_last_stolen = true;
}

// FUNCTION: GAME 0x60dc90
void G_LoadSaveClass::GarageShowResults()
{
    show_results = true;
}

// The cars the garage got and lost since the last results (shown_cars), those of the same descriptor in both left out;
// the menu where there are any, then the garage's cars are the shown ones
// FUNCTION: GAME 0x60dca0
void G_LoadSaveClass::GarageShowResultsProc()
{
    std::vector<uint> removed;
    std::vector<uint> added;
    int i;
    for (i = 0; i < MAX_GARAGE_CARS; i++)
    {
        if (garage[i].car != shown_cars[i])
        {
            if (garage[i].car)
                added.push_back(garage[i].car);
            if (shown_cars[i])
                removed.push_back(shown_cars[i]);
        }
    }
    for (uint *p = added.begin(); p < added.end(); p++)
    {
        for (uint *q = removed.begin(); q < removed.end(); q++)
        {
            if (*p == *q)
            {
                added.erase(p);
                removed.erase(q);
                p--;
                break;
            }
        }
    }
    if (added.size() || removed.size())
    {
        GM_Menu *menu = new GM_GarageSummary(&added, &removed);
        GM_Menu::ExecuteMenu(menu, false, true);
    }
    for (i = 0; i < MAX_GARAGE_CARS; i++)
        shown_cars[i] = garage[i].car;
}

// PS2's also puts the car on the radar
// FUNCTION: GAME 0x60dec0
void G_LoadSaveClass::CarIsStolen(C_car *car)
{
    last_stolen = car;
}

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
    steal_cars |= 1 << bit;
}

// FUNCTION: GAME 0x60df20
bool G_LoadSaveClass::CanSteal(uchar *model)
{
    return steal_cars & mafiaCarDatabase.GetCar(mafiaCarDatabase.GetIdxByModel((char *)model))->steal_mask ? true : false;
}

// FUNCTION: GAME 0x60df60
bool G_LoadSaveClass::IsCarIn(C_car *car)
{
    for (int i = 0; i < MAX_GARAGE_CARS; i++)
    {
        if (garage[i].actor == car)
            return true;
    }
    return false;
}

// EnableSteal's for the free ride's cars
// FUNCTION: GAME 0x60df90
void G_LoadSaveClass::FreerideEnableCar(uchar c)
{
    uint bit = 0;
    if (c >= 'A' && c <= 'Z')
        bit = c - 'A';
    if (c >= 'a' && c <= 'z')
        bit = c - 'a';
    freeride_cars |= 1 << bit;
}

// FUNCTION: GAME 0x60dfd0
uint G_LoadSaveClass::FreerideGetEnabledCars()
{
    return freeride_cars;
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
    if (settings.car_camera < 7 || settings.car_camera >= 0x10)
        settings.car_camera = 7;
}

// FUNCTION: GAME 0x60e140
uint G_LoadSaveClass::GetControlTextID(int index)
{
    if (index >= 0 && index < NUM_CONTROLS)
        return controlTextIDs[index];
    return TEXT_MAFIA;
}

// Draws the line scrolled up by time, while it is between 100 above the screen's top and its bottom (PS2's; the PC adds
// CREDIT_HEADING_LINE). The budget leaves the last two cases' OutTextScaled out of line.
// FUNCTION: GAME 0x60e160
void RenderOneCredit(G_CreditsStruct *credit, float time)
{
    float y = credit->y - time;
    if (y < -100.0f || y > 600.0f)
        return;
    switch (credit->type)
    {
    case CREDIT_HEADING:
        mafiaIndicators.OutTextScaled(credit->title, 390.0f, y + 30.0f, 20.0f, 30.0f, 0xffffffff, OUTTEXT_CENTER, 1,
            NULL);
        break;
    case CREDIT_ROLE:
    default:
        mafiaIndicators.OutTextScaled(credit->text, 370.0f, y, 20.0f, 25.0f, 0xffffffff, OUTTEXT_RIGHT, 0, NULL);
        mafiaIndicators.OutTextScaled(credit->text2, 410.0f, y, 20.0f, 25.0f, 0xffffffff, 0, 0, NULL);
        break;
    case CREDIT_TITLE_NAME:
        mafiaIndicators.OutTextScaled(credit->title, 390.0f, y + 30.0f, 20.0f, 25.0f, 0xffffffff, OUTTEXT_CENTER, 1,
            NULL);
        mafiaIndicators.OutTextScaled(credit->text, 390.0f, y + 60.0f, 20.0f, 25.0f, 0xffffffff, OUTTEXT_CENTER, 0,
            NULL);
        break;
    case CREDIT_TITLE_BESIDE:
        mafiaIndicators.OutTextScaled(credit->title, 370.0f, y, 20.0f, 25.0f, 0xffffffff, OUTTEXT_RIGHT, 1, NULL);
        mafiaIndicators.OutTextScaled(credit->text, 410.0f, y + 2.0f, 20.0f, 20.0f, 0xffffffff, 0, 0, NULL);
        break;
    case CREDIT_TWO_NAMES:
        mafiaIndicators.OutTextScaled(credit->text, 0.0f, y, 400.0f, 20.0f, 0xffffffff, OUTTEXT_CENTER, 0, NULL);
        mafiaIndicators.OutTextScaled(credit->text2, 400.0f, y, 400.0f, 20.0f, 0xffffffff, OUTTEXT_CENTER, 0, NULL);
        break;
    case CREDIT_TITLE:
        mafiaIndicators.OutTextScaled(credit->title, 390.0f, y + 30.0f, 20.0f, 20.0f, 0xffffffff, OUTTEXT_CENTER, 1,
            NULL);
        break;
    case CREDIT_NAME:
        mafiaIndicators.OutTextScaled(credit->text, 390.0f, y, 20.0f, 20.0f, 0xffffffff, OUTTEXT_CENTER, 0, NULL);
        break;
    case CREDIT_HEADING_LINE:
        mafiaIndicators.OutTextScaled(credit->title, 390.0f, y, 20.0f, 30.0f, 0xffffffff, OUTTEXT_CENTER, 1, NULL);
        break;
    }
}

// Gives each line its text and its place, the first at the screen's bottom. CREDIT_HEADING_LINE has a case of its own
// with the default's body (its jump table has an entry for 'H').
// FUNCTION: GAME 0x60e630
void ParseCredits()
{
    float y = 600.0f;
    for (uint i = 0; i < g_numOfCredits; i++)
    {
        g_credits[i].title = textDatabase.GetTextOrNULL((uint)g_credits[i].title);
        g_credits[i].y = y;
        switch (g_credits[i].type)
        {
        case CREDIT_HEADING:
            y += 60.0f;
            break;
        case CREDIT_ROLE:
            y += 25.0f;
            break;
        case CREDIT_TITLE_NAME:
            y += 80.0f;
            break;
        case CREDIT_TITLE_BESIDE:
            y += 25.0f;
            break;
        case CREDIT_TWO_NAMES:
            y += 20.0f;
            break;
        case CREDIT_TITLE:
            y += 20.0f;
            break;
        case CREDIT_NAME:
            y += 25.0f;
            break;
        case CREDIT_HEADING_LINE:
            y += 30.0f;
            break;
        default:
            y += 30.0f;
            break;
        }
    }
}

// Scrolls the credits up until the last line is 100 above the screen's top or a key is pressed, with the credits' font
// (LoadFont's language_credits). PS2's also stops the cars' sounds first and mutes the music around it.
// FUNCTION: GAME 0x60e710
void G_LoadSaveClass::ExecuteCredits()
{
    // STRING: GAME 0x6397b0
    int file = dtaOpen("tables\\credits.def", false);
    if (file < 0)
        return;
    int size = dtaSeek(file, 0, SEEK_END);
    if (size <= 0)
    {
        dtaClose(file);
        return;
    }
    mafiaIndicators.LoadFont(true, 0);
    g_numOfCredits = size / sizeof(G_CreditsStruct);
    g_credits = new G_CreditsStruct[g_numOfCredits];
    dtaSeek(file, 0, SEEK_SET);
    dtaRead(file, g_credits, size);
    dtaClose(file);
    ParseCredits();
    float time = 0.0f;
    if (g_numOfCredits)
    {
        do
        {
            igraph->ProcessWinMessages();
            igraph->UpdateMouseData();
            igraph->UpdateKeyboardData();
            if (igraph->ReadKey())
                break;
            int delta = igraph->GetTimer(1, 200);
            igraph->ResetRenderProps();
            igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
            igraph->BeginScene();
            igraph->SetState(DXS_ALPHA, ALPHASTATE_BLEND);
            for (uint i = 0; i < g_numOfCredits; i++)
                RenderOneCredit(&g_credits[i], time);
            igraph->EndScene();
            igraph->Present();
            time += delta * .03f;
        } while (!(time > g_credits[g_numOfCredits - 1].y + 100.0f));
    }
    delete[] g_credits;
    mafiaIndicators.LoadFont(false, 0);
}

// FUNCTION: GAME 0x60e910
void G_LoadSaveClass::FreerideEnable(uint level)
{
    freeride_enabled = level;
}

// FUNCTION: GAME 0x60e920
void G_LoadSaveClass::FreerideStoreA(uint car, uint flags, float traffic, float pedestrians, float police)
{
    freeride.car = car;
    freeride.flags = flags;
    freeride.traffic_density = traffic;
    freeride.pedestrian_density = pedestrians;
    freeride.police_density = police;
}

// FUNCTION: GAME 0x60e960
void G_LoadSaveClass::FreerideStoreB(uint location)
{
    freeride.location = location;
}

// FUNCTION: GAME 0x60e970
uint G_LoadSaveClass::FreerideGetState()
{
    return freeride.location;
}

// FREERIDE_TRAFFSETUP's: in Free Ride Extreme the traffic takes the free ride menu's densities, the police's for both
// the cars and the pedestrians. The cars that can be stolen are the race cars.
// FUNCTION: GAME 0x60e980
void G_LoadSaveClass::FreerideSetupFromScript()
{
    if (freeride_enabled >= FREERIDE_EXTREME)
    {
        C_traffic_car::elems_coefff = freeride.traffic_density;
        C_traffic_car::police_coefff = freeride.police_density;
        C_traffic_generator::ratio_all = freeride.pedestrian_density;
        C_traffic_generator::ratio_police = freeride.police_density;
    }
    steal_cars = race_cars;
}

// Tommy and his car at the frame emeth_<location> (emeth_1 where there is none), before the game's Init, as PS2's
// FreerideSetupPlayer and FreerideSetupCar (which the PC's has written out, both actors and models first)
// FUNCTION: GAME 0x60e9d0
void G_LoadSaveClass::FreerideSetup()
{
    char name[0x100];
    freeride.active = true;
    C_actor *player = act_mission->CreateActor(ACTOR_PLAYER);
    C_actor *car = act_mission->CreateActor(ACTOR_CAR);
    I3D_model *player_model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
    // STRING: GAME 0x6397e4
    player_model->SetName("Tommy");
    I3D_model *car_model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
    // STRING: GAME 0x6397d8
    car_model->SetName("TomyhoAuto");
    mafiaCarDatabase.GetCarModel(freeride.car, name, false);
    model_cache.Open(car_model, name, 0, NULL, NULL, NULL);
    // STRING: GAME 0x6397cc
    sprintf(name, "emeth_%d", freeride.location);
    I3D_frame *start = act_mission->GetScene()->FindChildFrame(name, ENUMF_ALL);
    if (!start)
        // STRING: GAME 0x6397c4
        start = act_mission->GetScene()->FindChildFrame("emeth_1", ENUMF_ALL);
    if (start)
    {
        car_model->SetPos(start->GetWorldPos());
        car_model->SetDir(start->GetWorldDir(), 0.0f);
        car_model->LinkTo(act_mission->GetScene()->GetPrimarySector(), 0);
        player_model->SetPos(start->GetWorldPos());
        player_model->SetDir(start->GetWorldDir(), 0.0f);
        player_model->LinkTo(act_mission->GetScene()->GetPrimarySector(), 0);
    }
    car_model->Update();
    player_model->Update();
    act_mission->GetScene()->AddFrame(car_model);
    act_mission->GetScene()->AddFrame(player_model);
    car_model->Release();
    player_model->Release();
    car->Init(car_model);
    if (freeride.flags & FREERIDE_ROOF_OFF)
        C_car::OdjebajTuStrechu(car_model, true);
    // STRING: GAME 0x6368a8
    model_cache.Open(player_model, "Tommy.i3d", 0, NULL, NULL, NULL);
    player->Init(player_model);
    act_mission->AddActor(car, false);
    act_mission->AddActor(player, false);
}

// After the game's Init: Tommy in his car, the mission's numbers cleared, the location used
// FUNCTION: GAME 0x60eef0
void G_LoadSaveClass::FreerideSetupAfter()
{
    C_human *player = (C_human *)act_mission->FindActorByName("Tommy");
    C_car *car = (C_car *)act_mission->FindActorByName("TomyhoAuto");
    if (car && player)
        player->Intern_UseCar(car, 0);
    act_mission->SetMissionNumber(0);
    act_mission->KeepMissionNumber();
    freeride.location = 0;
}

// The vectors' destruction only, after FreerideSetupAfter as PS2's
// FUNCTION: GAME 0x60ef50
G_LoadSaveClass::~G_LoadSaveClass()
{
}
