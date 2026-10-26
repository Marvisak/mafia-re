// The menus (PS2 g_menu.cpp; the PC's file is the one at 0x5e8d10-0x5ece6c, docs/game-files.md). Written so far: the
// constructor and ExecuteMenu, which main and G_LoadSaveClass::Init run; what ExecuteMenu calls is stubs.

#include "ui/g_menu.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "input/controls.h"
#include "mission/Mission.h"
#include "mission/game.h"

#include "IGraph/IGraph.h"
#include "I3D/I3D.h"
#include "stub.h"

// Its initializer starts the file (0x5e8d10)
// GLOBAL: GAME 0x6bd888
S_vector2 GM_Menu::menuScreenScale(1.0f, 1.0f);
// GLOBAL: GAME 0x6bd890
GM_Menu *GM_Menu::activeMenuWindow = NULL;
// GLOBAL: GAME 0x6bd8a8
int GM_Menu::zz_globalReturnCode = 0;
// GLOBAL: GAME 0x6bd8ac
uint GM_Menu::zz_blinkTime = 0;
// GLOBAL: GAME 0x6bd8b4
int GM_Menu::menuCount = 0;
// GLOBAL: GAME 0x6bd8dc
uint GM_Menu::lastKameraTargetID = 0;

#pragma auto_inline(off)

// The original's copy lies in the file at 0x549140, most likely a folded one
// STUB: GAME 0x5498f0
int GM_Menu::OnKey(uchar key, uint)
{
    StubLog("GM_Menu::OnKey");
    return 0;
}

// STUB: GAME 0x5e9960
void GM_Menu::LoadResources()
{
    StubLog("GM_Menu::LoadResources");
}

#pragma auto_inline(on)

// PS2's loads the resources in ExecuteMenu
// FUNCTION: GAME 0x5ea1f0
GM_Menu::GM_Menu() : parent(NULL), unknown_8(0), unknown_c(0), unknown_20(false), unknown_24(0), unknown_28(0)
{
    flags = MENUF_UNKNOWN_10000;
    if (!menuCount)
        LoadResources();
    menuCount++;
}

#pragma auto_inline(off)

// STUB: GAME 0x5ea2e0
void GM_Menu::Create()
{
    StubLog("GM_Menu::Create %s", OnBeforeCreate());
}

// STUB: GAME 0x5ea690
void GM_Menu::Destroy()
{
    StubLog("GM_Menu::Destroy");
}

// Ends every menu with the main menu's quit, as the stub of ExecuteMenu did, so that a run goes through
// STUB: GAME 0x5ea8c0
void GM_Menu::Tick(int mouse_rx, int mouse_ry, int mouse_rz, uint buttons, uint key, uint shift_keys, uint time)
{
    StubLog("GM_Menu::Tick");
    zz_globalReturnCode = 0x18;
}

#pragma auto_inline(on)

// PS2's is the same loop without the mouse, the joysticks and the controls: it reads the controller, draws the menu
// itself and checks the CD's tray. The PC's draws it from the scene's callback (Main.cpp), after the frame.
// last_key's 0xff never reaches the loop: VC6 gives the switch's int-to-float temporary the same stack slot (the
// original's too), so the first frames test the low byte of the screen's width as the key held.
// FUNCTION: GAME 0x5eba40
int GM_Menu::ExecuteMenu(GM_Menu *menu, bool tick_mission, bool)
{
    uint buttons = 0;
    uint delay = 1000;
    uchar last_key = 0xff;
    bool controls_read = false;
    if (!menu)
        return 0;
    switch (parheliaIndex)
    {
    case PARHELIA_DUAL:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 1600.0f);
        break;
    case PARHELIA_TRIPLE:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 2400.0f);
        break;
    default:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 800.0f);
    }
    menuScreenScale.y = igraph->Scrn_sy() * (1.0f / 600.0f);
    mafiaIndicators.unknown_40a8.push_back(mafiaIndicators.unknown_40a4);
    mafiaIndicators.unknown_40a4 =
        (mafiaIndicators.unknown_40a4 & (INDF_UNKNOWN_4000 | INDF_UNKNOWN_8000 | INDF_UNKNOWN_20000)) | INDF_MENU;
    SetCameraTargetID(0);
    zz_globalReturnCode = 0;
    if (!menu->unknown_8)
        menu->Create();
    activeMenuWindow = menu;
    while (!zz_globalReturnCode)
    {
        uint time = igraph->GetTimer(16, 249);
        igraph->ProcessWinMessages();
        igraph->UpdateMouseData();
        igraph->UpdateKeyboardData();
        uchar key = igraph->ReadKey();
        for (int i = joy.size(); i--;)
            joy[i]->Update(JOY_UPDATE_RESET);
        if (activeMenuWindow && activeMenuWindow->flags & MENUF_INVENTORY_CLOSES)
        {
            controls.Update(false);
            float *pressed;
            controls_read = true;
            controls.GetValues(pressed, CONTROLS_PRESSED);
            if (pressed[CTRL_INVENTORY] != 0.0f)
                key = DIK_ESCAPE;
        }
        if (tick_mission)
            act_mission->Tick(time);
        else
            act_mission->game->camera.Tick(time);
        if (zz_blinkTime > time)
            zz_blinkTime -= time;
        else
            zz_blinkTime = 1000 - time;
        // A key held repeats after 400 ms, then every 100 ms
        if (key)
        {
            last_key = key;
            delay = 400;
        }
        else if (last_key)
        {
            if (delay > time)
                delay -= time;
            else if (igraph->TestKey(last_key))
            {
                delay = 100;
                key = last_key;
            }
            else
                last_key = 0;
        }
        uint last_buttons = buttons;
        buttons = igraph->GetMouseButtons();
        activeMenuWindow->Tick(igraph->Mouse_rx(), igraph->Mouse_ry(), igraph->Mouse_rz(),
            ((last_buttons & ~buttons) << 4 | (buttons & ~last_buttons)) << 4 | buttons, key, igraph->GetShiftKeys(false),
            time);
        if (lastKameraTargetID)
            TickCamera(time);
        if (!zz_globalReturnCode)
        {
            act_mission->scene->Render();
            igraph->Present();
        }
    }
    activeMenuWindow->Destroy();
    activeMenuWindow = NULL;
    if (mafiaIndicators.unknown_40a8.size())
    {
        mafiaIndicators.unknown_40a4 = mafiaIndicators.unknown_40a8.back();
        mafiaIndicators.unknown_40a8.erase(mafiaIndicators.unknown_40a8.end() - 1);
    }
    if (controls_read)
    {
        controls.Update(false);
        controls.Update(false);
    }
    return zz_globalReturnCode;
}

#pragma auto_inline(off)

// STUB: GAME 0x5ebe80
void GM_Menu::SetCameraTargetID(uint id)
{
    StubLog("GM_Menu::SetCameraTargetID %u", id);
}

// STUB: GAME 0x5ec8a0
void GM_Menu::TickCamera(uint time)
{
    StubLog("GM_Menu::TickCamera");
}

#pragma auto_inline(on)
