// The race cameras (PS2 c_racecam.cpp; the PC's file is the one at 0x596200-0x59641c, docs/game-files.md), which
// starts with the constructor: an actor whose frame is a camera of the race (G_RaceCamera), which the game's camera
// shows the race from while a championship race runs.

#include "camera/c_racecam.h"

#include "C_chunk.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/racing.h"

// FUNCTION: GAME 0x596200
C_racecam::C_racecam()
{
    type = ACTOR_RACECAM;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x596240
C_racecam::~C_racecam()
{
}

// The linker's folded copy (0x4aa490)
bool C_racecam::SaveGameSave(void **buf)
{
    return true;
}

// The linker's folded copy (0x4aa490)
bool C_racecam::SaveGameLoad(void **buf)
{
    return true;
}

// The camera on the frame, with its defaults and no gates yet (PS2's the same)
// FUNCTION: GAME 0x596250
bool C_racecam::Init(I3D_frame *frm)
{
    if (!frm)
        return false;
    if (!C_actor::Init(frm))
        return false;
    race_camera.unknown_4 = 70.0f;
    race_camera.unknown_8 = 70.0f;
    race_camera.unknown_c = 1.0f;
    race_camera.gate_from = 0;
    race_camera.gate_to = 0;
    race_camera.frame = frm;
    return true;
}

// FUNCTION: GAME 0x5962b0
bool C_racecam::SaveData(C_chunk *chunk)
{
    chunk->Write(&race_camera, sizeof(race_camera));
    return true;
}

// The camera read whole, its frame the actor's again (PS2's the same)
// FUNCTION: GAME 0x5962e0
bool C_racecam::LoadData(C_chunk *chunk)
{
    chunk->Read(&race_camera, sizeof(race_camera));
    race_camera.frame = frame;
    return true;
}

// FUNCTION: GAME 0x596360
void C_racecam::DuplicateFrom(C_actor *act)
{
    C_actor::DuplicateFrom(act);
    race_camera = ((C_racecam *)act)->race_camera;
}

// The camera's gates cleared, the camera given to the game's camera during a championship race (CRacing), and the
// actor out of the actors' ticks (PS2's the same)
// FUNCTION: GAME 0x596380
void C_racecam::GameInit()
{
    C_actor::GameInit();
    unknown_65 = true;
    race_camera.frame = frame;
    race_camera.gate_from = 0;
    race_camera.gate_to = 0;
    if (CRacing)
        act_mission->game->camera.AddRaceCamera(&race_camera);
    SetActState(ACTSTATE_NONE);
}

// FUNCTION: GAME 0x5963d0
void C_racecam::GameDone()
{
    C_actor::GameDone();
    act_mission->game->camera.RemoveRaceCamera(&race_camera);
}
