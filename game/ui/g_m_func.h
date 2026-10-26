#pragma once

#include "ui/g_menu.h"

// The main menu's choices, what ExecuteMenu returns for it. The names are ours.
enum E_MAINMENU_RESULT {
    MAINMENU_TUTORIAL = 0x11,
    MAINMENU_NEW_GAME = 0x12,           // the intro mission, then the first one
    MAINMENU_LOAD_GAME = 0x13,
    MAINMENU_RACING = 0x15,
    MAINMENU_INTRO = 0x16,
    MAINMENU_CREDITS = 0x17,
    MAINMENU_QUIT = 0x18,
    MAINMENU_CARCYCLOPEDIA = 0x19,
    MAINMENU_PROFILE_CHANGE = 0x1a,
    MAINMENU_EXTREME = 0x1b,
    MAINMENU_FREEITALY = 0x1c,
    MAINMENU_FREERIDE = 0x1d,
    MAINMENU_FREERIDENOC = 0x1e,
    MAINMENU_FREEKRAJINA = 0x1f,
    MAINMENU_FREEKRAJINANOC = 0x20,
};

// The main menu (PS2 GM_MainMenu, g_m_func.cpp). Its constructor is inline: main's has the flag after the base's
// constructor and the vtable.
// VTABLE: GAME 0x625eb8
class GM_MainMenu : public GM_Menu
{
public:
    GM_MainMenu() { flags |= 0x80000; }

    // "mainmenurac" or "mainmenu"
    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint) { return 0; }
    virtual int OnClick(uint id);
    virtual int OnChange(GM_Component *, GM_Component *);
};

// The loading screen (PS2 GM_LoadProgress, g_m_func.cpp), 0x40 bytes: "loadprogress", whose bar the mission's opening
// fills to mission_part and the game's Init the rest. C_mission::Open creates one into mafiaLoadSave.progress, the
// load callback (cbLoadScene) one of its own without it. Only what those two use so far.
// VTABLE: GAME 0x6275c0
class GM_LoadProgress : public GM_Menu
{
public:
    float mission_part;                 // 0x30, the bar's part the mission's opening fills (the name is ours)
    int unknown_34;                     // 0x34
    uint unknown_38;                    // 0x38
    uchar *unknown_3c;                  // 0x3c

    // The menus' screen scale too
    GM_LoadProgress(uint, float mission_part, uchar *);

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnTick(uint time);
    virtual int OnKey(uchar key, uint);

    // The bar at progress (0 to 1) of the mission's opening
    void SetProgressMissionOpen(float progress);
    // Draws the screen and presents it
    void Present();
};
