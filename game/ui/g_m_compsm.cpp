// The last file of the menus' components (PS2 g_m_compsm.cpp, which has the big static texts, the carousel and the
// color selection; the PC's file is most likely the one at 0x5dfdb0-0x5e063c, docs/game-files.md): the factory of the
// PC's 'grid', then its class's functions, and the components' shared scalar deleting destructor. Written so far:
// nothing; the factory is a stub (G_StubFactory, g_m_comps.h), which logs each item a menu asks it for and makes
// nothing, so the menu skips it.

#include "ui/g_m_comps.h"

// The factory (the name and its class's are ours, after the type), its comment the original's global
// 0x6bd79c
G_StubFactory GM_GridFactory('grid', "GM_Grid");
