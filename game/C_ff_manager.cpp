// The force feedback (PS2 C_ff_manager.cpp; the PC's file is most likely the one at 0x444430-0x44525c, which starts
// with its constructor's helpers, docs/game-files.md). Written so far: Reset.

#include "C_ff_manager.h"

// FUNCTION: GAME 0x4446a0
void C_ff_manager::Reset()
{
    unknown_20 = 0;
}
