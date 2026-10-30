#pragma once

#include "ui/g_menu.h"

// The main menu's choices, what ExecuteMenu returns for it. The names are ours.
enum E_MAINMENU_RESULT {
    MAINMENU_TUTORIAL = 0x11,
    MAINMENU_NEW_GAME = 0x12,           // the intro mission, then the first one
    MAINMENU_LOAD_GAME = 0x13,
    MAINMENU_MULTIPLAYER = 0x14,        // its item's, which neither menu has; main has no case for it
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
    GM_MainMenu() { flags |= MENUF_UNKNOWN_80000; }

    // "mainmenurac" or "mainmenu"
    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint) { return 0; }
    // An item ends the main menu with its choice (E_MAINMENU_RESULT) or opens its menu (g_m_func.cpp)
    virtual int OnClick(uint id);
    // The camera to the frame of the item that became active
    virtual int OnChange(GM_Component *active, GM_Component *);
};

// A message with an OK button, "messagebox" (PC only: PS2's GM_QuestionBox is this one or the question by a flag, the
// PC has a class for each; the name is ours, after its menu), 0x38 bytes. A key or a click closes it.
// VTABLE: GAME 0x627524
class GM_MessageBox : public GM_Menu
{
public:
    // The message and the title, of the text database (no title for an id it doesn't have)
    GM_MessageBox(uint text_id, uint title_id);

    virtual const char *OnBeforeCreate();
    // The texts, the window grown to fit the message
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint);
    virtual int OnClick(uint id);

    char *text;                         // 0x30, item 0x82's (the names are ours)
    char *title;                        // 0x34, the window's, NULL to keep the menu's
};

// The question box's items, tables\menu\questionbox.mnu (the names are ours)
#define QUESTIONBOX_WINDOW 0x82         // its text the title
#define QUESTIONBOX_TEXT 0x83           // the question
#define QUESTIONBOX_YES 0x8a
#define QUESTIONBOX_NO 0x8b

// The codes the question box closes with, which the parent's OnCloseChild gets; Escape closes it with -1 (the names
// are ours)
#define QUESTIONBOX_CODE_NO 0
#define QUESTIONBOX_CODE_YES 1

// What ExecuteMenu returns for the question box run without a parent menu, Escape's being No's (the names are ours)
#define QUESTIONBOX_RESULT_YES 0xb
#define QUESTIONBOX_RESULT_NO 0xc

// A question with Yes and No, "questionbox" (PS2 GM_QuestionBox, whose message box is the question without No by a
// flag; PS2's takes the texts' ids), 0x38 bytes
// VTABLE: GAME 0x627554
class GM_QuestionBox : public GM_Menu
{
public:
    // The window's title and the question, NULL for none
    GM_QuestionBox(char *title, char *text);

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    // Escape closes it as No does, but with its own code
    virtual int OnKey(uchar key, uint);
    virtual int OnClick(uint id);

    char *title;                        // 0x30 (the names are ours)
    char *text;                         // 0x34
};

// What ExecuteMenu returns for the mission's failure menu, its items' clicks (the names are ours, after the items'
// texts in tables\menu\gamefailed.mnu, by value where none is known): "Exit to Main Menu" (item 0x3ea), "Retry" (item
// 0x3e8), and one its OnClick doesn't return, which the script's ENDOFMISSION handles too
#define GAMEFAILED_RESULT_EXIT 0x4fb
#define GAMEFAILED_RESULT_4FC 0x4fc
#define GAMEFAILED_RESULT_RETRY 0x4fd

// The mission's failure, "gamefailed" (PS2 GM_GameFailed, g_m_func.cpp), 0x34 bytes: "Game Over" with the failure's
// text, Retry and Exit to Main Menu. The mission's script runs it (ENDOFMISSION).
// VTABLE: GAME 0x624898
class GM_GameFailed : public GM_Menu
{
public:
    // FUNCTION: GAME 0x47a3e0
    GM_GameFailed(uint text)
    {
        text_id = text;
    }

    // FUNCTION: GAME 0x47a470
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x634414
        return "gamefailed";
    }
    // The text in item 0x64, the items below moved down by its height
    virtual int OnCreate();
    // Escape doesn't close it: the vtable's is a folded copy of an empty one (0x605020)
    virtual int OnKey(uchar key, uint) { return 0; }
    // Closes the menus with GAMEFAILED_RESULT_RETRY or GAMEFAILED_RESULT_EXIT
    virtual int OnClick(uint id);

    uint text_id;                       // 0x30, of the text database, the failure's (the name is ours)
};

class G_Inventory;

// The gun shop, "gunshop" (PS2 GM_GunShop, g_m_func.cpp), 0x43c bytes (PS2's 0x438): the player's inventory and the
// shop's weapons, "Inventory (Drop)" and "Select (Pick Up)". The mission's script runs it (GUNSHOP_MENU). PS2's
// RefreshInventory and destructor aren't in the PC's vtable.
// VTABLE: GAME 0x6248c8
class GM_GunShop : public GM_Menu
{
public:
    // FUNCTION: GAME 0x47a480
    GM_GunShop(G_Inventory *inventory)
    {
        this->inventory = inventory;
    }

    // FUNCTION: GAME 0x47a4a0
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x634420
        return "gunshop";
    }
    virtual int OnCreate();
    // The vtable's is a folded copy of an empty one (0x605020)
    virtual int OnKey(uchar key, uint) { return 0; }
    virtual int OnClick(uint id);

    G_Inventory *inventory;             // 0x30 (PS2 0x2c), the player's (the name is ours)
    uchar unknown_34[0x408];
};

// The loading screen's progress bar (its menu's item 0x12c, a "pbar"; the name is ours)
#define LOADPROGRESS_BAR 0x12c

// The loading screen (PS2 GM_LoadProgress, g_m_func.cpp), 0x40 bytes: "loadprogress", whose bar the mission's opening
// fills to mission_part and the game's Init the rest. C_mission::Open creates one into mafiaLoadSave.progress, the
// load callback (cbLoadScene) one of its own without it. Only what those two use so far.
// VTABLE: GAME 0x6275c0
class GM_LoadProgress : public GM_Menu
{
public:
    float mission_part;                 // 0x30, the bar's part the mission's opening fills (the name is ours)
    int unknown_34;                     // 0x34
    // 0x38, a title of the text database, which OnCreate shows with "press <ESC> to Abort", 0 for none (the name is
    // ours)
    uint title;
    // 0x3c, the mission whose picture and name OnCreate shows, act_mission's while NULL (the name is ours)
    uchar *mission_name;

    // The menus' screen scale too
    GM_LoadProgress(uint title, float mission_part, uchar *mission_name);

    virtual const char *OnBeforeCreate();
    // The screen's items: the title, the two pictures, and the mission's picture and name from tables\load.def
    virtual int OnCreate();
    virtual int OnTick(uint time);
    virtual int OnKey(uchar key, uint);

    // The bar at progress (0 to 1) of the whole loading (PS2's is out of line, the PC's callers inline it)
    void SetRawProgress(float progress) { SetFloat(LOADPROGRESS_BAR, progress); }
    // The bar at progress (0 to 1) of the mission's opening
    void SetProgressMissionOpen(float progress);
    // The game's Init's part of the bar, after the mission's (PS2 name)
    void SetProgressGameInit(float progress);
    // Draws the screen and presents it
    void Present();
};

class C_car;
struct S_garage_item;

// The garage's menu at a mission's end, "garageendofmission" (the name is ours, after the menu; PS2 has its garage menus
// in g_m_garage.cpp), 0x3c bytes: G_LoadSaveClass::GarageGameDone runs it while it keeps more cars than the garage has
// room for, to choose which go. Its constructor is inline (GarageGameDone has it), its vtable 0x627a0c. The original's
// functions are in two places, OnCreate, OnClick and vfunc_13 in this file's range and the two names in
// g_loadsave.cpp's, a base class's and a derived one's most likely; one class here, its vtable not the original's.
class GM_GarageEndOfMission : public GM_Menu
{
public:
    GM_GarageEndOfMission(std::vector<S_garage_item> *items, std::vector<uint> *discard, std::vector<C_car *> *cars)
        : items(items), discard(discard), cars(cars)
    {
    }

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    // The vtable's is a folded copy of an empty one (0x5e3fd0)
    virtual int OnKey(uchar key, uint) { return 0; }
    virtual int OnClick(uint id);
    // The garage manager's menu's name, "garagemanager" (the name is ours)
    virtual const char *vfunc_12();
    virtual void vfunc_13();

    std::vector<S_garage_item> *items;  // 0x30, GarageGameDone's cars (the names are ours)
    std::vector<uint> *discard;         // 0x34, the indices of items to give up
    std::vector<C_car *> *cars;         // 0x38, the mission's cars GarageGameDone was given
};
