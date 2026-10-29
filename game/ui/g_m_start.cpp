// The profiles' menu (PS2 g_m_start.cpp, where PS2's GM_ProfileSelect, GM_ProfileCreate and GM_ProfileDelete are; the
// PC's file is the one at 0x5e84c0-0x5e8d0c, docs/game-files.md), written: the profiles' menu, which
// G_LoadSaveClass::Init runs, the name dialog GM_ProfileCreate and the deletion GM_ProfileDelete.

#include "ui/g_m_start.h"

#include <string.h>

#include "ui/g_loadsave.h"
#include "ui/g_m_comps.h"
#include "ui/g_m_func.h"
#include "ui/g_textdb.h"

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

// The name dialog cancelled asks for it again only without a profile; closed otherwise, also with Escape's -1, it creates
// the profile of new_name, which keeps the last name where OK didn't set it. Any child's closing creates the menu again
// with the profiles there are now. PS2's creates the profile on a memory card.
// FUNCTION: GAME 0x5e86a0
int GM_ProfileSelect::OnCloseChild(GM_Menu *child, uint id, uint code)
{
    if (id == PROFILESELECT_CREATE)
    {
        if (!code)
        {
            int count = mafiaLoadSave.ProfileGetList()->size();
            if (count)
                dialog = PROFILEDIALOG_NONE;
            else
                dialog = PROFILEDIALOG_CREATE;
        }
        else if (mafiaLoadSave.ProfileCreate((uchar *)new_name))
            dialog = PROFILEDIALOG_NONE;
        else
            dialog = PROFILEDIALOG_EXISTS;
    }
    MenuExchange();
    SetDisabled(PROFILESELECT_DELETE, mafiaLoadSave.ProfileGetList()->size() > 1);
    return 0;
}

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

// The edit takes PROFILECREATE_NAME_LENGTH characters and starts with the profiles' menu's name. ReadChar's characters,
// which the edit adds, come from the window messages (KBINIT_WINMESSAGES). PS2's builds its on-screen keyboard instead.
// FUNCTION: GAME 0x5e8a40
int GM_ProfileCreate::OnCreate()
{
    flags &= ~MENUF_UNKNOWN_10000;
    SetDword(PROFILECREATE_NAME, PROFILECREATE_NAME_LENGTH);
    SetText(PROFILECREATE_NAME, (uchar *)name);
    igraph->KeyboardInit(KBINIT_WINMESSAGES);
    return 0;
}

// The keyboard as InitSystem set it up. PS2's does nothing.
// FUNCTION: GAME 0x5e8a80
int GM_ProfileCreate::OnClose()
{
    igraph->KeyboardInit(KBINIT_EXCLUSIVE);
    return 0;
}

// OK copies the edit's name into the profiles' menu's; both close the dialog, or without a parent menu end ExecuteMenu.
// PS2's also has its on-screen keyboard's keys.
// FUNCTION: GAME 0x5e8aa0
int GM_ProfileCreate::OnClick(uint id)
{
    switch (id)
    {
    case PROFILECREATE_OK:
        strncpy(name, (char *)GetText(PROFILECREATE_NAME), PROFILESELECT_NAME_SIZE);
        if (!open_id)
            MenuCloseAll(PROFILECREATE_CODE_OK, PROFILECREATE_CREATED);
        else
            MenuClose(PROFILECREATE_CODE_OK);
        break;
    case PROFILECREATE_CANCEL:
        if (!open_id)
            MenuCloseAll(PROFILECREATE_CODE_CANCEL, PROFILECREATE_CANCELLED);
        else
            MenuClose(PROFILECREATE_CODE_CANCEL);
        break;
    }
    return 0;
}

// PS2's lists the memory cards' profiles
// FUNCTION: GAME 0x5e8b10
int GM_ProfileDelete::OnCreate()
{
    flags &= ~MENUF_UNKNOWN_10000;
    std::vector<G_Profile_FileHead> *profiles = mafiaLoadSave.ProfileGetList();
    float height = 0.0f;
    int count = profiles->size();
    if (count)
    {
        std::vector<G_Profile_FileHead>::iterator it = profiles->begin();
        uint id = PROFILEDELETE_PROFILE;
        SetText(id, it->name);
        for (it++; it < profiles->end(); it++)
        {
            PridejComponentPod(id, id + 1, PROFILEDELETE_LIST, PROFILEDELETE_CANCEL, 0, 0);
            id++;
            GM_Component *component = FindComponentByID(id);
            if (component)
                height += component->sy;
            SetText(id, it->name);
        }
        GM_Component *window = FindComponentByID(PROFILEDELETE_WINDOW);
        if (window)
        {
            window->sy += height;
            window->rel_y -= height * .5f;
        }
    }
    return 0;
}

// Any code but No's deletes the profile, also Escape's -1 (PS2's takes Yes's only)
// FUNCTION: GAME 0x5e8c00
int GM_ProfileDelete::OnCloseChild(GM_Menu *child, uint id, uint code)
{
    if (id >= PROFILEDELETE_PROFILE && id < PROFILEDELETE_PROFILE_END && code)
    {
        mafiaLoadSave.ProfileDelete(id - PROFILEDELETE_PROFILE);
        MenuClose(PROFILEDELETE_CODE_DELETED);
    }
    return 0;
}

// A profile's item asks whether to delete it: "Delete", with its name as the question
// FUNCTION: GAME 0x5e8c40
int GM_ProfileDelete::OnClick(uint id)
{
    switch (id)
    {
    case PROFILEDELETE_CANCEL:
        MenuClose((uint)-1);
        break;
    default:
        if (id >= PROFILEDELETE_PROFILE && id < PROFILEDELETE_PROFILE_END)
            MenuOpen(new GM_QuestionBox(textDatabase.GetText(TEXT_DELETE), (char *)GetText(id)), id);
    }
    return 0;
}
