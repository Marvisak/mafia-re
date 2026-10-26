#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class GM_Component;
class I3D_sound;
class ITexture;

// GM_Menu::flags (the names are ours: by value where the meaning isn't known yet)
#define MENUF_UNKNOWN_10000 0x10000     // set by the constructor
#define MENUF_UNKNOWN_80000 0x80000     // the main menu's
#define MENUF_INVENTORY_CLOSES 0x100000 // ExecuteMenu reads the controls: the inventory's acts as Escape

// GM_Menu::Draw's flags, which reach the components' Draw (the names are ours)
#define MENUDRAW_ALL 0xffffffff
#define MENUDRAW_CURSOR 0x10000000      // the cursor's blink: cleared for the second's last 300 ms
#define MENUDRAW_UNKNOWN_40000000 0x40000000 // cleared for the parent menu

// Sets IGraph's viewport, clipped to the screen, and turns the z-buffer off (PS2's, a global function of g_menu.cpp)
void SetViewport(int x, int y, int sx, int sy);

// A menu (PS2 g_menu.cpp), 0x30 bytes. The PC's has twelve virtual functions, PS2's in its order without the
// destructor, Destroy and MartinovaKlavesa. The defaults are the base's vtable's: the empty ones are folded copies
// (0x409220, 0x409210, 0x605020), OnBeforeExchange and OnExchange call the menu's own OnBeforeCreate and OnCreate,
// and the copies of the inline ones are in the run of out-of-line accessors at 0x47a400. PS2's has the members in the
// same order, 4 bytes lower (its vtable is at the end). Only what ExecuteMenu, Create and Draw use so far.
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
    // Creates the menu's components from its definition, activates the first one it can, calls OnCreate, plays the open
    // sound and tells the parent
    void Create();
    // The components of the definition, tables\menu\<name>.mnu
    void CreateCompsByDef(const char *name);
    // Deletes the menu with its components
    void Destroy();
    // Draws the menu's components (its parent's first, where MENUF_UNKNOWN_10000 is set) and, over the active menu,
    // the mouse's cursor
    void Draw(uint flags);
    // A frame of the menu: the mouse's movement and buttons (the buttons released, pressed and down, 4 bits each), the
    // key pressed or repeated, IGraph's shift keys and the frame's time
    void Tick(int mouse_rx, int mouse_ry, int mouse_rz, uint buttons, uint key, uint shift_keys, uint time);
    // Sets the value of the component of that id, if the menu has one
    void SetFloat(uint id, float value);
    // Activates the component of that id, or else the next one that can be activated
    void ActivateByIDOrNext(uint id);
    // The menu's component of that id, NULL for none
    GM_Component *FindComponentByID(uint id);
    // Of the components that can be activated, the one with the lowest id above id, or else the lowest one
    GM_Component *FindComponentByIDNext(uint id);
    // Runs the menu until one of its functions sets the result (zz_globalReturnCode), then destroys it and returns the
    // result. While it runs it ticks the mission, or only the game's camera; the HUD's flags are pushed and the menus'
    // set. PS2's has a fourth bool, which both calls in its main pass as false; neither build reads the third.
    static int ExecuteMenu(GM_Menu *menu, bool tick_mission, bool);
    // The camera the menu looks through, 0 for none
    static void SetCameraTargetID(uint id);
    static void TickCamera(uint time);

    GM_Menu *parent;                    // 0x4, PS2's Create calls its OnCreateChild
    // 0x8, what Draw draws, with its children: ExecuteMenu creates the menu while it is NULL (the name is ours)
    GM_Component *root;
    GM_Component *active;               // 0xc, the active component, which has COMPF_ACTIVE (the name is ours)
    std::vector<GM_Component *> components; // 0x10, PS2's Destroy deletes them
    bool unknown_20;                    // 0x20, Create plays the open sound while it is false
    int unknown_24;                     // 0x24
    int unknown_28;                     // 0x28, what Create passes to the parent's OnCreateChild
    uint flags;                         // 0x2c, MENUF_*

    // The screen relative to 800x600: x of the HUD's screen with Matrox Parhelia's two or three screens (PS2's relative
    // to 640x480)
    static S_vector2 menuScreenScale;
    // The menu ExecuteMenu runs
    static GM_Menu *activeMenuWindow;
    // The mouse (PC only, the names are ours): Tick clamps its position to the screen and moves the cursor there while
    // the mouse is in use, from a move of more than 10 pixels, a button or the wheel until a key moves the activation
    static bool mouseInUse;
    static int cursorX, cursorY;
    static int mouseX, mouseY;
    // What ExecuteMenu returns: it runs the menu while this is 0
    static int zz_globalReturnCode;
    // Counts a second down for the blinking cursor
    static uint zz_blinkTime;
    // The menus that exist (PC only, the name is ours): the first one loads the resources, the last one releases them
    static int menuCount;
    // The resources (PS2's names). PS2 has no close sound and leaves it NULL; the PC loads it and adds the clip.
    static I3D_sound *menuSoundOpen;
    static I3D_sound *menuSoundClose;
    static I3D_sound *menuSoundChange;
    static ITexture *textureMenuTop;
    static ITexture *textureMenuBottom;
    static ITexture *textureSponka;     // PC only, the name is ours, after its file
    static ITexture *textureProgress;
    // PS2's memory card's and its wait's, which the PC doesn't load (or the two are PS2's _gm_precached and
    // _gm_precachedNumOf, which follow them there)
    static ITexture *textureMemory;
    static ITexture *textureWait;
    static uint lastKameraTargetID;
};
