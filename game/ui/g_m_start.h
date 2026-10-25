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

// The main menu (PS2 GM_MainMenu; the file is a guess, PS2's g_m_start.cpp). Its constructor is inline: main's has the
// flag after the base's constructor and the vtable.
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
