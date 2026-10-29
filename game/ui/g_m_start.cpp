// The profiles' menu (PS2 g_m_start.cpp, where PS2's GM_ProfileSelect, GM_ProfileCreate and GM_ProfileDelete are; the
// PC's file is the one at 0x5e84c0-0x5e8d0c, docs/game-files.md). Written so far: OnCreate, which GM_Menu::Create
// calls when G_LoadSaveClass::Init runs the menu, OnTick, which GM_Menu::Tick calls, and OnClick, the items' clicks;
// the rest is stubs.

#include "ui/g_m_start.h"

#include <string.h>

#include "ui/g_loadsave.h"
#include "ui/g_m_comps.h"
#include "ui/g_m_func.h"
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
            window->rel_y -= height * .5f;
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
        dialog = PROFILEDIALOG_NONE;
    else
        dialog = PROFILEDIALOG_CREATE;
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

#pragma auto_inline(on)

// The name dialog stays asked for: the profiles' menu doesn't tick while its child is open, and OnCloseChild then asks
// for the next. PS2's is the memory card's.
// FUNCTION: GAME 0x5e8760
int GM_ProfileSelect::OnTick(uint time)
{
    switch (dialog)
    {
    case PROFILEDIALOG_CREATE:
        MenuOpen(new GM_ProfileCreate(new_name), PROFILESELECT_CREATE);
        break;
    case PROFILEDIALOG_DELETE:
        if (mafiaLoadSave.ProfileGetList()->size() > 1)
            MenuOpen(new GM_ProfileDelete, PROFILESELECT_DELETE);
        break;
    case PROFILEDIALOG_EXISTS:
        MenuOpen(new GM_MessageBox(TEXT_PROFILE_EXISTS, 0), 0);
        dialog = PROFILEDIALOG_CREATE;
        break;
    }
    return 0;
}

// A profile's item makes it the profile and ends the menu, also for "< none >"; the two dialogs open over the menu, the
// deletion only with more than one profile. PS2's reads the memory card's profile it chose.
// FUNCTION: GAME 0x5e88d0
int GM_ProfileSelect::OnClick(uint id)
{
    switch (id)
    {
    case PROFILESELECT_CREATE:
        MenuOpen(new GM_ProfileCreate(new_name), PROFILESELECT_CREATE);
        break;
    case PROFILESELECT_DELETE:
        if (mafiaLoadSave.ProfileGetList()->size() > 1)
            MenuOpen(new GM_ProfileDelete, PROFILESELECT_DELETE);
        break;
    case PROFILESELECT_QUIT_GAME:
        MenuCloseAll((uint)-1, PROFILESELECT_QUIT);
        break;
    default:
        if (id >= PROFILESELECT_PROFILE && id < PROFILESELECT_PROFILE_END)
        {
            if (id - PROFILESELECT_PROFILE < mafiaLoadSave.ProfileGetList()->size())
                mafiaLoadSave.ProfileSelect(id - PROFILESELECT_PROFILE);
            MenuCloseAll((uint)-1, PROFILESELECT_CHOSEN);
        }
    }
    return 0;
}

#pragma auto_inline(off)

// STUB: GAME 0x5e8a40
int GM_ProfileCreate::OnCreate()
{
    StubLog("GM_ProfileCreate::OnCreate");
    return 0;
}

// STUB: GAME 0x5e8a80
int GM_ProfileCreate::OnClose()
{
    StubLog("GM_ProfileCreate::OnClose");
    return 0;
}

// STUB: GAME 0x5e8aa0
int GM_ProfileCreate::OnClick(uint id)
{
    StubLog("GM_ProfileCreate::OnClick");
    return 0;
}

// STUB: GAME 0x5e8b10
int GM_ProfileDelete::OnCreate()
{
    StubLog("GM_ProfileDelete::OnCreate");
    return 0;
}

// STUB: GAME 0x5e8c00
int GM_ProfileDelete::OnCloseChild(GM_Menu *child, uint, uint)
{
    StubLog("GM_ProfileDelete::OnCloseChild");
    return 0;
}

// STUB: GAME 0x5e8c40
int GM_ProfileDelete::OnClick(uint id)
{
    StubLog("GM_ProfileDelete::OnClick");
    return 0;
}

#pragma auto_inline(on)
