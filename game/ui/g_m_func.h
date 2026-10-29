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
