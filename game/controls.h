#pragma once

#include "Main.h"

class IJoy;

// The player's controls (PC only, the names are ours): the keyboard, the mouse and the joysticks, which the menus and
// the game read. Main.cpp has the one object. Only what InitSystem uses so far.
class C_controls
{
public:
    // Takes IGraph and the joysticks InitJoy found; fails if one of them is missing
    bool Init(IGraph *igraph, IJoy **joys, int count);
};

extern C_controls controls;
