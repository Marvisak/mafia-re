// The menus (PS2 g_menu.cpp; the PC's file is the one at 0x5e8d10-0x5ece6c, docs/game-files.md). Written so far: what
// ExecuteMenu runs as far as the components (the constructor, the destructor and the resources, Create,
// CreateCompsByDef, Destroy, Draw and the activation), SetCameraTargetID, the wrappers by id the loading screen and the
// profiles' menu call, and PridejComponentPod; the components, Tick, TickCamera and the rest are stubs.

#include "ui/g_menu.h"
#include "ui/g_m_comps.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "input/controls.h"
#include "mission/Mission.h"
#include "mission/game.h"

#include <stdio.h>

#include "IGraph/IGraph.h"
#include "I3D/I3D.h"
#include "rw_data.h"
#include "stub.h"

// Its initializer starts the file (0x5e8d10)
// GLOBAL: GAME 0x6bd888
S_vector2 GM_Menu::menuScreenScale(1.0f, 1.0f);
// GLOBAL: GAME 0x6bd890
GM_Menu *GM_Menu::activeMenuWindow = NULL;
// GLOBAL: GAME 0x6bd894
bool GM_Menu::mouseInUse = false;
// GLOBAL: GAME 0x6bd898
int GM_Menu::cursorX = 0;
// GLOBAL: GAME 0x6bd89c
int GM_Menu::cursorY = 0;
// GLOBAL: GAME 0x6bd8a0
int GM_Menu::mouseX = 0;
// GLOBAL: GAME 0x6bd8a4
int GM_Menu::mouseY = 0;
// GLOBAL: GAME 0x6bd8a8
int GM_Menu::zz_globalReturnCode = 0;
// GLOBAL: GAME 0x6bd8ac
uint GM_Menu::zz_blinkTime = 0;
// GLOBAL: GAME 0x6bd8b0
G_FactoryItem *G_GlobalFactory::first = NULL;
// GLOBAL: GAME 0x6bd8b4
int GM_Menu::menuCount = 0;
// GLOBAL: GAME 0x6bd8b8
I3D_sound *GM_Menu::menuSoundOpen = NULL;
// GLOBAL: GAME 0x6bd8bc
I3D_sound *GM_Menu::menuSoundClose = NULL;
// GLOBAL: GAME 0x6bd8c0
I3D_sound *GM_Menu::menuSoundChange = NULL;
// GLOBAL: GAME 0x6bd8c4
ITexture *GM_Menu::textureMenuTop = NULL;
// GLOBAL: GAME 0x6bd8c8
ITexture *GM_Menu::textureMenuBottom = NULL;
// GLOBAL: GAME 0x6bd8cc
ITexture *GM_Menu::textureSponka = NULL;
// GLOBAL: GAME 0x6bd8d0
ITexture *GM_Menu::textureProgress = NULL;
// GLOBAL: GAME 0x6bd8d4
ITexture *GM_Menu::textureMemory = NULL;
// GLOBAL: GAME 0x6bd8d8
ITexture *GM_Menu::textureWait = NULL;
// GLOBAL: GAME 0x6bd8dc
uint GM_Menu::lastKameraTargetID = 0;
// GLOBAL: GAME 0x6bd8e0
I3D_camera *GM_Menu::tc_frame = NULL;
// GLOBAL: GAME 0x6bd8e4
float GM_Menu::circleTime = 0.0f;
// PS2's, a plain global, which TickCamera keeps between 0 and 1
// GLOBAL: GAME 0x6bd8e8
float circleScale = 0.0f;

// The original's .bss has these in the reverse of PS2's order, from tc_actCamera (0x6bd878) down to circlePos
// (0x6bd830), as another file's precompiled header would give them (g_indicators.cpp's the same); ours are hashed.
// Below them, 0x6bd884 is 4 bytes nothing reads (PS2 has GM_Menu::lastKey there).
// GLOBAL: GAME 0x6bd878
S_vector GM_Menu::tc_actCamera;
// GLOBAL: GAME 0x6bd86c
S_vector GM_Menu::tc_actTarget;
// GLOBAL: GAME 0x6bd860
S_vector GM_Menu::tc_destCamera;
// GLOBAL: GAME 0x6bd854
S_vector GM_Menu::tc_destTarget;
// GLOBAL: GAME 0x6bd848
S_vector GM_Menu::tc_speedCamera;
// GLOBAL: GAME 0x6bd83c
S_vector GM_Menu::tc_speedTarget;
// GLOBAL: GAME 0x6bd830
S_vector GM_Menu::circlePos;

// A main menu item's camera frame (the names are ours)
struct S_menu_target
{
    uint id;                            // the item's component
    const char *name;                   // the scene's frame
};

// The frames the main menu's camera goes to (PS2's name), the file's first data; the last one ends the table
// GLOBAL: GAME 0x6384d8
S_menu_target _GM_MainMenuTargets[] = {
    {990001, "camera_start"},
    {990101, "intro_pos"},
    {990102, "training_pos"},
    {990103, "new_game_pos"},
    {990104, "load_game_pos"},
    {990105, "free_ride_pos"},
    {990106, "free_ride_pos"},
    {990107, "racing_pos"},
    {990108, "multiplayer_pos"},
    {990109, "carcyclopedia_pos"},
    {990110, "options_pos"},
    {990111, "credits_pos"},
    {990112, "profile_pos"},
    {990113, "exit_pos"},
    {990411, "single_pos"},
    {990412, "champion_pos"},
    {990413, "multiplayer_pos"},
    {990416, "racing_pos"},
    {0, ""},
};

#pragma auto_inline(off)

// The original's copy lies in the file at 0x549140, most likely a folded one
// STUB: GAME 0x5498f0
int GM_Menu::OnKey(uchar key, uint)
{
    StubLog("GM_Menu::OnKey");
    return 0;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x5e98a0
void SetViewport(int x, int y, int sx, int sy)
{
    LS3D_VIEWPORT viewport;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    viewport.x = x < 0 ? 0 : x;
    viewport.y = y < 0 ? 0 : y;
    viewport.width = sx;
    viewport.height = sy;
    if (viewport.x + viewport.width > igraph->Scrn_sx())
        viewport.width = igraph->Scrn_sx() - viewport.x;
    if (viewport.y + viewport.height > igraph->Scrn_sy())
        viewport.height = igraph->Scrn_sy() - viewport.y;
    igraph->SetViewPort(&viewport);
    driver->SetState(RS_USEZB, false);
}

// PS2's loads two more textures, its memory card's and its wait's, and no close sound
// FUNCTION: GAME 0x5e9960
void GM_Menu::LoadResources()
{
    if (!textureMenuTop)
        // STRING: GAME 0x6386b0
        igraph->CreateITexture("papir5a.tga", NULL, ITC_ALPHA, &textureMenuTop);
    if (!textureMenuBottom)
        // STRING: GAME 0x6386a4
        igraph->CreateITexture("papir3.tga", NULL, ITC_ALPHA, &textureMenuBottom);
    if (!textureSponka)
        // STRING: GAME 0x638698
        igraph->CreateITexture("sponka.tga", NULL, ITC_ALPHA, &textureSponka);
    if (!textureProgress)
        // STRING: GAME 0x63868c
        igraph->CreateITexture("kulicky.tga", NULL, ITC_ALPHA, &textureProgress);
    if (!menuSoundOpen)
    {
        menuSoundOpen = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        menuSoundOpen->SetSoundType(I3DSOUND_AMBIENT);
        // STRING: GAME 0x638678
        menuSoundOpen->Open("sounds\\menuopen.wav", 0, NULL, NULL);
        menuSoundOpen->SetOutVol(1.0f);
        menuSoundOpen->SetVolume(1.0f);
        menuSoundOpen->Update();
        menuSoundOpen->SetOn(false);
    }
    if (!menuSoundClose)
    {
        menuSoundClose = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        menuSoundClose->SetSoundType(I3DSOUND_AMBIENT);
        // STRING: GAME 0x638660
        menuSoundClose->Open("sounds\\menuclose.wav", 0, NULL, NULL);
        menuSoundClose->SetOutVol(1.0f);
        menuSoundClose->SetVolume(1.0f);
        menuSoundClose->Update();
        menuSoundClose->SetOn(false);
    }
    if (!menuSoundChange)
    {
        menuSoundChange = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        menuSoundChange->SetSoundType(I3DSOUND_AMBIENT);
        // STRING: GAME 0x638648
        menuSoundChange->Open("sounds\\menuchange.wav", 0, NULL, NULL);
        menuSoundChange->SetOutVol(1.0f);
        menuSoundChange->SetVolume(1.0f);
        menuSoundChange->Update();
        menuSoundChange->SetOn(false);
    }
}

// PS2's releases its two sounds only
// FUNCTION: GAME 0x5e9b40
void GM_Menu::UnloadResources()
{
    if (textureMenuTop)
        textureMenuTop->Release();
    textureMenuTop = NULL;
    if (textureMenuBottom)
        textureMenuBottom->Release();
    textureMenuBottom = NULL;
    if (textureSponka)
        textureSponka->Release();
    textureSponka = NULL;
    if (textureProgress)
        textureProgress->Release();
    textureProgress = NULL;
    if (menuSoundOpen)
    {
        menuSoundOpen->Release();
        menuSoundOpen = NULL;
    }
    if (menuSoundClose)
    {
        menuSoundClose->Release();
        menuSoundClose = NULL;
    }
    if (menuSoundChange)
    {
        menuSoundChange->Release();
        menuSoundChange = NULL;
    }
}

// FUNCTION: GAME 0x5e9d60
void GM_Component::BaseDraw(uint draw_flags)
{
    if (flags & COMPF_VISIBLE)
    {
        draw_flags &= flags | 0xff000000;
        Draw(draw_flags);
        for (std::vector<GM_Component *>::iterator it = children.begin(); it < children.end(); it++)
            (*it)->BaseDraw(draw_flags);
    }
}

// FUNCTION: GAME 0x5e9da0
void GM_Component::BaseAfterMove(uint draw_flags)
{
    AfterMove(draw_flags);
    for (std::vector<GM_Component *>::iterator it = children.begin(); it < children.end(); it++)
        (*it)->BaseAfterMove(draw_flags);
}

// PS2's loads the resources in ExecuteMenu
// FUNCTION: GAME 0x5ea1f0
GM_Menu::GM_Menu() : parent(NULL), root(NULL), active(NULL), unknown_20(false), unknown_24(0), unknown_28(0)
{
    flags = MENUF_UNKNOWN_10000;
    if (!menuCount)
        LoadResources();
    menuCount++;
}

// FUNCTION: GAME 0x5ea270
GM_Menu::~GM_Menu()
{
    if (--menuCount <= 0)
    {
        UnloadResources();
        menuCount = 0;
    }
}

// PS2's frees the components' array
// FUNCTION: GAME 0x5ea2e0
void GM_Menu::Create()
{
    components.clear();
    CreateCompsByDef(OnBeforeCreate());
    ActivateByIDOrNext(1);
    OnCreate();
    if (!unknown_20 && menuSoundOpen)
    {
        menuSoundOpen->SetOn(false, true);
        menuSoundOpen->SetOn(true, true);
    }
    if (parent)
        parent->OnCreateChild(this, unknown_28);
    OnChange(active, NULL);
}

// The menu's header (the names are ours)
struct S_menu_header
{
    char magic[4];                      // "Menu"
    uint version;                       // 1
    uint count;                         // 0x8, of the items
    uint unknown_c;
};

// PS2's takes a char *, and looks into the menus it preloaded first; the PC's looks for the language's menu first,
// tables\menu\<name>_<language>.mnu
// FUNCTION: GAME 0x5ea370
void GM_Menu::CreateCompsByDef(const char *name)
{
    char path[256];
    S_menu_header header;
    if (!name)
        return;
    int file = -1;
    uint language = mafiaIndicators.GetLanguage();
    if (language)
    {
        // STRING: GAME 0x6386d0
        sprintf(path, "tables\\menu\\%s_%c%c.mnu", name, language & 0xff, (language >> 8) & 0xff);
        file = dtaOpen(path, false);
    }
    if (file < 0)
    {
        // STRING: GAME 0x6386bc
        sprintf(path, "tables\\menu\\%s.mnu", name);
        file = dtaOpen(path, false);
        if (file < 0)
            return;
    }
    dtaRead(file, &header, sizeof(header));
    GM_DesignStruct *design = new GM_DesignStruct[header.count];
    dtaRead(file, design, header.count * sizeof(GM_DesignStruct));
    dtaClose(file);
    // The components after an 'upup' item are the root's children
    GM_Component *component = NULL;
    GM_Component *parent = NULL;
    for (uint i = 0; i < header.count; i++)
    {
        if (design[i].type == 'upup')
        {
            if (parent)
                while (parent->parent)
                    parent = parent->parent;
            continue;
        }
        component = G_GlobalFactory::MakeComponent(design[i].type, &design[i], parent);
        if (!component)
            continue;
        components.push_back(component);
        if (!root)
        {
            root = component;
            parent = component;
            component->x += levyOkraj.x;
        }
        if (component->CanBeParent())
            parent = component;
    }
    delete[] design;
}

// PS2's deletes the menu through its virtual destructor
// FUNCTION: GAME 0x5ea690
void GM_Menu::Destroy()
{
    for (std::vector<GM_Component *>::iterator it = components.begin(); it < components.end(); it++)
        delete *it;
    delete this;
}

// PS2's draws the components only without the menu's flag 0x200000, and no cursor
// FUNCTION: GAME 0x5ea7e0
void GM_Menu::Draw(uint draw_flags)
{
    driver->SetState(RS_USEZB, false);
    if (zz_blinkTime > 700)
        draw_flags &= ~MENUDRAW_CURSOR;
    if (parent && flags & MENUF_UNKNOWN_10000)
        parent->Draw(draw_flags & ~MENUDRAW_UNKNOWN_40000000);
    if (root)
    {
        SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
        root->BaseAfterMove(draw_flags);
        root->BaseDraw(draw_flags);
    }
    if (activeMenuWindow == this)
    {
        SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
        mafiaIndicators.DrawCursor((float)cursorX, (float)cursorY);
    }
    driver->SetState(RS_USEZB, true);
}

#pragma auto_inline(off)

// Ends every menu with the main menu's quit, as the stub of ExecuteMenu did, so that a run goes through
// STUB: GAME 0x5ea8c0
void GM_Menu::Tick(int mouse_rx, int mouse_ry, int mouse_rz, uint buttons, uint key, uint shift_keys, uint time)
{
    StubLog("GM_Menu::Tick");
    zz_globalReturnCode = 0x18;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x5eaed0
void GM_Menu::SetDisabled(uint id, bool enabled)
{
    GM_Component *component = FindComponentByID(id);
    if (enabled)
    {
        if (component)
            component->flags |= COMPF_ENABLED;
    }
    else if (component)
        component->flags &= ~COMPF_ENABLED;
}

// FUNCTION: GAME 0x5eaf20
void GM_Menu::SetHidden(uint id, bool visible)
{
    GM_Component *component = FindComponentByID(id);
    if (visible)
    {
        if (component)
            component->flags |= COMPF_VISIBLE;
    }
    else if (component)
        component->flags &= ~COMPF_VISIBLE;
}

// FUNCTION: GAME 0x5eafd0
void GM_Menu::SetTextID(uint id, uint text_id)
{
    GM_Component *component = FindComponentByID(id);
    if (component)
        component->SetTextID(text_id);
}

// FUNCTION: GAME 0x5eb010
void GM_Menu::SetText(uint id, uchar *text)
{
    GM_Component *component = FindComponentByID(id);
    if (component)
        component->SetText(text);
}

// FUNCTION: GAME 0x5eb030
void GM_Menu::SetString(uint id, uchar *text)
{
    GM_Component *component = FindComponentByID(id);
    if (component)
        component->SetString(text);
}

// FUNCTION: GAME 0x5eb070
void GM_Menu::SetFloat(uint id, float value)
{
    GM_Component *component = FindComponentByID(id);
    if (component)
        component->SetFloat(value);
}

// FUNCTION: GAME 0x5eb210
void GM_Menu::ActivateByIDOrNext(uint id)
{
    if (active)
        active->flags &= ~COMPF_ACTIVE;
    active = FindComponentByID(id);
    if (active)
    {
        if (active->flags & COMPF_ENABLED && active->flags & COMPF_VISIBLE && active->flags & COMPF_UNKNOWN_8000)
            active->flags |= COMPF_ACTIVE;
        else
            active = NULL;
    }
    if (!active)
        active = FindComponentByIDNext(id);
    if (active)
        active->flags |= COMPF_ACTIVE;
}

// FUNCTION: GAME 0x5eb2f0
GM_Component *GM_Menu::FindComponentByID(uint id)
{
    for (std::vector<GM_Component *>::iterator it = components.begin(); it < components.end(); it++)
        if ((*it)->id == id)
            return *it;
    return NULL;
}

// FUNCTION: GAME 0x5eb3c0
GM_Component *GM_Menu::FindComponentByIDNext(uint id)
{
    GM_Component *found = NULL;
    uint found_id = (uint)-1;
    std::vector<GM_Component *>::iterator it;
    for (it = components.begin(); it < components.end(); it++)
        if ((*it)->id > id && (*it)->flags & COMPF_ENABLED && (*it)->flags & COMPF_VISIBLE &&
            (*it)->flags & COMPF_UNKNOWN_8000 && (*it)->id < found_id)
        {
            found_id = (*it)->id;
            found = *it;
        }
    if (found)
        return found;
    for (it = components.begin(); it < components.end(); it++)
        if ((*it)->flags & COMPF_ENABLED && (*it)->flags & COMPF_VISIBLE && (*it)->flags & COMPF_UNKNOWN_8000 &&
            (*it)->id < found_id)
        {
            found_id = (*it)->id;
            found = *it;
        }
    return found;
}

inline GM_Component *G_GlobalFactory::MakeDup(GM_Component *component)
{
    for (G_FactoryItem *item = first; item; item = item->next)
        if (item->type == component->GetType())
            return item->MakeDup(component);
    return NULL;
}

// The window's component isn't tested for NULL, the others are (PS2 tests all four)
// FUNCTION: GAME 0x5eb7d0
void GM_Menu::PridejComponentPod(uint id, uint new_id, uint grow_id, uint move_id1, uint move_id2, uint move_id3)
{
    GM_Component *src = FindComponentByID(id);
    if (!src)
        return;
    GM_Component *dup = G_GlobalFactory::MakeDup(src);
    if (!dup)
        return;
    components.push_back(dup);
    dup->y = src->y + src->sy;
    dup->id = new_id;
    dup->AfterMove(0);
    if (grow_id)
    {
        GM_Component *window = FindComponentByID(grow_id);
        window->sy += dup->sy;
    }
    GM_Component *component;
    if (move_id1)
    {
        component = FindComponentByID(move_id1);
        if (component)
            component->y += dup->sy;
    }
    if (move_id2)
    {
        component = FindComponentByID(move_id2);
        if (component)
            component->y += dup->sy;
    }
    if (move_id3)
    {
        component = FindComponentByID(move_id3);
        if (component)
            component->y += dup->sy;
    }
}

// PS2's is the same loop without the mouse, the joysticks and the controls: it reads the controller, draws the menu
// itself and checks the CD's tray. The PC's draws it from the scene's callback (Main.cpp), after the frame.
// last_key's 0xff never reaches the loop: VC6 gives the switch's int-to-float temporary the same stack slot (the
// original's too), so the first frames test the low byte of the screen's width as the key held.
// FUNCTION: GAME 0x5eba40
int GM_Menu::ExecuteMenu(GM_Menu *menu, bool tick_mission, bool)
{
    uint buttons = 0;
    uint delay = 1000;
    uchar last_key = 0xff;
    bool controls_read = false;
    if (!menu)
        return 0;
    switch (parheliaIndex)
    {
    case PARHELIA_DUAL:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 1600.0f);
        break;
    case PARHELIA_TRIPLE:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 2400.0f);
        break;
    default:
        menuScreenScale.x = igraph->Scrn_sx() * (1.0f / 800.0f);
    }
    menuScreenScale.y = igraph->Scrn_sy() * (1.0f / 600.0f);
    mafiaIndicators.unknown_40a8.push_back(mafiaIndicators.unknown_40a4);
    mafiaIndicators.unknown_40a4 =
        (mafiaIndicators.unknown_40a4 & (INDF_UNKNOWN_4000 | INDF_UNKNOWN_8000 | INDF_UNKNOWN_20000)) | INDF_MENU;
    SetCameraTargetID(MENUCAM_NONE);
    zz_globalReturnCode = 0;
    if (!menu->root)
        menu->Create();
    activeMenuWindow = menu;
    while (!zz_globalReturnCode)
    {
        uint time = igraph->GetTimer(16, 249);
        igraph->ProcessWinMessages();
        igraph->UpdateMouseData();
        igraph->UpdateKeyboardData();
        uchar key = igraph->ReadKey();
        for (int i = joy.size(); i--;)
            joy[i]->Update(JOY_UPDATE_RESET);
        if (activeMenuWindow && activeMenuWindow->flags & MENUF_INVENTORY_CLOSES)
        {
            controls.Update(false);
            float *pressed;
            controls_read = true;
            controls.GetValues(pressed, CONTROLS_PRESSED);
            if (pressed[CTRL_INVENTORY] != 0.0f)
                key = DIK_ESCAPE;
        }
        if (tick_mission)
            act_mission->Tick(time);
        else
            act_mission->game->camera.Tick(time);
        if (zz_blinkTime > time)
            zz_blinkTime -= time;
        else
            zz_blinkTime = 1000 - time;
        // A key held repeats after 400 ms, then every 100 ms
        if (key)
        {
            last_key = key;
            delay = 400;
        }
        else if (last_key)
        {
            if (delay > time)
                delay -= time;
            else if (igraph->TestKey(last_key))
            {
                delay = 100;
                key = last_key;
            }
            else
                last_key = 0;
        }
        uint last_buttons = buttons;
        buttons = igraph->GetMouseButtons();
        activeMenuWindow->Tick(igraph->Mouse_rx(), igraph->Mouse_ry(), igraph->Mouse_rz(),
            ((last_buttons & ~buttons) << 4 | (buttons & ~last_buttons)) << 4 | buttons, key, igraph->GetShiftKeys(false),
            time);
        if (lastKameraTargetID)
            TickCamera(time);
        if (!zz_globalReturnCode)
        {
            act_mission->scene->Render();
            igraph->Present();
        }
    }
    activeMenuWindow->Destroy();
    activeMenuWindow = NULL;
    if (mafiaIndicators.unknown_40a8.size())
    {
        mafiaIndicators.unknown_40a4 = mafiaIndicators.unknown_40a8.back();
        mafiaIndicators.unknown_40a8.erase(mafiaIndicators.unknown_40a8.end() - 1);
    }
    if (controls_read)
    {
        controls.Update(false);
        controls.Update(false);
    }
    return zz_globalReturnCode;
}

// PS2's is the same switch, its cases in this order. Open: the original calls S_vector's constructor of
// "camera_start"'s `dir * 10.0f` out of line too (its frame is 0x58 bytes, ours 0x4c); two empty inline calls after
// the camera's Update, or between there and case 0's sum, give that and the original's instruction count, so its
// source had about that many more inline calls there.
// FUNCTION: GAME 0x5ebe80
void GM_Menu::SetCameraTargetID(uint id)
{
    I3D_frame *frame;
    lastKameraTargetID = id;
    switch (id)
    {
    case MENUCAM_START:
        tc_frame = act_mission->scene->current_camera;
        // STRING: GAME 0x638638
        frame = act_mission->scene->FindFrame("camera_start", ENUMF_CAMERA);
        if (frame)
        {
            tc_actCamera = frame->GetWorldPos();
            tc_actTarget = tc_actCamera + frame->GetWorldDir() * 10.0f;
        }
        else if (tc_frame)
        {
            tc_actCamera = tc_frame->GetWorldPos();
            tc_actTarget = tc_actCamera + tc_frame->GetWorldDir() * 10.0f;
        }
        else
        {
            tc_actCamera = S_vector(0.0f, 0.0f, 1.0f);
            tc_actTarget = S_vector(0.0f, 0.0f, 0.0f);
        }
        tc_speedCamera.Zero();
        tc_speedTarget.Zero();
        tc_destCamera = tc_actCamera;
        tc_destTarget = tc_actTarget;
        if (tc_frame)
        {
            tc_frame->SetPos(tc_actCamera);
            tc_frame->SetDir(tc_actTarget - tc_actCamera, 0.0f);
            tc_frame->Update();
        }
        lastKameraTargetID = 0;
        circleTime = 0.0f;
        circleScale = 0.0f;
        break;
    case MENUCAM_NONE:
        tc_frame = act_mission->scene->current_camera;
        if (tc_frame)
        {
            tc_actCamera = tc_frame->GetWorldPos();
            tc_actTarget = tc_actCamera + tc_frame->GetWorldDir();
        }
        else
        {
            tc_actCamera = S_vector(0.0f, 0.0f, 1.0f);
            tc_actTarget = S_vector(0.0f, 0.0f, 0.0f);
        }
        tc_speedCamera.Zero();
        tc_speedTarget.Zero();
        tc_destCamera = tc_actCamera;
        tc_destTarget = tc_actTarget;
        break;
    default:
        for (uint i = 0; i < sizeof(_GM_MainMenuTargets) / sizeof(*_GM_MainMenuTargets); i++)
            if (_GM_MainMenuTargets[i].id == id)
            {
                frame = act_mission->scene->FindFrame(_GM_MainMenuTargets[i].name, ENUMF_ALL);
                if (frame)
                {
                    tc_destCamera = frame->GetWorldPos();
                    tc_destTarget = tc_destCamera + frame->GetWorldDir();
                }
            }
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x5ec8a0
void GM_Menu::TickCamera(uint time)
{
    StubLog("GM_Menu::TickCamera");
}

#pragma auto_inline(on)
