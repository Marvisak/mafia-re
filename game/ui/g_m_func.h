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

// What ExecuteMenu returns for the game's menu (the names are ours): its Exit Game (the quit question's Yes) and Resume
// Game or Escape; and one GameLoopSingle updates the controls for, which no menu found returns (the name is a guess).
// Its Load Game returns MAINMENU_LOAD_GAME.
#define GAMEMENU_RESULT_EXIT 0x21
#define GAMEMENU_RESULT_CONTROLS 0x22
#define GAMEMENU_RESULT_RESUME 0x24

// The game's menu, "gamemenu" (PS2 GM_GameMenu, g_m_func.cpp, which has OnCreate, OnCloseChild and OnTick of its own),
// 0x30 bytes, which GameLoopSingle runs on Escape: Exit Game, Load Game, Options and Resume Game. Its constructor is the
// compiler's, inlined.
// VTABLE: GAME 0x625e58
class GM_GameMenu : public GM_Menu
{
public:
    // FUNCTION: GAME 0x5beaf0
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x63793c
        return "gamemenu";
    }
    // Escape resumes the game
    virtual int OnKey(uchar key, uint);
    // Exit Game asks first, Load Game and Options open their menus, Resume Game closes it
    virtual int OnClick(uint id);
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
struct S_GameItem;

// The inventory, "inventory" or, with an item in the car's place (G_Inventory::in_car), "inventorycar" (PC only, the
// names are ours, after its menus), 0x38 bytes: the player's inventory and the vector the items the hand drops go into.
// C_player's AI_walk, AI_drive and AI_car_fire run it on CTRL_INVENTORY.
// VTABLE: GAME 0x627584
class GM_Inventory : public GM_Menu
{
public:
    GM_Inventory(G_Inventory *inventory, std::vector<S_GameItem> *dropped);

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnCloseChild(GM_Menu *child, uint, uint);
    virtual int OnKey(uchar key, uint);
    virtual int OnClick(uint id);

    // The item's name into the component of that id, its second parameter unused (PC only, the name is ours)
    void SetItem(uint id, uint, S_GameItem *item);

    G_Inventory *inventory;             // 0x30, the player's (the names are ours)
    std::vector<S_GameItem> *dropped;   // 0x34
};

// GM_ItemPickUp's items (the names are ours): the item list's ids from PICKUP_ITEM_FIRST, an item's index in the list it
// is given added; an item with id PICKUP_TEXT is no item but a choice, its ammo_loaded the text's id
#define PICKUP_ITEM_FIRST 0xa9
#define PICKUP_ITEM_LAST 0x10d
#define PICKUP_TEXT 1
// The menu's result for the items taken, which are in picked then (C_player::Do_AB_OwnerNULL; the name is ours)
#define PICKUP_TAKE 0xa8
// Its result for Exit, Escape and the key 0, and where it has no items (the name is ours)
#define PICKUP_EXIT 0xa7

// The items' pick-up, "itempickup" (PS2 GM_ItemPickUp, g_m_func.cpp), 0x54 bytes: the player's inventory and the lists
// of items to take; C_player::Do_AB_OwnerCar runs it with two choices at a petrol pump. The members' names are ours.
// VTABLE: GAME 0x625920
class GM_ItemPickUp : public GM_Menu
{
public:
    GM_ItemPickUp(G_Inventory *inventory, std::vector<G_Inventory *> *inventories, std::vector<S_GameItem> *items,
        std::vector<S_GameItem> *dropped, std::vector<S_GameItem> *picked) : inventory(inventory),
        inventories(inventories), items(items), dropped(dropped), picked(picked) {}

    // FUNCTION: GAME 0x594f90
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x636ec8
        return "itempickup";
    }
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint);
    virtual int OnClick(uint id);
    // Takes the item of menu_items at idx: one of items into picked, also into dropped with drop (the inventory
    // had no place for it), an inventory's out of the inventories (PS2's name, the parameters' are ours)
    void PickUp(uint idx, bool drop);

    G_Inventory *inventory;             // 0x30
    // 0x34, the other inventories the items can come from, a dead body's (PS2's G_Inventory::ItemPickUp's last
    // parameter)
    std::vector<G_Inventory *> *inventories;
    std::vector<S_GameItem> *items;     // 0x38, what the menu offers
    std::vector<S_GameItem> *dropped;   // 0x3c, what the inventory couldn't keep, to drop
    std::vector<S_GameItem> *picked;    // 0x40, the items taken (PICKUP_TAKE)
    // 0x44, the items the menu lists, the i-th the item ITEMPICKUP_LIST + i's: items' own, then those of each of
    // inventories (G_Inventory::EnemyToList)
    std::vector<S_GameItem *> menu_items;
};

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

    // The item's name into the component of that id, its second parameter unused (PC only, the name is ours)
    void SetItem(uint id, uint, S_GameItem *item);
    // The inventory's places and items into their components (PS2's name; PS2's does nothing)
    void RefreshInventory();

    G_Inventory *inventory;             // 0x30 (PS2 0x2c), the player's (the names are ours)
    uchar unknown_34[4];                // 0x34 (PS2 0x30)
    // 0x38 (PS2 0x34), the shop's weapons, OnCreate's: their ids in gameItemTable, in the list's order
    uint weapons[0x100];
    uint num_weapons;                   // 0x438 (PS2 0x434)
};

// What ExecuteMenu would return for the loading screen's Escape, "press <ESC> to Abort" (the name is ours): nothing runs
// the screen through ExecuteMenu
#define LOADPROGRESS_RESULT_ABORT 0x3a

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

// The scalar deleting destructor and the implicit destructor (a jump to GM_Menu's) of C_game::Init's delete, in
// game.cpp's range
// SYNTHETIC: GAME 0x5a3c30
// ??_GGM_LoadProgress@@QAEPAXI@Z

// SYNTHETIC: GAME 0x5a3c50
// ??1GM_LoadProgress@@QAE@XZ

// The out-of-line return of 0 the linker kept of a folded group, GM_LoadProgress::OnTick's and
// GM_GarageSummary::OnClick's entry: by name, as which of the two copies our linker keeps changes with the link
// SYNTHETIC: GAME 0x5e4d50
// ?OnClick@GM_GarageSummary@@UAEHI@Z

// The two quit questions' OnBeforeCreate, the same code ("exityesno"), folded into the first copy: by name, as which
// of the two copies our linker keeps changes with the link
// FUNCTION: GAME 0x5e08e0
// ?OnBeforeCreate@GM_GameMenuExit@@UAEPBDXZ

// The message box's OnKey and OnClick, the same code as the simple message's (g_m_func.cpp), the original's copies the
// message box's: by name, as which of the two copies our linker keeps changes with the link
// FUNCTION: GAME 0x5e22d0
// ?OnKey@GM_SimpleMessage@@UAEHEI@Z

// FUNCTION: GAME 0x5e22f0
// ?OnClick@GM_SimpleMessage@@UAEHI@Z

class C_car;
struct S_garage_item;

// What ExecuteMenu returns for the garage's menu at a mission's end, a car's item clicked: GarageGameDone doesn't
// read it (the name is ours)
#define GARAGEENDOFMISSION_DONE 0x4f8

// The garage's menu at a mission's end, "garageendofmission" (the name is ours, after the menu; PS2 has its garage menus
// in g_m_garage.cpp), 0x3c bytes: G_LoadSaveClass::GarageGameDone runs it while it keeps more cars than the garage has
// room for, to choose which go. Its constructor is inline (GarageGameDone has it). Its functions are in two places,
// OnCreate and OnClick in this file's range, OnBeforeCreate in g_loadsave.cpp's, as the garage manager's menu's.
// VTABLE: GAME 0x627a0c
class GM_GarageEndOfMission : public GM_Menu
{
public:
    GM_GarageEndOfMission(std::vector<S_garage_item> *items, std::vector<uint> *discard, std::vector<C_car *> *cars)
        : items(items), discard(discard), cars(cars)
    {
    }

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint);
    virtual int OnClick(uint id);

    std::vector<S_garage_item> *items;  // 0x30, GarageGameDone's cars (the names are ours)
    std::vector<uint> *discard;         // 0x34, the indices of items to give up
    std::vector<C_car *> *cars;         // 0x38, the mission's cars GarageGameDone was given
};

// GM_GarageManager's results (the names are ours): an item was clicked, CarManager runs the menu again; the button
// 5000 closed it
#define GARAGEMANAGER_AGAIN 0x4f9
#define GARAGEMANAGER_DONE 0x4fa

// The garage manager's menu, "garagemanager" (the name is ours, after the menu), 0x40 bytes: G_LoadSaveClass::CarManager
// runs it with the cars it found, one item each (from 2000), until it ends with GARAGEMANAGER_DONE. A click on an item
// adds it to destroy or takes it out again and ends the menu with GARAGEMANAGER_AGAIN. Its constructor is inline
// (CarManager has it); OnBeforeCreate is in g_loadsave.cpp's range, the others in this file's.
// VTABLE: GAME 0x627a3c
class GM_GarageManager : public GM_Menu
{
public:
    GM_GarageManager(std::vector<S_garage_item> *items, std::vector<uint> *destroy, uint *active, C_car *cant_get_out)
        : items(items), destroy(destroy), active(active), cant_get_out(cant_get_out)
    {
    }

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint);
    virtual int OnClick(uint id);

    std::vector<S_garage_item> *items;  // 0x30, CarManager's cars (the names are ours)
    std::vector<uint> *destroy;         // 0x34, the indices of the items chosen to destroy
    uint *active;                       // 0x38, the item clicked last, active again when the menu runs again
    C_car *cant_get_out;                // 0x3c, G_LoadSaveClass::cant_get_out, whose item is disabled
};

// The garage's results at a mission's end, "garagesummary" (PS2's GM_GarageSummary, in PS2's g_m_garage.cpp), 0x38
// bytes: G_LoadSaveClass::GarageShowResultsProc runs it with the cars the garage got and lost since the last results.
// Its constructor and OnBeforeCreate are inline, OnBeforeCreate's copy in g_loadsave.cpp's range; OnCreate and OnKey
// are in this file's.
// VTABLE: GAME 0x627a6c
class GM_GarageSummary : public GM_Menu
{
public:
    GM_GarageSummary(std::vector<uint> *added, std::vector<uint> *removed) : added(added), removed(removed) {}

    // FUNCTION: GAME 0x60deb0
    virtual const char *OnBeforeCreate()
    {
        // STRING: GAME 0x6397a0
        return "garagesummary";
    }
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint);
    // 0, as PS2's: the vtable's entry is a folded copy of another file's (0x5e4d50, whose by-name marker is above)
    virtual int OnClick(uint id);

    std::vector<uint> *added;           // 0x30, the cars' descriptors (the names are ours)
    std::vector<uint> *removed;         // 0x34
};
