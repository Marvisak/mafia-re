// The game's start and its main loop: PS2's Main.cpp (main, InitSystem, CloseSystem, GameLoopSingle,
// InitErrorMessageBox, MessageBoxGraph). In the original 0x5bce00-0x5c25ac. Written so far: main, InitSystem,
// InitErrorMessageBox and CloseSystem, and the PC's InitJoy, CloseJoy and GraphErrorHandler; SceneCallback and
// GameLoopSingle are stubs.
// Open: the engine's IGraph.h brings LS3DF's I3d_math.h, whose static zero vector gives this file a dynamic initializer
// the original's files don't have (docs/game-files.md). The .bss names: PS2's isound, driver and the caches' don't give
// the original's order (driver, isound, igraph, setting), so at least those had other names on the PC.

#include "Main.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_m_start.h"
#include "ui/g_m_carc.h"
#include "ui/racing.h"
#include "ui/g_loadsave.h"
#include "ui/g_indicators.h"
#include "collision/g_colls.h"
#include "Winstuff.h"
#include "I3D_cache.h"
#include "actors/C_actor.h"
#include "actors/C_animations.h"
#include "vehicles/C_car.h"
#include "vehicles/C_Vehicle.h"
#include "input/C_ff_manager.h"
#include "mission/C_program.h"
#include "input/controls.h"
#include "ui/console.h"
#include "tools.h"

#include <settings_registry.h>
#include <process.h>
#include <stdio.h>
#include <vector>

#include "IGraph/IGraph.h"
#include "I3D/I3D.h"
#include "ISound/ISound.h"
#include "ijoy.h"
#include "rw_data.h"
#include "stub.h"
#include "res/resource.h"

// GLOBAL: GAME 0x647df8
C_mission main_mission;
// GLOBAL: GAME 0x63788c
C_mission *act_mission = &main_mission;
// The game's archives, in the order InitSystem opens them and gives them their keys: their places in dta_names and
// RW_data. a1.dta can stay on the first CD. The names are ours.
enum E_ARCHIVE {
    ARCHIVE_A8,
    ARCHIVE_B8,                         // the only one that may be missing
    ARCHIVE_A2,
    ARCHIVE_A6,
    ARCHIVE_A0,
    ARCHIVE_A1,
    ARCHIVE_A3,
    ARCHIVE_AC,
    ARCHIVE_A4,
    ARCHIVE_AA,
    ARCHIVE_A5,
    ARCHIVE_A7,
    ARCHIVE_A9,
    ARCHIVE_AB,
    NUM_ARCHIVES,
};

// The keys InitSystem passes to UnlockPack. a3.dta, ac.dta and a4.dta share one, a0.dta and ab.dta another.
const __int64 ARCHIVE_KEY_A8 = 0xec45d8ce6a63fa71i64;
const __int64 ARCHIVE_KEY_B8 = 0xb60f823f79e21cdbi64;
const __int64 ARCHIVE_KEY_A2 = 0xb6399e191417d340i64;
const __int64 ARCHIVE_KEY_A6 = 0x5055da68728e2db9i64;
const __int64 ARCHIVE_KEY_A0 = 0xec48fe177f3d9b74i64;
const __int64 ARCHIVE_KEY_A1 = 0x0900210ee7375f59i64;
const __int64 ARCHIVE_KEY_A3 = 0x771f3888a94b8d3ci64;
const __int64 ARCHIVE_KEY_AC = 0x771f3888a94b8d3ci64;
const __int64 ARCHIVE_KEY_A4 = 0x771f3888a94b8d3ci64;
const __int64 ARCHIVE_KEY_AA = 0x67da216ed4ad90c6i64;
const __int64 ARCHIVE_KEY_A5 = 0xea3404204f4bb0c6i64;
const __int64 ARCHIVE_KEY_A7 = 0xe266fe62f4f03a72i64;
const __int64 ARCHIVE_KEY_A9 = 0x5b763446959d1117i64;
const __int64 ARCHIVE_KEY_AB = 0xec48fe177f3d9b74i64;

// GLOBAL: GAME 0x637890
const char *dta_names[NUM_ARCHIVES] = {
    // STRING: GAME 0x637930
    "a8.dta",
    // STRING: GAME 0x637928
    "b8.dta",
    // STRING: GAME 0x637920
    "a2.dta",
    // STRING: GAME 0x637918
    "a6.dta",
    // STRING: GAME 0x637910
    "a0.dta",
    // STRING: GAME 0x637908
    "a1.dta",
    // STRING: GAME 0x637900
    "a3.dta",
    // STRING: GAME 0x6378f8
    "ac.dta",
    // STRING: GAME 0x6378f0
    "a4.dta",
    // STRING: GAME 0x6378e8
    "aa.dta",
    // STRING: GAME 0x6378e0
    "a5.dta",
    // STRING: GAME 0x6378d8
    "a7.dta",
    // STRING: GAME 0x6378d0
    "a9.dta",
    // STRING: GAME 0x6378c8
    "ab.dta",
};
// The original's has a dynamic initializer (0x5bd590)
// GLOBAL: GAME 0x647b28
C_ff_manager ff_manager;
// PS2's has 12 archives
// GLOBAL: GAME 0x647be4
C_rw_data *RW_data[NUM_ARCHIVES];
// The joysticks InitJoy found (PS2's is a vector<C_Joy *>)
// GLOBAL: GAME 0x647c1c
std::vector<IJoy *> joy;
// Constructed by its dynamic initializer (0x5bd450)
// GLOBAL: GAME 0x647c30
C_controls controls;
// The original's three have dynamic initializers
// GLOBAL: GAME 0x647d80
C_I3D_anim_cache anim_cache;
// GLOBAL: GAME 0x647da8
C_I3D_sound_cache sound_cache;
// GLOBAL: GAME 0x647dd0
C_I3D_model_cache model_cache;
// GLOBAL: GAME 0x647ed8
I3D_driver *driver;
// GLOBAL: GAME 0x647edc
ISND_driver *isound;
// GLOBAL: GAME 0x647ee0
IGraph *igraph;
// GLOBAL: GAME 0x647ee4
S_settings setting;
// The name is ours
// GLOBAL: GAME 0x647f24
IJoyBase *joy_base;
// PS2's is initialized data
// GLOBAL: GAME 0x647f28
C_actor *NewMissionActors[2] = {NULL, NULL};
// GLOBAL: GAME 0x647f34
bool _is_intro_ = false;

static bool InitSystem(void *hinstance, bool &fatal);
static void InitErrorMessageBox(LS3D_RESULT result, int base);

// The joysticks (PC only, the name is ours): every device IJoy finds, with force feedback reset and turned on. Its y
// axis is inverted only when CreateJoy fails, which leaves the joystick NULL: CreateJoy returns true on success.
// FUNCTION: GAME 0x5bd5f0
void InitJoy(HINSTANCE hinstance)
{
    if (!CreateJoyBase(&joy_base, hinstance, igraph->GetMainHWND(), NULL, 0))
        return;
    int count = joy_base->NumDevices();
    for (int i = 0; i < count; i++)
    {
        IJoy *j;
        if (!joy_base->CreateJoy(&j, i, JOY_BACKGROUND))
            j->SetAxisMode(JOY_Y, JOY_AXIS_INVERT);
        joy.push_back(j);
        j->SendFFCommand(JOY_FF_RESET);
        j->SendFFCommand(JOY_FF_STOP_ALL);
        j->SendFFCommand(JOY_FF_SET_ACTUATORS_OFF);
        j->SendFFCommand(JOY_FF_SET_ACTUATORS_ON);
    }
}

// Releases the joysticks and IJoy (PC only, the name is ours)
// FUNCTION: GAME 0x5bd870
void CloseJoy()
{
    for (int i = 0; i < joy.size(); i++)
        joy[i]->Release();
    joy.clear();
    if (joy_base)
    {
        joy_base->Release();
        joy_base = NULL;
    }
}

#pragma auto_inline(off)

// The scene's callback (PC only, the name is ours), after the scene and after the whole frame: at the end of the frame
// it draws the HUD and the active menu (0x6bd890) over it. The stub logs its first call only.
// STUB: GAME 0x5bd8d0
uint __stdcall SceneCallback(SCN_CALLBACK_MESSAGE message, uint param1, uint param2)
{
    static bool logged = false;
    if (!logged)
    {
        StubLog("SceneCallback");
        logged = true;
    }
    return 0;
}

// STUB: GAME 0x5be750
void GameLoopSingle()
{
    StubLog("GameLoopSingle");
}

#pragma auto_inline(on)

// IGraph's error handler (PC only, the name is ours). Out of video memory, the game saves the settings without true
// color textures for the next start, says so and quits.
// FUNCTION: GAME 0x5beb00
void __cdecl GraphErrorHandler(LS3D_RESULT result, E_SEVERITY severity, const char *message)
{
    if (result == I3DERR_OUTOFMEM)
    {
        setting.texture_true_color = false;
        SaveSettings(setting);
        igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
        igraph->Present();
        // STRING: GAME 0x637948
        MessageBox(igraph ? igraph->GetMainHWND() : NULL, "error: #8888\r\nVideo memory overflow...", NULL,
            MB_ICONERROR);
        igraph->Close();
        exit(0);
    }
}

// The loop runs the main menu, then what the player chose, until they quit or choose another profile; a new profile
// restarts it. Each choice that plays a mission opens it, loads its collisions and runs it, and the menu's mission is
// opened again on the next pass. The cases are in the original's order, which a single jump table keeps.
// Every exit after the CD check leaves through the end, as PS2's main jumps to its own: with a return inside the loop,
// VC6 saves ebp, esi and edi at the start, where the original saves them after the CD check.
// FUNCTION: GAME 0x5bebe0
int main(HINSTANCE hInstance)
{
    SECURITY_ATTRIBUTES mutex_attributes;
    mutex_attributes.nLength = sizeof(mutex_attributes);
    mutex_attributes.lpSecurityDescriptor = NULL;
    mutex_attributes.bInheritHandle = FALSE;
    // STRING: GAME 0x637b48
    CreateMutex(&mutex_attributes, FALSE, "Mafia Launcher Super Mutex Shit");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return 0;
    // STRING: GAME 0x637b38
    if (!TEV("Main program", ENGINE_VERSION))
        return 0;
    if (!WaitForCD())
        return 0;
    if (!LoadSettings(setting))
    {
        // STRING: GAME 0x637b2c
        _spawnl(_P_NOWAIT, "setup.exe", "setup.exe", NULL);
        goto end;
    }

    SetCursor(NULL);
    bool fatal;
    // An int: the original widens the result once (and eax, 0xff). Assigned apart: the setup.exe branch's goto passes
    // the declaration.
    int ok;
    ok = InitSystem(hInstance, fatal);
    if (fatal)
    {
        CloseSystem();
        goto end;
    }
    if (!ok)
        goto end;

    act_mission->options.SetLevelOfDetail(setting.level_of_detail);
    act_mission->options.SetDrawDistance(setting.draw_distance);
    act_mission->options.SetGameEffects(setting.game_effects);
    act_mission->options.SetGameSounds(setting.game_sounds);
    act_mission->options.SetShadows(setting.shadows);
    act_mission->options.SetParticles(setting.particles);
    _is_intro_ = false;
    act_mission->game->Init();

    int choice;
    bool restart;
    do
    {
        choice = 0;
        // InitSystem opened the menu's mission for the first pass
        bool reload = false;
        while (choice != MAINMENU_QUIT && choice != MAINMENU_PROFILE_CHANGE)
        {
            // The original stores both through a zero register of their own, an inlined memset's; two assignments use
            // the function's (ebx)
            memset(NewMissionActors, 0, sizeof(NewMissionActors));
            igraph->NullTimer();
            if (reload)
            {
                // STRING: GAME 0x636230
                if (act_mission->Open("00menu", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                act_mission->game->Init();
            }
            reload = true;
            choice = GM_Menu::ExecuteMenu(new GM_MainMenu, true, true);

            switch (choice)
            {
            case MAINMENU_LOAD_GAME:
                act_mission->Close();
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameLoad();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_TUTORIAL:
                act_mission->Close();
                // STRING: GAME 0x637b20
                if (act_mission->Open("tutorial", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x637b04
                gifCollision.LoadCollision("missions\\tutorial\\tree.klz", true);
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameNewGame();
                act_mission->game->Init();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_NEW_GAME:
                act_mission->Close();
                // STRING: GAME 0x63437c
                if (act_mission->Open("intro", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x637aec
                gifCollision.LoadCollision("missions\\intro\\tree.klz", true);
                act_mission->ResetUnknown70();
                act_mission->game->Init();
                _is_intro_ = true;
                GameLoopSingle();
                _is_intro_ = false;
                act_mission->Close();
                // STRING: GAME 0x637ae4
                if (act_mission->Open("mise01", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x637ac8
                gifCollision.LoadCollision("missions\\mise01\\tree.klz", true);
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameNewGame();
                act_mission->game->Init();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_FREEITALY:
                act_mission->Close();
                // STRING: GAME 0x637abc
                if (act_mission->Open("freeitaly", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x637aa0
                gifCollision.LoadCollision("missions\\freeitaly\\tree.klz", true);
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameNewGame();
                mafiaLoadSave.FreerideSetup();
                act_mission->game->Init();
                mafiaLoadSave.FreerideSetupAfter();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_FREERIDE:
                act_mission->Close();
                // STRING: GAME 0x637a94
                if (act_mission->Open("freeride", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x637a78
                gifCollision.LoadCollision("missions\\freeride\\tree.klz", true);
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameNewGame();
                mafiaLoadSave.FreerideSetup();
                act_mission->game->Init();
                mafiaLoadSave.FreerideSetupAfter();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_FREERIDENOC:
                act_mission->Close();
                // STRING: GAME 0x637a6c
                if (act_mission->Open("freeridenoc", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x637a4c
                gifCollision.LoadCollision("missions\\freeridenoc\\tree.klz", true);
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameNewGame();
                mafiaLoadSave.FreerideSetup();
                act_mission->game->Init();
                mafiaLoadSave.FreerideSetupAfter();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_FREEKRAJINA:
                act_mission->Close();
                // STRING: GAME 0x637a40
                if (act_mission->Open("freekrajina", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x637a20
                gifCollision.LoadCollision("missions\\freekrajina\\tree.klz", true);
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameNewGame();
                mafiaLoadSave.FreerideSetup();
                act_mission->game->Init();
                mafiaLoadSave.FreerideSetupAfter();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_FREEKRAJINANOC:
                act_mission->Close();
                // STRING: GAME 0x637a10
                if (act_mission->Open("freekrajinanoc", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x6379ec
                gifCollision.LoadCollision("missions\\freekrajinanoc\\tree.klz", true);
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameNewGame();
                mafiaLoadSave.FreerideSetup();
                act_mission->game->Init();
                mafiaLoadSave.FreerideSetupAfter();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_EXTREME:
                act_mission->Close();
                // STRING: GAME 0x6379e4
                if (act_mission->Open("extreme", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x6379c8
                gifCollision.LoadCollision("missions\\extreme\\tree.klz", true);
                act_mission->ResetUnknown70();
                mafiaLoadSave.SaveGameNewGame();
                act_mission->game->Init();
                GameLoopSingle();
                act_mission->Close();
                break;
            case MAINMENU_RACING:
                // The championship keeps the menu's mission
                CRacing = new C_racing;
                ExecuteRacingMenu();
                delete CRacing;
                CRacing = NULL;
                reload = false;
                break;
            case MAINMENU_CARCYCLOPEDIA:
                act_mission->Close();
                // STRING: GAME 0x6379bc
                if (act_mission->Open("autosalon", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                // STRING: GAME 0x6379a0
                gifCollision.LoadCollision("missions\\autosalon\\tree.klz", true);
                act_mission->ResetUnknown70();
                act_mission->game->Init();
                mafiaIndicators.ParheliaSetCenter(true);
                GM_Menu::ExecuteMenu(new GM_Carcyclopedia, false, true);
                mafiaIndicators.ParheliaSetCenter(false);
                act_mission->Close();
                break;
            case MAINMENU_INTRO:
                act_mission->Close();
                if (act_mission->Open("intro", false, (uint)-1, true))
                {
                    CloseSystem();
                    goto end;
                }
                gifCollision.LoadCollision("missions\\intro\\tree.klz", true);
                act_mission->ResetUnknown70();
                act_mission->game->Init();
                _is_intro_ = true;
                GameLoopSingle();
                _is_intro_ = false;
                act_mission->Close();
                break;
            case MAINMENU_CREDITS:
                mafiaLoadSave.ExecuteCredits();
                reload = false;
                break;
            }
        }
        // Through a flag: tested in the loop's condition directly, the call's result gets test al, al where the
        // original compares it with the zero register (PS2 materializes the value too)
        restart = choice == MAINMENU_PROFILE_CHANGE ? mafiaLoadSave.ProfileChange() : false;
    } while (restart);

    act_mission->Close();
    setting.level_of_detail = act_mission->options.GetLevelOfDetail();
    setting.draw_distance = act_mission->options.GetDrawDistance();
    setting.game_effects = act_mission->options.game_effects;
    setting.game_sounds = act_mission->options.game_sounds;
    setting.shadows = act_mission->options.GetShadows();
    setting.particles = act_mission->options.particles;
    SaveSettings(setting);
    CloseSystem();
end:
    return 0;
}

// Starts the engine and loads what the game needs throughout: the archives and their keys, the version check, IGraph,
// the sound, I3D, the mission's scene, the input, the caches' folders, the tables, the language and the logo videos.
// False after a message box if something is missing. Once it got that far, fatal is set if the profiles can't be set
// up; main reads it also after a failure, when it isn't set.
// The version check and the language are blocks of their own: the language's buffer shares the version's stack slots.
// Open: from the fatal store on, the code generator's registers are one step on in the rotation from the original's
// (edx and cl for its ecx and al), and so are those of the version check's failure, which VC6 generates last. A
// temporary of ours the original doesn't have, most likely in the language's block: initializing the buffer with {0}
// moves it the other way.
// FUNCTION: GAME 0x5bf5a0
static bool InitSystem(void *hinstance, bool &fatal)
{
    dtaSetDtaFirstForce();
    // Unsigned: the loops that count up compare the byte offset VC6 walks with jb
    uint i;
    for (i = 0; i < NUM_ARCHIVES; i++)
        RW_data[i] = dtaCreate(dta_names[i]);
    char text[512];
    if (!RW_data[ARCHIVE_A1] && FindGameFile("a1.dta", text))
        RW_data[ARCHIVE_A1] = dtaCreate(text);
    // Every archive but b8.dta has to be there
    for (i = NUM_ARCHIVES; i--;)
    {
        if (!RW_data[i] && i != ARCHIVE_B8)
        {
            // STRING: GAME 0x637c3c
            sprintf(text, "error: #3075\r\nFile  \"%s\"  not found...", dta_names[i]);
            MessageBox(NULL, text, "Init error", MB_ICONERROR);
            return false;
        }
    }

    if (RW_data[ARCHIVE_A8])
        RW_data[ARCHIVE_A8]->UnlockPack(ARCHIVE_KEY_A8);
    if (RW_data[ARCHIVE_B8])
        RW_data[ARCHIVE_B8]->UnlockPack(ARCHIVE_KEY_B8);
    if (RW_data[ARCHIVE_A2])
        RW_data[ARCHIVE_A2]->UnlockPack(ARCHIVE_KEY_A2);
    if (RW_data[ARCHIVE_A6])
        RW_data[ARCHIVE_A6]->UnlockPack(ARCHIVE_KEY_A6);
    if (RW_data[ARCHIVE_A0])
        RW_data[ARCHIVE_A0]->UnlockPack(ARCHIVE_KEY_A0);
    if (RW_data[ARCHIVE_A1])
        RW_data[ARCHIVE_A1]->UnlockPack(ARCHIVE_KEY_A1);
    if (RW_data[ARCHIVE_A3])
        RW_data[ARCHIVE_A3]->UnlockPack(ARCHIVE_KEY_A3);
    if (RW_data[ARCHIVE_AC])
        RW_data[ARCHIVE_AC]->UnlockPack(ARCHIVE_KEY_AC);
    if (RW_data[ARCHIVE_A4])
        RW_data[ARCHIVE_A4]->UnlockPack(ARCHIVE_KEY_A4);
    if (RW_data[ARCHIVE_AA])
        RW_data[ARCHIVE_AA]->UnlockPack(ARCHIVE_KEY_AA);
    if (RW_data[ARCHIVE_A5])
        RW_data[ARCHIVE_A5]->UnlockPack(ARCHIVE_KEY_A5);
    if (RW_data[ARCHIVE_A7])
        RW_data[ARCHIVE_A7]->UnlockPack(ARCHIVE_KEY_A7);
    if (RW_data[ARCHIVE_A9])
        RW_data[ARCHIVE_A9]->UnlockPack(ARCHIVE_KEY_A9);
    if (RW_data[ARCHIVE_AB])
        RW_data[ARCHIVE_AB]->UnlockPack(ARCHIVE_KEY_AB);

    // The data's version against the patch's, two letters each
    {
        char version[2] = {'?', '?'};
        char patch_version[2] = {'?', '?'};
        // STRING: GAME 0x637ca8
        int file = dtaOpen("tables\\version.txt", false);
        if (file != DTA_INVALID_HANDLE)
        {
            dtaRead(file, version, 2);
            dtaClose(file);
        }
        // STRING: GAME 0x637c94
        file = dtaOpen("patch\\version.txt", false);
        if (file != DTA_INVALID_HANDLE)
        {
            dtaRead(file, patch_version, 2);
            dtaClose(file);
        }
        if (patch_version[0] != version[0] || patch_version[1] != version[1])
        {
            for (i = 0; i < NUM_ARCHIVES; i++)
            {
                if (RW_data[i])
                {
                    RW_data[i]->Release();
                    RW_data[i] = NULL;
                }
            }
            // STRING: GAME 0x637b78
            sprintf(text, "error: #7453\r\nMaster data:\t'%c%c'\n\rPatch data:\t'%c%c'", version[0], version[1],
                patch_version[0], patch_version[1]);
            MessageBox(NULL, text, "Init error", MB_ICONERROR);
            return false;
        }
    }

    // The window at 50, 50 and a child window the size of the screen mode to render into
    IGRAPH_INIT_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.adapter = setting.adapter;
    desc.hinstance = (HINSTANCE)hinstance;
    desc.error_handler = GraphErrorHandler;
    SetRect((RECT *)&desc.x, 50, 50, setting.width, setting.height);
    SetRect(&desc.child_rect, 0, 0, setting.width, setting.height);
    // IGRAPH_HWTL is set either way. VC6 evaluates the last five terms in the reverse of their order here (the original
    // evaluates the clipping's first); bpp is assigned after the flags, whose computation loads it last.
    desc.flags = (setting.multipass_rendering ? IGRAPH_MULTIPASS : 0) |
        (setting.antialiasing == D3DMULTISAMPLE_4_SAMPLES ? IGRAPH_FSAA4 : 0) |
        (setting.antialiasing == D3DMULTISAMPLE_2_SAMPLES ? IGRAPH_FSAA2 : 0) |
        (setting.vsync ? IGRAPH_VSYNC : 0) | (setting.suspend_when_inactive ? IGRAPH_SLEEP : 0) |
        (setting.triple_buffering ? IGRAPH_TRIPLE_BUFFER : 0) | (setting.fullscreen ? IGRAPH_FULLSCREEN : 0) |
        (setting.hardware_transform ? IGRAPH_HWTL : 0) | (setting.clip_always ? IGRAPH_CLIP_ALWAYS : 0) |
        IGRAPH_NO_DEBUG_KEYS | IGRAPH_CHILD_WINDOW | IGRAPH_HWTL;
    desc.bpp = setting.bit_depth;
    desc.refresh_rate = setting.refresh_rate;
    igraph = GetIGraph();
    if (!igraph)
    {
        // STRING: GAME 0x637c64
        MessageBox(NULL, "error: #4103\r\nIGraph not found...", "Init error", MB_ICONERROR);
        return false;
    }
    LS3D_RESULT result = igraph->Init(&desc);
    if (result != I3D_OK)
    {
        InitErrorMessageBox(result, 4200);
        return false;
    }
    char name[128];
    LoadResString((HINSTANCE)hinstance, IDS_APP_TITLE, name, 127);
    igraph->SetAppName(name);
    igraph->ResetRenderProps();
    igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    igraph->Present();
    igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    igraph->Present();
    igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    igraph->Present();
    SetWindowIcon((HINSTANCE)hinstance, igraph->GetMainHWND());

    // A sound failure leaves the game without sound
    if (!setting.disable_sound)
    {
        uint speakers = 0;
        switch (setting.sound_type)
        {
        case SOUND_TYPE_HEADPHONES:
            speakers = ISNDINIT_HEADPHONES;
            break;
        case SOUND_TYPE_STEREO:
            speakers = ISNDINIT_STEREO;
            break;
        case SOUND_TYPE_QUAD:
            speakers = ISNDINIT_QUAD;
            break;
        case SOUND_TYPE_SURROUND:
            speakers = ISNDINIT_SURROUND;
            break;
        }
        isound = ISndGetDriver();
        // The driver doesn't test ISNDINIT_VERBOSE. VC6 evaluates the first two terms the other way round.
        result = isound->Init(igraph->GetMainHWND(), 22050, 2, 16,
            (setting.disable_eax ? ISNDINIT_NOEAX : 0) | (setting.software_mixing_only ? ISNDINIT_NOHWCHANNELS : 0) |
            speakers | ISNDINIT_VERBOSE);
        if (result < I3D_OK)
        {
            InitErrorMessageBox(result, 8810);
            isound = NULL;
        }
    }

    driver = I3DGetDriver();
    result = driver->Init(0);
    if (result < I3D_OK)
    {
        InitErrorMessageBox(result, 5630);
        return false;
    }
    driver->SetState(RS_FAST_FIND, true);
    driver->SetState(RS_LOADMIPMAP, true);
    driver->SetState(RS_FOG, true);
    driver->SetState(RS_ZBIAS_PROJECTOR, 8);
    driver->SetState(RS_ZBIAS_SHADOW, 8);
    driver->SetState(RS_USESHADOWS, true);
    driver->SetState(RS_UNKNOWN_44, false);
    driver->SetState(RS_UNKNOWN_45, false);
    driver->SetState(RS_UNKNOWN_46, false);
    driver->SetState(RS_UNKNOWN_47, false);
    driver->SetState(RS_USE_OCCLUSION, true);
    driver->SetState(RS_TEXTURETRUECOLOR, setting.texture_true_color);
    driver->SetState(RS_TEXTURELOWDETAIL, setting.texture_low_detail);
    driver->SetState(RS_TEXTURECOMPRESS, setting.texture_compressed);
    driver->SetState(RS_LMTRUECOLOR, setting.lightmap_true_color);
    driver->SetState(RS_MIPMAP, D3DTEXF_LINEAR);
    igraph->SetState(DXS_ALPHAREF, 200);
    driver->SetState(RS_TEXTURELOWDETAIL, setting.texture_low_detail);
    act_mission->Init(driver);
    act_mission->scene->SetCallback(SceneCallback, 1 << SCN_CALLBACK_POST_SCENE | 1 << SCN_CALLBACK_POST_RENDER);
    igraph->KeyboardInit(KBINIT_EXCLUSIVE);
    igraph->MouseInit(MOUSEMODE_EXCLUSIVE);
    ShowCursor(FALSE);
    InitJoy((HINSTANCE)hinstance);
    controls.Init(igraph, joy.begin(), joy.size());
    ff_manager.Reset();

    // STRING: GAME 0x637c34
    igraph->AddMapsDir("Maps", false);
    // STRING: GAME 0x637c2c
    model_cache.AddDir("Models");
    // STRING: GAME 0x637c24
    sound_cache.AddDir("Sounds");
    // STRING: GAME 0x637c1c
    anim_cache.AddDir("Anims");
    ConsolesInit();
    C_car::LoadTyres();
    C_car::LoadMaterials();
    C_car::LoadParticles();
    // STRING: GAME 0x637c0c
    LoadPatchFile("patch\\a1.bin");
    // STRING: GAME 0x637bfc
    LoadPatchFile("patch\\a2.bin");
    // STRING: GAME 0x637bec
    LoadPatchFile("patch\\a3.bin");
    // STRING: GAME 0x637bdc
    LoadPatchFile("patch\\a4.bin");
    InitAnimIndex();
    // STRING: GAME 0x637bc8
    sound_table.LoadTables("tables\\sounds.dat");

    // The language is the data's version, two letters: their swapped bytes make the multicharacter constant. The
    // uppercase test subtracts 0x20 where lowercasing adds it.
    {
        int file = dtaOpen("tables\\version.txt", false);
        if (file != DTA_INVALID_HANDLE)
        {
            char language[4] = {0, 0, 0, 0};
            if (dtaRead(file, language, 2) == 2)
            {
                if (language[0] >= 'A' && language[0] <= 'Z')
                    language[0] -= 0x20;
                if (language[1] >= 'A' && language[1] <= 'Z')
                    language[1] -= 0x20;
                language[2] = language[0];
                language[0] = language[1];
                language[1] = language[2];
                language[2] = 0;
                mafiaLoadSave.LanguageSelect(*(uint *)language);
            }
            else
                mafiaLoadSave.LanguageSelect('en');
            dtaClose(file);
        }
        else
            mafiaLoadSave.LanguageSelect('en');
    }

    // STRING: GAME 0x637bc0
    PlayLogo("logo1", 800.0f, 600.0f);
    // STRING: GAME 0x637bb8
    PlayLogo("logo3", 800.0f, 600.0f);
    // STRING: GAME 0x637bb0
    PlayLogo("logo2", 704.0f, 400.0f);
    fatal = !mafiaLoadSave.Init();
    return true;
}

// The message box of an engine error at the start: the error's text and a number, the error code's low word plus the
// caller's base. The unknown error's format has no %.
// FUNCTION: GAME 0x5bfec0
static void InitErrorMessageBox(LS3D_RESULT result, int base)
{
    char text[128];
    int code = HRESULT_CODE(result) + base;
    switch (result)
    {
    case I3DERR_NOFILE:
        // STRING: GAME 0x637f40
        sprintf(text, "error: #%i\r\nNo file...", code);
        break;
    case I3DERR_OUTOFMEM:
        // STRING: GAME 0x637f20
        sprintf(text, "error: #%i\r\nOut of memory...", code);
        break;
    case I3DERR_BADTEXTUREFORMAT:
        // STRING: GAME 0x637efc
        sprintf(text, "error: #%i\r\nBad texture format...", code);
        break;
    case I3DERR_BADTEXTURE:
        // STRING: GAME 0x637ee0
        sprintf(text, "error: #%i\r\nBad texture...", code);
        break;
    case I3DERR_GENERIC:
        // STRING: GAME 0x637ec8
        sprintf(text, "error: #%i\r\nGeneric...", code);
        break;
    case I3DERR_OBJECTNOTFOUND:
        // STRING: GAME 0x637ea8
        sprintf(text, "error: #%i\r\nObject not found...", code);
        break;
    case I3DERR_INVALIDPARAMS:
        // STRING: GAME 0x637e88
        sprintf(text, "error: #%i\r\nInvalid param...", code);
        break;
    case I3DERR_NOTLOADED:
        // STRING: GAME 0x637e6c
        sprintf(text, "error: #%i\r\nNot loaded...", code);
        break;
    case I3DERR_NOTINITIALIZED:
        // STRING: GAME 0x637e4c
        sprintf(text, "error: #%i\r\nNot initialized...", code);
        break;
    case I3DERR_FILECORRUPTED:
        // STRING: GAME 0x637e2c
        sprintf(text, "error: #%i\r\nFile corupted...", code);
        break;
    case I3DERR_NOFILE1:
        // STRING: GAME 0x637e10
        sprintf(text, "error: #%i\r\nNo file 1...", code);
        break;
    case I3DERR_CYCLICLINK:
        // STRING: GAME 0x637df4
        sprintf(text, "error: #%i\r\nCyclic link...", code);
        break;
    case I3DERR_CANCELED:
        // STRING: GAME 0x637ddc
        sprintf(text, "error: #%i\r\nCanceled...", code);
        break;
    case I3DERR_UNSUPPORTED:
        // STRING: GAME 0x637dc0
        sprintf(text, "error: #%i\r\nUnsupported...", code);
        break;
    case I3DERR_NO3DHARDWARE:
        // STRING: GAME 0x637da0
        sprintf(text, "error: #%i\r\nNo 3D hardware...", code);
        break;
    case I3DERR_BUFFERLOST:
        // STRING: GAME 0x637d84
        sprintf(text, "error: #%i\r\nBuffer lost...", code);
        break;
    case I3DERR_USERBREAK:
        // STRING: GAME 0x637d68
        sprintf(text, "error: #%i\r\nUser break...", code);
        break;
    case I3DERR_FILESYSTEM:
        // STRING: GAME 0x637d44
        sprintf(text, "error: #%i\r\nFile system error...", code);
        break;
    case I3DERR_BADVERSION:
        // STRING: GAME 0x637d24
        sprintf(text, "error: #%i\r\nBad file version...", code);
        break;
    case I3DERR_BADTIMESTAMP:
        // STRING: GAME 0x637d04
        sprintf(text, "error: #%i\r\nBad file time...", code);
        break;
    case I3DERR_LIMITEXCEEDED:
        // STRING: GAME 0x637ce8
        sprintf(text, "error: #%i\r\nOut of limit...", code);
        break;
    default:
        // STRING: GAME 0x637cbc
        sprintf(text, "error: 0x08X\r\nUnknown graph init error...", result);
        break;
    }
    // STRING: GAME 0x637c88
    MessageBox(NULL, text, "Init error", MB_ICONERROR);
}

// Closes what InitSystem started, as PS2's: the profiles and the game's tables, the caches, the tracks, the engine's
// drivers, the joysticks and the archives. The caches' Clear and the tracks' clear are inlined, and the inline budget
// gives the original's mix: the first two caches copy inline and destroy out of line, the animation cache copies out of
// line too, and the tracks are destroyed inline, each through its scalar deleting destructor with 0.
// Open: the original restores ebx, which only the clears use, between the driver's load and its test; ours after the
// test (100%*).
// FUNCTION: GAME 0x5c0080
void CloseSystem()
{
    mafiaLoadSave.Close();
    DoneAnimIndex();
    C_Vehicle::CloseTyres();
    C_Vehicle::CloseVehicleMaterials();
    C_Vehicle::CloseParticles();
    ConsolesDone();
    act_mission->UnInit();
    model_cache.Clear();
    sound_cache.Clear();
    anim_cache.Clear();
    C_animations_machine_de_luxe::ClearTracks();
    UnloadPatchFiles();
    if (driver)
    {
        driver->Close();
        driver = NULL;
    }
    ff_manager.Close();
    CloseJoy();
    if (isound)
    {
        isound->Close();
        isound = NULL;
    }
    if (igraph)
    {
        igraph->Close();
        igraph = NULL;
    }
    // Unsigned, as InitSystem's
    for (uint i = 0; i < NUM_ARCHIVES; i++)
    {
        if (RW_data[i])
        {
            RW_data[i]->Release();
            RW_data[i] = NULL;
        }
    }
    sound_table.FreeTables();
}
