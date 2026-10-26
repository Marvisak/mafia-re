// The main menu (PS2 g_m_func.cpp, where PS2's GM_MainMenu is; the PC's file is the one at 0x5e0640-0x5e48ec,
// docs/game-files.md). OnChange (0x5e4a50) lies in the range of the next file but one. Written so far: OnBeforeCreate;
// the rest is stubs of what main needs to link.

#include "ui/g_m_func.h"

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

// STUB: GAME 0x5e4a50
int GM_MainMenu::OnChange(GM_Component *, GM_Component *)
{
    StubLog("GM_MainMenu::OnChange");
    return 0;
}

#pragma auto_inline(on)
