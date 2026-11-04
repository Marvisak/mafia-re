// The menus' components (PS2 g_m_comps.cpp; the PC's file is the one at 0x5d87e0-0x5dd65c, docs/game-files.md): the
// factories of 29 components' classes, each a global whose initializer registers it, in PS2's order without PS2's own
// classes. The classes are in g_m_comps.h, their functions in the class: the file's code is the factories' Make and
// MakeDup, each class's functions after the first one that needs its vtable. The two other files of components,
// g_m_comps2.cpp and g_m_compsm.cpp, have nine more. Written whole: every factory with its class. After them the
// functions of the stub factories (G_StubFactory), ours, which the other two files' classes not written yet still have,
// and a stub of g_menu.cpp's RectangleBB.

#include "ui/g_m_comps.h"

#include "stub.h"

// The factories (PS2's names), in the original's order
// GLOBAL: GAME 0x6bd70c
G_Factory<GM_Component> GM_ComponentFactory;
// GLOBAL: GAME 0x6bd6fc
G_Factory<GM_Nahrada> GM_NahradaFactory;
// CreateCompsByDef never asks it: it handles 'upup' itself
// GLOBAL: GAME 0x6bd6ec
G_Factory<GM_UpUp> GM_UpUpFactory;
// GLOBAL: GAME 0x6bd6dc
G_Factory<GM_Window> GM_WindowFactory;
// GLOBAL: GAME 0x6bd6cc
G_Factory<GM_WindowClean> GM_WindowCleanFactory;
// GLOBAL: GAME 0x6bd6bc
G_Factory<GM_WindowScroll> GM_WindowScrollFactory;
// GLOBAL: GAME 0x6bd6ac
G_Factory<GM_StaticText> GM_StaticTextFactory;
// GLOBAL: GAME 0x6bd69c
G_Factory<GM_StaticTextMem> GM_StaticTextMemFactory;
// GLOBAL: GAME 0x6bd68c
G_Factory<GM_StaticTextNum> GM_StaticTextNumFactory;
// GLOBAL: GAME 0x6bd67c
G_Factory<GM_StaticTextTime> GM_StaticTextTimeFactory;
// GLOBAL: GAME 0x6bd66c
G_Factory<GM_Line> GM_LineFactory;
// GLOBAL: GAME 0x6bd65c
G_Factory<GM_Rectangle> GM_RectangleFactory;
// GLOBAL: GAME 0x6bd64c
G_Factory<GM_Orezavac> GM_OrezavacFactory;
// GLOBAL: GAME 0x6bd63c
G_Factory<GM_RectangleTexture> GM_RectangleTextureFactory;
// GLOBAL: GAME 0x6bd62c
G_Factory<GM_ProgressBar> GM_ProgressBarFactory;
// GLOBAL: GAME 0x6bd61c
G_Factory<GM_MenuItem> GM_MenuItemFactory;
// GLOBAL: GAME 0x6bd60c
G_Factory<GM_MenuItemMem> GM_MenuItemMemFactory;
// GLOBAL: GAME 0x6bd5fc
G_Factory<GM_MenuItemNum> GM_MenuItemNumFactory;
// GLOBAL: GAME 0x6bd5ec
G_Factory<GM_RaceRoundItem> GM_RaceRoundItemFactory;
// GLOBAL: GAME 0x6bd5dc
G_Factory<GM_Button> GM_ButtonFactory;
// GLOBAL: GAME 0x6bd5cc
G_Factory<GM_ButtonClean> GM_ButtonCleanFactory;
// GLOBAL: GAME 0x6bd5bc
G_Factory<GM_Slider> GM_SliderFactory;
// GLOBAL: GAME 0x6bd5ac
G_Factory<GM_SliderInteger> GM_SliderIntegerFactory;
// GLOBAL: GAME 0x6bd59c
G_Factory<GM_NetPlayer> GM_NetPlayerFactory;
// GLOBAL: GAME 0x6bd58c
G_Factory<GM_TextEdit> GM_TextEditFactory;
// GLOBAL: GAME 0x6bd57c
G_Factory<GM_CheckBox> GM_CheckBoxFactory;
// GLOBAL: GAME 0x6bd56c
G_Factory<GM_ButtonSelect> GM_ButtonSelectFactory;
// GLOBAL: GAME 0x6bd55c
G_Factory<GM_ButtonSelectBool> GM_ButtonSelectBoolFactory;
// GLOBAL: GAME 0x6bd54c
G_Factory<GM_EditNum> GM_EditNumFactory;

#pragma auto_inline(off)

G_StubFactory::G_StubFactory(uint type, const char *name) : name(name)
{
    this->type = type;
    G_FactoryItem::Make = Make;
    G_FactoryItem::MakeDup = MakeDup;
    globalFactory.Register(this);
}

// The stub factory of the type, which MakeComponent found first
static G_StubFactory *FindStubFactory(uint type)
{
    G_FactoryItem *item = G_GlobalFactory::first;
    while (item->type != type)
        item = item->next;
    return (G_StubFactory *)item;
}

// The class, its type and the item's id, which the menu then skips
GM_Component *G_StubFactory::Make(GM_DesignStruct *design, GM_Component *)
{
    uint type = design->type;
    StubLog("%s ('%c%c%c%c'): menu item 0x%x", FindStubFactory(type)->name, (type >> 24) & 0xff, (type >> 16) & 0xff,
        (type >> 8) & 0xff, type & 0xff, design->id);
    return NULL;
}

// Never reached: no component of a stub factory's class exists to be copied
GM_Component *G_StubFactory::MakeDup(GM_Component *component)
{
    StubLog("%s::MakeDup", FindStubFactory(component->GetType())->name);
    return NULL;
}

// g_menu.cpp's (0x5e91b0, between RectangleProgress and Triangle), here for now: only the player row calls it
// STUB: GAME 0x5e91b0
void RectangleBB(float x, float y, float sx, float sy, uint style)
{
    StubLog("RectangleBB");
}

#pragma auto_inline(on)
