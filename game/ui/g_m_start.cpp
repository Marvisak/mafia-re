// The profiles' menu (PS2 g_m_start.cpp, where PS2's GM_ProfileSelect, GM_ProfileCreate and GM_ProfileDelete are; the
// PC's file is the one at 0x5e84c0-0x5e8d0c, docs/game-files.md). Not written yet: only what G_LoadSaveClass::Init
// needs to link.

#include "ui/g_m_start.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x5e84c0
int GM_ProfileSelect::OnCreate()
{
    StubLog("GM_ProfileSelect::OnCreate");
    return 0;
}

// STUB: GAME 0x5e86a0
int GM_ProfileSelect::OnCloseChild(GM_Menu *child, uint, uint)
{
    StubLog("GM_ProfileSelect::OnCloseChild");
    return 0;
}

// STUB: GAME 0x5e8760
int GM_ProfileSelect::OnTick(uint time)
{
    StubLog("GM_ProfileSelect::OnTick");
    return 0;
}

// STUB: GAME 0x5e88d0
int GM_ProfileSelect::OnClick(uint id)
{
    StubLog("GM_ProfileSelect::OnClick");
    return 0;
}

#pragma auto_inline(on)
