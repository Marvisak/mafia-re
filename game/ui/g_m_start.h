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

// The text database's "Default" and "This Profile Already Exists" (the names are ours)
#define TEXT_DEFAULT 0x84
#define TEXT_PROFILE_EXISTS 0x83

// GM_ProfileSelect::dialog, the dialog its OnTick opens (the names are ours)
#define PROFILEDIALOG_NONE 0
#define PROFILEDIALOG_CREATE 1          // the new profile's name: OnCreate's without a profile
#define PROFILEDIALOG_DELETE 2          // which profile to delete: nothing sets it, OnClick opens the dialog itself
#define PROFILEDIALOG_EXISTS 3          // "This Profile Already Exists", then the name again: where creating one failed

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
    // Opens the dialog of PROFILEDIALOG_*, the frame after OnCreate or OnCloseChild asked for it
    virtual int OnTick(uint time);

    // 0x30, PROFILEDIALOG_*, which OnTick opens: OnCreate's PROFILEDIALOG_CREATE without a profile, OnCloseChild's after
    // the name dialog (the name is ours)
    int dialog;
    // 0x34, "Default" from OnCreate, the name dialog's, which OnCloseChild creates the profile with (the name is ours)
    char new_name[0x20];
};

// The new profile's name, "profilecreate" (PS2 GM_ProfileCreate, g_m_start.cpp), 0x34 bytes, which the profiles' menu
// opens with PROFILESELECT_CREATE. Its constructor is inline, as PS2's. The copies of its and GM_ProfileDelete's
// OnBeforeCreate follow GM_ProfileSelect::OnTick, the first function that needs them, in the original; ours come at
// the file's end: under /YX they come due once the file's own source outweighs the headers' inline code before them,
// and OnCloseChild, before OnTick, isn't written yet.
// VTABLE: GAME 0x6277d0
class GM_ProfileCreate : public GM_Menu
{
public:
    GM_ProfileCreate(char *name) { this->name = name; }

    // FUNCTION: GAME 0x5e88b0
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x6384b8
        return "profilecreate";
    }
    virtual int OnCreate();
    virtual int OnClose();
    virtual int OnClick(uint id);

    char *name;                         // 0x30, the profiles' menu's new_name (the name is ours)
};

// Which profile to delete, "profiledelete" (PS2 GM_ProfileDelete, g_m_start.cpp), 0x30 bytes, which the profiles' menu
// opens with PROFILESELECT_DELETE. Its constructor is the compiler's, inlined.
// VTABLE: GAME 0x627800
class GM_ProfileDelete : public GM_Menu
{
public:
    // FUNCTION: GAME 0x5e88c0
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x6384c8
        return "profiledelete";
    }
    virtual int OnCreate();
    virtual int OnCloseChild(GM_Menu *child, uint, uint);
    virtual int OnClick(uint id);
};
