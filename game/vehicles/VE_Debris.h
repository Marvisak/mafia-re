#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class I3D_frame;

// The debris and the flashes of explosions (PS2 S_DEBRIS and S_FLASH, VE_Debris.cpp), 0x1c and 0x44 bytes (PS2's 0x20
// and 0x60), which the file's copy loops move. The members' names are ours. A piece of debris: its frame, which Done
// unlinks and releases, falls and turns by its roll until it falls faster than 20 m/s.
struct S_DEBRIS
{
    I3D_frame *frame;                   // 0x0 (PS2 0x10)
    S_vector velocity;                  // 0x4 (PS2 0x0)
    float roll;                         // 0x10 (PS2 0x14), SetDir's, kept within 0 to 2 pi
    float roll_speed;                   // 0x14 (PS2 0x18), per second
    bool alive;                         // 0x18 (PS2 0x1c), Tick removes the piece once it is cleared
};

// A flash: a line from its head to its tail, each moving at its speed under the gravity (the tail only after 0.05 s),
// both drifting
struct S_FLASH
{
    S_vector head_speed;                // 0x0 (PS2 0x0)
    S_vector tail_speed;                // 0xc (PS2 0x10)
    S_vector drift;                     // 0x18 (PS2 0x20)
    S_vector head;                      // 0x24 (PS2 0x30)
    S_vector tail;                      // 0x30 (PS2 0x40)
    float time;                         // 0x3c (PS2 0x50), how long it has been flying
    float life_time;                    // 0x40 (PS2 0x54)
};

// The explosions' debris (PS2 C_Debris, VE_Debris.cpp; the PC's file is the one at 0x564890-0x56615c, which has its
// methods), the game's 0x240, 0x2c bytes. PS2's has its gravity before the vectors. Its constructor and destructor are
// the compiler's.
class C_Debris
{
public:
    // PS2 names: C_game's Init and Done call them. Init takes the gravity's acceleration (the parameter's name is ours)
    bool Init(float gravity);
    // Releases the debris' frames and forgets the debris and the flashes
    bool Done();
    // The debris and the flashes by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // Debris thrown at pos (PS2 name; the parameters' names are ours)
    void GenerateDebris(const S_vector &pos, int num, float size, float speed, float time, const S_vector *dir);

    std::vector<S_DEBRIS> debris;       // 0x0 (PS2 0x10, gif::vector<S_DEBRIS>)
    std::vector<S_FLASH> flashes;       // 0x10 (PS2 0x1c)
    S_vector gravity;                   // 0x20 (PS2 0x0), downwards, set by Init (the name is ours)
};

// The debris' erase, which Tick's pop_back calls out of line
// TEMPLATE: GAME 0x5660e0
// std::vector<S_DEBRIS,std::allocator<S_DEBRIS> >::erase

// Its destructor's copy, which the game's destructor and its constructor's unwinding call (Mission.cpp's range), the
// same code as C_using_object's (tools.h), folded
// SYNTHETIC: GAME 0x5479e0
// ??1C_Debris@@QAE@XZ
