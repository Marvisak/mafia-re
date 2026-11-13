// The weapons put down (PS2 C_drop_in_weapon.cpp; the PC's file is most likely the one at 0x4435b0-0x443a2c,
// docs/game-files.md, between C_driver.cpp and C_ff_manager.cpp as PS2's sorts), which starts with the constructor: a
// physical thing the player can pick up through its using object, or a box the player put down (GAMEITEM_NONE, the
// box's number in the item's ammo_loaded), which goes back to the scripts' boxes once it rests.

#include "actors/C_drop_in_weapon.h"

#include "mission/Mission.h"
#include "mission/game.h"
#include "g_material.h"

// FUNCTION: GAME 0x4435b0
C_drop_in_weapon::C_drop_in_weapon()
{
    type = ACTOR_DROP_IN_WEAPON;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x443640
C_drop_in_weapon::~C_drop_in_weapon()
{
}

// The using object follows the frame, 0.4 m above it
// FUNCTION: GAME 0x443660
void C_drop_in_weapon::Update(uint time)
{
    C_PhThing::Update(time);
    using_object.pos = frame->GetWorldPos();
    using_object.pos.y += .4f;
}

// Once the thing rests (ACTSTATE_INACTIVE) the actor leaves the game: a weapon's frame stays as a using object of its
// own, which frees the frame, a box goes back to the scripts' boxes. Out of the game (ACTSTATE_NONE) a weapon's frame
// is unlinked and released.
// FUNCTION: GAME 0x4436c0
void C_drop_in_weapon::ChangeState()
{
    switch (act_state)
    {
    case ACTSTATE_INACTIVE:
        act_mission->game->InvalidateActor(this, false);
        act_mission->game->RemoveTemporaryActor(this);
        act_mission->game->using_object.DelObject(&using_object);
        if (item.id > GAMEITEM_NONE)
        {
            C_using_object::S_using_object *obj = new C_using_object::S_using_object;
            obj->pos = frame->GetWorldPos();
            obj->radius_2 = .5f;
            obj->matrix = NULL;
            obj->pos.y += .4f;
            obj->flags = USINGF_DELETE | USINGF_FRAME;
            obj->frame = frame;
            obj->actor = NULL;
            obj->item = item;
            obj->item.using_object = obj;
            act_mission->game->using_object.AddObject(obj);
        }
        else
        {
            act_mission->game->BoxPlacementAdd(frame, item.ammo_loaded);
            act_mission->game->BoxPlacementRecompile();
        }
        break;
    case ACTSTATE_NONE:
        act_mission->game->using_object.DelObject(&using_object);
        if (item.id > GAMEITEM_NONE && frame)
        {
            frame->LinkTo(NULL, 0);
            frame->Release();
            frame = NULL;
        }
        break;
    }
}

// A weapon's using object for the player, 0.4 m above the frame, with the actor's item
// FUNCTION: GAME 0x443870
void C_drop_in_weapon::GameInit()
{
    C_PhThing::GameInit();
    unknown_c0 = 0x80000;
    if (item.id > GAMEITEM_NONE)
    {
        using_object.pos = frame->GetWorldPos();
        using_object.radius_2 = .5f;
        using_object.pos.y += .4f;
        using_object.frame = frame;
        using_object.matrix = NULL;
        using_object.flags = 0;
        using_object.actor = this;
        using_object.item = item;
        using_object.item.using_object = &using_object;
        act_mission->game->using_object.AddObject(&using_object);
    }
}

// The frame stays where the thing lies: its placement is taken as the one the base's GameDone puts back. A weapon
// still in the game is unlinked and released.
// FUNCTION: GAME 0x443950
void C_drop_in_weapon::GameDone()
{
    if (frame)
    {
        saved_pos = frame->GetPos();
        saved_rot = frame->GetRot();
        saved_parent = frame->m_pParent;
    }
    C_PhThing::GameDone();
    if (item.id > GAMEITEM_NONE && act_state == ACTSTATE_ACTIVE && frame)
    {
        frame->LinkTo(NULL, 0);
        frame->Release();
    }
}
