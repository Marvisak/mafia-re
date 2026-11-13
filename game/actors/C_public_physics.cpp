// The public physics (PS2 C_public_physics.cpp; the PC's file is the one at 0x47c320-0x47c53c, docs/game-files.md),
// whose constructor (0x47c3a0) comes after the initializer of its list of temporary ones.

#include "actors/C_public_physics.h"

#include "mission/Mission.h"
#include "mission/game.h"

// GLOBAL: GAME 0x63d78c
std::vector<C_public_physics *> C_public_physics::temporary_things;

// FUNCTION: GAME 0x47c3a0
C_public_physics::C_public_physics()
{
    type = ACTOR_PUBLIC_PHYSICS;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x47c3e0
C_public_physics::~C_public_physics()
{
}

// The base's, then the frame kept (PS2's the same). The PC also counts the things that come into the game while it
// runs: past MAX_TEMPORARY_THINGS the oldest is taken out of the game.
// FUNCTION: GAME 0x47c3f0
void C_public_physics::GameInit()
{
    C_PhThing::GameInit();
    unknown_65 = true;
    frame->m_RefCount++;
    if (act_mission->game->initialized)
    {
        bool found = false;
        for (int i = temporary_things.size(); i--;)
        {
            if (temporary_things[i] == this)
            {
                found = true;
                break;
            }
        }
        if (!found)
            temporary_things.push_back(this);
        if (temporary_things.size() > MAX_TEMPORARY_THINGS)
        {
            act_mission->game->InvalidateActor(temporary_things[0], false);
            act_mission->game->RemoveTemporaryActor(temporary_things[0]);
            temporary_things.erase(temporary_things.begin());
        }
    }
}

// The base's, then GameInit's reference released (PS2's the same)
// FUNCTION: GAME 0x47c4d0
void C_public_physics::GameDone()
{
    C_PhThing::GameDone();
    frame->Release();
}

// FUNCTION: GAME 0x47c4f0
bool C_public_physics::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    return C_PhThing::Collision(act, v1, v2, v3, f, frm);
}

// FUNCTION: GAME 0x47c500
bool C_public_physics::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    C_actor *act, uint u, I3D_frame *frm)
{
    return C_PhThing::Hit(type, v1, v2, v3, f, act, u, frm);
}
