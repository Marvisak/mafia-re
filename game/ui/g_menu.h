#pragma once

#include "Main.h"

class GM_Component;

// A menu (PS2 g_menu.cpp), 0x30 bytes. The PC's has twelve virtual functions, PS2's in its order without the
// destructor, Destroy and MartinovaKlavesa. The defaults are the base's vtable's: the empty ones are folded copies
// (0x409220, 0x409210, 0x605020), OnBeforeExchange and OnExchange call the menu's own OnBeforeCreate and OnCreate,
// and the copies of the inline ones are in the run of out-of-line accessors at 0x47a400. Only what main uses so far.
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

    // Runs the menu until the player leaves it and returns the choice. PS2's has a fourth bool, which both calls in
    // its main pass as false.
    static int ExecuteMenu(GM_Menu *menu, bool, bool);

    uchar unknown_4[0x28];
    uint flags;                         // 0x2c, 0x10000 from the constructor
};
