// The actors' animations (PS2 C_animations.cpp, which has C_animations_machine_de_luxe; the PC's file is most likely
// the one at 0x40df70-0x411bdc, which starts with the tracks' initializer, docs/game-files.md). Not written yet: only
// what CloseSystem needs to link.
// Open: the tracks' destructor (0x40dfb0, registered by 0x40dfa0) tests and sets a guard byte (0x63d2d4) of its own.

#include "actors/C_animations.h"

// GLOBAL: GAME 0x63d2d8
std::vector<C_animations_machine_de_luxe::S_track> C_animations_machine_de_luxe::tracks;
