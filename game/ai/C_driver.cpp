// The cars' AI drivers (PS2 C_driver.cpp; the PC's file is the one at 0x43bd90-0x4435ac, docs/game-files.md), which
// starts with the constructor (0x43bd90) and the destructor. Written so far: the destructor, which the chases' Done
// calls.

#include "ai/C_driver.h"

// FUNCTION: GAME 0x43bee0
C_driver::~C_driver()
{
    unknown_160.clear();
}
