#pragma once

#include "ui/g_menu.h"

// What ExecuteMenu returns for the profile menu when the player quits the game there (G_LoadSaveClass::Init; the name
// is ours)
#define PROFILESELECT_QUIT 0xf

// The profiles' menu's items, tables\menu\profileselect.mnu (the names are ours)
#define PROFILESELECT_WINDOW 3          // "Player Profile:"
#define PROFILESELECT_LIST 0xa          // the frame around the profiles
#define PROFILESELECT_PROFILE 0x64      // the first profile, "< none >" without one; OnCreate adds the others after it
#define PROFILESELECT_CREATE 0xc9       // "Create New Profile"
#define PROFILESELECT_DELETE 0xca       // "Delete Profile"
#define PROFILESELECT_QUIT_GAME 0xcb    // "Quit Game"

// The profiles the menu takes: with as many, it can't create another (the name is ours)
#define PROFILESELECT_MAX_PROFILES 12

// The text database's "Default" (the name is ours)
#define TEXT_DEFAULT 0x84

// The menu of the player's profiles, "profileselect" (PS2 GM_ProfileSelect, g_m_start.cpp), 0x54 bytes, which
// G_LoadSaveClass::Init runs before the main menu: its items are the profile, "Create New Profile", "Delete Profile"
// and "Quit Game". Its constructor is the compiler's, inlined. OnBeforeCreate's and OnKey's copies are in the file of
// G_LoadSaveClass::Init, which creates the menu.
// VTABLE: GAME 0x6279dc
class GM_ProfileSelect : public GM_Menu
{
public:
    // FUNCTION: GAME 0x605010
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x6395e4
        return "profileselect";
    }
    virtual int OnCreate();
    virtual int OnCloseChild(GM_Menu *child, uint, uint);
    virtual int OnKey(uchar key, uint) { return 0; }
    virtual int OnClick(uint id);
    virtual int OnTick(uint time);

    int unknown_30;                     // 0x30, 1 without a profile, else 0
    char new_name[0x20];                // 0x34, "Default" from OnCreate (the name is ours)
};
