#pragma once

#include "actors/C_actor.h"
#include "camera/g_camera.h"

// The race cameras (PS2 C_racecam, c_racecam.cpp), 0x88 bytes (PS2's 0xa0), which C_mission::CreateActor creates for
// ACTOR_RACECAM.
// VTABLE: GAME 0x625950
class C_racecam : public C_actor
{
public:
    C_racecam();
    virtual ~C_racecam();
    // The vtable's other overrides (the slots' names are C_actor's)
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize()
    {
        return 0;
    }
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    // 0x70, the camera on the actor's frame, which GameInit gives the game's camera (the name is ours)
    G_RaceCamera race_camera;
};

// SYNTHETIC: GAME 0x596220
// C_racecam::`scalar deleting destructor'
