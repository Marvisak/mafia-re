// The menus' components (PS2 g_m_comps.cpp; the PC's file is the one at 0x5d87e0-0x5dd65c, docs/game-files.md): the
// factories of 29 components' classes, each a global whose initializer registers it, in PS2's order without PS2's own
// classes. The classes are in g_m_comps.h, their functions in the class: the file's code is the factories' Make and
// MakeDup, each class's functions after the first one that needs its vtable. The two other files of components,
// g_m_comps2.cpp and g_m_compsm.cpp, have nine more. Written so far: the factories of the base class, the window, the
// clean window, the static text, the rectangle, the rectangle's texture, the progress bar and the menu item; the others'
// classes aren't written yet, and a menu skips their items.

#include "ui/g_m_comps.h"

// The factories (PS2's names), in the original's order; a type not written yet is a comment with its global's address
// GLOBAL: GAME 0x6bd70c
G_Factory<GM_Component> GM_ComponentFactory;
// GM_NahradaFactory, 0x6bd6fc ('xxxx')
// GM_UpUpFactory, 0x6bd6ec ('upup', which CreateCompsByDef handles itself)
// GLOBAL: GAME 0x6bd6dc
G_Factory<GM_Window> GM_WindowFactory;
// GLOBAL: GAME 0x6bd6cc
G_Factory<GM_WindowClean> GM_WindowCleanFactory;
// GM_WindowScrollFactory, 0x6bd6bc ('wins')
// GLOBAL: GAME 0x6bd6ac
G_Factory<GM_StaticText> GM_StaticTextFactory;
// GM_StaticTextMemFactory, 0x6bd69c ('mtxt')
// GM_StaticTextNumFactory, 0x6bd68c ('ntxt')
// GM_StaticTextTimeFactory, 0x6bd67c ('htxt')
// GM_LineFactory, 0x6bd66c ('line')
// GLOBAL: GAME 0x6bd65c
G_Factory<GM_Rectangle> GM_RectangleFactory;
// GM_OrezavacFactory, 0x6bd64c ('orez')
// GLOBAL: GAME 0x6bd63c
G_Factory<GM_RectangleTexture> GM_RectangleTextureFactory;
// GLOBAL: GAME 0x6bd62c
G_Factory<GM_ProgressBar> GM_ProgressBarFactory;
// GLOBAL: GAME 0x6bd61c
G_Factory<GM_MenuItem> GM_MenuItemFactory;
// GM_MenuItemMemFactory, 0x6bd60c ('itmm')
// GM_MenuItemNumFactory, 0x6bd5fc ('iten')
// GM_RaceRoundItemFactory, 0x6bd5ec ('rari')
// GM_ButtonFactory, 0x6bd5dc ('butt')
// GM_ButtonCleanFactory, 0x6bd5cc ('butc')
// GM_SliderFactory, 0x6bd5bc ('slid')
// GM_SliderIntegerFactory, 0x6bd5ac ('sldi')
// GM_NetPlayerFactory, 0x6bd59c ('netp')
// GM_TextEditFactory, 0x6bd58c ('edit')
// GM_CheckBoxFactory, 0x6bd57c ('chck')
// GM_ButtonSelectFactory, 0x6bd56c ('bsel')
// GM_ButtonSelectBoolFactory, 0x6bd55c ('bslb')
// GM_EditNumFactory, 0x6bd54c ('edin')
