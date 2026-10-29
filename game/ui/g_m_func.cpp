// The main menu (PS2 g_m_func.cpp, where PS2's GM_MainMenu is; the PC's file is the one at 0x5e0640-0x5e48ec,
// docs/game-files.md). The original's OnChange is folded into the racing menu's (0x5e4a50, in the next file but one).
// The PC's message box (from 0x5e2170) and question box (from 0x5e2310), then the loading screen (GM_LoadProgress, from
// 0x5e3aa0), PS2's in PS2's order. Written so far: the main menu's OnBeforeCreate, OnCreate and OnChange, the message
// box's OnBeforeCreate and constructor, the question box, the loading screen's constructor, OnBeforeCreate, OnCreate,
// OnTick, SetProgressMissionOpen and Present;
// the rest is stubs of what main, C_mission::Open and the profiles' menu need to link.

#include "ui/g_m_func.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_comps.h"
#include "ui/g_textdb.h"
#include "mission/Mission.h"

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
// 990108 is in neither menu, the multiplayer's camera frame's
#define MAINMENU_ITEM_CARCYCLOPEDIA 990109
#define MAINMENU_ITEM_OPTIONS 990110
#define MAINMENU_ITEM_CREDITS 990111
#define MAINMENU_ITEM_CHANGE_PROFILE 990112
#define MAINMENU_ITEM_QUIT 990113

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

// The text database's "press <ESC> to Abort" (the name is ours)
#define TEXT_PRESS_ESC_TO_ABORT 0x2ce

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

#pragma auto_inline(off)

// STUB: GAME 0x5e06e0
int GM_MainMenu::OnClick(uint id)
{
    StubLog("GM_MainMenu::OnClick");
    return 0;
}

#pragma auto_inline(on)

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

#pragma auto_inline(off)

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

#pragma auto_inline(on)

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

// Returns 0, as PS2's: the vtable's entry is a folded copy of another file's, apart from the base's in-class one
// FUNCTION: GAME 0x5e4d50 FOLDED
int GM_LoadProgress::OnTick(uint time)
{
    return 0;
}

#pragma auto_inline(off)

// STUB: GAME 0x5e3cd0
int GM_LoadProgress::OnKey(uchar key, uint)
{
    StubLog("GM_LoadProgress::OnKey");
    return 0;
}

#pragma auto_inline(on)

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

#pragma auto_inline(off)

// STUB: GAME 0x5e3da0
int GM_GarageEndOfMission::OnCreate()
{
    StubLog("GM_GarageEndOfMission::OnCreate");
    return 0;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5e3fe0
int GM_GarageEndOfMission::OnClick(uint id)
{
    StubLog("GM_GarageEndOfMission::OnClick %u", id);
    return 0;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5e41b0
void GM_GarageEndOfMission::vfunc_13()
{
    StubLog("GM_GarageEndOfMission::vfunc_13");
}

#pragma auto_inline(on)
