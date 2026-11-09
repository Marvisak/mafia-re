// The player (PS2 C_player.cpp; the PC's file is the one at 0x58f160-0x5961fc, docs/game-files.md), written whole:
// SetPlayerNaviPoint, then the constructor (0x58f170), the controls on foot (AI_walk), in the driver's seat
// (AI_drive) and in another seat (AI_car_fire), AI, the action button (Do_AB_OwnerNULL, Do_AB_OwnerCar), getting
// into or stealing a car (Can_UseCar), the sniper mode, the fade (SetAlpha and the alpha objects), the car damage,
// the controls' lock and the HUD's weapon name and aim. PS2's FireMagnetism, FireDriveFindTarget,
// FindNearestEnemyToTurn, ZkurvenaPrebijeciFunkceAdvanced, the GoToCar_* functions and NetDirect have no PC copy.
// Open: AI_walk's stack slots, AI_drive's square order in its normalizations and the horn's compare.

#include "actors/C_player.h"

#include "actors/C_detector.h"
#include "actors/C_pumper.h"

#include "ai/C_person_traffic.h"
#include "ai/jp_ai_entity.h"
#include "ai/jp_ai_main.h"
#include "camera/g_camera.h"
#include "collision/g_colls.h"
#include "g_material.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_func.h"
#include "ui/racing.h"
#include "IGraph/IGraph.h"
#include "vehicles/C_car.h"
#include "I3D_cache.h"

#include "stub.h"

extern S_fire_action _fire_action_aimed_[];

// The petrol pump's two choices in the item pick-up menu, the text database's (the names are ours)
#define TEXT_PUMP_LEAVE_CAR 0x7d5
#define TEXT_PUMP_REFUEL 0x7d6

// The sounds of the items picked up (Do_AB_OwnerNULL), the guns' (the items from the Colt Detective Special to the
// Mosin-Nagant) and the others' (the sound table's; the names are guesses)
#define SOUND_PICK_UP_WEAPON 0xee
#define SOUND_PICK_UP_ITEM 0xef

static void cbAlpha(I3D_frame *frm, std::vector<I3D_object *> &objects);
static void cbDelAlpha(I3D_frame *frm, std::vector<I3D_object *> &objects);

// GLOBAL: GAME 0x6478b0
bool C_player::can_car_damage;
// GLOBAL: GAME 0x6478b4
I3D_frame *C_player::navi_frame;

// The secondary bindings' values of the car's controls when the primary binding was last pressed, which AI_drive
// takes the secondary's value again from once it moved more than 0.5 away (PC only, the names are ours)
// GLOBAL: GAME 0x6478ac
static float secondary_accelerate;
// GLOBAL: GAME 0x6478a8
static float secondary_brake;
// GLOBAL: GAME 0x6478a4
static float secondary_clutch;
// GLOBAL: GAME 0x6478a0
static float secondary_steer_left;
// GLOBAL: GAME 0x64789c
static float secondary_steer_right;

// FUNCTION: GAME 0x58f160
void SetPlayerNaviPoint(I3D_frame *frm)
{
    C_player::navi_frame = frm;
}

// The properties' highest values: the energy 200, the limbs' 40 and the shooting's 1, as the player's voice 3
// FUNCTION: GAME 0x58f170
C_player::C_player()
{
    max_properties[HUMAN_PROPERTY_ENERGY_HAND_L] = 40.0f;
    max_properties[HUMAN_PROPERTY_ENERGY_HAND_R] = 40.0f;
    max_properties[HUMAN_PROPERTY_ENERGY_LEG_L] = 40.0f;
    max_properties[HUMAN_PROPERTY_ENERGY_LEG_R] = 40.0f;
    type = ACTOR_PLAYER;
    voice_type = 3;
    max_properties[HUMAN_PROPERTY_ENERGY] = 200.0f;
    max_properties[HUMAN_PROPERTY_SHOOTING] = 1.0f;
}

// FUNCTION: GAME 0x58f220
C_player::~C_player()
{
}

// A human who is the player's kind (human_type 1)
// FUNCTION: GAME 0x58f260
bool C_player::Init(I3D_frame *frm)
{
    if (!C_human::Init(frm))
        return false;
    human_type = 1;
    return true;
}

// FUNCTION: GAME 0x58f2a0
bool C_player::SaveData(C_chunk *chunk)
{
    return C_human::SaveData(chunk) ? true : false;
}

// FUNCTION: GAME 0x58f2c0
bool C_player::LoadData(C_chunk *chunk)
{
    return C_human::LoadData(chunk) ? true : false;
}

// FUNCTION: GAME 0x58f2e0
int C_player::SaveGameGetSize()
{
    return C_human::SaveGameGetSize() + sizeof(S_player_savegame);
}

// C_human's, then the player's struct
// FUNCTION: GAME 0x58f2f0
bool C_player::SaveGameSave(void **buf)
{
    if (!C_human::SaveGameSave(buf))
        return false;
    S_player_savegame *s = (S_player_savegame *)*buf;
    s->version = PLAYER_SAVEGAME_VERSION;
    s->primary_controls = primary_controls;
    s->turn_last = turn_last;
    s->turn_idle = turn_idle;
    s->turn_pending = turn_pending;
    s->fade_alpha = fade_alpha;
    s->fade_target = fade_target;
    s->recoil = recoil;
    s->aim_time = aim_time;
    s->horn = horn;
    s->always_run = always_run;
    s->controls_locked = controls_locked;
    *buf = (uchar *)*buf + sizeof(S_player_savegame);
    return true;
}

// C_human's, then the player's struct; the player fades in from nothing, and as the game's player he comes out of
// the sniper mode, the HUD's health and aim following
// FUNCTION: GAME 0x58f380
bool C_player::SaveGameLoad(void **buf)
{
    if (!C_human::SaveGameLoad(buf))
        return false;
    S_player_savegame *s = (S_player_savegame *)*buf;
    if (s->version != PLAYER_SAVEGAME_VERSION)
        return false;
    primary_controls = s->primary_controls;
    turn_last = s->turn_last;
    turn_idle = s->turn_idle;
    turn_pending = s->turn_pending;
    recoil = s->recoil;
    aim_time = s->aim_time;
    horn = s->horn;
    always_run = s->always_run;
    controls_locked = s->controls_locked;
    fade_alpha = 0.0f;
    fade_target = 1.0f;
    *buf = (uchar *)*buf + sizeof(S_player_savegame);
    if (act_mission->game->player == this)
    {
        if (act_mission->game->camera.mode == CAMERA_CP_SNIPER)
            SetSniperMode(false);
        if (properties[HUMAN_PROPERTY_ENERGY] > max_properties[HUMAN_PROPERTY_ENERGY])
            properties[HUMAN_PROPERTY_ENERGY] = max_properties[HUMAN_PROPERTY_ENERGY];
        uint lives = (int)(properties[HUMAN_PROPERTY_ENERGY] / max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
        if (alive && lives < 1)
            lives = 1;
        mafiaIndicators.PlayerSetLives(lives);
        IndicatorSetAimed();
    }
    return true;
}

// The player on foot, by the controls' values and their presses (C_controls::GetValues'): on use_timer's period the
// HUD's mark of an object to use within reach, the thief indicator's bar while a throw is held, then, alive and
// standing, walking or seated: walk or run (always_run, CTRL_WALK inverting it), the four directions and the side roll
// (a side key alone tapped twice within 200 ms jumps to that side, the profile's side_roll), the mouse's look up and
// down (the tilt) and its turn (at most the profile's aiming_speed_x a tick, the turn animations while it turns), the
// walks, jump, the city map, the inventory's menu, hide, drop, the weapons' cycle, the horn's sound, fire (a firearm
// aims along the camera through the collisions of actors, to the face of the actor's model there), sniper mode,
// reload, toggle run and crouch
// FUNCTION: GAME 0x58f4b0
void C_player::AI_walk(uint time, float *values, float *pressed)
{
    if (use_timer.Tick(time))
    {
        S_vector d = frame->GetWorldDir();
        d.Normalize();
        d *= .8f;
        d.y += .9f;
        if (act_mission->GetGame()->using_object.IsAnyObjectNear(frame->GetWorldPos() + d, frame->GetWorldDir(),
                frame->GetParent()))
            mafiaIndicators.AddFlags(INDF_ACTION);
        else
            mafiaIndicators.RemoveFlags(INDF_ACTION);
    }
    if (throw_swung)
    {
        int t = act_mission->GetGame()->GetGameTime() - throw_time;
        if (t > 400)
        {
            t -= 400;
            mafiaIndicators.AddFlags(INDF_THIEF);
            if (t > 1600)
                t = 1600;
            float f = t * (1.0f / 1600);
            mafiaIndicators.ThiefIndicatorSetValue(f);
        }
    }
    if (!alive)
        return;
    if (action_state != HUMAN_ACTION_STAND && action_state != HUMAN_ACTION_WALK && action_state != HUMAN_ACTION_CAR)
        return;

    bool walk = !always_run;
    if (values[CTRL_WALK] > .25f || values[CTRL_WALK + CTRL_SECONDARY] > .25f)
        walk = !walk;
    bool forward = values[CTRL_FORWARD] > .25f || values[CTRL_FORWARD + CTRL_SECONDARY] > .25f;
    bool backward = values[CTRL_BACKWARD] > .25f || values[CTRL_BACKWARD + CTRL_SECONDARY] > .25f;
    bool left = values[CTRL_LEFT] > .25f || values[CTRL_LEFT + CTRL_SECONDARY] > .25f;
    bool right = values[CTRL_RIGHT] > .25f || values[CTRL_RIGHT + CTRL_SECONDARY] > .25f;
    if (!walk && move_ratio < .5f)
        walk = true;

    // The side roll: a side key alone pressed, let go and pressed again within 200 ms
    bool idle = !forward && !backward && !left && !right;
    bool side = !forward && !backward && (left && !right || !left && right);
    if (side)
    {
        if (side_step_state > 0)
        {
            if (side_step_left && right || !side_step_left && left)
            {
                side = false;
                side_step_state = 0;
            }
        }
        else
            side_step_left = left;
    }
    if (mafiaLoadSave.settings.side_roll)
    {
        switch (side_step_state)
        {
        case 0:
            if (side)
            {
                side_step_state = 1;
                side_step_time = act_mission->GetGame()->GetGameTime();
            }
            break;
        case 1:
            if (idle)
            {
                uint t = act_mission->GetGame()->GetGameTime();
                side_step_state = t - side_step_time > 200 ? 0 : 2;
                side_step_time = t;
            }
            else if (!side)
            {
                side_step_state = 0;
                side_step_time = act_mission->GetGame()->GetGameTime();
            }
            break;
        case 2:
            if (side)
            {
                uint t = act_mission->GetGame()->GetGameTime();
                side_step_state = t - side_step_time > 200 ? 0 : 3;
                side_step_time = t;
                if (side_step_state == 3 && pressed[CTRL_JUMP] < .25f && pressed[CTRL_JUMP + CTRL_SECONDARY] < .25f)
                {
                    anim_id = left ? ANIM_STRAFL1 : ANIM_STRAFR1;
                    pressed[CTRL_JUMP] = 1.0f;
                }
            }
            else if (!idle)
            {
                side_step_state = 0;
                side_step_time = act_mission->GetGame()->GetGameTime();
            }
            break;
        case 3:
            if (!side)
                side_step_state = 0;
            break;
        }
    }
    else
        side_step_state = 0;
    if (right && left)
    {
        left = false;
        right = false;
    }
    if (forward && backward)
    {
        backward = false;
        forward = false;
    }
    Do_Breath();

    // The mouse: up and down the tilt, sideways the turn
    float y = mafiaLoadSave.settings.aiming_sensitivity_y * values[CTRL_AIM + CTRL_SECONDARY];
    if (mafiaLoadSave.settings.mouse_inverted)
        y *= -1.0f;
    if (act_mission->GetGame()->GetCamera().mode == CAMERA_CP_SNIPER)
        y *= .05f;
    tilt += y;
    if (tilt > .85f)
        tilt = .85f;
    else if (tilt < .1f)
        tilt = .1f;
    float dt = time * .001f;
    float x = mafiaLoadSave.settings.aiming_sensitivity_x * values[CTRL_AIM] * 5.0f;
    if (act_mission->GetGame()->GetCamera().mode == CAMERA_CP_SNIPER)
        x *= .05f;
    bool turning = anim_id == ANIM_LEFT1 || anim_id == ANIM_RIGHT1;
    float ax = (float)fabs(x);
    if (5.0f * (float)fabs(y) > ax)
        turning = turning || ax > .04f;
    else
        turning = turning || ax > .02f;
    if (x > 0.0f && turn_last < 0.0f || x < 0.0f && turn_last > 0.0f)
        turn_pending = x;
    else
        turn_pending += x;
    if (x == 0.0f)
        turn_idle += dt;
    else
    {
        turn_idle = 0.0f;
        turn_last = x;
    }
    float turn = turn_pending;
    float max = mafiaLoadSave.settings.aiming_speed_x * dt * 40.0f;
    if (fabs(turn) > max)
        turn = turn > 0.0f ? max : -max;
    turn_pending -= turn;
    float angle = -turn;
    if (turn_idle < .12f)
    {
        if (turning)
            next_anim_id = turn_last < 0.0f ? ANIM_LEFT1 : ANIM_RIGHT1;
    }
    else
        turn_pending = 0.0f;
    if (angle != 0.0f)
    {
        S_matrix m;
        m.Identity();
        float c = (float)cos(angle);
        float s = (float)sin(angle);
        m(0, 0) = m(2, 2) = c;
        m(2, 0) = -(m(0, 2) = s);
        dir = dir.RotateByMatrix(m);
    }

    if (forward)
    {
        if (left)
            Go_FrontLeft(!walk);
        else if (right)
            Go_FrontRight(!walk);
        else
            Go_Front(!walk);
    }
    else if (backward)
    {
        if (left)
            Go_BackLeft();
        else if (right)
            Go_BackRight();
        else
            Go_Back();
    }
    else if (left)
        Go_Left(!walk);
    else if (right)
        Go_Right(!walk);
    if (pressed[CTRL_JUMP] > .25f || pressed[CTRL_JUMP + CTRL_SECONDARY] > .25f)
        Do_Jump();
    if (values[CTRL_MAP] != 0.0f)
        mafiaIndicators.AddFlags(INDF_MAP);
    else
        mafiaIndicators.RemoveFlags(INDF_MAP);

    // The inventory's menu, hide and drop, the weapons' cycle
    if (pressed[CTRL_INVENTORY] > .25f && !box && !dead_body)
    {
        std::vector<S_GameItem> dropped;
        uint id = inventory.GetItemInRightHand(NULL);
        GM_Menu::ExecuteMenu(new GM_Inventory(&inventory, &dropped), false, true);
        if (id != inventory.GetItemInRightHand(NULL))
            Do_ChangeWeapon(HUMAN_WEAPON_ON_OFF, dropped.size() > 0);
        ShowWeaponName();
        if (dropped.size())
            DropOutItems(dropped);
    }
    if (pressed[CTRL_HIDE_WEAPON] > .25f)
    {
        if (box)
            Do_FuckingBoxDrop();
        else if (dead_body)
            Do_DeadBodyDrop();
        else
        {
            Do_Holster();
            ShowWeaponName();
        }
    }
    if (pressed[CTRL_DROP_WEAPON] > .25f)
    {
        if (box)
            Do_FuckingBoxDrop();
        else if (dead_body)
            Do_DeadBodyDrop();
        else
        {
            Do_WeaponDrop();
            ShowWeaponName();
        }
    }
    if ((pressed[CTRL_CYCLE_UP] > .25f || pressed[CTRL_CYCLE_UP + CTRL_SECONDARY] > .25f) && !box && !dead_body)
    {
        std::vector<S_GameItem> dropped;
        uint id = inventory.GetItemInRightHand(NULL);
        inventory.CycleUp(&dropped);
        if (id != inventory.GetItemInRightHand(NULL))
            Do_ChangeWeapon(HUMAN_WEAPON_ON_OFF, dropped.size() > 0);
        ShowWeaponName();
        if (dropped.size())
            DropOutItems(dropped);
    }
    if ((pressed[CTRL_CYCLE_DOWN] > .25f || pressed[CTRL_CYCLE_DOWN + CTRL_SECONDARY] > .25f) && !box && !dead_body)
    {
        std::vector<S_GameItem> dropped;
        uint id = inventory.GetItemInRightHand(NULL);
        inventory.CycleDown(&dropped);
        if (id != inventory.GetItemInRightHand(NULL))
            Do_ChangeWeapon(HUMAN_WEAPON_ON_OFF, dropped.size() > 0);
        ShowWeaponName();
        if (dropped.size())
            DropOutItems(dropped);
    }

    // The horn's control on foot: its sound, at most every 2 s
    if (pressed[CTRL_HORN] != 0.0f || pressed[CTRL_HORN + CTRL_SECONDARY] != 0.0f)
    {
        if (act_mission->GetGame()->GetGameTime() - horn_time > 2000)
        {
            horn_time = act_mission->GetGame()->GetGameTime();
            if (!horn_sound)
            {
                horn_sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
                if (horn_sound)
                {
                    horn_sound->SetOn(false);
                    horn_sound->SetPos(S_vector(0.0f, 1.5f, 0.0f));
                    horn_sound->SetVolume(1.0f);
                    horn_sound->SetOutVol(0.0f);
                    horn_sound->SetLoop(false);
                    horn_sound->SetSoundType(I3DSOUND_POINT);
                    horn_sound->SetCone(PI * 2.0f, PI * 2.0f);
                    horn_sound->SetRange(5.0f, 15.0f, 1.0f, 0.0f);
                    horn_sound->LinkTo(frame, 0);
                    horn_sound->Update();
                    // STRING: GAME 0x636ebc
                    if (sound_cache.Open(horn_sound, "7b_zip.wav", 0, NULL, NULL, NULL) != I3D_OK)
                    {
                        horn_sound->Release();
                        horn_sound = NULL;
                    }
                }
            }
            if (horn_sound)
                horn_sound->SetOn(true);
        }
    }

    // Fire
    if ((values[CTRL_FIRE] > .25f || values[CTRL_FIRE + CTRL_SECONDARY] > .25f) && !dead_body && !box && alive &&
        (action_state == HUMAN_ACTION_STAND || action_state == HUMAN_ACTION_WALK || action_state == HUMAN_ACTION_CAR))
    {
        switch (weapon_kind)
        {
        case WEAPON_KIND_PISTOL:
        case WEAPON_KIND_RIFLE:
        case WEAPON_KIND_SMG:
            if (shot_frame && inventory.GetAmmo(NULL))
            {
                if (!mafiaIndicators.IsShown(INDF_CROSSHAIR))
                    IndicatorSetAimed();
                loading_shells = false;
                if (fire_action[fire_step].action == FIRE_ACTION_END)
                {
                    anim_machine.DelExtendedAnim(4.0f);
                    // The camera's aim, 100 m, through the humans' collisions (those of an actor) to the face its
                    // model has there, at most 10 times
                    I3D_camera *cam = act_mission->GetScene()->GetActiveCamera();
                    S_vector from = cam->GetWorldPos();
                    S_vector dir = cam->GetWorldDir();
                    dir.Normalize();
                    dir *= 100.0f;
                    S_vector hit_pos;
                    int i = 0;
                    do
                    {
                        i++;
                        gifCollision.DynSetNextCollOwnerMult(colls, num_colls, NULL);
                        g_collision_header *coll = (g_collision_header *)gifCollision.TestLineH(from, dir, &hit_pos,
                            NULL, COLF_UNKNOWN_1, COLF2_UNKNOWN_200000 | COLF2_UNKNOWN_400000);
                        gifCollision.DynSetNextCollOwner(NULL, NULL);
                        if (!coll)
                        {
                            hit_pos = from + dir;
                            break;
                        }
                        if (!(coll->flags & COLLHF_UNKNOWN_40) || !coll->owner)
                            break;
                        I3D_COLLISION col;
                        if (act_mission->GetScene()->TestColHierarchy(from, dir, coll->frame, col, 0, NULL, 0, NULL))
                        {
                            S_vector n = dir;
                            n.Normalize();
                            n *= col.dist;
                            hit_pos = from + n;
                            break;
                        }
                        float rest = dir.Magnitude() - (hit_pos - from).Magnitude() - .5f;
                        dir.Normalize();
                        from = hit_pos + dir * .5f;
                        dir *= rest;
                    } while (i < 10);
                    // In the sniper mode and on a rail vehicle along the camera's aim from the weapon
                    if (act_mission->GetGame()->GetCamera().mode == CAMERA_CP_SNIPER)
                        from = shot_frame->GetWorldPos() + cam->GetWorldDir() * 100.0f;
                    else if (car && car->type == ACTOR_RAILWAY)
                        from = shot_frame->GetWorldPos() + cam->GetWorldDir() * 100.0f;
                    else
                        from = hit_pos;
                    Do_Shoot(true, &from);
                    int spare;
                    uint ammo = inventory.GetAmmo(&spare);
                    mafiaIndicators.PlayerSetAmmo(ammo, spare);
                }
            }
            else if (!inventory.GetAmmo(NULL))
                Shooting();
            break;
        case WEAPON_KIND_THROWN:
        {
            S_vector d = act_mission->GetScene()->GetActiveCamera()->GetWorldDir();
            Do_Shoot(true, &d);
            break;
        }
        case WEAPON_KIND_FISTS:
        case WEAPON_KIND_KNUCKLEDUSTER:
        case WEAPON_KIND_KNIFE:
        case WEAPON_KIND_BAT:
            // A strike, the thief indicator's bar filling while it is held
            if (strike_anim_time < 0)
            {
                Do_Shoot(true, NULL);
                int t = act_mission->GetGame()->GetGameTime() - strike_time;
                if (t > 200)
                {
                    t -= 200;
                    mafiaIndicators.AddFlags(INDF_THIEF);
                    if (t > 300)
                        t = 300;
                    mafiaIndicators.ThiefIndicatorSetValue(t * (1.0f / 300));
                }
            }
            break;
        }
    }
    else
    {
        Do_Shoot(false, NULL);
        mafiaIndicators.RemoveFlags(INDF_THIEF);
    }
    if (pressed[CTRL_SNIPER] > .25f && !box && !dead_body && inventory.GetWeaponInRightHand()->flags & ITEMF_SCOPE)
        SetSniperMode(act_mission->GetGame()->GetCamera().mode != CAMERA_CP_SNIPER);
    if (pressed[CTRL_RELOAD] > .25f && !box && !dead_body)
        Do_Reload();
    if (pressed[CTRL_TOGGLE_RUN] > .25f)
        always_run = !always_run;
    bool crouch = values[CTRL_CROUCH] > .25f || values[CTRL_CROUCH + CTRL_SECONDARY] > .25f;
    if (car && car->type == ACTOR_CAR && seat == -1)
        crouch = true;
    Do_Crouched(crouch);
    if (crouch && !aimed)
        Do_Aimed(true);
}

// The player in the driver's seat (PS2 name; the PC's code is its own): the engine started by the throttle or the
// brake (an empty tank makes him complain, once a car), the car's controls with the primary binding winning over the
// secondary (primary_controls), the camera's look around, the handbrake, the gears, the horn (the traffic hears it, the
// script's event fires), the tachometer, the damage bar, the gearing, the speed limiter, the city map, the inventory
// menu and the weapons. With a pistol fire shoots out of the window at what the camera's line hits (through what a
// collision of an actor's covers but its faces don't), the other weapons aim with the free look; aimed, throw_dir is
// where the camera's line hits, 100 m ahead at the most.
// FUNCTION: GAME 0x5917f0
void C_player::AI_drive(uint time, float *values, float *pressed)
{
    if (!IsReady())
        return;
    // The primary bindings' values first, then the secondary's: the original's order of the loads
    float accelerate = values[CTRL_ACCELERATE];
    float brake = values[CTRL_BRAKE];
    float steer_left = values[CTRL_STEER_LEFT];
    float steer_right = values[CTRL_STEER_RIGHT];
    float clutch = values[CTRL_CLUTCH];
    float accelerate2 = values[CTRL_ACCELERATE + CTRL_SECONDARY];
    float brake2 = values[CTRL_BRAKE + CTRL_SECONDARY];
    float steer_left2 = values[CTRL_STEER_LEFT + CTRL_SECONDARY];
    float steer_right2 = values[CTRL_STEER_RIGHT + CTRL_SECONDARY];
    float clutch2 = values[CTRL_CLUTCH + CTRL_SECONDARY];
    if (!car->engine_on && (values[CTRL_ACCELERATE] > 0.25f || values[CTRL_ACCELERATE + CTRL_SECONDARY] > 0.25f ||
        values[CTRL_BRAKE] > 0.25f || values[CTRL_BRAKE + CTRL_SECONDARY] > 0.25f))
    {
        car->SetEngineOn(true, false);
        if (car->tank_fuel == 0.0f && no_fuel_car != car)
        {
            NewScream(rnd(2) + VOICE_NO_FUEL);
            no_fuel_car = car;
        }
    }
    // The secondary binding's value counts only while the primary one is off and, after a press of the primary,
    // once it moved more than 0.5 from where it was then
    if (accelerate != 0.0f)
    {
        primary_controls |= PRIMARY_ACCELERATE;
        secondary_accelerate = accelerate2;
    }
    else if (primary_controls & PRIMARY_ACCELERATE)
    {
        if ((float)fabs(accelerate2 - secondary_accelerate) > 0.5f)
            primary_controls &= ~PRIMARY_ACCELERATE;
    }
    else
        accelerate = accelerate2;
    if (brake != 0.0f)
    {
        primary_controls |= PRIMARY_BRAKE;
        secondary_brake = brake2;
    }
    else if (primary_controls & PRIMARY_BRAKE)
    {
        if ((float)fabs(brake2 - secondary_brake) > 0.5f)
            primary_controls &= ~PRIMARY_BRAKE;
    }
    else
        brake = brake2;
    if (clutch != 0.0f)
    {
        primary_controls |= PRIMARY_CLUTCH;
        secondary_clutch = clutch2;
    }
    else if (primary_controls & PRIMARY_CLUTCH)
    {
        if ((float)fabs(clutch2 - secondary_clutch) > 0.5f)
            primary_controls &= ~PRIMARY_CLUTCH;
    }
    else
        clutch = clutch2;
    if (steer_left == 0.0f && steer_right == 0.0f)
    {
        if (primary_controls & PRIMARY_STEER)
        {
            if ((float)fabs(steer_left2 - secondary_steer_left) > 0.5f ||
                (float)fabs(steer_right2 - secondary_steer_right) > 0.5f)
                primary_controls &= ~PRIMARY_STEER;
        }
        else
        {
            steer_left = steer_left2;
            steer_right = steer_right2;
        }
    }
    else
    {
        primary_controls |= PRIMARY_STEER;
        secondary_steer_left = steer_left2;
        secondary_steer_right = steer_right2;
    }
    C_car *c = car;
    c->SetAnalogInputs(!(primary_controls & PRIMARY_STEER), !(primary_controls & PRIMARY_ACCELERATE),
        !(primary_controls & PRIMARY_BRAKE), !(primary_controls & PRIMARY_CLUTCH));
    c->SetPower(accelerate, true);
    c->SetBrake(brake);
    c->SetSteer(steer_right - steer_left);
    c->SetClutch(1.0f - clutch);
    float look_left = values[CTRL_LOOK_LEFT] > 0.25f || values[CTRL_LOOK_LEFT + CTRL_SECONDARY] > 0.25f;
    float look_right = values[CTRL_LOOK_RIGHT] > 0.25f || values[CTRL_LOOK_RIGHT + CTRL_SECONDARY] > 0.25f;
    act_mission->game->camera.LookAround(look_right > 0.5f ? (look_left > 0.5f ? LOOK_BEHIND : LOOK_RIGHT) :
        (look_left > 0.5f ? LOOK_LEFT : LOOK_AHEAD));
    bool handbrake = values[CTRL_HANDBRAKE] != 0.0f || values[CTRL_HANDBRAKE + CTRL_SECONDARY] != 0.0f;
    c->SetHandbrake(handbrake);
    if (pressed[CTRL_GEAR_UP] != 0.0f || pressed[CTRL_GEAR_UP + CTRL_SECONDARY] != 0.0f)
        c->SetGear(c->gear + 1);
    if (pressed[CTRL_GEAR_DOWN] != 0.0f || pressed[CTRL_GEAR_DOWN + CTRL_SECONDARY] != 0.0f)
        c->SetGear(c->gear - 1);
    if (horn)
    {
        if (values[CTRL_HORN] == 0.0f && values[CTRL_HORN + CTRL_SECONDARY] == 0.0f)
        {
            horn = false;
            c->horn = false;
        }
    }
    else if (values[CTRL_HORN] != 0.0f || values[CTRL_HORN + CTRL_SECONDARY] != 0.0f)
    {
        horn = true;
        c->horn = true;
        C_traffic_generator::t_horn += 1000;
        if (C_traffic_generator::t_horn > 1000)
            C_traffic_generator::t_horn = 1000;
        act_mission->game->PHorn_Horn();
    }
    mafiaIndicators.TachometrSetGear(car->gear, car->GetAutoGearing());
    mafiaIndicators.TachometrSetValues(car->GetSpeed(), car->GetRotations() / car->GetMaxRotations(),
        car->GetTankFuel() / car->GetTankSize(), car->GetOdometer());
    if (can_car_damage)
        mafiaIndicators.car_damage = 1.0f - car->ShowCarEnergy();
    if (pressed[CTRL_GEARING] > 0.25f)
    {
        manual_gearing = !manual_gearing;
        car->SetAutoGearing(!manual_gearing);
    }
    if (!CRacing && (pressed[CTRL_SPEED_LIMITER] > 0.25f || pressed[CTRL_SPEED_LIMITER + CTRL_SECONDARY] > 0.25f))
    {
        if (car->speed_limit > 800.0f)
        {
            car->SetSpeedLimit(16.0f);
            speed_limiter = true;
            mafiaIndicators.show_flags |= INDF_SPEED_LIMITER;
        }
        else
        {
            car->SetSpeedLimit(1000.0f);
            speed_limiter = false;
            mafiaIndicators.show_flags &= ~INDF_SPEED_LIMITER;
        }
    }
    if (values[CTRL_MAP] != 0.0f)
        mafiaIndicators.show_flags |= INDF_MAP;
    else
        mafiaIndicators.show_flags &= ~INDF_MAP;
    if (pressed[CTRL_INVENTORY] > 0.25f && !CRacing)
    {
        std::vector<S_GameItem> dropped;
        GM_Menu::ExecuteMenu(new GM_Inventory(&inventory, &dropped), false, true);
        ChangeWeaponModel();
        ShowWeaponName();
        if (aimed)
            Do_Aimed(false);
    }
    if (pressed[CTRL_CYCLE_UP] > 0.25f || pressed[CTRL_CYCLE_UP + CTRL_SECONDARY] > 0.25f)
    {
        std::vector<S_GameItem> dropped;
        inventory.CycleUp(&dropped);
        ChangeWeaponModel();
        ShowWeaponName();
        if (aimed)
            Do_Aimed(false);
    }
    if (pressed[CTRL_CYCLE_DOWN] > 0.25f || pressed[CTRL_CYCLE_DOWN + CTRL_SECONDARY] > 0.25f)
    {
        std::vector<S_GameItem> dropped;
        inventory.CycleDown(&dropped);
        ChangeWeaponModel();
        ShowWeaponName();
        if (aimed)
            Do_Aimed(false);
    }
    if (pressed[CTRL_HIDE_WEAPON] > 0.25f)
    {
        std::vector<S_GameItem> dropped;
        inventory.Holster(&dropped, true, true);
        ChangeWeaponModel();
        ShowWeaponName();
        if (aimed)
            Do_Aimed(false);
    }
    if (pressed[CTRL_RELOAD] > 0.25f)
        Do_Reload();
    if (!(values[CTRL_FIRE] > 0.25f) && !(values[CTRL_FIRE + CTRL_SECONDARY] > 0.25f))
        Do_Shoot(false, NULL);
    else
    {
        switch (weapon_kind)
        {
        case WEAPON_KIND_PISTOL:
            if (shot_frame && fire_action[fire_step].action == FIRE_ACTION_END)
            {
                I3D_camera *cam = act_mission->scene->GetActiveCamera();
                S_vector pos = cam->GetWorldPos();
                // Normalized again: the original's second normalization squares the source's members and puts the
                // reciprocal into sq's slot, SetNormalized's shape
                S_vector dir;
                dir.SetNormalized(cam->GetWorldDir());
                dir *= 100.0f;
                // The line goes on past an actor's collision whose faces it misses (a do-while: VC6 doesn't rotate
                // it, the original jumps back to the test; the other loop forms rotate or lay the miss out inline)
                S_vector hit;
                do
                {
                    g_collision_header *coll = (g_collision_header *)gifCollision.TestLineH(pos, dir, &hit, NULL,
                        COLF_UNKNOWN_1, COLF2_UNKNOWN_200000 | COLF2_UNKNOWN_400000);
                    if (!coll)
                    {
                        hit = pos + dir;
                        break;
                    }
                    if (!(coll->flags & COLLHF_UNKNOWN_40) || !coll->owner)
                        break;
                    I3D_COLLISION col;
                    if (act_mission->scene->TestColHierarchy(pos, dir, coll->frame, col, 0, NULL, 0, NULL))
                        break;
                    float len = dir.Magnitude() - (hit - pos).Magnitude() - 0.5f;
                    dir.Normalize();
                    pos = hit + dir * 0.5f;
                    dir *= len;
                }
                while (true);
                // The target in pos, whose slot the original's copy takes
                pos = hit;
                Do_Shoot(true, &pos);
                aim_time = 300000;
                int spare;
                uint ammo = inventory.GetAmmo(&spare);
                mafiaIndicators.PlayerSetAmmo(ammo, spare);
            }
            break;
        case WEAPON_KIND_FISTS:
        case WEAPON_KIND_RIFLE:
        case WEAPON_KIND_SMG:
        case WEAPON_KIND_THROWN:
        case WEAPON_KIND_KNUCKLEDUSTER:
        case WEAPON_KIND_KNIFE:
        case WEAPON_KIND_BAT:
            if (!CRacing && !aimed)
            {
                aim_time = 300000;
                Do_Aimed(true);
                act_mission->game->camera.EnableFreeLook(true, true);
                car_free_look = true;
            }
            break;
        }
    }
    if (aimed)
    {
        I3D_camera *cam = act_mission->scene->GetActiveCamera();
        S_vector dir = cam->GetWorldDir();
        S_vector pos = cam->GetWorldPos();
        dir *= 100.0f;
        if (!gifCollision.TestLineH(pos, dir, &throw_dir, NULL, COLF_UNKNOWN_1,
            COLF2_UNKNOWN_200000 | COLF2_UNKNOWN_400000))
            throw_dir = pos + dir;
    }
}

// The player in a car's other seat (PS2 name): the city map while CTRL_MAP is held, the inventory's menu (not in a
// race), the weapon cycled or holstered, the reload; with the fire held a pistol or a Thompson shoots where the camera
// looks, at most 100 m on (on through the frames of the collisions that let shots through, COLLHF_UNKNOWN_40 with an
// owner, where the frame's faces aren't hit), another weapon aims with the camera's free look; while aimed, the point the
// camera looks at, at most 100 m on, is the aim's (throw_dir)
// FUNCTION: GAME 0x592b70
void C_player::AI_car_fire(uint time, float *values, float *pressed)
{
    if (!IsReady())
        return;
    if (values[CTRL_MAP] != 0.0f)
        mafiaIndicators.show_flags |= INDF_MAP;
    else
        mafiaIndicators.show_flags &= ~INDF_MAP;
    if (pressed[CTRL_INVENTORY] > 0.25f && !CRacing)
    {
        std::vector<S_GameItem> dropped;
        GM_Menu::ExecuteMenu(new GM_Inventory(&inventory, &dropped), false, true);
        ChangeWeaponModel();
        ShowWeaponName();
        if (aimed)
            Do_Aimed(false);
    }
    if (pressed[CTRL_CYCLE_UP] > 0.25f || pressed[CTRL_CYCLE_UP + CTRL_SECONDARY] > 0.25f)
    {
        std::vector<S_GameItem> dropped;
        inventory.CycleUp(&dropped);
        ChangeWeaponModel();
        ShowWeaponName();
        if (aimed)
            Do_Aimed(false);
    }
    if (pressed[CTRL_CYCLE_DOWN] > 0.25f || pressed[CTRL_CYCLE_DOWN + CTRL_SECONDARY] > 0.25f)
    {
        std::vector<S_GameItem> dropped;
        inventory.CycleDown(&dropped);
        ChangeWeaponModel();
        ShowWeaponName();
        if (aimed)
            Do_Aimed(false);
    }
    if (pressed[CTRL_HIDE_WEAPON] > 0.25f)
    {
        std::vector<S_GameItem> dropped;
        inventory.Holster(&dropped, true, true);
        ChangeWeaponModel();
        ShowWeaponName();
        if (aimed)
            Do_Aimed(false);
    }
    if (pressed[CTRL_RELOAD] > 0.25f)
        Do_Reload();
    if (values[CTRL_FIRE] > 0.25f || values[CTRL_FIRE + CTRL_SECONDARY] > 0.25f)
    {
        switch (weapon_kind)
        {
        case WEAPON_KIND_PISTOL:
        case WEAPON_KIND_SMG:
            if (shot_frame && fire_action[fire_step].action == FIRE_ACTION_END)
            {
                I3D_frame *cam = act_mission->scene->current_camera;
                S_vector from = cam->GetWorldPos();
                S_vector d = cam->GetWorldDir();
                d.Normalize();
                d *= 100.0f;
                S_vector pos;
                I3D_COLLISION col;
                while (true)
                {
                    g_collision_header *coll = (g_collision_header *)gifCollision.TestLineH(from, d, &pos, NULL,
                        COLF_UNKNOWN_1, COLF2_UNKNOWN_200000 | COLF2_UNKNOWN_400000);
                    if (!coll)
                    {
                        pos = from + d;
                        break;
                    }
                    else if (!(coll->flags & COLLHF_UNKNOWN_40) || !coll->owner)
                        break;
                    if (act_mission->scene->TestColHierarchy(from, d, coll->frame, col, 0, NULL, 0, NULL))
                        break;
                    float len = d.Magnitude() - (pos - from).Magnitude() - 0.5f;
                    d.Normalize();
                    from = pos + d * 0.5f;
                    d *= len;
                }
                from = pos;
                Do_Shoot(true, &from);
                aim_time = 300000;
                int spare;
                uint ammo = inventory.GetAmmo(&spare);
                mafiaIndicators.PlayerSetAmmo(ammo, spare);
            }
            break;
        case WEAPON_KIND_FISTS:
        case WEAPON_KIND_RIFLE:
        case WEAPON_KIND_THROWN:
        case WEAPON_KIND_KNUCKLEDUSTER:
        case WEAPON_KIND_KNIFE:
        case WEAPON_KIND_BAT:
            if (!aimed)
            {
                aim_time = 300000;
                Do_Aimed(true);
                act_mission->game->camera.EnableFreeLook(true, true);
                car_free_look = true;
            }
            break;
        }
    }
    else
        Do_Shoot(false, NULL);
    if (aimed)
    {
        I3D_frame *cam = act_mission->scene->current_camera;
        S_vector dir = cam->GetWorldDir();
        S_vector from = cam->GetWorldPos();
        dir *= 100.0f;
        if (!gifCollision.TestLineH(from, dir, &throw_dir, NULL, COLF_UNKNOWN_1,
                COLF2_UNKNOWN_200000 | COLF2_UNKNOWN_400000))
            throw_dir = from + dir;
    }
}

// The player's frame (C_actor's AI): the HUD's car damage bar runs out on foot; then the human's, and while he lives
// the fade to the camera's (out while it looks from his eyes), the controls: the camera's change, the objectives, on foot,
// driving or shooting from another seat (with the controls locked the tachometer only), his pose turned to where the
// camera looks on foot, the aim lowered once aim_time runs out, the recoil's decay, the action button and the theft
// given up once the action button is let go
// FUNCTION: GAME 0x5937b0
void C_player::AI(uint time)
{
    if (car)
    {
        car_damage_time = -1;
        damage_car = NULL;
    }
    else if (damage_car && car_damage_time > 0)
    {
        mafiaIndicators.car_damage = 1.0f - damage_car->ShowCarEnergy();
        car_damage_time -= time;
        if (car_damage_time <= 0)
        {
            car_damage_time = -1;
            damage_car = NULL;
            mafiaIndicators.show_flags &= ~INDF_CAR_DAMAGE;
        }
    }
    C_human::AI(time);
    if (dead)
        return;
    fade_target = act_mission->GetGame()->GetCamera().LookFromEyes() ? 0.0f : 1.0f;
    if ((float)fabs(fade_alpha - fade_target) > MRG_ZERO)
    {
        float step = time * .003;
        if (fade_alpha > fade_target)
            step *= -1.0f;
        fade_alpha += step;
        if (fade_alpha > 1.0f)
            fade_alpha = 1.0f;
        if (fade_alpha < 0.0f)
            fade_alpha = 0.0f;
        SetAlpha(fade_alpha);
    }
    float *values;
    controls.GetValues(values, 0);
    float *pressed;
    controls.GetValues(pressed, CONTROLS_PRESSED);
    if (!controls_locked && pressed[CTRL_CAMERA] != 0.0f && (car ? car->type != ACTOR_RAILWAY : true) &&
        (using_car ? using_car->type != ACTOR_RAILWAY : true))
    {
        act_mission->game->camera.SetMode(CAMERA_NEXT, true);
        bool hide = act_mission->game->camera.unknown_22;
        if (frame)
            frame->SetOn(!hide);
        if (car && car->type == ACTOR_CAR)
            car->unknown_490 = hide;
    }
    if (values[CTRL_OBJECTIVES] > 0.25f)
        mafiaLoadSave.ObjectivesDisplay();
    if (!controls_locked)
    {
        if (car && car->type == ACTOR_CAR)
        {
            if (car->subtype == ACTORSUB_CAR_EX && seat == -1)
                AI_walk(time, values, pressed);
            else
            {
                if (!CRacing)
                {
                    if (car->GetSpeed() < 0.5f)
                        mafiaIndicators.show_flags |= INDF_ACTION;
                    else
                        mafiaIndicators.show_flags &= ~INDF_ACTION;
                }
                if (!seat)
                    AI_drive(time, values, pressed);
                else
                    AI_car_fire(time, values, pressed);
            }
        }
        else
            AI_walk(time, values, pressed);
    }
    else if (car && car->type == ACTOR_CAR && !seat)
    {
        mafiaIndicators.TachometrSetGear(car->gear, car->auto_gearing);
        mafiaIndicators.TachometrSetValues(car->GetSpeed(), car->GetRotations() / car->GetMaxRotations(),
            car->GetTankFuel() / car->GetTankSize(), car->GetOdometer());
        if (can_car_damage)
            mafiaIndicators.car_damage = 1.0f - car->ShowCarEnergy();
    }
    if (!car && !using_car && !controls_locked && !look_frame)
    {
        I3D_camera *cam = act_mission->scene->GetActiveCamera();
        S_vector pos = cam->GetWorldPos() + cam->GetWorldDir() * 10.0f;
        if (!weapon_kind)
            PoseNullRotate();
        else if (aimed)
            PoseSetPoseAimed(pos);
        else
            PoseSetPoseNormal(pos);
    }
    if (aimed && !loading_shells && !fire_held && !crouching && !car_free_look &&
        act_mission->game->camera.mode != CAMERA_CP_SNIPER)
    {
        aim_time -= time;
        if (aim_time < 0)
        {
            aim_time = 0;
            Do_Aimed(false);
        }
    }
    if (aimed && !car && !Can_Aimed())
    {
        aim_time = 0;
        Do_Aimed(false);
    }
    if (recoil > 0.0f)
    {
        float d = time * 0.001f;
        if (act_mission->game->camera.mode == CAMERA_CP_SNIPER)
            d *= 0.05f;
        recoil -= d;
        if (recoil < 0.0f)
            recoil = 0.0f;
        mafiaIndicators.PlayerSetAccuracy(recoil * 3.0f + 1.0f);
        act_mission->game->camera.SetRecoil(recoil);
    }
    if (IsReady() && !controls_locked &&
        (pressed[CTRL_ACTION] > 0.25f || pressed[CTRL_ACTION + CTRL_SECONDARY] > 0.25f))
    {
        if (car)
        {
            switch (car->type)
            {
            case ACTOR_CAR:
                if (car->subtype != ACTORSUB_CAR_EX || seat != -1)
                    Do_AB_OwnerCar();
                break;
            case ACTOR_RAILWAY:
                Do_AB_OwnerNULL();
                break;
            }
        }
        else
            Do_AB_OwnerNULL();
    }
    if (action_state == HUMAN_ACTION_STEAL && steal_time - action_time > 750 && values[CTRL_ACTION] < 0.25f &&
        values[CTRL_ACTION + CTRL_SECONDARY] < 0.25f && (!menu_choice || !(igraph->GetMouseButtons() & MBUT_LEFT)))
    {
        action_state = HUMAN_ACTION_STAND;
        mafiaIndicators.show_flags &= ~INDF_THIEF;
        stealing_car = NULL;
        action_time = -1;
        if (weapon_model)
            weapon_model->SetOn(true);
        if (weapon_model2)
            weapon_model2->SetOn(true);
        act_mission->game->Stop3DSound(steal_sound, false, 0.001f);
        act_mission->game->shvestky.CarSteal(this, false, false);
        act_mission->game->camera.EnableFreeLook(false, false);
    }
}

// FUNCTION: GAME 0x594060
void C_player::GameInit()
{
    primary_controls = 0xffff;
    turn_last = 0;
    turn_pending = 0;
    turn_idle = 0;
    always_run = true;
    horn = false;
    recoil = 0.0f;
    aim_time = 0;
    controls_locked = false;
    steal_sound = -1;
    car_damage_time = -1;
    damage_car = NULL;
    side_step_state = 0;
    side_step_time = 0;
    shown_weapon = -1;
    car_free_look = false;
    manual_gearing = false;
    use_timer.Reset(0x218);
    fade_alpha = 1.0f;
    alpha_base = NULL;
    I3D_frame *base = frame->FindChildFrame("base", ENUMF_ALL);
    if (base && base->GetType() == FRAME_VISUAL &&
        (((I3D_visual *)base)->GetVisualType() == I3D_VISUAL_SINGLEMESH ||
         ((I3D_visual *)base)->GetVisualType() == I3D_VISUAL_SINGLEMORPH))
        alpha_base = (I3D_object *)base;
    alpha_objects.clear();
    C_human::GameInit();
    inventory.messages = true;
    horn_sound = NULL;
    horn_time = 0;
    idle_anims = true;
}

// The sound at 0xaf8 stopped and released
// FUNCTION: GAME 0x594180
void C_player::GameDone()
{
    if (horn_sound)
    {
        horn_sound->SetOn(false);
        horn_sound->Release();
        horn_sound = NULL;
    }
    C_human::GameDone();
}

// With the controls locked he can be hit only where the police allow it
// FUNCTION: GAME 0x5941c0
bool C_player::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
                         I3D_frame *frm)
{
    if (controls_locked && !AIManager.police.SemTrefenaLocklaKurva(this))
        return false;
    return C_human::Collision(act, v1, v2, v3, f, frm);
}

// Likewise; as the game's player he comes out of the sniper mode, and the HUD's health follows (never 0 while he lives)
// FUNCTION: GAME 0x594210
bool C_player::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
                   uint u, I3D_frame *frm)
{
    if (controls_locked && !AIManager.police.SemTrefenaLocklaKurva(this))
        return false;
    C_human::Hit(type, v1, v2, v3, f, act, u, frm);
    if (act_mission->game->player == this)
    {
        if (act_mission->game->camera.mode == CAMERA_CP_SNIPER)
            SetSniperMode(false);
        uint lives = (int)(properties[HUMAN_PROPERTY_ENERGY] / max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
        if (alive && lives < 1)
            lives = 1;
        mafiaIndicators.PlayerSetLives(lives);
    }
    return true;
}

// The driver's steering linearity from the profile's options (the menu's "Nonlinear / Linear")
// FUNCTION: GAME 0x5942c0
void C_player::UpdateControls()
{
    if (act_mission->game->initialized && car && car->type == ACTOR_CAR && !seat)
        car->SetSteeringLinearity(mafiaLoadSave.settings.steering_linearity);
}

// The action button on foot (PS2 name): a dead body carried is dropped. Else the objects to use near the player (looked
// for 0.8 in front of him and 0.9 up) go into the item pick-up menu: the items he takes are picked up (the weapons'
// sound or the other items'), what his inventory can't hold dropped. An action chosen (an object of
// GAMEITEM_SPECIAL_ACTION) is done: a detector used, a dead body searched (the menu again with the body's inventory)
// or moved, an entity's script given EVENT_USE_AB, a door, a car (into it, stolen where CAR_MUSTSTEAL locks it, its
// driver thrown out), a train, a box, a pedestrian talked to
// FUNCTION: GAME 0x594300
void C_player::Do_AB_OwnerNULL()
{
    if (dead_body) {
        Do_DeadBodyDrop();
        return;
    }
    std::vector<S_GameItem> items;
    S_vector off = frame->GetWorldDir();
    off.Normalize();
    off *= .8f;
    off.y += .9f;
    act_mission->GetGame()->using_object.FindNearObjects(frame->GetWorldPos() + off, frame->GetWorldDir(), items,
        frame->GetParent());
    if (items.size()) {
        std::vector<S_GameItem> dropped, picked;
        std::vector<G_Inventory *> inventories;
        uint result = GM_Menu::ExecuteMenu(new GM_ItemPickUp(&inventory, &inventories, &items, &dropped, &picked), false,
            true);
        if (result == PICKUP_TAKE) {
            // The items taken
            if (picked.size()) {
                int sound = -1;
                for (int i = picked.size(); i--;) {
                    C_using_object::S_using_object *obj = (C_using_object::S_using_object *)picked[i].using_object;
                    if (obj->item.id < numOfGameItems)
                        sound = obj->item.id >= GAMEITEM_COLT_DETECTIVE && obj->item.id <= GAMEITEM_MOSIN_NAGANT ?
                            SOUND_PICK_UP_WEAPON : SOUND_PICK_UP_ITEM;
                    PickObject(obj->pos);
                    if (obj->actor)
                        act_mission->GetGame()->RemoveTemporaryActor(obj->actor);
                    else
                        act_mission->GetGame()->using_object.DelObject(obj);
                }
                if (sound > -1) {
                    I3D_sound *snd = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
                    if (snd) {
                        snd->SetPos(frame->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f));
                        sound_table.ConstructSound(snd, sound);
                        snd->SetSoundType(I3DSOUND_AMBIENT);
                        snd->SetOn(true);
                        act_mission->GetGame()->Play3DSound(snd);
                    }
                }
            }
            if (dropped.size())
                DropOutItems(dropped);
            ChangeWeaponModel();
        } else if (result >= PICKUP_ITEM_FIRST && result <= PICKUP_ITEM_LAST) {
            // An action
            result -= PICKUP_ITEM_FIRST;
            if (items[result].id == PICKUP_TEXT) {
                C_using_object::S_using_object *obj = (C_using_object::S_using_object *)items[result].using_object;
                if (obj->actor && obj->actor->GetType() == ACTOR_DETECTOR)
                    ((C_detector *)obj->actor)->UseDetector();
                else if (obj->actor && obj->actor->GetType() == ACTOR_ENTITY) {
                    switch (items[result].ammo_loaded) {
                    case USING_ACTION_SEARCH_DEAD_BODY:
                        if (obj->actor->GetType() == ACTOR_PLAYER || obj->actor->GetType() == ACTOR_ENTITY) {
                            // The body's inventory in the menu
                            std::vector<S_GameItem> body_dropped, body_picked;
                            inventories.push_back(&((C_human *)obj->actor)->inventory);
                            items.clear();
                            GM_Menu::ExecuteMenu(new GM_ItemPickUp(&inventory, &inventories, &items, &body_dropped,
                                &body_picked), false, true);
                            if (body_dropped.size())
                                DropOutItems(body_dropped);
                            ChangeWeaponModel();
                        }
                        break;
                    case USING_ACTION_MOVE_DEAD_BODY:
                        Do_DeadBody(obj->actor->frame);
                        break;
                    default:
                        ((C_entity *)obj->actor)->SetEvent(EVENT_USE_AB);
                    }
                } else {
                    switch (items[result].ammo_loaded) {
                    case USING_ACTION_OPEN_DOOR:
                    case USING_ACTION_CLOSE_DOOR:
                    case USING_ACTION_USE_SWITCH:
                        Use_Actor(obj->actor, 0, 0, 0);
                        break;
                    case USING_ACTION_GET_IN_FRONT:
                    case USING_ACTION_GET_IN_BACK: {
                        if (GetCar())
                            break;
                        menu_choice = items.size() > 1;
                        int seat = ((C_car *)obj->actor)->GetUSindex(obj);
                        if (seat > -1) {
                            if (((C_car *)obj->actor)->must_steal) {
                                if (items[result].ammo_loaded == USING_ACTION_GET_IN_FRONT)
                                    Can_UseCar((C_car *)obj->actor, seat);
                            } else
                                Use_Actor(obj->actor, 1, seat, 0);
                        }
                        break;
                    }
                    case USING_ACTION_GET_IN_AS_DRIVER: {
                        if (GetCar())
                            break;
                        menu_choice = items.size() > 1;
                        int seat = ((C_car *)obj->actor)->GetUSindex(obj);
                        if (seat > -1) {
                            if (((C_car *)obj->actor)->must_steal)
                                Can_UseCar((C_car *)obj->actor, seat);
                            else
                                Use_Actor(obj->actor, 1, seat, 1);
                        }
                        break;
                    }
                    case USING_ACTION_BOARD_TRAIN:
                    case USING_ACTION_BOARD_TROLLEY:
                        if (GetCar())
                            break;
                        Use_Actor(obj->actor, 1, 0, 0);
                        if (frame)
                            frame->SetOn(false);
                        break;
                    case USING_ACTION_EXIT_TRAIN:
                    case USING_ACTION_EXIT_TROLLEY:
                        if (GetCar() && GetCar()->GetType() == ACTOR_RAILWAY)
                            Use_Actor(obj->actor, 2, 0, 0);
                        break;
                    case USING_ACTION_HIJACK_CAR: {
                        // The driver thrown out, unless the car can't be taken: a traffic's car that is a police car
                        // (or has 0x2105 set), or, without the cheat, a car the garage's profile can't steal that is
                        // a traffic's or CAR_MUSTSTEAL's
                        if (GetCar())
                            break;
                        if (!obj->actor || obj->actor->GetType() != ACTOR_CAR)
                            break;
                        C_car *c = (C_car *)obj->actor;
                        bool traffic = c->traffic_car;
                        bool can = !(c->unknown_2105 || c->police_car) || !traffic;
                        if (can && !(bool)(act_mission->GetGame()->cheats & GAMEF_STEAL_ALL) &&
                            !mafiaLoadSave.CanSteal((uchar *)c->model_name))
                            can = !traffic && !((C_car *)obj->actor)->must_steal;
                        if (can) {
                            int seat = ((C_car *)obj->actor)->GetUSindex(obj);
                            if (seat > -1) {
                                C_human *owner = ((C_car *)obj->actor)->GetOwner(seat);
                                if (owner && (owner->GetType() == ACTOR_PLAYER || owner->GetType() == ACTOR_ENTITY)) {
                                    // Not a friend's who lives
                                    if (owner->dead || owner->human_type != HUMAN_TYPE_FRIEND)
                                        Do_ThrowCocotFromCar((C_car *)obj->actor, seat);
                                } else
                                    Do_ThrowCocotFromCar((C_car *)obj->actor, seat);
                            }
                        } else
                            NewScream(rnd(3) + VOICE_CANT_STEAL);
                        break;
                    }
                    case USING_ACTION_PICK_UP_BOX:
                        if (Do_FuckingBox(obj->frame)) {
                            act_mission->GetGame()->BoxPlacementDel(obj->frame);
                            act_mission->GetGame()->BoxPlacementRecompile();
                        }
                        break;
                    case USING_ACTION_DROP_BOX:
                        act_mission->GetGame()->BoxPlacementGetNextDest(box, obj->frame);
                        Do_FuckingBox(NULL);
                        break;
                    case USING_ACTION_TALK:
                        C_traffic_generator::PlayerPromluvilNaTehoChuja(obj->frame);
                        break;
                    }
                }
            }
        }
    }
}

// The action button in a car (PS2 name): out of the aim first; a passenger's tells the followers. The driver gets out
// of a car that stands, turning its engine off first, and next to a petrol pump whose attendant can serve the car he
// chooses between getting out and refuelling (the item pick-up menu with two texts)
// FUNCTION: GAME 0x594fa0
void C_player::Do_AB_OwnerCar()
{
    if (CRacing)
        return;
    if (aimed) {
        Do_Aimed(false);
        car_free_look = false;
        act_mission->game->camera.EnableFreeLook(false, false);
        return;
    }
    if (seat)
        AIManager.follow.Event(4, this);
    if (car->speed > .5f)
        return;
    if (car->engine_on) {
        car->SetEngineOn(false, false);
        return;
    }
    C_pumper *pumper = NULL;
    std::vector<C_actor *> &actors = act_mission->actors;
    for (int i = actors.size(); i--; ) {
        if (actors[i]->type == ACTOR_PUMPER) {
            pumper = (C_pumper *)actors[i];
            int can = pumper->CanPump(car);
            if (can == PUMPER_CAN_PUMP)
                break;
            pumper = NULL;
            if (can == PUMPER_BUSY)
                return;
        }
    }
    if (pumper) {
        std::vector<S_GameItem> items;
        S_GameItem item;
        items.push_back(item);
        items.back().id = PICKUP_TEXT;
        items.back().ammo_loaded = TEXT_PUMP_LEAVE_CAR;
        items.push_back(item);
        items.back().id = PICKUP_TEXT;
        items.back().ammo_loaded = TEXT_PUMP_REFUEL;
        std::vector<S_GameItem> dropped, picked;
        std::vector<G_Inventory *> inventories;
        GM_ItemPickUp *menu = new GM_ItemPickUp(&inventory, &inventories, &items, &dropped, &picked);
        uint result = GM_Menu::ExecuteMenu(menu, false, true);
        if (result >= PICKUP_ITEM_FIRST && result <= PICKUP_ITEM_LAST) {
            result -= PICKUP_ITEM_FIRST;
            if (items[result].id == PICKUP_TEXT) {
                switch (items[result].ammo_loaded) {
                case TEXT_PUMP_LEAVE_CAR:
                    if (horn) {
                        horn = false;
                        if (car && car->type == ACTOR_CAR)
                            car->horn = false;
                    }
                    Use_Actor(car, 2, 0, 0);
                    break;
                case TEXT_PUMP_REFUEL:
                    act_mission->game->shvestky.ZlikvidujVsechnyNasraneVyhozeneCuraky();
                    pumper->Start(car);
                    break;
                }
            }
        }
    }
    else
        Use_Actor(car, 2, 0, 0);
}

// The HUD's crosshair and ammunition for the weapon in the hand: hidden in the sniper mode, behind a box or the dead
// body, and for a passenger of a car
// FUNCTION: GAME 0x5952f0
void C_player::IndicatorSetAimed()
{
    if (weapon_kind > 0 && weapon_kind < 4)
    {
        if (act_mission->game->camera.mode == CAMERA_CP_SNIPER || box || dead_body)
        {
            mafiaIndicators.show_flags &= ~INDF_CROSSHAIR;
        }
        else if (car && car->type == ACTOR_CAR)
        {
            if (seat == -1)
                mafiaIndicators.show_flags |= INDF_CROSSHAIR;
        }
        else if (!using_car || using_car->type != ACTOR_CAR)
            mafiaIndicators.show_flags |= INDF_CROSSHAIR;
        mafiaIndicators.show_flags |= INDF_AMMO;
    }
    else
        mafiaIndicators.show_flags &= ~(INDF_AMMO | INDF_CROSSHAIR);
}

// A shot of the weapon: it stays aimed 3 seconds, its recoil is added to the player's (a tenth of it in the sniper
// mode) and the HUD's accuracy, the camera's tilt and the game's fire event follow
// FUNCTION: GAME 0x595390
void C_player::ShootDeAimed(S_item_def *weapon)
{
    aim_time = 3000;
    float r = weapon->recoil;
    if (act_mission->game->camera.mode == CAMERA_CP_SNIPER)
        r *= 0.1f;
    recoil += r;
    if (recoil > 1.0f)
        recoil = rnd() * 0.2f + 0.8f;
    mafiaIndicators.PlayerSetAccuracy(recoil * 3.0f + 1.0f);
    act_mission->game->camera.SetRecoil(recoil);
    act_mission->game->PFire_Fire();
}

// The sniper mode: the camera's, the weapon's scope, the w-buffer back on out of it; the player's model is hidden
// in it unless the camera says otherwise. Only a weapon with a scope and a player on foot can go into it
// FUNCTION: GAME 0x595440
void C_player::SetSniperMode(bool on)
{
    if (!on)
    {
        if (act_mission->game->camera.mode == CAMERA_CP_SNIPER)
        {
            SetWBuffer(true);
            sniper_mode = false;
            act_mission->game->camera.SetSniperMode(false);
            if (frame)
                frame->SetOn(!act_mission->game->camera.unknown_22);
        }
        return;
    }
    if (act_mission->game->camera.mode != CAMERA_CP_SNIPER)
    {
        S_item_def *weapon = inventory.GetWeaponInRightHand();
        if (!car && (weapon->flags & ITEMF_SCOPE))
        {
            driver->SetState(RS_WBUFFER, true);
            sniper_mode = true;
            act_mission->game->camera.SetSniperMode(true);
            act_mission->game->camera.SetSniperFov(0.12f);
            if (frame)
                frame->SetOn(!act_mission->game->camera.unknown_22);
            if (!aimed)
            {
                aimed = true;
                fire_action = _fire_action_aimed_;
                fire_step = 0;
            }
        }
    }
}

// The model changed (C_human::_intern_ChangeModel's): the alpha base again from the new model's "base", the alpha
// objects forgotten, and the player visible
// FUNCTION: GAME 0x595570
void C_player::_intern_ChangeModel()
{
    fade_alpha = 1.0f;
    alpha_base = NULL;
    I3D_frame *base = frame->FindChildFrame("base", ENUMF_ALL);
    if (base && base->GetType() == FRAME_VISUAL &&
        (((I3D_visual *)base)->GetVisualType() == I3D_VISUAL_SINGLEMESH ||
         ((I3D_visual *)base)->GetVisualType() == I3D_VISUAL_SINGLEMORPH))
        alpha_base = (I3D_object *)base;
    alpha_objects.clear();
}

// Into the car's seat, or stealing it where it is locked (Do_AB_OwnerNULL calls it for a car CAR_MUSTSTEAL locked):
// the police told of a police car first; a car the garage's profile can't steal (and without the cheat) makes the
// player say so. He stands at the seat's place for getting in (the animation's track), short of the car's body when
// that is in the way, and plays the stealing (HUMAN_ACTION_STEAL, the car database's time, 2 to 30 s), the thief's bar
// on the HUD and the unlocking's sound looping at him; the free look on, the weapons hidden, and the police told
// FUNCTION: GAME 0x5955f0
void C_player::Can_UseCar(C_car *car, int seat)
{
    if (!car)
        return;
    if (car->police_car && act_mission->game->shvestky.active)
        act_mission->game->shvestky.CorujuPoldumMasinu(this, car);
    if (!mafiaLoadSave.CanSteal((uchar *)car->model_name) && !(bool)(act_mission->game->cheats & GAMEF_STEAL_ALL)) {
        NewScream(rnd(3) + VOICE_CANT_STEAL);
        return;
    }
    bool b1, b2, b3, b4;
    if (!car->GetSeatProperty(seat, b1, b2, b3, b4))
        return;
    int id = GetAnimIDForUseCar(b1, b2, b3, b4, false, car->seats.size() && !car->seats[0].unknown_20);
    S_vector p, d;
    if (car->GetPlacementFromTCK(seat, id, p, d)) {
        d.Normalize();
        dir = d;
        pos = p;
        pos.y += _get_delta_nodyn_y_(pos);
        // Short of the car's body along the way to the seat's place
        p.y += 1.0f;
        d *= 2.0f;
        I3D_COLLISION col;
        if (act_mission->scene->TestColHierarchy(p, d, car->frame, col, 0, NULL, 0, NULL) && col.dist > .5f) {
            float f = col.dist - .5f;
            d.Normalize();
            pos += d * f;
            pos.y += _get_delta_nodyn_y_(pos);
        }
        frame->SetPos(pos);
        frame->SetDir(dir, 0.0f);
        frame->Update();
    }
    C_traffic_generator::VyfunOdTeMasiny(car);
    action_state = HUMAN_ACTION_STEAL;
    action_time = mafiaCarDatabase.GetStealTime(car->model_name);
    if (action_time < 2000)
        action_time = 2000;
    if (action_time > 30000)
        action_time = 30000;
    stealing_car = car;
    steal_time = action_time;
    mafiaIndicators.show_flags |= INDF_THIEF;
    mafiaIndicators.ThiefIndicatorSetValue(0.0f);
    next_anim_id = ANIM_KRADENI_AUTA01;
    PersonAnim(0.0f, true, false);
    if (weapon_model)
        weapon_model->SetOn(false);
    if (weapon_model2)
        weapon_model2->SetOn(false);
    PoseNullRotate();
    aimed = false;
    Do_Crouched(false);
    act_mission->game->camera.EnableFreeLook(true, true);
    velocity.Zero();
    velocity_ramp = 0.0f;
    I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    sound_table.ConstructSound(sound, SOUND_CAR_UNLOCK);
    sound->SetLoop(true);
    sound->SetPos(0.0f, 1.0f, 0.0f);
    sound->Update();
    sound->LinkTo(frame, 0);
    steal_sound = act_mission->game->Play3DSound(sound);
    act_mission->game->shvestky.CarSteal(this, true, false);
}

// The car the player strikes, if it shows its damage (SHOWCARDAMAGE's): the HUD's bar for 3 seconds
// FUNCTION: GAME 0x595c30
void C_player::SetCarDamage(C_car *car)
{
    if (car && car->show_damage)
    {
        damage_car = car;
        car_damage_time = 3000;
        mafiaIndicators.show_flags |= INDF_CAR_DAMAGE;
    }
}

// The car gone (PS2 name): the bar hidden, unless the player drives a car
// FUNCTION: GAME 0x595c60
void C_player::InvalidateCarDamage(C_car *car)
{
    if (car && car == damage_car)
    {
        damage_car = NULL;
        car_damage_time = -1;
        if (!this->car || this->car->type != ACTOR_CAR)
            mafiaIndicators.show_flags &= ~INDF_CAR_DAMAGE;
    }
}

// The frame's objects (and its children's) added to the ones that fade with the player, and faded to his alpha
// FUNCTION: GAME 0x595cb0
void C_player::AddAlphaObject(I3D_frame *frm)
{
    cbAlpha(frm, alpha_objects);
    if (fade_alpha < 1.0f)
        SetAlpha(fade_alpha);
}

// FUNCTION: GAME 0x595cf0
static void cbAlpha(I3D_frame *frm, std::vector<I3D_object *> &objects)
{
    if (frm->GetType() == FRAME_VISUAL && ((I3D_visual *)frm)->GetVisualType() == I3D_VISUAL_OBJECT) {
        bool found = false;
        for (int i = objects.size(); i; ) {
            i--;
            if (objects[i] == (I3D_object *)frm) {
                found = true;
                break;
            }
        }
        if (!found)
            objects.push_back((I3D_object *)frm);
    }
    for (int i = frm->NumChildren(); i; ) {
        i--;
        cbAlpha(frm->GetChild(i), objects);
    }
}

// The frame's objects dropped again
// FUNCTION: GAME 0x595d70
void C_player::DelAlphaObject(I3D_frame *frm)
{
    cbDelAlpha(frame, alpha_objects);
}

// FUNCTION: GAME 0x595d90
static void cbDelAlpha(I3D_frame *frm, std::vector<I3D_object *> &objects)
{
    for (int j = objects.size(); j; ) {
        j--;
        if (objects[j] == (I3D_object *)frm) {
            objects[j] = objects.back();
            objects.pop_back();
            break;
        }
    }
    for (int i = frm->NumChildren(); i; ) {
        i--;
        cbDelAlpha(frm->GetChild(i), objects);
    }
}

// The alpha given to the model and to the objects added
// FUNCTION: GAME 0x595e10
void C_player::SetAlpha(float alpha)
{
    if (alpha_base)
        alpha_base->SetTransparency(alpha);
    for (int i = alpha_objects.size(); i--;)
        alpha_objects[i]->SetTransparency(alpha);
}

// The name of the weapon in the hand in the console (the text database's 0xce4 on, by the item's id), once for each
// weapon; in a car not for the empty hand while the car holds an item
// FUNCTION: GAME 0x595e70
void C_player::ShowWeaponName()
{
    if (inventory.GetItemInRightHand(NULL) == shown_weapon)
        return;
    shown_weapon = inventory.GetItemInRightHand(NULL);
    if (car && car->type == ACTOR_CAR)
    {
        if (inventory.right_hand.id || !inventory.car_item.id)
            mafiaIndicators.ConsoleAddTextByID(inventory.GetItemInRightHand(NULL) + 0xce4, 0xffffff);
    }
    else
        mafiaIndicators.ConsoleAddTextByID(inventory.GetItemInRightHand(NULL) + 0xce4, 0xffffff);
}

// Locked, the player stops what he is doing: a theft is cancelled, a car or a rail vehicle got into or out of is
// finished at once, and he stands, his animations stopped; the city map is hidden
// FUNCTION: GAME 0x595f00
void C_player::LockControls(bool lock)
{
    controls_locked = lock;
    if (!lock)
        return;
    mafiaIndicators.show_flags &= ~INDF_MAP;
    if (action_state > 0)
    {
        if (action_state > 6)
        {
            if (action_state != HUMAN_ACTION_STEAL)
                return;
            mafiaIndicators.show_flags &= ~INDF_THIEF;
            stealing_car = NULL;
            action_time = -1;
            if (weapon_model)
                weapon_model->SetOn(true);
            if (weapon_model2)
                weapon_model2->SetOn(true);
            act_mission->game->Stop3DSound(steal_sound, false, 0.001f);
            act_mission->game->shvestky.CarSteal(this, false, false);
            act_mission->game->camera.EnableFreeLook(false, false);
        }
        switch (vehicle_action)
        {
        case HUMAN_VEHICLE_CAR_IN:
            Intern_UseCar(true);
            break;
        case HUMAN_VEHICLE_CAR_OUT:
        case HUMAN_VEHICLE_CAR_THROWN_OUT:
            Intern_UseCar(false);
            break;
        case HUMAN_VEHICLE_RAILWAY_IN:
            Intern_UseRailway(true);
            break;
        case HUMAN_VEHICLE_RAILWAY_OUT:
            Intern_UseRailway(false);
            break;
        }
        vehicle_action = 0;
        using_car = NULL;
        action_state = HUMAN_ACTION_STAND;
        anim_machine.Reset();
        anim_machine.StopAnims();
        velocity.Zero();
        velocity_ramp = 0.0f;
        Do_Crouched(false);
        aimed = false;
        Do_Shoot(false, NULL);
        anim_id = 0;
        next_anim_id = 1;
        PersonAnim(0.0f, true, false);
        PoseNullRotate();
    }
}

// The BOXER cheat's hands: the model's "r_hand" and "l_hand" twice their size while it is on
// FUNCTION: GAME 0x596090
void C_player::SetBoxer(bool on)
{
    S_vector scale(1.0f, 1.0f, 1.0f);
    if (on) {
        scale.x = 2.0f;
        scale.y = 2.0f;
        scale.z = 2.0f;
    }
    I3D_frame *frm = frame->FindChildFrame("r_hand", ENUMF_ALL);
    if (frm) {
        frm->SetScale(scale);
        frm->Update();
    }
    frm = frame->FindChildFrame("l_hand", ENUMF_ALL);
    if (frm) {
        frm->SetScale(scale);
        frm->Update();
    }
}

// The player fully visible again (PS2 name), which the camera's Begin calls for the cameras outside his eyes
// FUNCTION: GAME 0x5961b0
void C_player::FadeReset()
{
    fade_alpha = 1.0f;
    fade_target = 1.0f;
    SetAlpha(fade_alpha);
}








