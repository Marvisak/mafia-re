// The menus (PS2 g_menu.cpp; the PC's file is the one at 0x5e8d10-0x5ece6c, docs/game-files.md). Not written yet: only
// what main needs to link.

#include "g_menu.h"

#include "stub.h"

#pragma auto_inline(off)

// The original's copy lies in the file at 0x549140, most likely a folded one
// STUB: GAME 0x5498f0
int GM_Menu::OnKey(uchar key, uint)
{
    StubLog("GM_Menu::OnKey");
    return 0;
}

// STUB: GAME 0x5ea1f0
GM_Menu::GM_Menu()
{
    StubLog("GM_Menu::GM_Menu");
    flags = 0x10000;
}

// STUB: GAME 0x5eba40
int GM_Menu::ExecuteMenu(GM_Menu *menu, bool, bool)
{
    StubLog("GM_Menu::ExecuteMenu %s", menu->OnBeforeCreate());
    // The main menu's quit
    return 0x18;
}

#pragma auto_inline(on)
