// More of the menus' components (PS2 g_m_comps2.cpp; the PC's file is most likely the one at 0x5dd660-0x5dfdac,
// docs/game-files.md): the factories of eight components' classes, PS2's combo box, combo window, list box, list of
// cars, view scroll, text input and big static text after one of the PC's own ('vidc'), then their classes' functions
// as in g_m_comps.cpp. Written so far: nothing; the factories are stubs (G_StubFactory, g_m_comps.h), which log each
// item a menu asks them for and make nothing, so the menu skips it.

#include "ui/g_m_comps.h"

// The factories (PS2's names; GM_VidcFactory's and its class's are ours, after the type), in the original's order, each
// comment the original's global
// 0x6bd78c
G_StubFactory GM_VidcFactory('vidc', "GM_Vidc");
// 0x6bd77c
G_StubFactory GM_ComboBoxFactory('comb', "GM_ComboBox");
// 0x6bd76c
G_StubFactory GM_ComboWindowFactory('cmbw', "GM_ComboWindow");
// 0x6bd75c
G_StubFactory GM_ListBoxFactory('list', "GM_ListBox");
// 0x6bd74c
G_StubFactory GM_ListCarsFactory('lstc', "GM_ListCars");
// 0x6bd73c
G_StubFactory GM_ViewScrollFactory('vscr', "GM_ViewScroll");
// 0x6bd72c
G_StubFactory GM_TextInputFactory('tinp', "GM_TextInput");
// 0x6bd71c
G_StubFactory GM_StaticTextBigFactory('stxb', "GM_StaticTextBig");
