#pragma once

#include "Main.h"

#include <vector>

// The effects the game loads from tables\effects.tbl ("Effects.bin" in its messages), PC only (PS2's mission has a
// particle manager instead), 0x38 bytes. The class's and the members' names are ours. Only what C_mission's Init and
// UnInit use so far.

// One of the table's effects, 0x3c bytes, deleted through its virtual destructor
class C_effect
{
public:
    virtual ~C_effect();
};

class C_effects
{
public:
    std::vector<void *> unknown_0;      // 0x0
    std::vector<C_effect *> effects;    // 0x10
    std::vector<void *> unknown_20;     // 0x20
    int unknown_30;                     // 0x30
    int unknown_34;                     // 0x34

    C_effects();
    ~C_effects();

    // Reads tables\effects.tbl into effects
    void Load();
    // Deletes the effects
    void Clear();
};

// The destructor of a vector whose elements have none, which the linker folded for every such vector; the original
// kept this file's copy. C_mission's destructor calls it out of line for its models.
// TEMPLATE: GAME 0x44b220
// std::vector<C_effect *,std::allocator<C_effect *> >::~vector<C_effect *,std::allocator<C_effect *> >
