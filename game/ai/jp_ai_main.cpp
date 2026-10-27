// The AI (PS2 jp_ai_main.cpp; the PC's file is most likely the one at 0x538580-0x538c7c, between jp_ai_groups.cpp and
// jp_ai_prislusnici.cpp as the names sort, docs/game-files.md), whose initializer constructs AIManager: the file has
// nothing but AIManager and the compiler-generated functions it brings, its constructor (0x538760) and destructor
// (0x5385c0) and what they and their unwinding call.

#include "ai/jp_ai_main.h"

// PS2 name
// GLOBAL: GAME 0x646c80
ai_manager AIManager;
