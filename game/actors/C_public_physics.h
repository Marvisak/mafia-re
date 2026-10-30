#pragma once

#include "actors/C_PhThings.h"

// The public physics (PS2 C_public_physics, C_public_physics.cpp), a physical thing without members of its own,
// 0x38c bytes (PS2's 0x650). Only the constructor so far, which C_mission::CreateActor calls for
// ACTOR_PUBLIC_PHYSICS.
// VTABLE: GAME 0x6248f8
class C_public_physics : public C_PhThing
{
public:
    C_public_physics();
    virtual ~C_public_physics();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443650 FOLDED
    virtual void AI(uint time)
    {
        C_PhThing::AI(time);
    }
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x432090 FOLDED
    virtual void Update(uint time)
    {
        C_PhThing::Update(time);
    }
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void NetDirect(uint time)
    {
        C_PhThing::NetDirect(time);
    }
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443850 FOLDED
    virtual bool Init(I3D_frame *frm)
    {
        return C_PhThing::Init(frm);
    }
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void DuplicateFrom(C_actor *act)
    {
        C_PhThing::DuplicateFrom(act);
    }
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    // C_game::Init clears it (the name and the type are ours)
    static std::vector<uint> unknown_63d78c;
};

// SYNTHETIC: GAME 0x47c3c0
// C_public_physics::`scalar deleting destructor'
