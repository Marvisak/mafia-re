// The game's menus (PS2 g_m_func.cpp, where PS2's GM_MainMenu is; the PC's file is the one at 0x5e0640-0x5e48ec,
// docs/game-files.md): the main menu, after its quit question's OnClick, the file's first function (its OnChange is
// folded into the racing menu's, 0x5e4a50, in the next file but one), the game's menu and its question, the options
// (from 0x5e0ad0, with the Audio & Video menu from 0x5e0bb0 and the controls from 0x5e1480), the free ride (from
// 0x5e1920, with the menus it opens: its track, its start and the simple message from 0x5e1d90), the PC's message box
// (from 0x5e2170) and question box (from 0x5e2310), the saved games (from 0x5e2400), the mission's failure (from
// 0x5e2880), the inventory (from 0x5e2990), the items' pick-up (from 0x5e2f20), the gun shop (from 0x5e3610), the
// loading screen (GM_LoadProgress, from 0x5e3aa0) and the garage's three menus (from 0x5e3da0), PS2's in PS2's order.
// Written whole; at the end the stubs of the car manufacturer's menu, whose functions are the next file but one's
// (range 106, PS2's g_m_race.cpp).

#include "ui/g_m_func.h"
#include "actors/g_inventory.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_comps.h"
#include "ui/g_textdb.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "vehicles/C_car.h"
#include "g_material.h"

#include <stdio.h>
#include <string.h>

#include "IGraph/IGraph.h"
#include "rw_data.h"
#include "stub.h"

// The main menu's items, tables\menu\mainmenu.mnu, and mainmenurac.mnu with Racing (the names are ours, after their
// texts). Decimal, as _GM_MainMenuTargets (g_menu.cpp) gives them: 9901 and the item's number.
#define MAINMENU_WINDOW 1               // "Main Menu:"
#define MAINMENU_ITEM_INTRO 990101
#define MAINMENU_ITEM_TUTORIAL 990102
#define MAINMENU_ITEM_NEW_GAME 990103
#define MAINMENU_ITEM_LOAD_GAME 990104
#define MAINMENU_ITEM_FREE_RIDE 990105
#define MAINMENU_ITEM_FREE_RIDE_EXTREME 990106
#define MAINMENU_ITEM_RACING 990107     // mainmenurac's only
#define MAINMENU_ITEM_MULTIPLAYER 990108 // in neither menu, though it has a camera frame and a case in OnClick
#define MAINMENU_ITEM_CARCYCLOPEDIA 990109
#define MAINMENU_ITEM_OPTIONS 990110
#define MAINMENU_ITEM_CREDITS 990111
#define MAINMENU_ITEM_CHANGE_PROFILE 990112
#define MAINMENU_ITEM_QUIT 990113

// The failure menu's items, tables\menu\gamefailed.mnu (the names are ours, after their texts)
#define GAMEFAILED_WINDOW 0xa           // "Game Over"
#define GAMEFAILED_TEXT 0x64            // the failure's text
#define GAMEFAILED_RETRY 0x3e8
#define GAMEFAILED_EXIT 0x3ea           // "Exit to Main Menu"

// The message box's items, tables\menu\messagebox.mnu (the names are ours)
#define MESSAGEBOX_WINDOW 1             // its text the title
#define MESSAGEBOX_TEXT 0x82            // the message
#define MESSAGEBOX_OK 0x83

// The saved games' items, tables\menu\loadgame.mnu (the names are ours, after their texts): the saved games' items are
// the first one's copies, numbered on from it, the newest first; Page Up and Page Down move by a page of them
#define LOADGAME_TOTAL 0xa              // "Total Saved Games: %d"
#define LOADGAME_LIST 4                 // the scroll window of the saved games' items
#define LOADGAME_ITEM 0x64              // the first saved game's
#define LOADGAME_PICTURE 0x3ea          // the active saved game's picture
#define LOADGAME_INFO 0x3f2             // "Enter-Select, Esc-Exit", the active saved game's head once loaded
#define LOADGAME_PAGE 18
// The ids below which items are saved games (the names are ours): for the keys and the click, and for the heads
#define LOADGAME_ITEM_END 1000
#define LOADGAME_ITEM_HEAD_END 500

// The text database's "Total Saved Games: %d" and the saved game's line, "Health: %d%%, Save Time: %d.%d.%d, %02d:%02d,
// Play Time: %dh %dmin %ds" (the names are ours)
#define TEXT_TOTAL_SAVED_GAMES 0xac
#define TEXT_SAVED_GAME_INFO 0xaf

// The loading screen's items, tables\menu\loadprogress.mnu (the names are ours)
#define LOADPROGRESS_SIDE 0x64          // the picture on the left, "loadbok.tga"
#define LOADPROGRESS_MISSION_PICTURE 0x65
#define LOADPROGRESS_LOGO 0x66          // "mafia.tga"
#define LOADPROGRESS_MISSION_NAME 0xc8
#define LOADPROGRESS_TITLE 0x190        // the constructor's text
#define LOADPROGRESS_ABORT 0x191        // under it, "press <ESC> to Abort"
// An 'R', a text and a second bar, which OnCreate hides
#define LOADPROGRESS_UNKNOWN_1000 1000
#define LOADPROGRESS_UNKNOWN_2000 2000
#define LOADPROGRESS_UNKNOWN_3000 3000

// The options' items, tables\menu\optionsb.mnu (the names are ours, after their texts)
#define OPTIONSB_WINDOW 2               // "Options:"
#define OPTIONSB_CONTROLS 0x64
#define OPTIONSB_AUDIO_VIDEO 0x65
#define OPTIONSB_BACK 0x190

// The Audio & Video menu's items, tables\menu\optionsa.mnu (the names are ours, after their texts)
#define OPTIONSA_GAMMA 0x50
#define OPTIONSA_SOUNDS 0x51
#define OPTIONSA_MUSIC 0x52
#define OPTIONSA_CARS 0x53
#define OPTIONSA_SPEECH 0x54
#define OPTIONSA_RESET 0x5a             // "Reset to Defaults"
#define OPTIONSA_BACK 0x5b
#define OPTIONSA_LOW 0x65               // the video options' levels
#define OPTIONSA_MEDIUM 0x66
#define OPTIONSA_HIGH 0x67
#define OPTIONSA_LEVEL_OF_DETAIL 0x6e
#define OPTIONSA_DRAW_DISTANCE 0x6f
#define OPTIONSA_BLOOD 0x70             // "Level of Blood"
#define OPTIONSA_GAME_EFFECTS 0x71      // "In Game Effects:"
#define OPTIONSA_GAME_SOUNDS 0x72       // "In Game Sounds:"
#define OPTIONSA_SHADOWS 0x73
#define OPTIONSA_PARTICLES 0x74

// The texts of the Audio & Video menu's selections (the names are ours, after the texts): Low, Medium and High by the
// level (S_mission_options' game effects, game sounds and particles), the shadows' from Very Low to Highest by
// S_mission_options::GetShadows' index
#define TEXT_QUALITY_LOW 0x105
#define TEXT_QUALITY_HIGH 0x107
#define TEXT_SHADOWS_VERY_LOW 0x119
#define TEXT_SHADOWS_HIGHEST 0x11e

// The controls' pages (GM_OptionsControls::page) and their buttons, the same in tables\menu\optionsc1.mnu to
// optionsc3.mnu (the names are ours, after their texts)
#define OPTIONSC_PAGE_PLAYER 1          // "optionsc1", "Player Controls:"
#define OPTIONSC_PAGE_CAR 2             // "optionsc2", "Car Controls:"
#define OPTIONSC_PAGE_OTHER 3           // "optionsc3", "Other Controls:"
#define OPTIONSC_PLAYER 0x32
#define OPTIONSC_CAR 0x33
#define OPTIONSC_OTHER 0x34
#define OPTIONSC_RESET 0x35             // "Reset to Defaults"
#define OPTIONSC_BACK 0x36
// The Other page's sliders and selections, optionsc3.mnu (the names are ours, after their texts, as the settings')
#define OPTIONSC_AIMING_SENSITIVITY_X 0x41
#define OPTIONSC_AIMING_SENSITIVITY_Y 0x42
#define OPTIONSC_AIMING_SPEED_X 0x43    // "Max Aiming Speed X"
#define OPTIONSC_STEERING_LINEARITY 0x44 // "Nonlinear / Linear"
#define OPTIONSC_CURSOR 0x45
#define OPTIONSC_SIDE_ROLL 0x47
#define OPTIONSC_MOUSE_INVERTED 0x49    // "Mouse control"
#define OPTIONSC_SPEEDOMETER 0x4b
#define OPTIONSC_SUBTITLES 0x4d
#define OPTIONSC_FF_VOLUME 0xbb9        // "Global Stick Volume"
#define OPTIONSC_FF_SUSPENSION 0xbba
#define OPTIONSC_FF_COLLISIONS 0xbbb
#define OPTIONSC_FF_REVS 0xbbc
#define OPTIONSC_FF_TIRE_GRIP 0xbbd
#define OPTIONSC_FF_PLAYER_MOVEMENT 0xbbe

// The texts of the Other page's selections (the names are ours, after the texts): the cursor's, CURSOR_* from
// TEXT_CURSOR, and each yes or no selection's for false and for true
#define TEXT_CURSOR 0xea                // "Crosshair"
#define TEXT_ENABLED 0xe4
#define TEXT_DISABLED 0xe5
#define TEXT_YES 0xf8
#define TEXT_NO 0xf9
#define TEXT_NORMAL 0xff
#define TEXT_INVERTED 0x100
#define TEXT_KILOMETERS_PER_HOUR 0x15c
#define TEXT_MILES_PER_HOUR 0x15d

// The quit question's items, tables\menu\exityesno.mnu: the question box's ids (the names are ours)
#define EXITYESNO_WINDOW 0x82           // its text set by OnCreate
#define EXITYESNO_YES 0x8a
#define EXITYESNO_NO 0x8b

// The text database's "press <ESC> to Abort", "Quit Game?" and "Exit Game?" (the names are ours)
#define TEXT_PRESS_ESC_TO_ABORT 0x2ce
#define TEXT_QUIT_GAME 0x77
#define TEXT_EXIT_GAME 0x95

// The inventory's items, tables\menu\inventory.mnu, and inventorycar.mnu with the car's (the names are ours, after
// their texts): the places' items, by G_Inventory's places, and the items', one in the menu, OnCreate's copies of it
// after it. The items' ids from 0x100 (INVENTORYMENU_ITEMS_MASK).
#define INVENTORYMENU_WINDOW 1          // "Inventory:"
#define INVENTORYMENU_HELP 4            // "Enter-Select, X-Drop", under the items
#define INVENTORYMENU_HAND 0x100        // "Hands:"
#define INVENTORYMENU_POCKET 0x102      // "Weapons:", the first of five
#define INVENTORYMENU_COAT 0x107        // "Coat:", the big weapon's
#define INVENTORYMENU_CAR 0x108         // inventorycar's "Car:"
#define INVENTORYMENU_PLACES_END 0x1ff  // the places' ids end below it
#define INVENTORYMENU_ITEM 0x200        // "Items:", the first
#define INVENTORYMENU_ITEMS_END 0x2ff   // and the items' below it
#define INVENTORYMENU_ITEMS_MASK 0xf00

// The gun shop's items, tables\menu\gunshop.mnu (the names are ours, after their texts): the inventory's places as
// the inventory's menu's (the coat's without a title of its own), the inventory's items' (which the menu doesn't have),
// the shop's weapons, one in the menu, OnCreate's copies of it after it, and Exit
#define GUNSHOP_WINDOW 1                // "Inventory (Drop)"
#define GUNSHOP_HAND 0x100
#define GUNSHOP_POCKET 0x102            // "Weapons:", the first of five
#define GUNSHOP_COAT 0x107              // under the pockets
#define GUNSHOP_PLACES_END 0x200
#define GUNSHOP_ITEM 0x200
#define GUNSHOP_ITEMS_END 0x258
#define GUNSHOP_WEAPON 0x3e8            // under "Select (Pick Up)", the first
#define GUNSHOP_EXIT 0x44c

// The text database's "< empty >", the items' names (gameItemTable's order, from GAMEITEM_NONE's "Empty hands"), and
// the gun shop's messages (the names are ours, after the texts)
#define TEXT_EMPTY 0x99
#define TEXT_ITEM_NAMES 0xdac
#define TEXT_NOT_ENOUGH_ROOM 0x528
#define TEXT_MUST_DROP_ITEM 0x529
#define TEXT_ALREADY_HAVE_WEAPON 0x52b

// The game's menu's items, tables\menu\gamemenu.mnu (the names are ours, after their texts)
#define GAMEMENU_WINDOW 1               // "Game menu:"
#define GAMEMENU_EXIT 0x66
#define GAMEMENU_LOAD_GAME 0x67
#define GAMEMENU_OPTIONS 0x6c
#define GAMEMENU_RESUME 0x6f

// The garage's menu at a mission's end's items, tables\menu\garageendofmission.mnu (the names are ours, after their
// texts): the cars' list's first item, whose copies OnCreate adds below it, and the ids OnCreate gives the list's items,
// GARAGEENDOFMISSION_CAR and a car's index in the garage's items, below GARAGEENDOFMISSION_CAR_END
#define GARAGEENDOFMISSION_WINDOW 1     // "Garage"
#define GARAGEENDOFMISSION_ITEM 2000    // "Mafia"
#define GARAGEENDOFMISSION_CAR 1000
#define GARAGEENDOFMISSION_CAR_END 1100

// The garage manager's items, tables\menu\garagemanager.mnu (the names are ours, after their texts): the cars' list's
// first item and its copies, GARAGEMANAGER_CAR and a car's index in the garage's items, below GARAGEMANAGER_CAR_END
#define GARAGEMANAGER_WINDOW 1          // "Garage"
#define GARAGEMANAGER_CAR 2000          // "You don't have any car."
#define GARAGEMANAGER_CAR_END 2100
#define GARAGEMANAGER_CLOSE 5000

// The garage's results' items, tables\menu\garagesummary.mnu (the names are ours, after their texts, by value where the
// menu doesn't have the item): the titles of the cars added and of those destroyed, each car's text a copy below its
// title. The menu has neither the second title nor items 2 and 3.
#define GARAGESUMMARY_WINDOW 1
#define GARAGESUMMARY_UNKNOWN_2 2       // OnCreate gives it the window's height
#define GARAGESUMMARY_UNKNOWN_3 3       // below the added cars
#define GARAGESUMMARY_CLOSE 0x64
#define GARAGESUMMARY_ADDED 1000        // "New cars have been added to the garage:"
#define GARAGESUMMARY_REMOVED 2000      // "Cars destroyed during the mission:"

// The garage's menus' texts (the names are ours, after the texts)
#define TEXT_GARAGE_ADDED 0x44d         // "New cars have been added to the garage:"
#define TEXT_GARAGE_REMOVED 0x452       // "Cars destroyed during the mission:"
#define TEXT_GARAGE_DESTROY 0x45f       // "(removed) ", before the name of a car chosen to destroy
#define TEXT_GARAGE_NO_CAR 0x477        // "You don't have any car."

// The item pick-up's items, tables\menu\itempickup.mnu (the names are ours, after their texts): the window, and the
// list's first item ("Exit" in the menu), which OnCreate gives the first item's text and copies below itself for each
// further one (ids from ITEMPICKUP_LIST up), then for its own Exit
#define ITEMPICKUP_WINDOW 1             // "Pickup/Use"
#define ITEMPICKUP_LIST 0x100
#define ITEMPICKUP_LIST_END 0x200       // OnClick's bound of the list's ids
#define ITEMPICKUP_EXIT 0x3e9           // OnCreate's, "0. Exit"
// How many items the menu lists at most (the name is ours)
#define ITEMPICKUP_MAX_ITEMS 63

// The texts of the items by their id (gameItemTable's), from the id 0's (the names are ours, after the texts): the
// console's "... collected" ("Nothing found" for 0; the items' names, TEXT_ITEM_NAMES, have "Empty hands" for 0 and
// "Special action" for PICKUP_TEXT); and the menu's "Exit"
#define TEXT_ITEM_COLLECTED 0xd48
#define TEXT_EXIT 0x9e

// The free ride's items, tables\menu\freeride.mnu (the names are ours, after their texts)
#define FREERIDE_PICTURE 0xa            // the track's picture, "freeride0.bmp" to "freeride4.bmp"
#define FREERIDE_TRAFFIC 0x15           // the densities' sliders
#define FREERIDE_PEDESTRIANS 0x16
#define FREERIDE_POLICE 0x17
// Check boxes the menu doesn't have (the names are guesses: the text database's "Wingmen:", "Paulie" and "Sam" follow
// the free ride menu's texts), each a bit of S_freeride::flags, the third FREERIDE_ROOF_OFF's
#define FREERIDE_PAULIE 0x19
#define FREERIDE_SAM 0x1a
#define FREERIDE_ROOF 0x1b
#define FREERIDE_TRACK 0x64             // the track's button, its text the track's
#define FREERIDE_CAR 0x65               // the car's button, its text the car's name
#define FREERIDE_BACK 0xc8
#define FREERIDE_OK 0xc9

// S_freeride::flags of the check boxes the free ride menu doesn't have, which nothing reads (the names are guesses,
// after the text database's "Paulie" and "Sam")
#define FREERIDE_WINGMAN_PAULIE 1
#define FREERIDE_WINGMAN_SAM 2

// The car the free ride menu starts with, tables\carindex.def's "taxi", "Falconer Yellowcar" (the name is ours)
#define FREERIDE_DEFAULT_CAR 0x17

// The texts of the free ride's tracks and of a car not chosen yet (the names are ours, after the texts)
#define TEXT_CITY_SMALL 0x41a
#define TEXT_CITY_DAYTIME 0x41b
#define TEXT_CITY_NIGHT 0x41c
#define TEXT_COUNTRYSIDE_DAYTIME 0x41d
#define TEXT_COUNTRYSIDE_NIGHT 0x41e
#define TEXT_NOT_SELECTED 0x2bf         // "<not selected>"

// The tracks' items, tables\menu\freeridetrack.mnu (the names are ours, after their texts)
#define FREERIDETRACK_PICTURE 0xa       // the active track's picture
#define FREERIDETRACK_BACK 0x64
#define FREERIDETRACK_CITY_SMALL 0x191
#define FREERIDETRACK_CITY_NIGHT 0x192
#define FREERIDETRACK_COUNTRYSIDE_NIGHT 0x193
#define FREERIDETRACK_CITY_DAYTIME 0x194
#define FREERIDETRACK_COUNTRYSIDE_DAYTIME 0x195

// The start's items, tables\menu\freeridestart.mnu (the names are ours, after their texts): a location's item is
// FREERIDESTART_LOCATIONS + the location (S_freeride::location, the frame emeth_<location>), 1 to 7, of which the menu
// has 4; the same four in the countryside, with other texts
#define FREERIDESTART_LOCATIONS 0x64
#define FREERIDESTART_BACK 0x64
#define FREERIDESTART_LITTLE_ITALY 0x65
#define FREERIDESTART_CENTRAL_ISLAND 0x66
#define FREERIDESTART_DOWN_TOWN 0x67
#define FREERIDESTART_HOBOKEN 0x68

// The texts of the start's locations in the countryside (the names are ours, after the texts)
#define TEXT_CHINA_TOWN_EXIT 0xfaa
#define TEXT_DAM 0xfab
#define TEXT_LAY_BY 0xfac
#define TEXT_MOTEL 0xfad

// The simple message's items, tables\menu\simplemessage.mnu (the names are ours): the message box's, with a second
// text where its OK is
#define SIMPLEMESSAGE_WINDOW 1
#define SIMPLEMESSAGE_TEXT 0x82
#define SIMPLEMESSAGE_TEXT2 0x83

// The menus the main menu opens, which only its OnClick creates. Their inline functions are declared in the classes and
// defined after OnClick, where the original has their copies: in the classes, their markers would come before the
// file's first function.

// The quit question, "exityesno" (PS2 GM_MainMenuExit, g_m_func.cpp), 0x30 bytes, which the main menu's Quit opens.
// Its constructor is the compiler's, inlined.
// VTABLE: GAME 0x62730c
class GM_MainMenuExit : public GM_Menu
{
public:
    virtual const char *OnBeforeCreate();
    // The window's text, "Quit Game?"
    virtual int OnCreate();
    // Yes ends the main menu with MAINMENU_QUIT, No closes the question
    virtual int OnClick(uint id);
};

// The options, "optionsb" (PS2 GM_Options, g_m_func.cpp, whose class is 0x4c bytes and has OnCreate, OnKey and OnTick
// of its own), 0x30 bytes, which the main menu's Options opens: Controls, Audio & Video and Back. Its constructor is
// inline, as the main menu's.
// VTABLE: GAME 0x62733c
class GM_Options : public GM_Menu
{
public:
    GM_Options() { flags |= MENUF_UNKNOWN_80000; }

    virtual const char *OnBeforeCreate();
    // Controls and Audio & Video open their menus, Back closes the options
    virtual int OnClick(uint id);
};

// The controls, "optionsc1" to "optionsc3" by the page (PS2 GM_OptionsControls, g_m_func.cpp, whose class is 0x34 bytes
// too: a pointer to the options' settings and three bytes; PS2's has one page and OnKey, SetUpTexts of its own), 0x34
// bytes, which the options' Controls opens on the player's page. Its constructor is inline: the page is stored before
// the vtable, an initializer. The bindings' items (GM_Vidc) read the profile's controls themselves.
// VTABLE: GAME 0x6273fc
class GM_OptionsControls : public GM_Menu
{
public:
    GM_OptionsControls() : page(OPTIONSC_PAGE_PLAYER) { flags |= MENUF_UNKNOWN_80000; }

    // The page's menu
    virtual const char *OnBeforeCreate();
    // The Other page's settings into its sliders and selections (the other pages have none of their ids), Back active
    virtual int OnCreate();
    // The profile's controls and settings take effect
    virtual int OnClose();
    // The page's buttons exchange the menu, the items set the controls
    virtual int OnClick(uint id);

    int page;                           // 0x30, OPTIONSC_PAGE_* (the name is ours)
};

// The audio and video settings, "optionsa" (PC only: PS2's GM_Options has the video settings it has; the name is ours,
// after its menu's title), 0x30 bytes, which the options' Audio & Video opens. Its constructor is inline, as the options'.
// VTABLE: GAME 0x62742c
class GM_OptionsAudioVideo : public GM_Menu
{
public:
    GM_OptionsAudioVideo() { flags |= MENUF_UNKNOWN_80000; }

    virtual const char *OnBeforeCreate();
    // The settings into the sliders and selections
    virtual int OnCreate();
    // An item sets its setting and applies it
    virtual int OnClick(uint id);
};

// The free ride's choices, "freeride" (PS2 GM_FreeRide, g_m_func.cpp, which has an OnChange of its own), 0x38 bytes,
// which the main menu's Free Ride opens: the track, the car and the densities of the traffic, the pedestrians and the
// police. Its constructor is inline, as the main menu's.
// VTABLE: GAME 0x62736c
class GM_FreeRide : public GM_Menu
{
public:
    GM_FreeRide() { flags |= MENUF_UNKNOWN_80000; }

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnCloseChild(GM_Menu *child, uint, uint);
    virtual int OnClick(uint id);

    // 0x30, the track, the main menu's result for it (MAINMENU_FREEITALY to MAINMENU_FREEKRAJINANOC), which the tracks'
    // menu sets and the free ride's OK ends the main menu with (the names are ours)
    uint track;
    uint car;                           // 0x34, the car's descriptor, which the car's manufacturer's menu sets
};

// The saved games, "loadgame" (PS2 GM_LoadGame, g_m_func.cpp, whose class is 0x284 bytes, with the preloaded pictures
// at its end), 0x270 bytes, which the main menu's and the game menu's Load Game open. Its constructor is inline, as the
// main menu's.
// VTABLE: GAME 0x62739c
class GM_LoadGame : public GM_Menu
{
public:
    GM_LoadGame() { flags |= MENUF_UNKNOWN_80000; }

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint);
    virtual int OnMouse(int x, int y, int z, uint, uint);
    virtual int OnClick(uint id);
    virtual int OnTick(uint time);
    virtual int OnChange(GM_Component *active, GM_Component *);

    // The names are ours
    char total_text[0x100];             // 0x30, item LOADGAME_TOTAL's
    G_SaveGame_FileHead head;           // 0x130, the active saved game's, which OnTick loads
    G_SaveGame_Info info;               // 0x148
    // 0x168, the ticks until OnTick loads the active saved game's head (1 from OnCreate, 4 from OnChange), 0 once loaded
    int head_delay;
    uint count;                         // 0x16c, of the saved games
    char info_text[0x100];              // 0x170, item LOADGAME_INFO's
};

// The game's quit question, "exityesno" as the main menu's (PS2 GM_GameMenuExit, g_m_func.cpp), 0x30 bytes, which the
// game's menu's Exit Game opens. Its constructor is the compiler's, inlined.
// VTABLE: GAME 0x6273cc
class GM_GameMenuExit : public GM_Menu
{
public:
    // The linker folded its copy with the main menu's question's (0x5e08e0)
    virtual const char *OnBeforeCreate();
    // The window's text, "Exit Game?"
    virtual int OnCreate();
    // Yes ends the game's menu with GAMEMENU_RESULT_EXIT, No closes the question
    virtual int OnClick(uint id);
};

// The menus the free ride's OnClick opens (PC only, the names are ours, after their menus). Their OnBeforeCreate are
// inline, defined after OnClick, where the original has their copies, with the car's.

// The free ride's tracks, "freeridetrack", 0x30 bytes, which the free ride's track button opens: a track's item closes
// it with the track, the main menu's result for it (MAINMENU_FREEITALY to MAINMENU_FREEKRAJINANOC), which the free
// ride's OnCloseChild gets. Its constructor is inline, as the free ride's.
// VTABLE: GAME 0x6274c4
class GM_FreeRideTrack : public GM_Menu
{
public:
    GM_FreeRideTrack() { flags |= MENUF_UNKNOWN_80000; }

    virtual const char *OnBeforeCreate();
    // Disables the tracks the free ride hasn't opened (G_LoadSaveClass::freeride_enabled)
    virtual int OnCreate();
    // A track closes the menu with the track, Back with -1
    virtual int OnClick(uint id);
    // The active track's picture
    virtual int OnChange(GM_Component *active, GM_Component *);
};

// The free ride's start, "freeridestart", 0x34 bytes, which the free ride's OK opens on any track but the small
// city's: a location's item stores it (G_LoadSaveClass::FreerideStoreB) and closes the menus with the track, the main
// menu's result. Its constructor is inline: the track is stored before the vtable, an initializer.
// VTABLE: GAME 0x627464
class GM_FreeRideStart : public GM_Menu
{
public:
    GM_FreeRideStart(uint track) : track(track) { flags |= MENUF_UNKNOWN_80000; }

    virtual const char *OnBeforeCreate();
    // The countryside's locations' texts
    virtual int OnCreate();
    virtual int OnClick(uint id);

    uint track;                         // 0x30, the free ride's (the name is ours)
};

// The car's manufacturer, "selectcarmanufacturer" (PS2 GM_SelectCarManufacturer, in PS2's racing menus), 0x3c bytes
// (PS2's 0x38), which the free ride's car button opens, and the racing menus too (range 106, at 0x5e4efc and
// 0x5e5dc9): a manufacturer's item opens the car models' menu (vtable 0x627740, 0x4c bytes), which puts the car it
// chooses into chosen; OnCloseChild then gives it to car and closes the menu with 1. Its constructor is inline (the
// free ride's OnClick has it), and its OnBeforeCreate's copy is this file's, after the free ride's OnClick; OnCreate,
// OnCloseChild and OnClick are range 106's, whose file isn't written yet: stubs at this file's end, the class here
// until that file has a header for it.
// VTABLE: GAME 0x627494
class GM_SelectCarManufacturer : public GM_Menu
{
public:
    GM_SelectCarManufacturer(uint *car, uint race_cars) : car(car), race_cars(race_cars) {}

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnCloseChild(GM_Menu *child, uint, uint);
    virtual int OnClick(uint id);

    uint *car;                          // 0x30, where the car's descriptor goes (the names are ours)
    uint chosen;                        // 0x34, the car models' menu's car, -1 from OnCreate
    // 0x38, the race cars the menu offers, RaceCarsGetEnabled's bits (the name is a guess), which OnCreate reads
    // outside a race
    uint race_cars;
};

// A message of one or two lines without a button, "simplemessage" (PC only, the name is ours, after its menu), 0x38
// bytes, which the gun shop opens: the message box's code (GM_MessageBox), with the second text where the message box
// has OK. Both constructors are out of line. Its OnKey and OnClick are the message box's: the linker kept the message
// box's copies of the folded groups (0x5e22d0, 0x5e22f0).
// VTABLE: GAME 0x6274f4
class GM_SimpleMessage : public GM_Menu
{
public:
    // The texts, the second NULL for none
    GM_SimpleMessage(char *text, char *text2);
    // The texts of the text database (no second text for an id it doesn't have)
    GM_SimpleMessage(uint text_id, uint text2_id);

    virtual const char *OnBeforeCreate();
    // The texts, the window grown to fit the first one
    virtual int OnCreate();
    virtual int OnKey(uchar key, uint);
    virtual int OnClick(uint id);

    char *text;                         // 0x30, item 0x82's (the names are ours)
    char *text2;                        // 0x34, item 0x83's, NULL for none
};

// The file's first function. The cases in the order of PS2's bodies: VC6's dec falls into the one written first.
// FUNCTION: GAME 0x5e0640
int GM_MainMenuExit::OnClick(uint id)
{
    switch (id)
    {
    case EXITYESNO_NO:
        MenuClose((uint)-1);
        break;
    case EXITYESNO_YES:
        MenuCloseAll((uint)-1, MAINMENU_QUIT);
        break;
    }
    return 0;
}

// A mission's entry in tables\load.def (the names are ours), 0x44 bytes: its loading screen's picture and name
struct S_load_def
{
    char mission[0x20];                 // the mission's folder, "mise01"
    char picture[0x20];                 // 0x20, "M01.tga"
    uint name_id;                       // 0x40, of the text database ("An Offer You Can't Refuse"), 0 for none
};

// PS2's is always "mainmenu", without Racing
// FUNCTION: GAME 0x5e0670
const char *GM_MainMenu::OnBeforeCreate()
{
    return RacingInMainMenu() ?
        // STRING: GAME 0x6381dc
        "mainmenurac" :
        // STRING: GAME 0x6381d0
        "mainmenu";
}

// The camera at "camera_start", Free Ride Extreme only once the whole free ride is open, the free ride's choices reset
// and Intro active. PS2's has no free ride here: it enables and activates the items by the memory card's saves and puts
// a text into item 4.
// FUNCTION: GAME 0x5e0690
int GM_MainMenu::OnCreate()
{
    SetCameraTargetID(MENUCAM_START);
    if (mafiaLoadSave.freeride_enabled <= FREERIDE_COUNTRYSIDE)
        SetDisabled(MAINMENU_ITEM_FREE_RIDE_EXTREME, false);
    mafiaLoadSave.FreerideStoreA(1, 0, 1.0f, 1.0f, 1.0f);
    ActivateByIDOrNext(MAINMENU_ITEM_INTRO);
    return 0;
}

// The camera goes to the frame of the item that became active. The linker kept the racing menu's identical copy
// (0x5e4a50, its vtable 0x625264), after its OnClick. PS2's also clears a member the PC's class doesn't have.
// FUNCTION: GAME 0x5e4a50 FOLDED
int GM_MainMenu::OnChange(GM_Component *active, GM_Component *)
{
    if (active)
        SetCameraTargetID(active->id);
    return 0;
}

// The cases in the items' order, as PS2's, whose first two choose the intro or a new game by the memory card's saves and
// open the PSX menus
// FUNCTION: GAME 0x5e06e0
int GM_MainMenu::OnClick(uint id)
{
    switch (id)
    {
    case MAINMENU_ITEM_INTRO:
        MenuCloseAll((uint)-1, MAINMENU_INTRO);
        break;
    case MAINMENU_ITEM_TUTORIAL:
        MenuCloseAll((uint)-1, MAINMENU_TUTORIAL);
        break;
    case MAINMENU_ITEM_NEW_GAME:
        MenuCloseAll((uint)-1, MAINMENU_NEW_GAME);
        break;
    case MAINMENU_ITEM_LOAD_GAME:
        MenuOpen(new GM_LoadGame, id);
        break;
    case MAINMENU_ITEM_FREE_RIDE:
        MenuOpen(new GM_FreeRide, id);
        break;
    case MAINMENU_ITEM_FREE_RIDE_EXTREME:
        MenuCloseAll((uint)-1, MAINMENU_EXTREME);
        break;
    case MAINMENU_ITEM_RACING:
        MenuCloseAll((uint)-1, MAINMENU_RACING);
        break;
    case MAINMENU_ITEM_MULTIPLAYER:
        MenuCloseAll((uint)-1, MAINMENU_MULTIPLAYER);
        break;
    case MAINMENU_ITEM_CARCYCLOPEDIA:
        MenuCloseAll((uint)-1, MAINMENU_CARCYCLOPEDIA);
        break;
    case MAINMENU_ITEM_OPTIONS:
        MenuOpen(new GM_Options, id);
        break;
    case MAINMENU_ITEM_CREDITS:
        MenuCloseAll((uint)-1, MAINMENU_CREDITS);
        break;
    case MAINMENU_ITEM_CHANGE_PROFILE:
        MenuCloseAll((uint)-1, MAINMENU_PROFILE_CHANGE);
        break;
    case MAINMENU_ITEM_QUIT:
        MenuOpen(new GM_MainMenuExit, id);
        break;
    }
    return 0;
}

// Folded with GM_GameMenuExit's: its marker is in the header, by name
inline const char *GM_MainMenuExit::OnBeforeCreate()
{
    // STRING: GAME 0x6381e8
    return "exityesno";
}

// FUNCTION: GAME 0x5e08f0
inline int GM_MainMenuExit::OnCreate()
{
    SetTextID(EXITYESNO_WINDOW, TEXT_QUIT_GAME);
    return 0;
}

// FUNCTION: GAME 0x5e0900
inline const char *GM_Options::OnBeforeCreate()
{
    // STRING: GAME 0x6381f4
    return "optionsb";
}

// The literal is main's (Main.cpp), the first object's that has it
// FUNCTION: GAME 0x5e0910
inline const char *GM_FreeRide::OnBeforeCreate()
{
    return "freeride";
}

// FUNCTION: GAME 0x5e0920
inline const char *GM_LoadGame::OnBeforeCreate()
{
    // STRING: GAME 0x638200
    return "loadgame";
}

// The game's menu (GameLoopSingle's, Main.cpp) and its quit question, in PS2's order: as the main menu's question, but
// Yes ends the game's menu
// FUNCTION: GAME 0x5e0930
int GM_GameMenuExit::OnClick(uint id)
{
    switch (id)
    {
    case EXITYESNO_NO:
        MenuClose((uint)-1);
        break;
    case EXITYESNO_YES:
        MenuCloseAll((uint)-1, GAMEMENU_RESULT_EXIT);
        break;
    }
    return 0;
}

// FUNCTION: GAME 0x5e0960
int GM_GameMenu::OnKey(uchar key, uint)
{
    if (key == DIK_ESCAPE)
        MenuCloseAll((uint)-1, GAMEMENU_RESULT_RESUME);
    return 0;
}

// FUNCTION: GAME 0x5e0980
int GM_GameMenu::OnClick(uint id)
{
    switch (id)
    {
    case GAMEMENU_EXIT:
        MenuOpen(new GM_GameMenuExit, id);
        break;
    case GAMEMENU_LOAD_GAME:
        MenuOpen(new GM_LoadGame, id);
        break;
    case GAMEMENU_OPTIONS:
        MenuOpen(new GM_Options, id);
        break;
    case GAMEMENU_RESUME:
        MenuCloseAll((uint)-1, GAMEMENU_RESULT_RESUME);
        break;
    }
    return 0;
}

inline const char *GM_GameMenuExit::OnBeforeCreate()
{
    return "exityesno";
}

// FUNCTION: GAME 0x5e0ab0
inline int GM_GameMenuExit::OnCreate()
{
    SetTextID(EXITYESNO_WINDOW, TEXT_EXIT_GAME);
    return 0;
}

// PS2's options have a screen slide and the vibration besides Controls.
// FUNCTION: GAME 0x5e0ad0
int GM_Options::OnClick(uint id)
{
    switch (id)
    {
    case OPTIONSB_CONTROLS:
        MenuOpen(new GM_OptionsControls, id);
        break;
    case OPTIONSB_AUDIO_VIDEO:
        MenuOpen(new GM_OptionsAudioVideo, id);
        break;
    case OPTIONSB_BACK:
        MenuClose((uint)-1);
        break;
    }
    return 0;
}

// FUNCTION: GAME 0x5e0bb0
inline const char *GM_OptionsAudioVideo::OnBeforeCreate()
{
    // STRING: GAME 0x63820c
    return "optionsa";
}

// The settings into the sliders and selections, the mission's video options from their ranges (the level of detail
// 0.4 to 2, the draw distance 0.3 to 1) to 0..1, Back active
// FUNCTION: GAME 0x5e0bc0
int GM_OptionsAudioVideo::OnCreate()
{
    flags &= ~MENUF_DRAW_PARENT;
    SetFloat(OPTIONSA_SOUNDS, mafiaLoadSave.settings.sound_volume);
    SetFloat(OPTIONSA_MUSIC, mafiaLoadSave.settings.music_volume);
    SetFloat(OPTIONSA_CARS, mafiaLoadSave.settings.car_volume);
    SetFloat(OPTIONSA_SPEECH, mafiaLoadSave.settings.speech_volume);
    SetFloat(OPTIONSA_GAMMA, mafiaLoadSave.settings.gamma);
    SetFloat(OPTIONSA_BLOOD, mafiaLoadSave.settings.blood_level);
    SetFloat(OPTIONSA_LEVEL_OF_DETAIL, (act_mission->options.level_of_detail - .4f) / 1.6f);
    SetFloat(OPTIONSA_DRAW_DISTANCE, (act_mission->options.draw_distance - .3f) / .7f);
    SetRangeDword(OPTIONSA_GAME_EFFECTS, TEXT_QUALITY_LOW, TEXT_QUALITY_HIGH);
    SetTextID(OPTIONSA_GAME_EFFECTS, TEXT_QUALITY_LOW + act_mission->options.game_effects);
    SetRangeDword(OPTIONSA_GAME_SOUNDS, TEXT_QUALITY_LOW, TEXT_QUALITY_HIGH);
    SetTextID(OPTIONSA_GAME_SOUNDS, TEXT_QUALITY_LOW + act_mission->options.game_sounds);
    SetRangeDword(OPTIONSA_SHADOWS, TEXT_SHADOWS_VERY_LOW, TEXT_SHADOWS_HIGHEST);
    SetTextID(OPTIONSA_SHADOWS, TEXT_SHADOWS_VERY_LOW + act_mission->options.GetShadows());
    SetRangeDword(OPTIONSA_PARTICLES, TEXT_QUALITY_LOW, TEXT_QUALITY_HIGH);
    SetTextID(OPTIONSA_PARTICLES, TEXT_QUALITY_LOW + act_mission->options.particles);
    ActivateByID(OPTIONSA_BACK);
    return 0;
}

// Each item sets its setting and applies it (the blood's applies itself), the mission's video options through
// UpdateSettings: through GetOptions, which VC6 evaluates before the item's value is read, where the member read
// directly comes after it. Low, Medium and High set the video options to their levels, the sliders' values to 0, .5 and 1
// (each read back from its slider); Reset to Defaults sets the audio and the gamma to the defaults, the video to High,
// and applies all of it.
// FUNCTION: GAME 0x5e0d40
int GM_OptionsAudioVideo::OnClick(uint id)
{
    switch (id)
    {
    case OPTIONSA_GAMMA:
        mafiaLoadSave.settings.gamma = GetFloat(id);
        mafiaLoadSave.Refresh(REFRESH_GAMMA);
        break;
    case OPTIONSA_SOUNDS:
        mafiaLoadSave.settings.sound_volume = GetFloat(id);
        mafiaLoadSave.Refresh(REFRESH_SOUND);
        break;
    case OPTIONSA_MUSIC:
        mafiaLoadSave.settings.music_volume = GetFloat(id);
        mafiaLoadSave.Refresh(REFRESH_MUSIC);
        break;
    case OPTIONSA_CARS:
        mafiaLoadSave.settings.car_volume = GetFloat(id);
        mafiaLoadSave.Refresh(REFRESH_CARS);
        break;
    case OPTIONSA_SPEECH:
        mafiaLoadSave.settings.speech_volume = GetFloat(id);
        mafiaLoadSave.Refresh(REFRESH_SPEECH);
        break;
    case OPTIONSA_RESET:
        mafiaLoadSave.DefaultSettings(DEFAULTS_AUDIO_VIDEO);
        SetFloat(OPTIONSA_SOUNDS, mafiaLoadSave.settings.sound_volume);
        SetFloat(OPTIONSA_MUSIC, mafiaLoadSave.settings.music_volume);
        SetFloat(OPTIONSA_CARS, mafiaLoadSave.settings.car_volume);
        SetFloat(OPTIONSA_SPEECH, mafiaLoadSave.settings.speech_volume);
        SetFloat(OPTIONSA_GAMMA, mafiaLoadSave.settings.gamma);
        SetFloat(OPTIONSA_BLOOD, mafiaLoadSave.settings.blood_level);
        act_mission->GetOptions().SetGameEffects(2);
        SetTextID(OPTIONSA_GAME_EFFECTS, TEXT_QUALITY_LOW + 2);
        act_mission->GetOptions().SetGameSounds(2);
        SetTextID(OPTIONSA_GAME_SOUNDS, TEXT_QUALITY_LOW + 2);
        act_mission->GetOptions().SetShadows(5);
        SetTextID(OPTIONSA_SHADOWS, TEXT_SHADOWS_VERY_LOW + 5);
        act_mission->GetOptions().SetParticles(2);
        SetTextID(OPTIONSA_PARTICLES, TEXT_QUALITY_LOW + 2);
        SetFloat(OPTIONSA_LEVEL_OF_DETAIL, 1.0f);
        act_mission->GetOptions().SetLevelOfDetail(GetFloat(OPTIONSA_LEVEL_OF_DETAIL) * 1.6f + .4f);
        SetFloat(OPTIONSA_DRAW_DISTANCE, 1.0f);
        act_mission->GetOptions().SetDrawDistance(GetFloat(OPTIONSA_DRAW_DISTANCE) * .7f + .3f);
        mafiaLoadSave.Refresh(REFRESH_ALL);
        act_mission->UpdateSettings();
        act_mission->game->UpdateControls();
        break;
    case OPTIONSA_BACK:
        MenuClose((uint)-1);
        break;
    case OPTIONSA_LOW:
        act_mission->GetOptions().SetGameEffects(0);
        SetTextID(OPTIONSA_GAME_EFFECTS, TEXT_QUALITY_LOW + 0);
        act_mission->GetOptions().SetGameSounds(0);
        SetTextID(OPTIONSA_GAME_SOUNDS, TEXT_QUALITY_LOW + 0);
        act_mission->GetOptions().SetShadows(0);
        SetTextID(OPTIONSA_SHADOWS, TEXT_SHADOWS_VERY_LOW + 0);
        act_mission->GetOptions().SetParticles(0);
        SetTextID(OPTIONSA_PARTICLES, TEXT_QUALITY_LOW + 0);
        SetFloat(OPTIONSA_LEVEL_OF_DETAIL, 0.0f);
        act_mission->GetOptions().SetLevelOfDetail(GetFloat(OPTIONSA_LEVEL_OF_DETAIL) * 1.6f + .4f);
        SetFloat(OPTIONSA_DRAW_DISTANCE, 0.0f);
        act_mission->GetOptions().SetDrawDistance(GetFloat(OPTIONSA_DRAW_DISTANCE) * .7f + .3f);
        act_mission->UpdateSettings();
        break;
    case OPTIONSA_MEDIUM:
        act_mission->GetOptions().SetGameEffects(1);
        SetTextID(OPTIONSA_GAME_EFFECTS, TEXT_QUALITY_LOW + 1);
        act_mission->GetOptions().SetGameSounds(1);
        SetTextID(OPTIONSA_GAME_SOUNDS, TEXT_QUALITY_LOW + 1);
        act_mission->GetOptions().SetShadows(2);
        SetTextID(OPTIONSA_SHADOWS, TEXT_SHADOWS_VERY_LOW + 2);
        act_mission->GetOptions().SetParticles(1);
        SetTextID(OPTIONSA_PARTICLES, TEXT_QUALITY_LOW + 1);
        SetFloat(OPTIONSA_LEVEL_OF_DETAIL, .5f);
        act_mission->GetOptions().SetLevelOfDetail(GetFloat(OPTIONSA_LEVEL_OF_DETAIL) * 1.6f + .4f);
        SetFloat(OPTIONSA_DRAW_DISTANCE, .5f);
        act_mission->GetOptions().SetDrawDistance(GetFloat(OPTIONSA_DRAW_DISTANCE) * .7f + .3f);
        act_mission->UpdateSettings();
        break;
    case OPTIONSA_HIGH:
        act_mission->GetOptions().SetGameEffects(2);
        SetTextID(OPTIONSA_GAME_EFFECTS, TEXT_QUALITY_LOW + 2);
        act_mission->GetOptions().SetGameSounds(2);
        SetTextID(OPTIONSA_GAME_SOUNDS, TEXT_QUALITY_LOW + 2);
        act_mission->GetOptions().SetShadows(5);
        SetTextID(OPTIONSA_SHADOWS, TEXT_SHADOWS_VERY_LOW + 5);
        act_mission->GetOptions().SetParticles(2);
        SetTextID(OPTIONSA_PARTICLES, TEXT_QUALITY_LOW + 2);
        SetFloat(OPTIONSA_LEVEL_OF_DETAIL, 1.0f);
        act_mission->GetOptions().SetLevelOfDetail(GetFloat(OPTIONSA_LEVEL_OF_DETAIL) * 1.6f + .4f);
        SetFloat(OPTIONSA_DRAW_DISTANCE, 1.0f);
        act_mission->GetOptions().SetDrawDistance(GetFloat(OPTIONSA_DRAW_DISTANCE) * .7f + .3f);
        act_mission->UpdateSettings();
        break;
    case OPTIONSA_LEVEL_OF_DETAIL:
        act_mission->GetOptions().SetLevelOfDetail(GetFloat(id) * 1.6f + .4f);
        act_mission->UpdateSettings();
        break;
    case OPTIONSA_DRAW_DISTANCE:
        act_mission->GetOptions().SetDrawDistance(GetFloat(id) * .7f + .3f);
        act_mission->UpdateSettings();
        break;
    case OPTIONSA_BLOOD:
        mafiaLoadSave.settings.blood_level = GetFloat(id);
        break;
    case OPTIONSA_GAME_EFFECTS:
        act_mission->GetOptions().SetGameEffects(GetTextID(id) - TEXT_QUALITY_LOW);
        act_mission->UpdateSettings();
        break;
    case OPTIONSA_GAME_SOUNDS:
        act_mission->GetOptions().SetGameSounds(GetTextID(id) - TEXT_QUALITY_LOW);
        act_mission->UpdateSettings();
        break;
    case OPTIONSA_SHADOWS:
        act_mission->GetOptions().SetShadows(GetTextID(id) - TEXT_SHADOWS_VERY_LOW);
        act_mission->UpdateSettings();
        break;
    case OPTIONSA_PARTICLES:
        act_mission->GetOptions().SetParticles(GetTextID(id) - TEXT_QUALITY_LOW);
        act_mission->UpdateSettings();
        break;
    }
    return 0;
}

// FUNCTION: GAME 0x5e1480
inline const char *GM_OptionsControls::OnBeforeCreate()
{
    switch (page)
    {
    case OPTIONSC_PAGE_PLAYER:
        // STRING: GAME 0x638218
        return "optionsc1";
    case OPTIONSC_PAGE_CAR:
        // STRING: GAME 0x638224
        return "optionsc2";
    }
    // STRING: GAME 0x638230
    return "optionsc3";
}

// FUNCTION: GAME 0x5e14a0
int GM_OptionsControls::OnCreate()
{
    flags &= ~MENUF_DRAW_PARENT;
    SetFloat(OPTIONSC_STEERING_LINEARITY, mafiaLoadSave.settings.steering_linearity);
    SetFloat(OPTIONSC_AIMING_SENSITIVITY_X, mafiaLoadSave.settings.aiming_sensitivity_x);
    SetFloat(OPTIONSC_AIMING_SENSITIVITY_Y, mafiaLoadSave.settings.aiming_sensitivity_y);
    SetFloat(OPTIONSC_AIMING_SPEED_X, mafiaLoadSave.settings.aiming_speed_x);
    SetRangeDword(OPTIONSC_CURSOR, TEXT_CURSOR + CURSOR_CROSSHAIR, TEXT_CURSOR + CURSOR_BOTH);
    SetTextID(OPTIONSC_CURSOR, TEXT_CURSOR + mafiaLoadSave.settings.cursor);
    SetFloat(OPTIONSC_FF_VOLUME, mafiaLoadSave.settings.ff_volume);
    SetFloat(OPTIONSC_FF_SUSPENSION, mafiaLoadSave.settings.ff_suspension);
    SetFloat(OPTIONSC_FF_COLLISIONS, mafiaLoadSave.settings.ff_collisions);
    SetFloat(OPTIONSC_FF_REVS, mafiaLoadSave.settings.ff_revs);
    SetFloat(OPTIONSC_FF_TIRE_GRIP, mafiaLoadSave.settings.ff_tire_grip);
    SetFloat(OPTIONSC_FF_PLAYER_MOVEMENT, mafiaLoadSave.settings.ff_player_movement);
    SetRangeDword(OPTIONSC_SIDE_ROLL, TEXT_NO, TEXT_YES);
    SetBool(OPTIONSC_SIDE_ROLL, mafiaLoadSave.settings.side_roll);
    SetRangeDword(OPTIONSC_MOUSE_INVERTED, TEXT_NORMAL, TEXT_INVERTED);
    SetBool(OPTIONSC_MOUSE_INVERTED, mafiaLoadSave.settings.mouse_inverted);
    SetRangeDword(OPTIONSC_SPEEDOMETER, TEXT_KILOMETERS_PER_HOUR, TEXT_MILES_PER_HOUR);
    SetBool(OPTIONSC_SPEEDOMETER, mafiaLoadSave.settings.speedometer_mph);
    SetRangeDword(OPTIONSC_SUBTITLES, TEXT_DISABLED, TEXT_ENABLED);
    SetBool(OPTIONSC_SUBTITLES, mafiaLoadSave.settings.subtitles);
    ActivateByID(OPTIONSC_BACK);
    return 0;
}

// The bindings to the controls' object, the HUD's textures again (the speedometer's units), and all of the settings
// FUNCTION: GAME 0x5e1620
int GM_OptionsControls::OnClose()
{
    mafiaLoadSave.ApplyControls();
    mafiaIndicators.GameInit();
    mafiaLoadSave.Refresh(REFRESH_ALL);
    return 0;
}

// The page's buttons exchange the menu for the page's, Reset to Defaults resets the page's controls and settings and
// applies them, Back closes it; the other items set their settings, the force feedback's applied, but the player
// movement's (which Refresh doesn't apply)
// FUNCTION: GAME 0x5e1650
int GM_OptionsControls::OnClick(uint id)
{
    switch (id)
    {
    case OPTIONSC_PLAYER:
        page = OPTIONSC_PAGE_PLAYER;
        MenuExchange();
        ActivateByID(id);
        break;
    case OPTIONSC_CAR:
        page = OPTIONSC_PAGE_CAR;
        MenuExchange();
        ActivateByID(id);
        break;
    case OPTIONSC_OTHER:
        page = OPTIONSC_PAGE_OTHER;
        MenuExchange();
        ActivateByID(id);
        break;
    case OPTIONSC_RESET:
        switch (page)
        {
        case OPTIONSC_PAGE_PLAYER:
            mafiaLoadSave.DefaultControls(DEFAULTS_PLAYER);
            mafiaLoadSave.DefaultSettings(DEFAULTS_PLAYER);
            break;
        case OPTIONSC_PAGE_CAR:
            mafiaLoadSave.DefaultControls(DEFAULTS_CAR);
            mafiaLoadSave.DefaultSettings(DEFAULTS_CAR);
            break;
        default:
            mafiaLoadSave.DefaultControls(DEFAULTS_OTHER);
            mafiaLoadSave.DefaultSettings(DEFAULTS_OTHER);
            break;
        }
        OnCreate();
        mafiaLoadSave.ApplyControls();
        mafiaLoadSave.Refresh(REFRESH_ALL);
        act_mission->game->UpdateControls();
        ForceUpdate();
        ActivateByID(id);
        break;
    case OPTIONSC_BACK:
        MenuClose((uint)-1);
        break;
    case OPTIONSC_STEERING_LINEARITY:
        mafiaLoadSave.settings.steering_linearity = GetFloat(id);
        break;
    case OPTIONSC_AIMING_SENSITIVITY_X:
        mafiaLoadSave.settings.aiming_sensitivity_x = GetFloat(id);
        break;
    case OPTIONSC_AIMING_SENSITIVITY_Y:
        mafiaLoadSave.settings.aiming_sensitivity_y = GetFloat(id);
        break;
    case OPTIONSC_AIMING_SPEED_X:
        mafiaLoadSave.settings.aiming_speed_x = GetFloat(id);
        break;
    case OPTIONSC_CURSOR:
        mafiaLoadSave.settings.cursor = GetTextID(id) - TEXT_CURSOR;
        break;
    case OPTIONSC_FF_VOLUME:
    case OPTIONSC_FF_SUSPENSION:
    case OPTIONSC_FF_COLLISIONS:
    case OPTIONSC_FF_REVS:
    case OPTIONSC_FF_TIRE_GRIP:
        mafiaLoadSave.settings.ff_volume = GetFloat(OPTIONSC_FF_VOLUME);
        mafiaLoadSave.settings.ff_suspension = GetFloat(OPTIONSC_FF_SUSPENSION);
        mafiaLoadSave.settings.ff_collisions = GetFloat(OPTIONSC_FF_COLLISIONS);
        mafiaLoadSave.settings.ff_revs = GetFloat(OPTIONSC_FF_REVS);
        mafiaLoadSave.settings.ff_tire_grip = GetFloat(OPTIONSC_FF_TIRE_GRIP);
        mafiaLoadSave.Refresh(REFRESH_FORCE_FEEDBACK);
        break;
    case OPTIONSC_FF_PLAYER_MOVEMENT:
        mafiaLoadSave.settings.ff_player_movement = GetFloat(id);
        mafiaLoadSave.Refresh(REFRESH_FF_PLAYER);
        break;
    case OPTIONSC_SIDE_ROLL:
        mafiaLoadSave.settings.side_roll = GetBool(id);
        break;
    case OPTIONSC_MOUSE_INVERTED:
        mafiaLoadSave.settings.mouse_inverted = GetBool(id);
        break;
    case OPTIONSC_SPEEDOMETER:
        mafiaLoadSave.settings.speedometer_mph = GetBool(id);
        break;
    case OPTIONSC_SUBTITLES:
        mafiaLoadSave.settings.subtitles = GetBool(id);
        break;
    }
    return 0;
}

// The free ride's choices before Free Ride Extreme is open: the densities' sliders and the check boxes disabled, the
// roof's deleted (with the small city only, disabled first). The densities at 1, the car the taxi, the track the small
// city, OK active. PS2's has a selection of two tracks and the police's check box, and no densities.
// FUNCTION: GAME 0x5e1920
int GM_FreeRide::OnCreate()
{
    flags &= ~MENUF_DRAW_PARENT;
    switch (mafiaLoadSave.freeride_enabled)
    {
    case FREERIDE_SMALL_CITY:
        SetDisabled(FREERIDE_ROOF, false);
    case FREERIDE_CITY:
    case FREERIDE_COUNTRYSIDE:
        SetDisabled(FREERIDE_PAULIE, false);
        SetDisabled(FREERIDE_SAM, false);
        SetDisabled(FREERIDE_TRAFFIC, false);
        SetDisabled(FREERIDE_PEDESTRIANS, false);
        SetDisabled(FREERIDE_POLICE, false);
        DeleteComponent(FREERIDE_ROOF);
        break;
    }
    SetFloat(FREERIDE_TRAFFIC, 1.0f);
    SetFloat(FREERIDE_PEDESTRIANS, 1.0f);
    SetFloat(FREERIDE_POLICE, 1.0f);
    car = FREERIDE_DEFAULT_CAR;
    if ((car & CAR_INDEX_MASK) > 0 && (car & CAR_INDEX_MASK) < mafiaCarDatabase.num_cars)
        SetText(FREERIDE_CAR, (uchar *)mafiaCarDatabase.GetCarName(car));
    else
        SetTextID(FREERIDE_CAR, TEXT_NOT_SELECTED);
    track = MAINMENU_FREEITALY;
    // STRING: GAME 0x63823c
    SetString(FREERIDE_PICTURE, (uchar *)"freeride0.bmp");
    ActivateByID(FREERIDE_OK);
    return 0;
}

// The tracks' menu's track with its text and picture, Back enabled; the car's manufacturer's car's name, or "<not
// selected>" for a descriptor out of the table
// FUNCTION: GAME 0x5e1a20
int GM_FreeRide::OnCloseChild(GM_Menu *child, uint id, uint code)
{
    if (id == FREERIDE_TRACK)
    {
        if (code != (uint)-1)
        {
            track = code;
            switch (code)
            {
            case MAINMENU_FREEITALY:
                SetTextID(FREERIDE_TRACK, TEXT_CITY_SMALL);
                SetString(FREERIDE_PICTURE, (uchar *)"freeride0.bmp");
                break;
            case MAINMENU_FREERIDE:
                SetTextID(FREERIDE_TRACK, TEXT_CITY_DAYTIME);
                // STRING: GAME 0x63827c
                SetString(FREERIDE_PICTURE, (uchar *)"freeride1.bmp");
                break;
            case MAINMENU_FREERIDENOC:
                SetTextID(FREERIDE_TRACK, TEXT_CITY_NIGHT);
                // STRING: GAME 0x63826c
                SetString(FREERIDE_PICTURE, (uchar *)"freeride2.bmp");
                break;
            case MAINMENU_FREEKRAJINA:
                SetTextID(FREERIDE_TRACK, TEXT_COUNTRYSIDE_DAYTIME);
                // STRING: GAME 0x63825c
                SetString(FREERIDE_PICTURE, (uchar *)"freeride3.bmp");
                break;
            case MAINMENU_FREEKRAJINANOC:
                SetTextID(FREERIDE_TRACK, TEXT_COUNTRYSIDE_NIGHT);
                // STRING: GAME 0x63824c
                SetString(FREERIDE_PICTURE, (uchar *)"freeride4.bmp");
                break;
            }
            SetDisabled(FREERIDE_BACK, true);
        }
    }
    else if (id == FREERIDE_CAR && code != (uint)-1)
    {
        if ((car & CAR_INDEX_MASK) > 0 && (car & CAR_INDEX_MASK) < mafiaCarDatabase.num_cars)
            SetText(FREERIDE_CAR, (uchar *)mafiaCarDatabase.GetCarName(car));
        else
            SetTextID(FREERIDE_CAR, TEXT_NOT_SELECTED);
    }
    return 0;
}

// The track and the car open their menus; OK stores the choices (the check boxes' flags, the densities) and ends the
// main menu with the small city's track, the start at location 1, or else opens the start's menu, which ends it with
// the track. PS2's toggles between its two tracks and ends the main menu on OK.
// FUNCTION: GAME 0x5e1b40
int GM_FreeRide::OnClick(uint id)
{
    switch (id)
    {
    case FREERIDE_TRACK:
        MenuOpen(new GM_FreeRideTrack, id);
        break;
    case FREERIDE_CAR:
        MenuOpen(new GM_SelectCarManufacturer(&car, mafiaLoadSave.RaceCarsGetEnabled()), id);
        break;
    case FREERIDE_BACK:
        MenuClose((uint)-1);
        break;
    case FREERIDE_OK:
        {
            // A local, read before the calls; the start's menu gets the member, read again
            uint chosen = track;
            uint freeride_flags = 0;
            if (GetBool(FREERIDE_PAULIE))
                freeride_flags |= FREERIDE_WINGMAN_PAULIE;
            if (GetBool(FREERIDE_SAM))
                freeride_flags |= FREERIDE_WINGMAN_SAM;
            if (GetBool(FREERIDE_ROOF))
                freeride_flags |= FREERIDE_ROOF_OFF;
            mafiaLoadSave.FreerideStoreA(car, freeride_flags, GetFloat(FREERIDE_TRAFFIC),
                GetFloat(FREERIDE_PEDESTRIANS), GetFloat(FREERIDE_POLICE));
            if (chosen != MAINMENU_FREEITALY)
                MenuOpen(new GM_FreeRideStart(track), id);
            else
            {
                mafiaLoadSave.FreerideStoreB(1);
                MenuCloseAll((uint)-1, MAINMENU_FREEITALY);
            }
        }
        break;
    }
    return 0;
}

// FUNCTION: GAME 0x5e1d90
inline const char *GM_FreeRideTrack::OnBeforeCreate()
{
    // STRING: GAME 0x63828c
    return "freeridetrack";
}

// FUNCTION: GAME 0x5e1da0
inline const char *GM_FreeRideStart::OnBeforeCreate()
{
    // STRING: GAME 0x63829c
    return "freeridestart";
}

// Without the free ride under it
// FUNCTION: GAME 0x5e1db0
inline const char *GM_SelectCarManufacturer::OnBeforeCreate()
{
    flags &= ~MENUF_DRAW_PARENT;
    // STRING: GAME 0x6382ac
    return "selectcarmanufacturer";
}

// The small city's picture, and the tracks the free ride hasn't opened disabled: with FREERIDE_SMALL_CITY all but the
// small city, with FREERIDE_CITY the city's and the countryside's daytime ones (so the nights open first)
// FUNCTION: GAME 0x5e1dd0
int GM_FreeRideTrack::OnCreate()
{
    SetString(FREERIDETRACK_PICTURE, (uchar *)"freeride0.bmp");
    switch (mafiaLoadSave.freeride_enabled)
    {
    case FREERIDE_SMALL_CITY:
        SetDisabled(FREERIDETRACK_CITY_NIGHT, false);
        SetDisabled(FREERIDETRACK_COUNTRYSIDE_NIGHT, false);
    case FREERIDE_CITY:
        SetDisabled(FREERIDETRACK_CITY_DAYTIME, false);
        SetDisabled(FREERIDETRACK_COUNTRYSIDE_DAYTIME, false);
        break;
    }
    return 0;
}

// FUNCTION: GAME 0x5e1e30
int GM_FreeRideTrack::OnChange(GM_Component *active, GM_Component *)
{
    if (active && active->id > FREERIDETRACK_BACK)
    {
        switch (active->id)
        {
        case FREERIDETRACK_CITY_SMALL:
            SetString(FREERIDETRACK_PICTURE, (uchar *)"freeride0.bmp");
            break;
        case FREERIDETRACK_CITY_NIGHT:
            SetString(FREERIDETRACK_PICTURE, (uchar *)"freeride2.bmp");
            break;
        case FREERIDETRACK_COUNTRYSIDE_NIGHT:
            SetString(FREERIDETRACK_PICTURE, (uchar *)"freeride4.bmp");
            break;
        case FREERIDETRACK_CITY_DAYTIME:
            SetString(FREERIDETRACK_PICTURE, (uchar *)"freeride1.bmp");
            break;
        case FREERIDETRACK_COUNTRYSIDE_DAYTIME:
            SetString(FREERIDETRACK_PICTURE, (uchar *)"freeride3.bmp");
            break;
        }
    }
    return 0;
}

// FUNCTION: GAME 0x5e1ec0
int GM_FreeRideTrack::OnClick(uint id)
{
    switch (id)
    {
    case FREERIDETRACK_BACK:
        MenuClose((uint)-1);
        break;
    case FREERIDETRACK_CITY_SMALL:
        MenuClose(MAINMENU_FREEITALY);
        break;
    case FREERIDETRACK_CITY_NIGHT:
        MenuClose(MAINMENU_FREERIDENOC);
        break;
    case FREERIDETRACK_COUNTRYSIDE_NIGHT:
        MenuClose(MAINMENU_FREEKRAJINANOC);
        break;
    case FREERIDETRACK_CITY_DAYTIME:
        MenuClose(MAINMENU_FREERIDE);
        break;
    case FREERIDETRACK_COUNTRYSIDE_DAYTIME:
        MenuClose(MAINMENU_FREEKRAJINA);
        break;
    }
    return 0;
}

// Without the free ride under it; the countryside's tracks name the locations otherwise
// FUNCTION: GAME 0x5e1f30
int GM_FreeRideStart::OnCreate()
{
    flags &= ~MENUF_DRAW_PARENT;
    if (track >= MAINMENU_FREEKRAJINA && track <= MAINMENU_FREEKRAJINANOC)
    {
        SetTextID(FREERIDESTART_LITTLE_ITALY, TEXT_CHINA_TOWN_EXIT);
        SetTextID(FREERIDESTART_CENTRAL_ISLAND, TEXT_DAM);
        SetTextID(FREERIDESTART_DOWN_TOWN, TEXT_LAY_BY);
        SetTextID(FREERIDESTART_HOBOKEN, TEXT_MOTEL);
    }
    return 0;
}

// A location's item stores it and ends the main menu with the track
// FUNCTION: GAME 0x5e1f90
int GM_FreeRideStart::OnClick(uint id)
{
    switch (id)
    {
    case FREERIDESTART_BACK:
        MenuClose((uint)-1);
        break;
    case FREERIDESTART_LOCATIONS + 1:
    case FREERIDESTART_LOCATIONS + 2:
    case FREERIDESTART_LOCATIONS + 3:
    case FREERIDESTART_LOCATIONS + 4:
    case FREERIDESTART_LOCATIONS + 5:
    case FREERIDESTART_LOCATIONS + 6:
    case FREERIDESTART_LOCATIONS + 7:
        mafiaLoadSave.FreerideStoreB(id - FREERIDESTART_LOCATIONS);
        MenuCloseAll((uint)-1, track);
        break;
    }
    return 0;
}

// The simple message (the gun shop's), the message box's code, without a marker for its OnKey and OnClick
// FUNCTION: GAME 0x5e1fe0
GM_SimpleMessage::GM_SimpleMessage(char *text, char *text2) : text(text)
{
    this->text2 = text2;
}

// FUNCTION: GAME 0x5e2010
const char *GM_SimpleMessage::OnBeforeCreate()
{
    // STRING: GAME 0x6382c4
    return "simplemessage";
}

// FUNCTION: GAME 0x5e2020
GM_SimpleMessage::GM_SimpleMessage(uint text_id, uint text2_id)
{
    text = textDatabase.GetText(text_id);
    text2 = textDatabase.GetTextOrNULL(text2_id);
}

// The texts, then the window, the first text and the second widened by what the first text is wider than its item, the
// window and the second text kept centered
// FUNCTION: GAME 0x5e2090
int GM_SimpleMessage::OnCreate()
{
    SetText(SIMPLEMESSAGE_TEXT, (uchar *)text);
    if (text2)
        SetText(SIMPLEMESSAGE_TEXT2, (uchar *)text2);
    GM_Component *message = FindComponentByID(SIMPLEMESSAGE_TEXT);
    if (message)
    {
        float grow = mafiaIndicators.TextSize((char *)message->text, message->sy, message->flags & COMPF_FONT, NULL) -
            message->sx;
        if (grow > 0.0f)
        {
            GM_Component *component = FindComponentByID(SIMPLEMESSAGE_WINDOW);
            if (component)
            {
                component->sx += grow;
                component->x -= grow * .5f;
            }
            component = FindComponentByID(SIMPLEMESSAGE_TEXT);
            if (component)
                component->sx += grow;
            component = FindComponentByID(SIMPLEMESSAGE_TEXT2);
            if (component)
                component->x += grow * .5f;
        }
    }
    return 0;
}

// Escape or Enter closes it, and the menus under it, as the message box: out of line, without a marker, its copy folded
// with the message box's (0x5e22d0), which the linker kept
int GM_SimpleMessage::OnKey(uchar key, uint)
{
    if (key == DIK_ESCAPE || key == DIK_RETURN)
        MenuCloseAll((uint)-1, (uint)-1);
    return 0;
}

// Any item's click closes it as the keys do; folded likewise (0x5e22f0)
int GM_SimpleMessage::OnClick(uint id)
{
    if (id)
        MenuCloseAll((uint)-1, (uint)-1);
    return 0;
}

// FUNCTION: GAME 0x5e2170
const char *GM_MessageBox::OnBeforeCreate()
{
    // STRING: GAME 0x6382d4
    return "messagebox";
}

// FUNCTION: GAME 0x5e2180
GM_MessageBox::GM_MessageBox(uint text_id, uint title_id)
{
    text = textDatabase.GetText(text_id);
    title = textDatabase.GetTextOrNULL(title_id);
}

// The texts, then the window, the message and OK widened by what the message is wider than its item, the window and OK
// kept centered
// FUNCTION: GAME 0x5e21f0
int GM_MessageBox::OnCreate()
{
    SetText(MESSAGEBOX_TEXT, (uchar *)text);
    if (title)
        SetText(MESSAGEBOX_WINDOW, (uchar *)title);
    GM_Component *message = FindComponentByID(MESSAGEBOX_TEXT);
    if (message)
    {
        float grow = mafiaIndicators.TextSize((char *)message->text, message->sy, message->flags & COMPF_FONT, NULL) -
            message->sx;
        if (grow > 0.0f)
        {
            GM_Component *component = FindComponentByID(MESSAGEBOX_WINDOW);
            if (component)
            {
                component->sx += grow;
                component->x -= grow * .5f;
            }
            component = FindComponentByID(MESSAGEBOX_TEXT);
            if (component)
                component->sx += grow;
            component = FindComponentByID(MESSAGEBOX_OK);
            if (component)
                component->x += grow * .5f;
        }
    }
    return 0;
}

// Escape or Enter closes it, and the menus under it. Folded with the simple message's: its marker is in the header, by
// name
int GM_MessageBox::OnKey(uchar key, uint)
{
    if (key == DIK_ESCAPE || key == DIK_RETURN)
        MenuCloseAll((uint)-1, (uint)-1);
    return 0;
}

// Any item's click (OK's) closes it as the keys do. Folded likewise: its marker is in the header, by name
int GM_MessageBox::OnClick(uint id)
{
    if (id)
        MenuCloseAll((uint)-1, (uint)-1);
    return 0;
}

// FUNCTION: GAME 0x5e2310
GM_QuestionBox::GM_QuestionBox(char *title, char *text) : title(title)
{
    this->text = text;
}

// FUNCTION: GAME 0x5e2340
const char *GM_QuestionBox::OnBeforeCreate()
{
    // STRING: GAME 0x6382e0
    return "questionbox";
}

// The title isn't tested for NULL, the question is
// FUNCTION: GAME 0x5e2350
int GM_QuestionBox::OnCreate()
{
    SetText(QUESTIONBOX_WINDOW, (uchar *)title);
    if (text)
        SetText(QUESTIONBOX_TEXT, (uchar *)text);
    return 0;
}

// FUNCTION: GAME 0x5e2380
int GM_QuestionBox::OnKey(uchar key, uint)
{
    if (key == DIK_ESCAPE)
    {
        if (open_id)
            MenuClose((uint)-1);
        else
            MenuCloseAll((uint)-1, QUESTIONBOX_RESULT_NO);
    }
    return 0;
}

// FUNCTION: GAME 0x5e23b0
int GM_QuestionBox::OnClick(uint id)
{
    switch (id)
    {
    case QUESTIONBOX_YES:
        if (open_id)
            MenuClose(QUESTIONBOX_CODE_YES);
        else
            MenuCloseAll(QUESTIONBOX_CODE_YES, QUESTIONBOX_RESULT_YES);
        break;
    case QUESTIONBOX_NO:
        if (open_id)
            MenuClose(QUESTIONBOX_CODE_NO);
        else
            MenuCloseAll(QUESTIONBOX_CODE_NO, QUESTIONBOX_RESULT_NO);
        break;
    }
    return 0;
}

// The profile's saved games into the list, the newest (the list's last) first, each item a copy of the one before it,
// the scroll window's range to the last item; or the first item hidden without any. The picture's head is loaded a tick
// later (OnTick). PS2's also preloads the pictures and shows the first one.
// FUNCTION: GAME 0x5e2400
int GM_LoadGame::OnCreate()
{
    uint id = LOADGAME_ITEM;
    std::vector<G_SaveGame_FileHead> *saves = mafiaLoadSave.SaveGameEnumFiles();
    sprintf(total_text, textDatabase.GetText(TEXT_TOTAL_SAVED_GAMES), saves->size());
    SetText(LOADGAME_TOTAL, (uchar *)total_text);
    count = saves->size();
    if (saves->size())
    {
        std::vector<G_SaveGame_FileHead>::iterator it = saves->end();
        SetTextID(id, (it - 1)->mission_name_id);
        for (it--; saves->begin() < it; it--)
        {
            PridejComponentPod(id, id + 1, 0, 0, 0, 0);
            id++;
            SetTextID(id, (it - 1)->mission_name_id);
        }
        GM_Component *last = FindComponentByID(id);
        GM_Component *list = FindComponentByID(LOADGAME_LIST);
        if (last && list)
            list->SetRangeFloat(0.0f, last->rel_y + last->sy - list->sy + 4.0f);
    }
    else
        SetHidden(LOADGAME_ITEM, false);
    head_delay = 1;
    // STRING: GAME 0x6382ec
    SetString(LOADGAME_PICTURE, (uchar *)"shot999.bmp");
    ActivateByID(LOADGAME_ITEM);
    return 0;
}

// Once the delay has run out, the active saved game's head: its picture and the line under the list (the health, when
// it was saved and the play time)
// FUNCTION: GAME 0x5e2570
int GM_LoadGame::OnTick(uint time)
{
    if (head_delay && !--head_delay && active && active->id >= LOADGAME_ITEM && active->id < LOADGAME_ITEM_HEAD_END)
    {
        mafiaLoadSave.SaveGameLoadHeader(count - (active->id - LOADGAME_ITEM) - 1, &head, &info);
        char picture[0x100];
        // STRING: GAME 0x6382f8
        sprintf(picture, "shot%03d.bmp", mafiaLoadSave.GetScreenShotIdx(info.number));
        SetString(LOADGAME_PICTURE, (uchar *)picture);
        sprintf(info_text, textDatabase.GetText(TEXT_SAVED_GAME_INFO), info.health, info.date & 0xff,
            (info.date >> 8) & 0xff, info.date >> 16, (info.time >> 16) & 0xff, (info.time >> 8) & 0xff,
            info.play_time / 1000 / 60 / 60, info.play_time / 1000 / 60 % 60, info.play_time / 1000 % 60);
        SetText(LOADGAME_INFO, (uchar *)info_text);
    }
    return 0;
}

// Escape closes the menu; Page Down and Page Up move the activation a page through the list, to its last or first item
// at its end (PS2's the same, without the mouse)
// FUNCTION: GAME 0x5e26b0
int GM_LoadGame::OnKey(uchar key, uint)
{
    switch (key)
    {
    case DIK_ESCAPE:
        MenuClose((uint)-1);
        break;
    case DIK_NEXT:
        if (active && active->id >= LOADGAME_ITEM && active->id < LOADGAME_ITEM_END)
        {
            ActivateByIDOrPrev(active->id + LOADGAME_PAGE);
            mouseInUse = false;
        }
        break;
    case DIK_PRIOR:
        if (active && active->id >= LOADGAME_ITEM && active->id < LOADGAME_ITEM_END)
        {
            if (active->id > LOADGAME_ITEM + LOADGAME_PAGE)
                ActivateByID(active->id - LOADGAME_PAGE);
            else
                ActivateByID(LOADGAME_ITEM);
            mouseInUse = false;
        }
        break;
    }
    return 0;
}

// The wheel over the list scrolls it, within its range
// FUNCTION: GAME 0x5e2750
int GM_LoadGame::OnMouse(int x, int y, int z, uint, uint)
{
    if (z)
    {
        float min, max;
        GM_Component *list = FindComponentByID(LOADGAME_LIST);
        if (list && list->IsOverClientArea((float)x, (float)y))
        {
            min = 0.0f;
            max = 0.0f;
            float pos = list->GetFloat();
            list->GetRangeFloat(min, max);
            pos -= z * menuScreenScale.y;
            if (pos < min)
                pos = min;
            if (pos > max)
                pos = max;
            list->SetFloat(pos);
        }
    }
    return 0;
}

// A saved game's item became active: its head is loaded a few ticks later (OnTick)
// FUNCTION: GAME 0x5e2810
int GM_LoadGame::OnChange(GM_Component *active, GM_Component *)
{
    if (active && active->id >= LOADGAME_ITEM && active->id < LOADGAME_ITEM_HEAD_END)
        head_delay = 4;
    return 0;
}

// A saved game's item selects it and ends the menus with MAINMENU_LOAD_GAME
// FUNCTION: GAME 0x5e2840
int GM_LoadGame::OnClick(uint id)
{
    if (id >= LOADGAME_ITEM && id < LOADGAME_ITEM_END)
    {
        mafiaLoadSave.SaveGameSelect(count - (id - LOADGAME_ITEM) - 1);
        MenuCloseAll((uint)-1, MAINMENU_LOAD_GAME);
    }
    return 0;
}

// The failure's text, the items below it moved down and the window grown by its height (kept centered); in the free
// ride Retry disabled and Exit to Main Menu active
// FUNCTION: GAME 0x5e2880
int GM_GameFailed::OnCreate()
{
    char *text = textDatabase.GetTextOrNULL(text_id);
    float height = 0.0f;
    SetText(GAMEFAILED_TEXT, (uchar *)text);
    GM_Component *component = FindComponentByID(GAMEFAILED_TEXT);
    if (component)
        height = component->GetFloat();
    component = FindComponentByID(GAMEFAILED_RETRY);
    if (component)
        component->rel_y += height;
    component = FindComponentByID(GAMEFAILED_EXIT);
    if (component)
        component->rel_y += height;
    component = FindComponentByID(GAMEFAILED_WINDOW);
    if (component)
    {
        component->sy += height;
        component->y -= height * .5f;
    }
    if (mafiaLoadSave.FreerideGetState())
    {
        SetDisabled(GAMEFAILED_RETRY, false);
        ActivateByID(GAMEFAILED_EXIT);
    }
    return 0;
}

// FUNCTION: GAME 0x5e2950
int GM_GameFailed::OnClick(uint id)
{
    switch (id)
    {
    case GAMEFAILED_EXIT:
        MenuCloseAll((uint)-1, GAMEFAILED_RESULT_EXIT);
        break;
    case GAMEFAILED_RETRY:
        MenuCloseAll((uint)-1, GAMEFAILED_RESULT_RETRY);
        break;
    }
    return 0;
}

// The item's gameItemTable flags of mask, none for an id past the table's end; and whether it is ITEMF_NO_MERGE, or no
// ITEMF_WEAPON, 0 for an id past the table's end (the names are ours; g_loadsave.cpp's G_Inventory functions have
// GetItemFlags and the like, static there): the inventory's SetItem keeps their results in a temporary, which the
// two-return spelling of GetItemFlags doesn't
static inline uint GetItemFlags(const S_GameItem *item, uint mask)
{
    if (item->id < numOfGameItems)
        return gameItemTable[item->id].flags & mask;
    return 0;
}
static inline int IsNoMerge(const S_GameItem *item)
{
    int r;
    if (item->id < numOfGameItems)
        r = gameItemTable[item->id].flags & ITEMF_NO_MERGE;
    else
        r = 0;
    return r;
}
static inline int IsNotWeapon(const S_GameItem *item)
{
    int r;
    if (item->id < numOfGameItems)
        r = !(gameItemTable[item->id].flags & ITEMF_WEAPON);
    else
        r = 0;
    return r;
}

// The item's name in the component of that id, a weapon's with its ammunition in it and in reserve, the component
// enabled; "< empty >" for no item, disabled. The second parameter is unused: the place's number, 1 for the hand, 3 to
// 7 for the pockets, 8 for the coat and 9 for the car (the name is ours, PC only)
// FUNCTION: GAME 0x5e2990
void GM_Inventory::SetItem(uint id, uint, S_GameItem *item)
{
    char text[0x100];
    if (item->id && item->id < numOfGameItems)
    {
        if (IsNoMerge(item) || IsNotWeapon(item))
            sprintf(text, "%s", textDatabase.GetText(item->id + TEXT_ITEM_NAMES));
        else
            // STRING: GAME 0x638308
            sprintf(text, "%s (%d/%d)", textDatabase.GetText(item->id + TEXT_ITEM_NAMES), item->ammo_loaded,
                item->ammo_spare);
        SetDisabled(id, true);
    }
    else
    {
        sprintf(text, "%s", textDatabase.GetText(TEXT_EMPTY));
        SetDisabled(id, false);
    }
    SetText(id, (uchar *)text);
}

// FUNCTION: GAME 0x5e2a90
GM_Inventory::GM_Inventory(G_Inventory *inventory, std::vector<S_GameItem> *dropped) : inventory(inventory),
    dropped(dropped)
{
}

// FUNCTION: GAME 0x5e2ac0
const char *GM_Inventory::OnBeforeCreate()
{
    return inventory->in_car ?
        // STRING: GAME 0x638320
        "inventorycar" :
        // STRING: GAME 0x638314
        "inventory";
}

// The places' items, then the items', each a copy of the one before it, the ones that can't be in the hand disabled
// (or the first disabled without any), the window active. While it is open, ExecuteMenu closes it on the inventory's
// control.
// FUNCTION: GAME 0x5e2ae0
int GM_Inventory::OnCreate()
{
    flags |= MENUF_INVENTORY_CLOSES;
    SetItem(INVENTORYMENU_HAND, 1, &inventory->right_hand);
    for (int i = 0; i < INVENTORY_POCKETS; i++)
        SetItem(INVENTORYMENU_POCKET + i, i + 3, &inventory->pockets[i]);
    SetItem(INVENTORYMENU_COAT, 8, &inventory->big_weapon);
    SetItem(INVENTORYMENU_CAR, 9, &inventory->car_item);
    if (inventory->num_items)
    {
        uint id = INVENTORYMENU_ITEM;
        SetText(id, (uchar *)textDatabase.GetText(inventory->items[0].id + TEXT_ITEM_NAMES));
        if (GetItemFlags(&inventory->items[0], ITEMF_NOT_IN_HAND))
            SetDisabled(id, false);
        else
            SetDisabled(id, true);
        for (uint j = 1; j < inventory->num_items; j++)
        {
            PridejComponentPod(id, id + 1, INVENTORYMENU_WINDOW, INVENTORYMENU_HELP, 0, 0);
            id++;
            SetText(id, (uchar *)textDatabase.GetText(inventory->items[j].id + TEXT_ITEM_NAMES));
            if (GetItemFlags(&inventory->items[j], ITEMF_NOT_IN_HAND))
                SetDisabled(id, false);
            else
                SetDisabled(id, true);
        }
    }
    else
        SetDisabled(INVENTORYMENU_ITEM, false);
    ActivateByIDOrNext(INVENTORYMENU_WINDOW);
    return 0;
}

// A child that closes with a code from GAMEITEM_FIRST, an item's id, gives the inventory a new item of it, its weapon
// loaded and the rest of its most ammunition in reserve, and closes the inventory. The inventory opens no child, and an
// Escape's -1 would pass the (unsigned) test.
// FUNCTION: GAME 0x5e2cc0
int GM_Inventory::OnCloseChild(GM_Menu *child, uint id, uint code)
{
    if (code >= GAMEITEM_FIRST)
    {
        S_GameItem item;
        item.id = code;
        S_item_def &def = gameItemTable[code];
        item.ammo_loaded = def.ammo_max;
        item.ammo_spare = def.spare_max - item.ammo_loaded;
        inventory->Insert(&item, false);
        MenuClose((uint)-1);
    }
    return 0;
}

// A or S clicks the active place or item (Enter clicks it through the component), Backspace or X drops its object
// (G_Inventory::Remove) and closes the menus, Escape or I closes them
// FUNCTION: GAME 0x5e2d30
int GM_Inventory::OnKey(uchar key, uint)
{
    switch (key)
    {
    case DIK_A:
    case DIK_S:
        if (active && active->id >= INVENTORYMENU_HAND && active->id < INVENTORYMENU_ITEMS_END)
            OnClick(active->id);
        break;
    case DIK_BACK:
    case DIK_X:
        if (active)
        {
            if (active->id >= INVENTORYMENU_HAND && active->id < INVENTORYMENU_PLACES_END)
            {
                switch (active->id - INVENTORYMENU_HAND)
                {
                case 0:
                    inventory->Remove(&inventory->right_hand, dropped);
                    MenuCloseAll((uint)-1, (uint)-1);
                    break;
                case INVENTORYMENU_COAT - INVENTORYMENU_HAND:
                    inventory->Remove(&inventory->big_weapon, dropped);
                    MenuCloseAll((uint)-1, (uint)-1);
                    break;
                default:
                    inventory->Remove(&inventory->pockets[active->id - INVENTORYMENU_POCKET], dropped);
                    MenuCloseAll((uint)-1, (uint)-1);
                }
            }
            if (active->id >= INVENTORYMENU_ITEM && active->id < INVENTORYMENU_ITEMS_END)
            {
                inventory->Remove(&inventory->items[active->id - INVENTORYMENU_ITEM], dropped);
                MenuCloseAll((uint)-1, (uint)-1);
            }
        }
        break;
    case DIK_ESCAPE:
    case DIK_I:
        MenuCloseAll((uint)-1, (uint)-1);
        break;
    }
    return 0;
}

// A place's or an item's click takes its object into the hand (G_Inventory::Select) and closes the menus
// FUNCTION: GAME 0x5e2e70
int GM_Inventory::OnClick(uint id)
{
    if (id & INVENTORYMENU_ITEMS_MASK)
    {
        if (id >= INVENTORYMENU_HAND && id < INVENTORYMENU_PLACES_END)
        {
            switch (id - INVENTORYMENU_HAND)
            {
            case 0:
                inventory->Select(0, INVENTORY_HAND, dropped);
                break;
            case INVENTORYMENU_COAT - INVENTORYMENU_HAND:
                inventory->Select(0, INVENTORY_BIG_WEAPON, dropped);
                break;
            case INVENTORYMENU_CAR - INVENTORYMENU_HAND:
                inventory->Select(0, INVENTORY_CAR, dropped);
                break;
            default:
                inventory->Select(id - INVENTORYMENU_POCKET, INVENTORY_POCKET, dropped);
            }
            MenuCloseAll((uint)-1, (uint)-1);
        }
        if (id >= INVENTORYMENU_ITEM && id < INVENTORYMENU_ITEMS_END)
        {
            inventory->Select(id - INVENTORYMENU_ITEM, INVENTORY_ITEM, dropped);
            MenuCloseAll((uint)-1, (uint)-1);
        }
    }
    return 0;
}

// The items into menu_items: items' own, then those each inventory (a body's) gives to take (EnemyToList). A single
// one is taken at once, an action chosen, unless the cheats have GAMEF_NO_AUTO_PICKUP; the menu is built all the same.
// Each item's text is its name, an action's its text, the first nine numbered for the keys DIK_1 to DIK_9, the others
// indented, up to ITEMPICKUP_MAX_ITEMS of them, then "0. Exit"; without items the menu closes with PICKUP_EXIT. PS2's
// loads the menus' resources for two items or more, takes the single item without the console's text and has one
// format for all the texts.
// FUNCTION: GAME 0x5e2f20
int GM_ItemPickUp::OnCreate()
{
    S_GameItem *item;
    if (items->size())
        for (item = items->begin(); item < items->end(); item++)
            menu_items.push_back(item);
    std::vector<S_GameItem *> list;
    if (inventories->size())
        for (G_Inventory **inv = inventories->begin(); inv < inventories->end(); inv++)
        {
            list.clear();
            (*inv)->EnemyToList(&list);
            for (S_GameItem **it = list.begin(); it < list.end(); it++)
                menu_items.push_back(*it);
        }
    if (menu_items.size() == 1 && !(bool)(act_mission->game->cheats & GAMEF_NO_AUTO_PICKUP))
    {
        if (menu_items[0]->id == PICKUP_TEXT)
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST);
        else
        {
            if (inventory->InsertNew(menu_items[0], dropped))
            {
                mafiaIndicators.ConsoleAddTextByID(TEXT_ITEM_COLLECTED + menu_items[0]->id, 0xffffff);
                PickUp(0, false);
            }
            else
            {
                mafiaIndicators.ConsoleAddTextByID(TEXT_ITEM_COLLECTED + menu_items[0]->id, 0xffffff);
                PickUp(0, true);
            }
            MenuCloseAll((uint)-1, PICKUP_TAKE);
        }
    }
    if (menu_items.size())
    {
        char text[256];
        S_GameItem **it = menu_items.begin();
        uint id = ITEMPICKUP_LIST;
        int item_id = (*it)->id;
        if (item_id == PICKUP_TEXT)
            // STRING: GAME 0x638340
            sprintf(text, "%d. %s", 1, textDatabase.GetText((*it)->ammo_loaded));
        else
            sprintf(text, "%d. %s", 1, textDatabase.GetText(TEXT_ITEM_NAMES + item_id));
        SetText(ITEMPICKUP_LIST, (uchar *)text);
        int count = 1;
        for (it++; it < menu_items.end(); it++)
        {
            PridejComponentPod(id, id + 1, ITEMPICKUP_WINDOW, 0, 0, 0);
            id++;
            item_id = (*it)->id;
            if (count < 9)
            {
                if (item_id == PICKUP_TEXT)
                    sprintf(text, "%d. %s", (count + 1) % 10, textDatabase.GetText((*it)->ammo_loaded));
                else
                    sprintf(text, "%d. %s", (count + 1) % 10, textDatabase.GetText(TEXT_ITEM_NAMES + item_id));
            }
            else
            {
                if (item_id == PICKUP_TEXT)
                    // STRING: GAME 0x638338
                    sprintf(text, "  %s", textDatabase.GetText((*it)->ammo_loaded));
                else
                    sprintf(text, "  %s", textDatabase.GetText(TEXT_ITEM_NAMES + item_id));
            }
            SetText(id, (uchar *)text);
            if (++count > ITEMPICKUP_MAX_ITEMS - 1)
                break;
        }
        PridejComponentPod(id, ITEMPICKUP_EXIT, ITEMPICKUP_WINDOW, 0, 0, 0);
        // STRING: GAME 0x638330
        sprintf(text, "0. %s", textDatabase.GetText(TEXT_EXIT));
        SetText(ITEMPICKUP_EXIT, (uchar *)text);
    }
    else
    {
        SetDisabled(ITEMPICKUP_LIST, false);
        MenuCloseAll((uint)-1, PICKUP_EXIT);
    }
    UtilCenterV(ITEMPICKUP_WINDOW);
    return 0;
}

// The keys 1 to 9 end the menu with their item's result (PICKUP_ITEM_FIRST and its index), as OnClick does an
// action's but without taking an item; Escape and 0 (Exit's) with PICKUP_EXIT
// FUNCTION: GAME 0x5e32c0
int GM_ItemPickUp::OnKey(uchar key, uint)
{
    switch (key)
    {
    case DIK_1:
        if (FindComponentByID(ITEMPICKUP_LIST))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST);
        break;
    case DIK_2:
        if (FindComponentByID(ITEMPICKUP_LIST + 1))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST + 1);
        break;
    case DIK_3:
        if (FindComponentByID(ITEMPICKUP_LIST + 2))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST + 2);
        break;
    case DIK_4:
        if (FindComponentByID(ITEMPICKUP_LIST + 3))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST + 3);
        break;
    case DIK_5:
        if (FindComponentByID(ITEMPICKUP_LIST + 4))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST + 4);
        break;
    case DIK_6:
        if (FindComponentByID(ITEMPICKUP_LIST + 5))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST + 5);
        break;
    case DIK_7:
        if (FindComponentByID(ITEMPICKUP_LIST + 6))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST + 6);
        break;
    case DIK_8:
        if (FindComponentByID(ITEMPICKUP_LIST + 7))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST + 7);
        break;
    case DIK_9:
        if (FindComponentByID(ITEMPICKUP_LIST + 8))
            MenuCloseAll((uint)-1, PICKUP_ITEM_FIRST + 8);
        break;
    case DIK_ESCAPE:
    case DIK_0:
        MenuCloseAll((uint)-1, PICKUP_EXIT);
        break;
    }
    return 0;
}

// Exit closes the menu with PICKUP_EXIT; an item's click ends it with its result where it is an action, and else
// puts the item into the inventory (the console tells it) and takes it, dropped where the inventory had no place for
// it, and ends the menu with PICKUP_TAKE
// FUNCTION: GAME 0x5e3480
int GM_ItemPickUp::OnClick(uint id)
{
    switch (id)
    {
    case ITEMPICKUP_EXIT:
        MenuCloseAll((uint)-1, PICKUP_EXIT);
        return 1;
    }
    if (id >= ITEMPICKUP_LIST && id < ITEMPICKUP_LIST_END)
    {
        if (menu_items[id - ITEMPICKUP_LIST]->id == PICKUP_TEXT)
            MenuCloseAll((uint)-1, id - ITEMPICKUP_LIST + PICKUP_ITEM_FIRST);
        else
        {
            if (inventory->InsertNew(menu_items[id - ITEMPICKUP_LIST], dropped))
            {
                mafiaIndicators.ConsoleAddTextByID(TEXT_ITEM_COLLECTED + menu_items[id - ITEMPICKUP_LIST]->id,
                    0xffffff);
                PickUp(id - ITEMPICKUP_LIST, false);
            }
            else
            {
                mafiaIndicators.ConsoleAddTextByID(TEXT_ITEM_COLLECTED + menu_items[id - ITEMPICKUP_LIST]->id,
                    0xffffff);
                PickUp(id - ITEMPICKUP_LIST, true);
            }
            MenuCloseAll((uint)-1, PICKUP_TAKE);
        }
    }
    return 0;
}

// The item of menu_items at idx taken (PS2 name): one of items into picked, and into dropped too with drop (the
// inventory had no place for it); one of an inventory's (a body's) taken out of the inventories
// FUNCTION: GAME 0x5e3560
void GM_ItemPickUp::PickUp(uint idx, bool drop)
{
    S_GameItem *item = menu_items[idx];
    if (items->size())
        for (S_GameItem *it = items->begin(); it < items->end(); it++)
            if (it == item)
            {
                picked->push_back(*it);
                if (drop)
                    dropped->push_back(*it);
                return;
            }
    if (inventories->size())
        for (G_Inventory **inv = inventories->begin(); inv < inventories->end(); inv++)
            (*inv)->RemoveByPtr(item);
}

// The shop's weapons, gameItemTable's items from GAMEITEM_FIRST that aren't ITEMF_NOT_SOLD, into the list, each a copy
// of the one before it (the first disabled without any), and the inventory's places. PS2's the same.
// FUNCTION: GAME 0x5e3610
int GM_GunShop::OnCreate()
{
    uint i;
    num_weapons = 0;
    for (i = GAMEITEM_FIRST; i < numOfGameItems; i++)
        if (!(gameItemTable[i].flags & ITEMF_NOT_SOLD))
        {
            weapons[num_weapons] = i;
            num_weapons++;
        }
    if (num_weapons)
    {
        uint id = GUNSHOP_WEAPON;
        SetTextID(id, weapons[0] + TEXT_ITEM_NAMES);
        for (i = 1; i < num_weapons; i++)
        {
            PridejComponentPod(id, id + 1, GUNSHOP_WINDOW, GUNSHOP_EXIT, 0, 0);
            id++;
            SetTextID(id, weapons[i] + TEXT_ITEM_NAMES);
        }
    }
    else
        SetDisabled(GUNSHOP_WEAPON, false);
    RefreshInventory();
    return 0;
}

// A place's click takes its object away (G_Inventory::RemoveByIdx), a weapon's puts a new one into the inventory, its
// weapon and reserve full, or where it has no place, a message says why: the inventory has one, or no room. Then the
// inventory's places again; Exit closes the menus. PS2's has only Exit and the refresh.
// FUNCTION: GAME 0x5e3700
int GM_GunShop::OnClick(uint id)
{
    switch (id)
    {
    case 0:
        break;
    case GUNSHOP_EXIT:
        MenuCloseAll((uint)-1, (uint)-1);
        break;
    default:
        if (id >= GUNSHOP_HAND && id < GUNSHOP_PLACES_END)
        {
            switch (id - GUNSHOP_HAND)
            {
            case 0:
                inventory->RemoveByIdx(0, INVENTORY_HAND);
                break;
            case GUNSHOP_COAT - GUNSHOP_HAND:
                inventory->RemoveByIdx(0, INVENTORY_BIG_WEAPON);
                break;
            default:
                inventory->RemoveByIdx(id - GUNSHOP_POCKET, INVENTORY_POCKET);
            }
        }
        if (id >= GUNSHOP_ITEM && id < GUNSHOP_ITEMS_END)
            inventory->RemoveByIdx(id - GUNSHOP_ITEM, INVENTORY_ITEM);
        if (id >= GUNSHOP_WEAPON && id < GUNSHOP_EXIT)
        {
            S_GameItem item;
            item.id = weapons[id - GUNSHOP_WEAPON];
            item.ammo_loaded = gameItemTable[item.id].ammo_max;
            item.ammo_spare = gameItemTable[item.id].spare_max;
            if (!inventory->Insert(&item, false))
            {
                if (inventory->IsIn(item.id, false))
                    MenuOpen(new GM_SimpleMessage("", textDatabase.GetText(TEXT_ALREADY_HAVE_WEAPON)), id);
                else
                    MenuOpen(new GM_SimpleMessage(TEXT_NOT_ENOUGH_ROOM, TEXT_MUST_DROP_ITEM), id);
            }
        }
        RefreshInventory();
        break;
    }
    return 0;
}

// The item's name in the component of that id, a weapon's with its ammunition in it and in reserve (but an
// ITEMF_NO_MERGE one's), the component enabled; "< empty >" for no item, disabled. The second parameter is unused: the
// place's number, 1 for the hand, 2 to 6 for the pockets, 8 for the coat (PC only, the name is ours)
// FUNCTION: GAME 0x5e38e0
void GM_GunShop::SetItem(uint id, uint, S_GameItem *item)
{
    char text[0x100];
    uint item_id = item->id;
    if (item_id && item_id < numOfGameItems)
    {
        if (GetItemFlags(item, ITEMF_NO_MERGE))
            // STRING: GAME 0x638354
            sprintf(text, " %s", textDatabase.GetText(item_id + TEXT_ITEM_NAMES));
        else
            // STRING: GAME 0x638348
            sprintf(text, " %s (%d/%d)", textDatabase.GetText(item_id + TEXT_ITEM_NAMES), item->ammo_loaded,
                item->ammo_spare);
        SetDisabled(id, true);
    }
    else
    {
        sprintf(text, " %s", textDatabase.GetText(TEXT_EMPTY));
        SetDisabled(id, false);
    }
    SetText(id, (uchar *)text);
}

// The inventory's places and items into their components (PS2's does nothing): the places' with SetItem, the items' by
// their names, the menu's four (which the gun shop's menu doesn't have) first "< empty >" and disabled
// FUNCTION: GAME 0x5e39c0
void GM_GunShop::RefreshInventory()
{
    if (inventory)
    {
        SetItem(GUNSHOP_HAND, 1, &inventory->right_hand);
        int i;
        for (i = 0; i < INVENTORY_POCKETS; i++)
            SetItem(GUNSHOP_POCKET + i, i + 2, &inventory->pockets[i]);
        SetItem(GUNSHOP_COAT, 8, &inventory->big_weapon);
        for (i = 0; i < INVENTORY_ITEMS; i++)
        {
            SetTextID(GUNSHOP_ITEM + i, TEXT_EMPTY);
            SetDisabled(GUNSHOP_ITEM + i, false);
        }
        for (uint j = 0; j < inventory->num_items; j++)
        {
            SetTextID(GUNSHOP_ITEM + j, inventory->items[j].id + TEXT_ITEM_NAMES);
            SetDisabled(GUNSHOP_ITEM + j, true);
        }
    }
}

// The menus' scale, relative to 800x600 (the HUD's screen of Parhelia's two or three): the constants are quotients, the
// reciprocals of the sizes
// FUNCTION: GAME 0x5e3aa0
GM_LoadProgress::GM_LoadProgress(uint title, float mission_part, uchar *mission_name) :
    mission_part(mission_part),
    title(title),
    mission_name(mission_name)
{
    switch (parheliaIndex)
    {
    case PARHELIA_DUAL:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 1600);
        break;
    case PARHELIA_TRIPLE:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 2400);
        break;
    default:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 800);
    }
    menuScreenScale.y = igraph->Scrn_sy() * (1.0f / 600);
}

// FUNCTION: GAME 0x5e3b80
const char *GM_LoadProgress::OnBeforeCreate()
{
    // STRING: GAME 0x638358
    return "loadprogress";
}

// PS2's loads the menus' resources, takes act_mission's name into the screen (without its folders), copies the entry's
// picture into the screen, has pictures and texts of its own for three missions, and shows the hidden items. Open: the
// original loads this (mov ecx, esi) for the mission name's SetTextID before it pushes the id, ours after; neither a reference
// or local for the entry, an inline helper, other types nor other declarations moved it.
// FUNCTION: GAME 0x5e3b90
int GM_LoadProgress::OnCreate()
{
    if (title)
    {
        SetTextID(LOADPROGRESS_TITLE, title);
        SetTextID(LOADPROGRESS_ABORT, TEXT_PRESS_ESC_TO_ABORT);
    }
    // STRING: GAME 0x638384
    SetString(LOADPROGRESS_SIDE, (uchar *)"loadbok.tga");
    // STRING: GAME 0x638378
    SetString(LOADPROGRESS_LOGO, (uchar *)"mafia.tga");
    S_load_def *defs = (S_load_def *)new char[8000];
    // STRING: GAME 0x638368
    int file = dtaOpen("tables\\load.def", false);
    if (file >= 0)
    {
        int read = dtaRead(file, defs, 8000);
        if (read > 0)
        {
            int count = read / sizeof(S_load_def);
            for (int i = 0; i < count; i++)
                if (!_stricmp(defs[i].mission, mission_name ? (char *)mission_name : act_mission->mission_name))
                {
                    SetString(LOADPROGRESS_MISSION_PICTURE, (uchar *)defs[i].picture);
                    if (defs[i].name_id)
                        SetTextID(LOADPROGRESS_MISSION_NAME, defs[i].name_id);
                    break;
                }
        }
        dtaClose(file);
    }
    delete[] (char *)defs;
    SetHidden(LOADPROGRESS_UNKNOWN_1000, false);
    SetHidden(LOADPROGRESS_UNKNOWN_2000, false);
    SetHidden(LOADPROGRESS_UNKNOWN_3000, false);
    return 0;
}

// Returns 0, as PS2's: the vtable's entry is a folded copy of another file's, apart from the base's in-class one (its
// by-name marker is in g_m_func.h)
int GM_LoadProgress::OnTick(uint time)
{
    return 0;
}

// Escape ends the loading screen's menus with LOADPROGRESS_RESULT_ABORT (PS2's the same)
// FUNCTION: GAME 0x5e3cd0
int GM_LoadProgress::OnKey(uchar key, uint)
{
    if (key == DIK_ESCAPE)
        MenuCloseAll((uint)-1, LOADPROGRESS_RESULT_ABORT);
    return 0;
}

// PS2's order of the sum: the mission's part, then the game's of the rest
// FUNCTION: GAME 0x5e3cf0
void GM_LoadProgress::SetProgressGameInit(float progress)
{
    SetRawProgress((1.0f - mission_part) * progress + mission_part);
}

// FUNCTION: GAME 0x5e3d20
void GM_LoadProgress::SetProgressMissionOpen(float progress)
{
    SetRawProgress(progress * mission_part);
}

// PS2's without ResetRenderProps
// FUNCTION: GAME 0x5e3d40
void GM_LoadProgress::Present()
{
    igraph->ResetRenderProps();
    igraph->Clear(0, 1.0f, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    igraph->BeginScene();
    igraph->SetState(DXS_ALPHA, ALPHASTATE_BLEND);
    Draw(MENUDRAW_ALL);
    igraph->EndScene();
    igraph->Present();
}

// The items' list, an item for each car not discarded yet (the first one the menu's, the others its copies below it),
// each with the id of the car's index (from GARAGEENDOFMISSION_CAR): the car's name and its variant's (of the
// database's index of its actor's model where the item has no descriptor), enabled but for the mission's cars (cars),
// which can't be discarded. The window centered.
// FUNCTION: GAME 0x5e3da0
int GM_GarageEndOfMission::OnCreate()
{
    if (!items)
        return 0;
    mafiaCarDatabase.LoadDetails();
    uint count = items->size() - discard->size();
    uint id = GARAGEENDOFMISSION_ITEM;
    for (uint i = 1; i < count; i++, id++)
        PridejComponentPod(id, id + 1, GARAGEENDOFMISSION_WINDOW, 0, 0, 0);
    uint shown = 0;
    for (uint j = 0; j < items->size(); j++)
    {
        uint *d;
        for (d = discard->begin(); d < discard->end(); d++)
        {
            if (*d == j)
                break;
        }
        if (d < discard->end())
            continue;
        GM_Component *component = FindComponentByID(GARAGEENDOFMISSION_ITEM + shown);
        if (component)
            component->id = GARAGEENDOFMISSION_CAR + j;
        uint car = (*items)[j].car;
        if (!car && (*items)[j].actor)
            car = mafiaCarDatabase.GetIdxByModel((*items)[j].actor->model_name);
        char text[256];
        strcpy(text, mafiaCarDatabase.GetCarName(car));
        strcat(text, " ");
        strcat(text, textDatabase.GetText(mafiaCarDatabase.GetCarSecondName(car, 0)));
        SetText(GARAGEENDOFMISSION_CAR + j, (uchar *)text);
        SetDisabled(GARAGEENDOFMISSION_CAR + j, true);
        shown++;
        if (cars)
        {
            for (C_car **c = cars->begin(); c < cars->end(); c++)
            {
                if (*c == (*items)[j].actor)
                {
                    SetDisabled(GARAGEENDOFMISSION_CAR + j, false);
                    break;
                }
            }
        }
    }
    UtilCenterV(GARAGEENDOFMISSION_WINDOW);
    mafiaCarDatabase.UnloadDetails();
    return 0;
}

// Empty, out of line: the original's copy, between OnCreate and OnClick, is the one the linker kept of its folded
// group (C_web_path::SelectLine's)
// FUNCTION: GAME 0x5e3fd0 FOLDED
int GM_GarageEndOfMission::OnKey(uchar key, uint)
{
    return 0;
}

// A car's item adds its index to discard and ends the menu (GarageGameDone runs it again while too many cars are left)
// FUNCTION: GAME 0x5e3fe0
int GM_GarageEndOfMission::OnClick(uint id)
{
    if (id >= GARAGEENDOFMISSION_CAR && id < GARAGEENDOFMISSION_CAR_END)
    {
        discard->push_back(id - GARAGEENDOFMISSION_CAR);
        MenuCloseAll((uint)-1, GARAGEENDOFMISSION_DONE);
    }
    return 0;
}

// The items' list, an item for each car (the first one the menu's, the others its copies below it), or the menu's
// "You don't have any car." disabled: the car's name and its variant's, of the descriptor of its actor where the item
// has none, "(removed) " before it for a car in destroy; the car the player can't get out of can't be chosen. The
// window centered, Close active, or the item clicked last if it is enabled.
// FUNCTION: GAME 0x5e41b0
int GM_GarageManager::OnCreate()
{
    if (!items)
        return 0;
    mafiaCarDatabase.LoadDetails();
    if (items->size())
    {
        for (uint i = 1; i < items->size(); i++)
        {
            uint id = GARAGEMANAGER_CAR + i;
            PridejComponentPod(id - 1, id, GARAGEMANAGER_WINDOW, GARAGEMANAGER_CLOSE, 0, 0);
        }
    }
    else
    {
        SetText(GARAGEMANAGER_CAR, (uchar *)textDatabase.GetText(TEXT_GARAGE_NO_CAR));
        SetDisabled(GARAGEMANAGER_CAR, false);
    }
    uint i = 0;
    for (S_garage_item *p = items->begin(); p < items->end(); p++)
    {
        char text[256];
        text[0] = 0;
        for (uint *d = destroy->begin(); d < destroy->end(); d++)
        {
            if (*d == i)
            {
                strcat(text, textDatabase.GetText(TEXT_GARAGE_DESTROY));
                break;
            }
        }
        uint car = p->car;
        if (!car && p->actor)
            car = mafiaCarDatabase.GetIDByActor(p->actor);
        uint level;
        mafiaCarDatabase.GetCarParams(car, NULL, &level, NULL);
        strcat(text, mafiaCarDatabase.GetCarName(car));
        strcat(text, " ");
        strcat(text, textDatabase.GetText(mafiaCarDatabase.GetCarSecondName(car, 0)));
        SetText(GARAGEMANAGER_CAR + i, (uchar *)text);
        if (p->actor == cant_get_out)
            SetDisabled(GARAGEMANAGER_CAR + i, false);
        i++;
    }
    mafiaCarDatabase.UnloadDetails();
    UtilCenterV(GARAGEMANAGER_WINDOW);
    ActivateByID(GARAGEMANAGER_CLOSE);
    if (active && *active && GetDisabled(*active))
        ActivateByID(*active);
    return 0;
}

// Escape ends the menu with GARAGEMANAGER_DONE
// FUNCTION: GAME 0x5e4450
int GM_GarageManager::OnKey(uchar key, uint)
{
    switch (key)
    {
    case DIK_ESCAPE:
        MenuCloseAll((uint)-1, GARAGEMANAGER_DONE);
        break;
    }
    return 0;
}

// A car's item takes its index out of destroy, or adds it, and ends the menu with GARAGEMANAGER_AGAIN, the item active
// again when CarManager runs it again; Close ends it with GARAGEMANAGER_DONE
// FUNCTION: GAME 0x5e4470
int GM_GarageManager::OnClick(uint id)
{
    if (!id)
        return 0;
    switch (id)
    {
    case GARAGEMANAGER_CLOSE:
        MenuCloseAll((uint)-1, GARAGEMANAGER_DONE);
        break;
    default:
        if (id >= GARAGEMANAGER_CAR && id < GARAGEMANAGER_CAR_END)
        {
            uint *d;
            for (d = destroy->begin(); d < destroy->end(); d++)
            {
                if (*d == id - GARAGEMANAGER_CAR)
                    break;
            }
            if (d < destroy->end())
                destroy->erase(d);
            else
                destroy->push_back(id - GARAGEMANAGER_CAR);
            if (active)
                *active = id;
            MenuCloseAll((uint)-1, GARAGEMANAGER_AGAIN);
        }
    }
    return 0;
}

// The cars added below the first title and those destroyed below the second, each car's name and its variant's, then
// the height of item GARAGESUMMARY_UNKNOWN_2 set to that of the window (PS2's has one list, with the cars' numbers, and
// centers the window)
// FUNCTION: GAME 0x5e4690
int GM_GarageSummary::OnCreate()
{
    if (!added || !removed)
        return 0;
    mafiaCarDatabase.LoadDetails();
    char text[256];
    uint id = GARAGESUMMARY_ADDED;
    if (added->size())
    {
        SetText(id, (uchar *)textDatabase.GetText(TEXT_GARAGE_ADDED));
        for (uint i = 0; i < added->size(); i++)
        {
            PridejComponentPod(id, GARAGESUMMARY_ADDED + 1 + i, GARAGESUMMARY_WINDOW, GARAGESUMMARY_REMOVED,
                GARAGESUMMARY_UNKNOWN_3, GARAGESUMMARY_CLOSE);
            id++;
            // STRING: GAME 0x638394
            sprintf(text, "%s %s", mafiaCarDatabase.GetCarName((*added)[i]),
                textDatabase.GetText(mafiaCarDatabase.GetCarSecondName((*added)[i], 0)));
            SetText(id, (uchar *)text);
        }
    }
    id = GARAGESUMMARY_REMOVED;
    if (removed->size())
    {
        SetText(id, (uchar *)textDatabase.GetText(TEXT_GARAGE_REMOVED));
        for (uint i = 0; i < removed->size(); i++)
        {
            PridejComponentPod(id, GARAGESUMMARY_REMOVED + 1 + i, GARAGESUMMARY_WINDOW, GARAGESUMMARY_CLOSE, 0, 0);
            id++;
            sprintf(text, "%s %s", mafiaCarDatabase.GetCarName((*removed)[i]),
                textDatabase.GetText(mafiaCarDatabase.GetCarSecondName((*removed)[i], 0)));
            SetText(id, (uchar *)text);
        }
    }
    float sy = 0.0f;
    GM_Component *component = FindComponentByID(GARAGESUMMARY_WINDOW);
    if (component)
        sy = component->sy;
    component = FindComponentByID(GARAGESUMMARY_UNKNOWN_2);
    if (component)
        component->sy = sy;
    mafiaCarDatabase.UnloadDetails();
    return 0;
}

// Any key closes it
// FUNCTION: GAME 0x5e48b0
int GM_GarageSummary::OnKey(uchar key, uint)
{
    MenuClose((uint)-1);
    return 0;
}

// Out of line, without a marker: the original's entry is the copy the linker kept of a folded group (0x5e4d50, whose
// by-name marker is in g_m_func.h)
int GM_GarageSummary::OnClick(uint id)
{
    return 0;
}

// The car's manufacturer's functions, range 106's (the racing menus' file, not written yet): stubs that log and run
// the base's. Its OnCloseChild's copy is the one the linker kept of a folded group (vtable 0x6277a0's, another of range
// 106's menus, has it too).
// STUB: GAME 0x5e5300
int GM_SelectCarManufacturer::OnCreate()
{
    StubLog("GM_SelectCarManufacturer::OnCreate");
    return GM_Menu::OnCreate();
}

// STUB: GAME 0x5e5560
int GM_SelectCarManufacturer::OnClick(uint id)
{
    StubLog("GM_SelectCarManufacturer::OnClick %u", id);
    return GM_Menu::OnClick(id);
}

// STUB: GAME 0x5e8340
int GM_SelectCarManufacturer::OnCloseChild(GM_Menu *child, uint id, uint code)
{
    StubLog("GM_SelectCarManufacturer::OnCloseChild");
    return GM_Menu::OnCloseChild(child, id, code);
}

