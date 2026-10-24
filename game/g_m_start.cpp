// The main menu, the file at 0x5e0640-0x5e48ec (docs/game-files.md); the name is a guess, PS2's g_m_start.cpp. OnChange
// (0x5e4a50) lies in the range of the next file but one. Not written yet: only what main needs to link.

#include "g_m_start.h"

#include "ls3df.h"

#pragma auto_inline(off)

// STUB: GAME 0x5e0670
const char *GM_MainMenu::OnBeforeCreate()
{
    dbgPrintf("STUB: GM_MainMenu::OnBeforeCreate");
    return "mainmenu";
}

// STUB: GAME 0x5e0690
int GM_MainMenu::OnCreate()
{
    dbgPrintf("STUB: GM_MainMenu::OnCreate");
    return 0;
}

// STUB: GAME 0x5e06e0
int GM_MainMenu::OnClick(uint id)
{
    dbgPrintf("STUB: GM_MainMenu::OnClick");
    return 0;
}

// STUB: GAME 0x5e4a50
int GM_MainMenu::OnChange(GM_Component *, GM_Component *)
{
    dbgPrintf("STUB: GM_MainMenu::OnChange");
    return 0;
}

#pragma auto_inline(on)
