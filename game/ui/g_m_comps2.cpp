// More of the menus' components (PS2 g_m_comps2.cpp; the PC's file is most likely the one at 0x5dd660-0x5dfdac,
// docs/game-files.md): the factories of eight components' classes, PS2's combo box, combo window, list box, list of
// cars, view scroll, text input and big static text after one of the PC's own ('vidc'), then their classes' functions
// as in g_m_comps.cpp. Written whole: the classes are in g_m_comps2.h.

// No precompiled header (#pragma hdrstop before the includes): the copies of each class's in-class functions follow
// its factory's Make, as the original's do, where under /YX they came after all the factories' functions.
#pragma hdrstop

#include "ui/g_m_comps2.h"

// The factories (PS2's names; GM_VidcFactory's and its class's are ours, after the type), in the original's order, each
// comment the original's global
// GLOBAL: GAME 0x6bd78c
G_Factory<GM_Vidc> GM_VidcFactory;
// GLOBAL: GAME 0x6bd77c
G_Factory<GM_ComboBox> GM_ComboBoxFactory;
// GLOBAL: GAME 0x6bd76c
G_Factory<GM_ComboWindow> GM_ComboWindowFactory;
// GLOBAL: GAME 0x6bd75c
G_Factory<GM_ListBox> GM_ListBoxFactory;
// GLOBAL: GAME 0x6bd74c
G_Factory<GM_ListCars> GM_ListCarsFactory;
// GLOBAL: GAME 0x6bd73c
G_Factory<GM_ViewScroll> GM_ViewScrollFactory;
// GLOBAL: GAME 0x6bd72c
G_Factory<GM_TextInput> GM_TextInputFactory;
// GLOBAL: GAME 0x6bd71c
G_Factory<GM_StaticTextBig> GM_StaticTextBigFactory;
