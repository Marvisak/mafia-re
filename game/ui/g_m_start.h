#pragma once

#include "ui/g_menu.h"

// What ExecuteMenu returns for the profile menu when the player quits the game there (G_LoadSaveClass::Init; the name
// is ours)
#define PROFILESELECT_QUIT 0xf

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

    uchar unknown_30[0x24];
};
