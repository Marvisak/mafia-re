#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class GM_Component;

// GM_Menu::flags (the names are ours: by value where the meaning isn't known yet)
#define MENUF_UNKNOWN_10000 0x10000     // set by the constructor
#define MENUF_UNKNOWN_80000 0x80000     // the main menu's
#define MENUF_INVENTORY_CLOSES 0x100000 // ExecuteMenu reads the controls: the inventory's acts as Escape

// A menu (PS2 g_menu.cpp), 0x30 bytes. The PC's has twelve virtual functions, PS2's in its order without the
// destructor, Destroy and MartinovaKlavesa. The defaults are the base's vtable's: the empty ones are folded copies
// (0x409220, 0x409210, 0x605020), OnBeforeExchange and OnExchange call the menu's own OnBeforeCreate and OnCreate,
// and the copies of the inline ones are in the run of out-of-line accessors at 0x47a400. PS2's has the members in the
// same order, 4 bytes lower (its vtable is at the end). Only what ExecuteMenu uses so far.
// VTABLE: GAME 0x627834
class GM_Menu
{
public:
    GM_Menu();

    // The menu's name
    virtual const char *OnBeforeCreate() = 0;
    virtual int OnCreate() { return 0; }
    virtual int OnClose() { return 0; }
    virtual const char *OnBeforeExchange() { return OnBeforeCreate(); }
    virtual int OnExchange() { return OnCreate(); }
    virtual int OnCreateChild(GM_Menu *child, uint) { return 0; }
    virtual int OnCloseChild(GM_Menu *child, uint, uint) { return 0; }
    virtual int OnKey(uchar key, uint);
    virtual int OnMouse(int x, int y, int z, uint, uint) { return 0; }
    virtual int OnClick(uint id) { return 0; }
    virtual int OnTick(uint time) { return 0; }
    virtual int OnChange(GM_Component *, GM_Component *) { return 0; }

    // The textures and sounds all menus share, which the first menu loads and the last one releases
    static void LoadResources();
    // Creates the menu's components from its definition (tables\menu\<OnBeforeCreate>.mnu) and calls OnCreate
    void Create();
    // Deletes the menu with its components
    void Destroy();
    // A frame of the menu: the mouse's movement and buttons (the buttons released, pressed and down, 4 bits each), the
    // key pressed or repeated, IGraph's shift keys and the frame's time
    void Tick(int mouse_rx, int mouse_ry, int mouse_rz, uint buttons, uint key, uint shift_keys, uint time);
    // Runs the menu until one of its functions sets the result (zz_globalReturnCode), then destroys it and returns the
    // result. While it runs it ticks the mission, or only the game's camera; the HUD's flags are pushed and the menus'
    // set. PS2's has a fourth bool, which both calls in its main pass as false; neither build reads the third.
    static int ExecuteMenu(GM_Menu *menu, bool tick_mission, bool);
    // The camera the menu looks through, 0 for none
    static void SetCameraTargetID(uint id);
    static void TickCamera(uint time);

    GM_Menu *parent;                    // 0x4, PS2's Create calls its OnCreateChild
    int unknown_8;                      // 0x8, ExecuteMenu creates the menu while it is 0
    int unknown_c;                      // 0xc
    std::vector<GM_Component *> components; // 0x10, PS2's Destroy deletes them
    bool unknown_20;                    // 0x20
    int unknown_24;                     // 0x24
    int unknown_28;                     // 0x28
    uint flags;                         // 0x2c, MENUF_*

    // The screen relative to 800x600: x of the HUD's screen with Matrox Parhelia's two or three screens (PS2's relative
    // to 640x480)
    static S_vector2 menuScreenScale;
    // The menu ExecuteMenu runs
    static GM_Menu *activeMenuWindow;
    // What ExecuteMenu returns: it runs the menu while this is 0
    static int zz_globalReturnCode;
    // Counts a second down for the blinking cursor
    static uint zz_blinkTime;
    // The menus that exist (PC only, the name is ours): the first one loads the resources, the last one releases them
    static int menuCount;
    static uint lastKameraTargetID;
};
