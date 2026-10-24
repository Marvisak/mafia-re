// The game's start and its main loop: PS2's Main.cpp (main, InitSystem, CloseSystem, GameLoopSingle,
// InitErrorMessageBox, MessageBoxGraph). In the original 0x5bce00-0x5c25ac. Written so far: main; InitSystem,
// CloseSystem and GameLoopSingle are stubs.
// Open: the engine's IGraph.h brings LS3DF's I3d_math.h, whose static zero vector gives this file a dynamic initializer
// the original's files don't have (docs/game-files.md).

#include "Main.h"
#include "Mission.h"
#include "game.h"
#include "g_m_start.h"
#include "g_m_carc.h"
#include "racing.h"
#include "g_loadsave.h"
#include "g_indicators.h"
#include "g_colls.h"
#include "Winstuff.h"

#include <settings_registry.h>
#include <process.h>

#include "IGraph/IGraph.h"

// The original's points to the mission object at 0x647df8, in .bss
// GLOBAL: GAME 0x63788c
C_mission *act_mission;
// GLOBAL: GAME 0x647ee0
IGraph *igraph;
// GLOBAL: GAME 0x647ee4
S_settings setting;
// PS2's is initialized data
// GLOBAL: GAME 0x647f28
C_actor *NewMissionActors[2] = {NULL, NULL};
// GLOBAL: GAME 0x647f34
bool _is_intro_ = false;

static bool InitSystem(void *hinstance, bool &fatal);

#pragma auto_inline(off)

// STUB: GAME 0x5be750
void GameLoopSingle()
{
    dbgPrintf("STUB: GameLoopSingle");
}

#pragma auto_inline(on)

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

#pragma auto_inline(off)

// STUB: GAME 0x5bf5a0
static bool InitSystem(void *hinstance, bool &fatal)
{
    dbgPrintf("STUB: InitSystem");
    fatal = false;
    return false;
}

// STUB: GAME 0x5c0080
void CloseSystem()
{
    dbgPrintf("STUB: CloseSystem");
}

#pragma auto_inline(on)
