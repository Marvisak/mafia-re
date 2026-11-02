#pragma once

#include "Main.h"
#include "tools.h"

#include <vector>

class IJoyEffect;

// The force feedback (PS2 C_ff_manager, C_ff_manager.cpp; the PC's drives the joysticks' effects), 0xbc bytes. The
// cars call most of its methods. Only what InitSystem, CloseSystem and C_game's Init, Done and Tick use so far.
class C_ff_manager
{
public:
    // An effect (PS2's name, whose S_effect is another design): the PC's are 0x3c to 0x48 bytes, by kind, with a vtable
    // (0x623d58 the base's, its constructor 0x444440). What the rest holds isn't known yet; the names are ours.
    struct S_effect
    {
        uchar unknown_0[0x20];
        int unknown_20;                 // 0x20, from the constructor; one that is set doesn't wait
        uint priority;                  // 0x24, from the constructor: a higher one stops a playing effect
        bool unknown_28;                // 0x28, from the constructor
        bool waiting;                   // 0x29, false from the constructor, set where another effect kept it waiting
        uchar unknown_2a[6];            // 0x2a
        int id;                         // 0x30, which the manager's functions take
        IJoyEffect *effect;             // 0x34, NULL if the joystick couldn't create it
    };

    // The effects, which GameDone releases; the second's it stops first (the names are ours)
    std::vector<S_effect *> effects;    // 0x0
    std::vector<S_effect *> effects_2;  // 0x10
    int unknown_20;                     // 0x20
    // 0x24, its period 1000 from the constructor: Tick tries the waiting effects each second (the name is ours)
    S_timer wait_timer;
    // 0x2c and 0x2d, which of the effects C_game::Init creates (the names are ours): GameInit sets them where the
    // player's (forward's secondary binding) and the car's controls (steer left's) are on a joystick with force feedback
    bool human_effects;
    bool car_effects;
    uchar unknown_2e[0x46];
    bool unknown_74;                    // 0x74, cleared by C_game::Init
    uchar unknown_75[0x47];

    C_ff_manager();
    ~C_ff_manager();

    // The name is ours. InitSystem calls it once the joysticks are there.
    void Reset();
    // The name is ours. CloseSystem calls it before it closes the joysticks; it does nothing.
    void Close();
    // PS2 names: what C_game's Init and Done call
    void GameInit();
    void GameDone();
    // Starts the effect of the id, moved to effects_2: false if it doesn't start, where another one of a priority as
    // high plays (it then waits) (the name is ours)
    bool Start(int id);
    // The waiting effect of the highest priority, NULL for none (the name is ours)
    S_effect *GetWaiting();
    // The effects by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // Starts the waiting effects, the highest priority first, until one can't start (the name is ours)
    void StartWaiting();
};

extern C_ff_manager ff_manager;

// The empty functions of the out-of-line kind the linker folded into one (0x4aabe0): Close above, C_web_path::
// DrawConnections, C_human::HideLegs and ~C_animations_machine_de_luxe, named after the copy our linker keeps (foldfix)
// SYNTHETIC: GAME 0x4aabe0
// ?Death@C_actor@@UAEXXZ

// The destructor, folded with C_sledovacka's, named after the copy our linker keeps (foldfix)
// FUNCTION: GAME 0x444670
// ??1C_sledovacka@@QAE@XZ
