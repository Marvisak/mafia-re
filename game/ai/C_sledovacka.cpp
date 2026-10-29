// The chases (PS2 C_sledovacka.cpp; the PC's file is the one at 0x4a0320-0x4aa2ac, docs/game-files.md), which starts
// with the constructor. Written so far: the constructor and the destructor, which the game's constructor and destructor
// call, and Init and Done, which its Init and Done call; what the script's commands call (RemoveCar, SetActionStatus)
// are stubs.

#include "ai/C_sledovacka.h"

#include "actors/C_actor.h"
#include "ai/C_driver.h"
#include "ai/road_web.h"
#include "mission/Mission.h"

#include "stub.h"

// FUNCTION: GAME 0x4a0320
C_sledovacka::C_sledovacka() :
    unknown_20(0)
{
}

// The same code as the force feedback's destructor, whose copy the linker kept (0x444670): the marker is that one's
C_sledovacka::~C_sledovacka()
{
}

// The bridges from the mission's last actor, as PS2's: the road node nearer to the bridge of the two ends of the link
// GetNearNode finds. The mission's roads and actors are locals, and the index is masked again where the node is taken,
// as PS2's.
// FUNCTION: GAME 0x4a0360
void C_sledovacka::Init()
{
    C_roads *roads = act_mission->roads;
    std::vector<C_actor *> &actors = act_mission->actors;
    for (int i = actors.size(); i--;)
    {
        if (actors[i]->GetType() == ACTOR_BRIDGE)
        {
            S_BRIDGE b;
            b.actor = actors[i];
            ushort n = roads->GetNearNode(actors[i]->GetWorldPos()) & NEARNODE_INDEX;
            S_vector d0 = roads->nodes[roads->links[n].unknown_14].pos - actors[i]->GetWorldPos();
            S_vector d1 = roads->nodes[roads->links[n].unknown_16].pos - actors[i]->GetWorldPos();
            if (d0.Square() < d1.Square())
                b.node = roads->links[n & NEARNODE_INDEX].unknown_14;
            else
                b.node = roads->links[n & NEARNODE_INDEX].unknown_16;
            bridges.push_back(b);
        }
    }
}

// The cars from the last one, as PS2's
// FUNCTION: GAME 0x4a0660
void C_sledovacka::Done()
{
    for (int i = elements.size(); i--;)
        delete elements[i].driver;
    elements.clear();
    bridges.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x4a1c30
void C_sledovacka::RemoveCar(C_car *car)
{
    StubLog("C_sledovacka::RemoveCar");
}

// STUB: GAME 0x4a86d0
void C_sledovacka::SetActionStatus(C_car *car, E_STATUS status)
{
    StubLog("C_sledovacka::SetActionStatus");
}

#pragma auto_inline(on)
