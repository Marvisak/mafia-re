#pragma once

#include "ui/g_menu.h"

// What ExecuteMenu returns for the profile menu (G_LoadSaveClass::Init; the names are ours)
#define PROFILESELECT_QUIT 0xf          // the player quits the game
#define PROFILESELECT_CHOSEN 0x10       // the player chose a profile

// The profiles' menu's items, tables\menu\profileselect.mnu (the names are ours)
#define PROFILESELECT_WINDOW 3          // "Player Profile:"
#define PROFILESELECT_LIST 0xa          // the frame around the profiles
#define PROFILESELECT_PROFILE 0x64      // the first profile, "< none >" without one; OnCreate adds the others after it
#define PROFILESELECT_PROFILE_END 0xc8  // past the profiles' ids
#define PROFILESELECT_CREATE 0xc9       // "Create New Profile"
#define PROFILESELECT_DELETE 0xca       // "Delete Profile"
#define PROFILESELECT_QUIT_GAME 0xcb    // "Quit Game"

// The profiles the menu takes: with as many, it can't create another (the name is ours)
#define PROFILESELECT_MAX_PROFILES 12

// GM_ProfileSelect::new_name's size, which the name dialog copies the name into (the name is ours)
#define PROFILESELECT_NAME_SIZE 0x20

// The name dialog's items, tables\menu\profilecreate.mnu (the names are ours)
#define PROFILECREATE_NAME 0xb          // the name's text edit, "Mafia" in the definition
#define PROFILECREATE_OK 0x65
#define PROFILECREATE_CANCEL 0x66

// The longest name the dialog's edit takes (the name is ours)
#define PROFILECREATE_NAME_LENGTH 10

// The codes the name dialog closes with, which the profiles' menu's OnCloseChild gets (the names are ours)
#define PROFILECREATE_CODE_CANCEL 0
#define PROFILECREATE_CODE_OK 1         // the name is in the profiles' menu's new_name

// What ExecuteMenu returns for the name dialog run without a parent menu (the names are ours; the game always opens it
// over the profiles' menu)
#define PROFILECREATE_CREATED 0xd
#define PROFILECREATE_CANCELLED 0xe

// The deletion's items, tables\menu\profiledelete.mnu, the profiles' menu's ids (the names are ours)
#define PROFILEDELETE_WINDOW 3          // "Delete Profile:"
#define PROFILEDELETE_LIST 0xa          // the frame around the profiles
#define PROFILEDELETE_PROFILE 0x64      // the first profile; OnCreate adds the others after it
#define PROFILEDELETE_PROFILE_END 0xc8  // past the profiles' ids
#define PROFILEDELETE_CANCEL 0xc9

// The code the deletion closes with once it deleted a profile, which the profiles' menu doesn't read (the name is ours)
#define PROFILEDELETE_CODE_DELETED 1

// The text database's "Default", "This Profile Already Exists" and "Delete" (the names are ours)
#define TEXT_DEFAULT 0x84
#define TEXT_PROFILE_EXISTS 0x83
#define TEXT_DELETE 0x82

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
    // The name dialog's closing creates the profile or asks for the dialog again; the menu is then created again for
    // the profiles there are now
    virtual int OnCloseChild(GM_Menu *child, uint, uint);
    virtual int OnKey(uchar key, uint) { return 0; }
    virtual int OnClick(uint id);
    // Opens the dialog of PROFILEDIALOG_*, the frame after OnCreate or OnCloseChild asked for it
    virtual int OnTick(uint time);

    // 0x30, PROFILEDIALOG_*, which OnTick opens: OnCreate's PROFILEDIALOG_CREATE without a profile, OnCloseChild's after
    // the name dialog (the name is ours)
    int dialog;
    // 0x34, "Default" from OnCreate, the name dialog's, which OnCloseChild creates the profile with (the name is ours)
    char new_name[PROFILESELECT_NAME_SIZE];
};

// The new profile's name, "profilecreate" (PS2 GM_ProfileCreate, g_m_start.cpp), 0x34 bytes, which the profiles' menu
// opens with PROFILESELECT_CREATE. Its constructor is inline, as PS2's. The copies of its and GM_ProfileDelete's
// OnBeforeCreate follow GM_ProfileSelect::OnTick, the first function that needs them, in the original; ours come at
// the file's end: under /YX they come due once the file's own source outweighs the headers' inline code before them,
// which with all of the file written it still doesn't (open: our headers have more inline code before them).
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
    // The name's edit, IGraph's keys from the window messages while the dialog is open
    virtual int OnCreate();
    // IGraph's keyboard exclusive again
    virtual int OnClose();
    virtual int OnClick(uint id);

    char *name;                         // 0x30, the profiles' menu's new_name (the name is ours)
};

// Which profile to delete, "profiledelete" (PS2 GM_ProfileDelete, g_m_start.cpp), 0x30 bytes, which the profiles' menu
// opens with PROFILESELECT_DELETE: a profile's item asks in a question box (GM_QuestionBox) whether to delete it. Its
// constructor is the compiler's, inlined.
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
    // The profiles' names in the menu, as the profiles' menu's OnCreate
    virtual int OnCreate();
    // The question box's closing deletes the profile and closes the menu
    virtual int OnCloseChild(GM_Menu *child, uint, uint);
    virtual int OnClick(uint id);
};
