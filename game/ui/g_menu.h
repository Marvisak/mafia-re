#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class GM_Component;
class I3D_camera;
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

// SetCameraTargetID's ids besides those of the main menu's items, which _GM_MainMenuTargets gives a frame each (the
// names are ours)
#define MENUCAM_NONE 0                  // the scene's camera where it is
#define MENUCAM_START ((uint)-1)        // the frame "camera_start", where the main menu opens

// Sets IGraph's viewport, clipped to the screen, and turns the z-buffer off (PS2's, a global function of g_menu.cpp)
void SetViewport(int x, int y, int sx, int sy);

// An item of a menu's definition, tables\menu\<name>.mnu (PS2's name; the members' names are ours), 0x20 bytes: what a
// component's constructor reads
struct GM_DesignStruct
{
    uint id;                            // the component's id
    uint type;                          // its factory's (G_FactoryItem::type), four letters ('pbar', 'upup')
    float x, y, sx, sy;                 // 0x8, on a screen of 800x600 (menuScreenScale)
    uint text_id;                       // 0x18, of the text database
    uint flags;                         // 0x1c, the component's (COMPF_*)
};

// A component's factory (PS2's names; the members' names are ours): PS2's G_Factory<T> is one for each component's class,
// a global the file's initializer registers
struct G_FactoryItem
{
    // A component of the class, the parent's child
    GM_Component *(*Make)(GM_DesignStruct *design, GM_Component *parent);
    // A copy of a component of the class
    GM_Component *(*MakeDup)(GM_Component *component);
    uint type;                          // 0x8, the class's GetType
    G_FactoryItem *next;                // 0xc
};

// The components' factories (PS2's names)
class G_GlobalFactory
{
public:
    void Register(G_FactoryItem *item)
    {
        item->next = first;
        first = item;
    }
    // A component of that type, NULL where no factory has it (PS2's asserts there; the name is ours). The type is a
    // parameter of its own: through the design, VC6 computes the design's address before the search, the original at
    // the call.
    static GM_Component *MakeComponent(uint type, GM_DesignStruct *design, GM_Component *parent)
    {
        for (G_FactoryItem *item = first; item; item = item->next)
            if (item->type == type)
                return item->Make(design, parent);
        return NULL;
    }
    // A copy of the component, from the factory of its class, NULL where no factory has it (the name is ours; inline,
    // g_menu.cpp)
    static GM_Component *MakeDup(GM_Component *component);

    // The last one registered, the start of the list
    static G_FactoryItem *first;
};

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
    // Not virtual on the PC: Destroy and the loading screen's owner (0x6068aa) call it directly
    ~GM_Menu();

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
    static void UnloadResources();
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
    // key pressed or repeated, IGraph's shift keys and the frame's time. Closes a menu without components, ticks the
    // menu and its components, gives the key or the mouse's click to the active component and else moves the
    // activation (the arrows and Tab, or the mouse over a component), then clicks what was clicked; a closing menu is
    // destroyed at the end, its parent active again, or without one ends ExecuteMenu with -1. Returns OnClick's result.
    int Tick(int mouse_rx, int mouse_ry, int mouse_rz, uint buttons, uint key, uint shift_keys, uint time);
    // Opens a menu over the active one (PS2's name): the active menu becomes its parent, which gets the id back in
    // OnCreateChild and OnCloseChild, and the new one is created and active. Called on the menu, which it doesn't read.
    void MenuOpen(GM_Menu *menu, uint id);
    // Closes the menu with that code (PS2's name): tells the parent (OnCloseChild), then OnClose. Tick destroys it at
    // the frame's end. PS2's ends ExecuteMenu with -1 here where the menu has no parent, the PC's Tick does.
    void MenuClose(uint code);
    // Enables the component of that id, if the menu has one, with true (COMPF_ENABLED), and disables it with false:
    // PS2's name, and its code the same
    void SetDisabled(uint id, bool enabled);
    // Shows the component of that id, if the menu has one, with true (COMPF_VISIBLE), and hides it with false: PS2's
    // name, and its code the same
    void SetHidden(uint id, bool visible);
    // Sets the text, of the text database, of the component of that id, if the menu has one
    void SetTextID(uint id, uint text_id);
    // Sets the text of the component of that id, if the menu has one
    void SetText(uint id, uchar *text);
    // Sets the string of the component of that id, if the menu has one
    void SetString(uint id, uchar *text);
    // Sets the value of the component of that id, if the menu has one
    void SetFloat(uint id, float value);
    // Activates the component of that id, or else the next one that can be activated
    void ActivateByIDOrNext(uint id);
    // The menu's component of that id, NULL for none
    GM_Component *FindComponentByID(uint id);
    // Of the components that can be activated, the one with the highest id below id, or else the highest one
    GM_Component *FindComponentByIDPrev(uint id);
    // Of the components that can be activated, the one with the lowest id above id, or else the lowest one
    GM_Component *FindComponentByIDNext(uint id);
    // The component that can be activated before the one of that id in the menu's list, around from its end, NULL
    // where no other can (PC only, the name is ours: the left arrow's and Shift+Tab's)
    GM_Component *FindComponentPrev(uint id);
    // The one after it, around from the list's start (PC only, the name is ours: the right arrow's and Tab's)
    GM_Component *FindComponentNext(uint id);
    // Adds a copy of the component of that id, with the new id, right below it (PS2's name, Czech for "add a component
    // below"), and makes room for it: the component of grow_id (the window around) grows by its height, and those of
    // the other three ids (the items below) move down by it. An id of 0 is none.
    void PridejComponentPod(uint id, uint new_id, uint grow_id, uint move_id1, uint move_id2, uint move_id3);
    // Runs the menu until one of its functions sets the result (zz_globalReturnCode), then destroys it and returns the
    // result. While it runs it ticks the mission, or only the game's camera; the HUD's flags are pushed and the menus'
    // set. PS2's has a fourth bool, which both calls in its main pass as false; neither build reads the third.
    static int ExecuteMenu(GM_Menu *menu, bool tick_mission, bool);
    // Where the menu's camera goes: an item's id moves it to the item's frame (_GM_MainMenuTargets), which TickCamera
    // then approaches; MENUCAM_START puts it at once at "camera_start" (or where the camera is), looking at a point 10
    // ahead, and MENUCAM_NONE takes the scene's camera as it is. Only an item's id leaves TickCamera running.
    static void SetCameraTargetID(uint id);
    static void TickCamera(uint time);

    GM_Menu *parent;                    // 0x4, PS2's Create calls its OnCreateChild
    // 0x8, what Draw draws, with its children: ExecuteMenu creates the menu while it is NULL (the name is ours)
    GM_Component *root;
    GM_Component *active;               // 0xc, the active component, which has COMPF_ACTIVE (the name is ours)
    std::vector<GM_Component *> components; // 0x10, PS2's Destroy deletes them
    // 0x20, set by MenuClose, for Tick (the name is ours); Create plays the open sound while it is false
    bool closing;
    int close_code;                     // 0x24, MenuClose's, 0 again once Tick destroyed the menu (the name is ours)
    // 0x28, MenuOpen's id, which the parent gets back in OnCreateChild and OnCloseChild (the name is ours)
    uint open_id;
    uint flags;                         // 0x2c, MENUF_*

    // The screen relative to 800x600: x of the HUD's screen with Matrox Parhelia's two or three screens (PS2's relative
    // to 640x480)
    static S_vector2 menuScreenScale;
    // The menu ExecuteMenu runs
    static GM_Menu *activeMenuWindow;
    // The mouse (PC only, the names are ours): Tick clamps its position to the screen and moves the cursor there while
    // the mouse is in use, from a move of more than 10 pixels from the cursor, a button or the wheel until a key moves
    // the activation
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
    // The menu camera (PS2's names): the id SetCameraTargetID was given, 0 once the camera stands, the scene's camera,
    // where it is and looks now, where it goes and its speeds there, and TickCamera's circling
    static uint lastKameraTargetID;
    static I3D_camera *tc_frame;
    static S_vector tc_actCamera, tc_actTarget;
    static S_vector tc_destCamera, tc_destTarget;
    static S_vector tc_speedCamera, tc_speedTarget;
    static S_vector circlePos;
    static float circleTime;
};
