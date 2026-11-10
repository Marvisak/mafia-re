// The main menu (PS2 g_m_func.cpp, where PS2's GM_MainMenu is; the PC's file is the one at 0x5e0640-0x5e48ec,
// docs/game-files.md), after its quit question's OnClick, the file's first function. The original's OnChange is folded
// into the racing menu's (0x5e4a50, in the next file but one). The menus it opens (the options from 0x5e0ad0, with the
// Audio & Video menu from 0x5e0bb0 and the controls from 0x5e1480, the free ride from 0x5e1920), the PC's message box
// (from 0x5e2170) and question box (from 0x5e2310), the saved games (from 0x5e2400), then the loading screen
// (GM_LoadProgress, from 0x5e3aa0), PS2's in PS2's order. Written so far: the main menu and its quit question, the
// options with the Audio & Video menu and the controls, the free ride's and the saved games' OnBeforeCreate, the
// message box's OnBeforeCreate and constructor, the question box, the loading screen's constructor, OnBeforeCreate,
// OnCreate, OnTick, SetProgressMissionOpen and Present; the rest is stubs of what main, C_mission::Open, the profiles'
// menu and the main menu need to link.

#include "ui/g_m_func.h"
#include "actors/g_inventory.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_comps.h"
#include "ui/g_textdb.h"
#include "mission/Mission.h"
#include "mission/game.h"

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

// The game's menu's items, tables\menu\gamemenu.mnu (the names are ours, after their texts)
#define GAMEMENU_WINDOW 1               // "Game menu:"
#define GAMEMENU_EXIT 0x66
#define GAMEMENU_LOAD_GAME 0x67
#define GAMEMENU_OPTIONS 0x6c
#define GAMEMENU_RESUME 0x6f

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

// The free ride's choices, "freeride" (PS2 GM_FreeRide, g_m_func.cpp), 0x38 bytes, which the main menu's Free Ride
// opens: the location, the car and the densities of the traffic, the pedestrians and the police. Its constructor is
// inline, as the main menu's.
// VTABLE: GAME 0x62736c
class GM_FreeRide : public GM_Menu
{
public:
    GM_FreeRide() { flags |= MENUF_UNKNOWN_80000; }

    virtual const char *OnBeforeCreate();
    virtual int OnCreate();
    virtual int OnCloseChild(GM_Menu *child, uint, uint);
    virtual int OnClick(uint id);

    uchar unknown_30[8];
};

// The saved games, "loadgame" (PS2 GM_LoadGame, g_m_func.cpp, whose class is 0x284 bytes, with a vector at its end the
// PC's constructor doesn't construct), 0x270 bytes, which the main menu's Load Game opens. Its constructor is inline,
// as the main menu's.
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

    uchar unknown_30[0x240];
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

// FUNCTION: GAME 0x5e08e0
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

// STUB: GAME 0x5e1920
int GM_FreeRide::OnCreate()
{
    StubLog("GM_FreeRide::OnCreate");
    return 0;
}

// STUB: GAME 0x5e1a20
int GM_FreeRide::OnCloseChild(GM_Menu *child, uint, uint)
{
    StubLog("GM_FreeRide::OnCloseChild");
    return 0;
}

// STUB: GAME 0x5e1b40
int GM_FreeRide::OnClick(uint id)
{
    StubLog("GM_FreeRide::OnClick %u", id);
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

// STUB: GAME 0x5e21f0
int GM_MessageBox::OnCreate()
{
    StubLog("GM_MessageBox::OnCreate");
    return 0;
}

// STUB: GAME 0x5e22d0
int GM_MessageBox::OnKey(uchar key, uint)
{
    StubLog("GM_MessageBox::OnKey");
    return 0;
}

// STUB: GAME 0x5e22f0
int GM_MessageBox::OnClick(uint id)
{
    StubLog("GM_MessageBox::OnClick");
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

// STUB: GAME 0x5e2400
int GM_LoadGame::OnCreate()
{
    StubLog("GM_LoadGame::OnCreate");
    return 0;
}

// STUB: GAME 0x5e2570
int GM_LoadGame::OnTick(uint time)
{
    StubLog("GM_LoadGame::OnTick");
    return 0;
}

// Escape closes the menu, as the original's does, so that a run gets back to the main menu
// STUB: GAME 0x5e26b0
int GM_LoadGame::OnKey(uchar key, uint)
{
    StubLog("GM_LoadGame::OnKey");
    if (key == DIK_ESCAPE)
        MenuClose((uint)-1);
    return 0;
}

// STUB: GAME 0x5e2750
int GM_LoadGame::OnMouse(int x, int y, int z, uint, uint)
{
    StubLog("GM_LoadGame::OnMouse");
    return 0;
}

// STUB: GAME 0x5e2810
int GM_LoadGame::OnChange(GM_Component *active, GM_Component *)
{
    StubLog("GM_LoadGame::OnChange");
    return 0;
}

// STUB: GAME 0x5e2840
int GM_LoadGame::OnClick(uint id)
{
    StubLog("GM_LoadGame::OnClick %u", id);
    return 0;
}

// STUB: GAME 0x5e2880
int GM_GameFailed::OnCreate()
{
    StubLog("GM_GameFailed::OnCreate");
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

// STUB: GAME 0x5e2ae0
int GM_Inventory::OnCreate()
{
    StubLog("GM_Inventory::OnCreate");
    return 0;
}

// STUB: GAME 0x5e2cc0
int GM_Inventory::OnCloseChild(GM_Menu *child, uint a, uint b)
{
    StubLog("GM_Inventory::OnCloseChild");
    return 0;
}

// STUB: GAME 0x5e2d30
int GM_Inventory::OnKey(uchar key, uint u)
{
    StubLog("GM_Inventory::OnKey");
    return GM_Menu::OnKey(key, u);
}

// STUB: GAME 0x5e2e70
int GM_Inventory::OnClick(uint id)
{
    StubLog("GM_Inventory::OnClick %u", id);
    return 0;
}

// STUB: GAME 0x5e2f20
int GM_ItemPickUp::OnCreate()
{
    StubLog("GM_ItemPickUp::OnCreate");
    return 0;
}

// STUB: GAME 0x5e32c0
int GM_ItemPickUp::OnKey(uchar key, uint u)
{
    StubLog("GM_ItemPickUp::OnKey");
    return GM_Menu::OnKey(key, u);
}

// STUB: GAME 0x5e3480
int GM_ItemPickUp::OnClick(uint id)
{
    StubLog("GM_ItemPickUp::OnClick");
    return 0;
}

// STUB: GAME 0x5e3610
int GM_GunShop::OnCreate()
{
    StubLog("GM_GunShop::OnCreate");
    return 0;
}

// STUB: GAME 0x5e3700
int GM_GunShop::OnClick(uint id)
{
    StubLog("GM_GunShop::OnClick");
    return 0;
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

// STUB: GAME 0x5e3cd0
int GM_LoadProgress::OnKey(uchar key, uint)
{
    StubLog("GM_LoadProgress::OnKey");
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

// STUB: GAME 0x5e3da0
int GM_GarageEndOfMission::OnCreate()
{
    StubLog("GM_GarageEndOfMission::OnCreate");
    return 0;
}

// Empty, out of line: the original's copy, between OnCreate and OnClick, is the one the linker kept of its folded
// group (C_web_path::SelectLine's)
// FUNCTION: GAME 0x5e3fd0 FOLDED
int GM_GarageEndOfMission::OnKey(uchar key, uint)
{
    return 0;
}

// STUB: GAME 0x5e3fe0
int GM_GarageEndOfMission::OnClick(uint id)
{
    StubLog("GM_GarageEndOfMission::OnClick %u", id);
    return 0;
}

// The garage manager's menu's overrides: stubs that log and run the base's
// STUB: GAME 0x5e41b0
int GM_GarageManager::OnCreate()
{
    StubLog("GM_GarageManager::OnCreate");
    return GM_Menu::OnCreate();
}

// STUB: GAME 0x5e4450
int GM_GarageManager::OnKey(uchar key, uint u)
{
    StubLog("GM_GarageManager::OnKey");
    return GM_Menu::OnKey(key, u);
}

// STUB: GAME 0x5e4470
int GM_GarageManager::OnClick(uint id)
{
    StubLog("GM_GarageManager::OnClick %u", id);
    return GM_Menu::OnClick(id);
}

// The garage's results' menu's overrides: stubs that log and run the base's
// STUB: GAME 0x5e4690
int GM_GarageSummary::OnCreate()
{
    StubLog("GM_GarageSummary::OnCreate");
    return GM_Menu::OnCreate();
}

// STUB: GAME 0x5e48b0
int GM_GarageSummary::OnKey(uchar key, uint u)
{
    StubLog("GM_GarageSummary::OnKey");
    return GM_Menu::OnKey(key, u);
}

// Out of line, without a marker: the original's entry is the copy the linker kept of a folded group (0x5e4d50, whose
// by-name marker is in g_m_func.h)
int GM_GarageSummary::OnClick(uint id)
{
    return 0;
}

