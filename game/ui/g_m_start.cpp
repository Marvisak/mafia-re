// The profiles' menu (PS2 g_m_start.cpp, where PS2's GM_ProfileSelect, GM_ProfileCreate and GM_ProfileDelete are; the
// PC's file is the one at 0x5e84c0-0x5e8d0c, docs/game-files.md). Written so far: OnCreate, which GM_Menu::Create
// calls when G_LoadSaveClass::Init runs the menu; the rest is stubs.

#include "ui/g_m_start.h"

#include <string.h>

#include "ui/g_loadsave.h"
#include "ui/g_m_comps.h"
#include "ui/g_textdb.h"
#include "stub.h"

// The profiles' names in the menu, an item for each below the first, the window grown by them and moved up by half as
// much. PS2's (memory cards' profiles, the selected one active) is another function.
// FUNCTION: GAME 0x5e84c0
int GM_ProfileSelect::OnCreate()
{
    flags &= ~MENUF_UNKNOWN_10000;
    std::vector<G_Profile_FileHead> *profiles = mafiaLoadSave.ProfileGetList();
    float height = 0.0f;
    strcpy(new_name, textDatabase.GetText(TEXT_DEFAULT));
    int count = profiles->size();
    if (count)
    {
        std::vector<G_Profile_FileHead>::iterator it = profiles->begin();
        uint id = PROFILESELECT_PROFILE;
        SetText(id, it->name);
        for (it++; it < profiles->end(); it++)
        {
            PridejComponentPod(id, id + 1, PROFILESELECT_LIST, PROFILESELECT_CREATE, PROFILESELECT_DELETE,
                PROFILESELECT_QUIT_GAME);
            id++;
            GM_Component *component = FindComponentByID(id);
            if (component)
                height += component->sy;
            SetText(id, it->name);
        }
        GM_Component *window = FindComponentByID(PROFILESELECT_WINDOW);
        if (window)
        {
            window->sy += height;
            window->y -= height * .5f;
        }
        if (profiles->size() <= 1)
            SetDisabled(PROFILESELECT_DELETE, false);
        if (profiles->size() >= PROFILESELECT_MAX_PROFILES)
            SetDisabled(PROFILESELECT_CREATE, false);
    }
    else
    {
        SetDisabled(PROFILESELECT_PROFILE, false);
        SetDisabled(PROFILESELECT_DELETE, false);
    }
    count = mafiaLoadSave.ProfileGetList()->size();
    if (count)
        unknown_30 = 0;
    else
        unknown_30 = 1;
    SetCameraTargetID(MENUCAM_START);
    return 0;
}

#pragma auto_inline(off)

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
