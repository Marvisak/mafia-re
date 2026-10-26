// The main menu (PS2 g_m_func.cpp, where PS2's GM_MainMenu is; the PC's file is the one at 0x5e0640-0x5e48ec,
// docs/game-files.md). OnChange (0x5e4a50) lies in the range of the next file but one. Then the loading screen
// (GM_LoadProgress, from 0x5e3aa0), PS2's in PS2's order. Written so far: the main menu's OnBeforeCreate, the loading
// screen's constructor, OnBeforeCreate and OnTick; the rest is stubs of what main and C_mission::Open need to link.

#include "ui/g_m_func.h"
#include "ui/g_indicators.h"

#include "IGraph/IGraph.h"
#include "stub.h"

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

#pragma auto_inline(off)

// STUB: GAME 0x5e0690
int GM_MainMenu::OnCreate()
{
    StubLog("GM_MainMenu::OnCreate");
    return 0;
}

// STUB: GAME 0x5e06e0
int GM_MainMenu::OnClick(uint id)
{
    StubLog("GM_MainMenu::OnClick");
    return 0;
}

#pragma auto_inline(on)

// The menus' scale, relative to 800x600 (the HUD's screen of Parhelia's two or three): the constants are quotients, the
// reciprocals of the sizes
// FUNCTION: GAME 0x5e3aa0
GM_LoadProgress::GM_LoadProgress(uint unknown, float mission_part, uchar *unknown2) :
    mission_part(mission_part),
    unknown_38(unknown),
    unknown_3c(unknown2)
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

#pragma auto_inline(off)

// STUB: GAME 0x5e3b90
int GM_LoadProgress::OnCreate()
{
    StubLog("GM_LoadProgress::OnCreate");
    return 0;
}

#pragma auto_inline(on)

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

// STUB: GAME 0x5e3d20
void GM_LoadProgress::SetProgressMissionOpen(float progress)
{
    StubLog("GM_LoadProgress::SetProgressMissionOpen");
}

// STUB: GAME 0x5e3d40
void GM_LoadProgress::Present()
{
    StubLog("GM_LoadProgress::Present");
}

// STUB: GAME 0x5e4a50
int GM_MainMenu::OnChange(GM_Component *, GM_Component *)
{
    StubLog("GM_MainMenu::OnChange");
    return 0;
}

#pragma auto_inline(on)
