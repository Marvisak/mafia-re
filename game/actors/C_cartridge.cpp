// The cartridge cases (PS2 C_cartridge.cpp; the PC's file is most likely the one at 0x4307d0-0x430b9c, after C_car_ex.cpp
// as the name sorts, docs/game-files.md), whose code the PS2 build doesn't have: a bottle's physics, a sound of the
// material at each hit near the camera, and out of the game once the case rests, falls too fast or has hit 15 times.

#include "actors/C_cartridge.h"

#include "mission/Mission.h"
#include "mission/game.h"
#include "g_material.h"
#include "tools.h"

// GLOBAL: GAME 0x63d6c8
I3D_sound *C_cartridge::sounds[3];

// FUNCTION: GAME 0x4307d0
C_cartridge::C_cartridge()
{
    use_coll = false;
    unknown_6d = false;
    type = ACTOR_CARTRIDGE;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x430820
C_cartridge::~C_cartridge()
{
}

// The linker folded it with C_stone's, the same code
// FUNCTION: GAME 0x430830 FOLDED
bool C_cartridge::Init(I3D_frame *frm)
{
    if (!frm)
        return false;
    return C_bottle::Init(frm) ? true : false;
}

// The bottle's step, then the material's sound in a free one of the sounds where the case hit something or stops
// within 10 of the camera, and the case out of the game where it stops
// FUNCTION: GAME 0x430850
void C_cartridge::AI(uint time)
{
    C_bottle::AI(time);
    bool done = speed < .01f && fall_speed < .01f || fall_speed > 50.0f;
    if (bounces > 15)
        done = true;
    if (hit || done)
    {
        if ((act_mission->scene->GetActiveCamera()->GetWorldPos() - pos).Square() < 100.0f)
        {
            for (int i = 0; i < 3; i++)
            {
                if (sounds[i] && !sounds[i]->IsPlaying(false))
                {
                    if (sound_table.ConstructSound(sounds[i], shotgun_shell ?
                        materialList[hit_material].shell_sound : materialList[hit_material].cartridge_sound))
                    {
                        sounds[i]->SetPos(pos);
                        act_mission->scene->SetFrameSector(sounds[i]);
                        sounds[i]->Update();
                        sounds[i]->SetOn(true);
                    }
                    break;
                }
            }
        }
        if (done)
        {
            act_mission->game->InvalidateActor(this, false);
            act_mission->game->RemoveTemporaryActor(this);
        }
    }
}

// The base's: the original's jumps there
// FUNCTION: GAME 0x430a60
void C_cartridge::Update(uint time)
{
    C_bottle::Update(time);
}

// The bottle's, with a case's radius and axis, no states and every collision tested
// FUNCTION: GAME 0x430a70
void C_cartridge::GameInit()
{
    C_bottle::GameInit();
    axis = S_vector(0.0f, 1.0f, 0.0f);
    set_states = false;
    radius = .1f;
    test_all = true;
    unknown_65 = true;
}

// The frame's placement saved again first, so that the bottle's leaves the case where it lies: C_actor's
// InternalPlacementReset written out, without its test of the frame
// FUNCTION: GAME 0x430ad0
void C_cartridge::GameDone()
{
    saved_pos = frame->GetPos();
    saved_rot = frame->GetRot();
    saved_parent = frame->m_pParent;
    C_bottle::GameDone();
}

// FUNCTION: GAME 0x430b50
void C_cartridge::SetShotgunShell(bool on)
{
    shotgun_shell = on;
    if (on)
        bounce = .2f;
}

// The linker's folded copy of the out-of-line kind (0x4aa430)
int C_cartridge::SaveGameGetSize()
{
    return 0;
}
