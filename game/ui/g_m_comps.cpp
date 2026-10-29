// The menus' components (PS2 g_m_comps.cpp; the PC's file is the one at 0x5d87e0-0x5dd65c, docs/game-files.md): the
// factories of 29 components' classes, each a global whose initializer registers it, in PS2's order without PS2's own
// classes. The classes are in g_m_comps.h, their functions in the class: the file's code is the factories' Make and
// MakeDup, each class's functions after the first one that needs its vtable. The two other files of components,
// g_m_comps2.cpp and g_m_compsm.cpp, have nine more. Written so far: the factories of the base class, the window, the
// clean window, the static text, the rectangle, the rectangle's texture, the progress bar and the menu item. The others
// are stub factories (G_StubFactory), which log each item a menu asks them for and make nothing, so the menu skips it.

#include "ui/g_m_comps.h"

#include "stub.h"

// The factories (PS2's names), in the original's order; a class not written yet has a stub factory with the class's
// type and name, its comment the original's global
// GLOBAL: GAME 0x6bd70c
G_Factory<GM_Component> GM_ComponentFactory;
// 0x6bd6fc
G_StubFactory GM_NahradaFactory('xxxx', "GM_Nahrada");
// 0x6bd6ec, which CreateCompsByDef never asks: it handles 'upup' itself
G_StubFactory GM_UpUpFactory('upup', "GM_UpUp");
// GLOBAL: GAME 0x6bd6dc
G_Factory<GM_Window> GM_WindowFactory;
// GLOBAL: GAME 0x6bd6cc
G_Factory<GM_WindowClean> GM_WindowCleanFactory;
// 0x6bd6bc
G_StubFactory GM_WindowScrollFactory('wins', "GM_WindowScroll");
// GLOBAL: GAME 0x6bd6ac
G_Factory<GM_StaticText> GM_StaticTextFactory;
// 0x6bd69c
G_StubFactory GM_StaticTextMemFactory('mtxt', "GM_StaticTextMem");
// 0x6bd68c
G_StubFactory GM_StaticTextNumFactory('ntxt', "GM_StaticTextNum");
// 0x6bd67c
G_StubFactory GM_StaticTextTimeFactory('htxt', "GM_StaticTextTime");
// 0x6bd66c
G_StubFactory GM_LineFactory('line', "GM_Line");
// GLOBAL: GAME 0x6bd65c
G_Factory<GM_Rectangle> GM_RectangleFactory;
// 0x6bd64c
G_StubFactory GM_OrezavacFactory('orez', "GM_Orezavac");
// GLOBAL: GAME 0x6bd63c
G_Factory<GM_RectangleTexture> GM_RectangleTextureFactory;
// GLOBAL: GAME 0x6bd62c
G_Factory<GM_ProgressBar> GM_ProgressBarFactory;
// GLOBAL: GAME 0x6bd61c
G_Factory<GM_MenuItem> GM_MenuItemFactory;
// 0x6bd60c
G_StubFactory GM_MenuItemMemFactory('itmm', "GM_MenuItemMem");
// 0x6bd5fc
G_StubFactory GM_MenuItemNumFactory('iten', "GM_MenuItemNum");
// 0x6bd5ec
G_StubFactory GM_RaceRoundItemFactory('rari', "GM_RaceRoundItem");
// 0x6bd5dc
G_StubFactory GM_ButtonFactory('butt', "GM_Button");
// 0x6bd5cc
G_StubFactory GM_ButtonCleanFactory('butc', "GM_ButtonClean");
// 0x6bd5bc
G_StubFactory GM_SliderFactory('slid', "GM_Slider");
// 0x6bd5ac
G_StubFactory GM_SliderIntegerFactory('sldi', "GM_SliderInteger");
// 0x6bd59c
G_StubFactory GM_NetPlayerFactory('netp', "GM_NetPlayer");
// 0x6bd58c
G_StubFactory GM_TextEditFactory('edit', "GM_TextEdit");
// 0x6bd57c
G_StubFactory GM_CheckBoxFactory('chck', "GM_CheckBox");
// 0x6bd56c
G_StubFactory GM_ButtonSelectFactory('bsel', "GM_ButtonSelect");
// 0x6bd55c
G_StubFactory GM_ButtonSelectBoolFactory('bslb', "GM_ButtonSelectBool");
// 0x6bd54c
G_StubFactory GM_EditNumFactory('edin', "GM_EditNum");

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

#pragma auto_inline(on)
