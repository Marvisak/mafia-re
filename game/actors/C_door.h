#pragma once

#include "actors/C_actor.h"

// The doors (PS2 C_door, C_door.cpp), 0x1b8 bytes (PS2's 0x200). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_DOOR, and the functions the script calls, as stubs.
class C_door : public C_actor
{
public:
    // The door's state (PS2's type; the names are ours, from the script's DOOR_OPEN, which opens a closed door and
    // closes an open one)
    enum E_state
    {
        STATE_OPEN,
        STATE_CLOSED,
        STATE_OPENING,
        STATE_CLOSING
    };

    C_door();

    // PS2 names, which the script calls; the parameters' names are ours
    // Opens or closes the door for the actor (the return type isn't known: nothing reads it)
    void SetState(E_state state, C_actor *actor, bool unknown1, bool unknown2);
    void EnableUsingObject(bool enable);
    // How far the door is open, 0 to 1
    void SetOpenAngle(float open);

    // The name is ours: C_game::Init's, its stores in this order
    void ResetUnknown110()
    {
        unknown_114 = 0;
        unknown_118 = 0;
        unknown_110 = 0;
    }

    E_state state;                      // 0x70 (PS2 0xf0, the name is ours)
    uchar unknown_74[0x9c];             // 0x74
    int unknown_110;                    // 0x110
    int unknown_114;                    // 0x114
    int unknown_118;                    // 0x118
    uchar unknown_11c[9];               // 0x11c
    bool locked;                        // 0x125 (PS2 0x175), DOOR_LOCK's (the name is ours)
    uchar unknown_126[0x92];            // 0x126

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b790
    E_state GetState() const
    {
        return state;
    }
};
