// The game's camera (PS2 g_camera.cpp; the PC's file is the one at 0x5ece70-0x5f4b5c, docs/game-files.md): its
// constructor, destructor and Reset, then Tick and what it calls, PS2's order. Written so far: those four, WantPosCar,
// WantPos, FreeLook and FreeLookBase.
// The file's uninitialized globals lie in the exact reverse of PS2's definition order (Tick's local statics oldS and
// oldSpeed lowest, then its three oldDirY, filteringBuffer, ..., oldDir, oldPos), with the zero-initialized ones after
// them in PS2's order (naklon to filteringLastPos, then Tick's rot, kl, kl2, cameraDirRot, cameraDirUpDown): what a
// precompiled header of another file (/Yu) gives (CLAUDE.md), as for g_indicators.cpp. They are defined here in that
// order. Open: the build compiles the file with /YX, so the .bss order isn't the original's.
// Also written: GameDone, SetCamera, SetPlayer, Link, SetMode, Begin, End and PopMode, which C_game's Init and Done
// reach, and SetCar and IsInDialog, which the scripts call; their other calls are stubs (SetSwing, LockAt, the dialog's
// DialogLook, DialogStart and DialogStop).

#include "camera/g_camera.h"

#include "stub.h"
#include "actors/C_human.h"
#include "actors/C_player.h"
#include "collision/g_colls.h"
#include "input/controls.h"
#include "mission/Mission.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/racing.h"
#include "vehicles/C_car.h"

#include <math.h>

#include "IGraph/IGraph.h"
#include "I3D/I3D.h"

// The longest frame Tick moves the camera by, in milliseconds (the name is ours)
#define MAX_TICK_TIME 83
// The mouse camera's samples (the name is ours)
#define FILTERING_SIZE 30
// The height of the player's eyes above his position (the name is ours)
#define EYE_HEIGHT 1.65f

// How fast the car cameras follow the car (PS2's name): it falls while the vehicle is unstable
// GLOBAL: GAME 0x6386e8
float faktorStability = 1.0f;

// The side of the dialog's two humans the dialog camera takes, 1 or -1 (PS2's name; DialogLook sets it)
// GLOBAL: GAME 0x6386f8
float dialogSide = 1.0f;

// The cameras (PS2's name), from 0x638700, after the file's other initialized data
// GLOBAL: GAME 0x638700
S_camera_params cameraParams[NUM_CAMERAS] = {
    {CAMERA_P_BEHIND, 0,
        {0.5f, 0.31f, 0.09f, 0.09f, 0.3f, 0.5f, 0.2f, 0.1f, 0.27f, 0.27f}, "P Behind", CAMPF_EYES},
    {CAMERA_P_IN, CAMERA_P_BEHIND,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "P In", CAMPF_EYES},
    {CAMERA_DEATH, 0,
        {0.5f, 0.29f, 0.09f, 0.09f, 0.3f, 0.5f, 0.2f, 0.1f, 0.27f, 0.27f}, "P Behind", 0},
    {CAMERA_C_BEHIND_HELLBOY_11, CAMERA_C_OVER,
        {0.08f, 0.14f, 0.27f, 0.35f, 0.86f, 0.4f, 0.2f, 0.6f, 0.14f, 0.54f}, "C Beh. Hellboy", 0},
    {CAMERA_C_BEHIND_HELLBOY, CAMERA_C_BEHIND_EMETH,
        {0.08f, 0.14f, 0.27f, 0.35f, 0.86f, 0.4f, 0.2f, 0.6f, 0.14f, 0.54f}, "C Beh. Hellboy", 0},
    {CAMERA_C_BEHIND_EMETH, CAMERA_C_BEHIND3,
        {0.15f, 0.05f, 0.6f, 0.2f, 0.33f, 0.33f, 0.08f, 0.65f, 0.11f, 0.28f}, "C Behind Emeth", 0},
    {CAMERA_C_BEHIND3, CAMERA_C_CHASE,
        {0.12f, 0.02f, 0.06f, 0.5f, 0.8f, 0.36f, 0.2f, 1.0f, 0.48f, 0.2f}, "C Behind3", 0},
    {CAMERA_C_CHASE, CAMERA_C_IN,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Chase", CAMPF_UNKNOWN_2},
    {CAMERA_C_IN, CAMERA_C_OVER,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C In", 0},
    {CAMERA_C_OVER, CAMERA_C_BEHIND_HELLBOY,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Over", 0},
    {CAMERA_C_RAILWAY_IN, 0,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C RailwayIn", CAMPF_UNKNOWN_1},
    {CAMERA_P_BEHIND_23, 0,
        {0.5f, 0.21f, 0.09f, 0.09f, 0.3f, 0.5f, 0.2f, 0.1f, 0.27f, 0.27f}, "P Behind", CAMPF_UNKNOWN_1},
    {CAMERA_CP_SNIPER, 0,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "CP Sniper", CAMPF_UNKNOWN_1},
    {CAMERA_C_LOOK_TO, 0,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C LookTo", CAMPF_UNKNOWN_8},
    {CAMERA_C_LOCK, 0,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Lock", CAMPF_UNKNOWN_8},
    {CAMERA_C_DIALOG, 0,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Dialog", CAMPF_UNKNOWN_8},
    {CAMERA_C_RACING, CAMERA_C_BEHIND_HELLBOY,
        {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Racing", 0},
};

// The car cameras' position and direction, filtered from frame to frame (PS2's names)
// GLOBAL: GAME 0x6bdaf8
S_vector oldPos;
// GLOBAL: GAME 0x6bdaec
S_vector oldDir;

// The on-foot camera's distance, filtered (PS2's name)
// GLOBAL: GAME 0x6bdadc
float oldDist;

// Where the player's eyes are and look, for the move into them (PS2's names)
// GLOBAL: GAME 0x6bdac8
S_vector camInPos;
// GLOBAL: GAME 0x6bdabc
S_vector camInDir;

// The point needLookFromEyes measures the player's head against (PS2's name)
// GLOBAL: GAME 0x6bdab0
S_vector camBehindPos;

// The race camera CAMERA_C_RACING shows (PS2's name)
// GLOBAL: GAME 0x6bdaac
G_RaceCamera *activeRaceCam;

// The death camera's position and the point it looked at (PS2's names), and how far it has turned to the killer
// GLOBAL: GAME 0x6bda90
S_vector cameraInDeath;
// GLOBAL: GAME 0x6bda84
S_vector destInDeath;
// GLOBAL: GAME 0x6bda80
float interpInDeath;

// The mouse camera's last 30 movements: x, y and the time (PS2's names)
// GLOBAL: GAME 0x6bd918
float filteringBuffer[FILTERING_SIZE][3];

// The camera's sway, 0 to 2 (PS2's name)
// GLOBAL: GAME 0x6bdb04
float naklon = 0.0f;

// How far the camera has dropped with the player crouching, up to 0.7 (PS2's name, Czech: "drop into a squat")
// GLOBAL: GAME 0x6bdb08
float poklesVDrepu = 0.0f;

// PS2's name; Begin clears it for "C In"
// GLOBAL: GAME 0x6bdb0c
float rotInAct = 0.0f;

// PS2's name
// GLOBAL: GAME 0x6bdb10
float needLookFromEyes = 0.0f;

// How far the camera has moved into the player's eyes, 0 to 1 (PS2's name)
// GLOBAL: GAME 0x6bdb14
float faktorVOcich = 0.0f;

// The dialog camera's two humans (PS2's names; DialogLook sets them)
// GLOBAL: GAME 0x6bdb18
C_human *dialogLookFrom = NULL;
// GLOBAL: GAME 0x6bdb1c
C_human *dialogLookTo = NULL;

// The free look's angles: the turn, 0 to 2 pi, and the tilt, -1.5 to 1.5 (PS2's names)
// GLOBAL: GAME 0x6bdb20
float freeLookX = 0.0f;
// GLOBAL: GAME 0x6bdb24
float freeLookZ = 0.0f;

// The next entry of filteringBuffer (PS2's name)
// GLOBAL: GAME 0x6bdb28
uint filteringLastPos = 0;

// PS2's also reserves its vectors (3 and 15 entries)
// FUNCTION: GAME 0x5ecf90
G_Camera::G_Camera()
{
    Reset();
}

// FUNCTION: GAME 0x5ed000
G_Camera::~G_Camera()
{
}

// PS2's order: the pushed states cleared before and after the values, the camera the profile's where PS2 takes 1
// FUNCTION: GAME 0x5ed040
void G_Camera::Reset()
{
    camera = NULL;
    player = NULL;
    car = NULL;
    link = NULL;
    mode = mafiaLoadSave.settings.camera;
    unknown_14 = mode;
    unknown_20 = false;
    unknown_21 = false;
    unknown_22 = false;
    unknown_24 = 5.0f;
    tilt = 0.5f;
    look_around_want = 0.0f;
    look_around = 0.0f;
    swing = false;
    swing_frame = NULL;
    push_pop.clear();
    unknown_8c = 0.0f;
    unknown_90 = 0.2f;
    params = cameraParams[0].values;
    free_look = false;
    push_pop.clear();
    needLookFromEyes = 0.0f;
}

// FUNCTION: GAME 0x5ed120
void G_Camera::GameDone()
{
    while (push_pop.size())
        PopMode(false);
}

// FUNCTION: GAME 0x5ed150
void G_Camera::SetCamera(I3D_camera *cam)
{
    End();
    camera = cam;
    Begin();
}

// FUNCTION: GAME 0x5ed170
void G_Camera::SetPlayer(C_human *human)
{
    End();
    player = human;
    Begin();
}

// PS2's order; on foot the profile's camera where PS2's takes CAMERA_P_BEHIND
// FUNCTION: GAME 0x5ed190
void G_Camera::SetCar(C_car *c)
{
    End();
    car = c;
    if (car)
        mode = mafiaLoadSave.settings.unknown_34;
    else
    {
        mode = mafiaLoadSave.settings.camera;
        free_look = false;
    }
    Begin();
    mode_changed = true;
}

// The profile's camera where PS2's takes CAMERA_P_BEHIND
// FUNCTION: GAME 0x5ed1e0
void G_Camera::Link(I3D_frame *frm)
{
    link = frm;
    if (link)
        SetMode(CAMERA_C_RAILWAY_IN, true);
    else
        SetMode(mafiaLoadSave.settings.camera, true);
}

#pragma auto_inline(off)

// STUB: GAME 0x5ed210
void G_Camera::SetSwing(bool on, float amount, I3D_frame *frm)
{
    StubLog("G_Camera::SetSwing");
}

#pragma auto_inline(on)

// PS2's without its test of a lock (0xa5) at the start and the car camera it sets again after Begin, for the car of
// the mission's player in a state of its own
// FUNCTION: GAME 0x5ed400
void G_Camera::SetMode(uint m, bool begin)
{
    if (m == CAMERA_NEXT && !unknown_14)
        return;
    if (begin)
        End();
    if (m == CAMERA_NEXT)
        m = unknown_14;
    if (m >= CAMERA_C_FIRST && m < CAMERA_C_END)
        mafiaLoadSave.settings.unknown_34 = m;
    if (m >= CAMERA_P_FIRST && m < CAMERA_P_END)
        mafiaLoadSave.settings.camera = m;
    mode = m;
    if (begin)
        Begin();
}

// The camera's frame, by its mode (the cameras' numbers, S_camera_params::id): PS2's cases in PS2's order. The car
// cameras and the on-foot ones each have a switch of their own; every case ends in the tail that clears mode_changed,
// which VC6 copies into the cases that end in a call. The S_vector constructors nested in the operators go out of line
// (0x402a30) once the inline budget runs out, from the car cameras on.
// FUNCTION: GAME 0x5ed4c0
void G_Camera::Tick(uint time)
{
    // The cases' shared vectors (PS2 keeps them at fixed places of its frame too): dir for the car cameras and on-foot
    // CAMERA_P_BEHIND and CAMERA_P_MOUSE, look for the car cameras' offset and the on-foot cameras' direction
    S_vector dir;
    S_vector look;
    // The camera entry's values, read from the start
    float *p = params;
    if (!camera)
        return;
    if (time > MAX_TICK_TIME)
        time = MAX_TICK_TIME;
    float dt = time / 1000.0f;
    if (mode == CAMERA_UNKNOWN_24 && !unknown_20)
        return;
    if (unknown_20)
        return;
    if (!player)
        return;
    naklon += dt * 0.3f;
    if (naklon > 2.0f)
        naklon -= 2.0f;
    switch (mode)
    {
    // "C LookTo": at the frame SetLookTo gave
    case CAMERA_C_LOOK_TO:
        {
            I3D_frame *frm = look_to;
            if (!frm)
                return;
            if (!camera)
                return;
            camera->SetDir(frm->GetWorldPos() - camera->GetPos(), 0.0f);
            camera->Update();
        }
        return;
    // Round the player, a turn in about 31 seconds
    case CAMERA_ROUND:
        {
            static float rot = 0.0f;
            rot += time * 0.0002f;
            if (rot > 2 * PI)
                rot -= 2 * PI;
            (S_vector &)up = S_vector(0.0f, 1.0f, 0.0f);
            S_vector d;
            d.y = 0.0f;
            d.x = (float)sin(rot);
            d.z = (float)cos(rot);
            WantPos(player->GetPos() + S_vector(0.0f, 2.0f, 0.0f), player->GetPos() + S_vector(0.0f, 2.0f, 0.0f) - d, d,
                dt);
        }
        return;
    // The player's death: the camera stays where it was and turns to the killer's neck
    case CAMERA_DEATH:
        {
            if (!camera)
                return;
            if (!player)
                return;
            interpInDeath += dt;
            C_actor *killer = player->killer;
            if (interpInDeath >= 0.0f && killer && killer->frame)
            {
                S_vector target;
                // STRING: GAME 0x632640
                I3D_frame *frm = killer->frame->FindChildFrame("neck", ENUMF_ALL);
                if (frm)
                    target = frm->GetWorldPos();
                else
                    target = killer->frame->GetWorldPos() + S_vector(0.0f, 1.5f, 0.0f);
                if (interpInDeath < 1.0f)
                    target = destInDeath * (1.0f - interpInDeath) + target * interpInDeath;
                camera->SetPos(cameraInDeath);
                camera->SetDir(target - cameraInDeath, 0.0f);
                camera->Update();
                return;
            }
            I3D_frame *frm = player->frame->FindChildFrame("neck", ENUMF_ALL);
            if (!frm)
                return;
            camera->SetPos(cameraInDeath);
            camera->SetDir(frm->GetWorldPos() - cameraInDeath, 0.0f);
            camera->Update();
            destInDeath = cameraInDeath + camera->GetWorldDir();
        }
        return;
    case CAMERA_C_LOCK:
        return;
    }
    // Into the player's eyes while the camera comes close to his head
    if (flags & CAMPF_EYES)
    {
        needLookFromEyes -= dt;
        if (needLookFromEyes < 0.0f)
            needLookFromEyes = 0.0f;
        if (player && camera)
        {
            if ((player->GetPos() + S_vector(0.0f, EYE_HEIGHT - poklesVDrepu, 0.0f) - camBehindPos).Square() < 1.0f)
                needLookFromEyes = 0.25f;
        }
        if (needLookFromEyes > 0.0f)
        {
            if (faktorVOcich < 1.0f)
            {
                faktorVOcich += time * 0.002f;
                if (faktorVOcich > 1.0f)
                    faktorVOcich = 1.0f;
            }
        }
        else if (faktorVOcich > 0.0f)
        {
            faktorVOcich -= time * 0.002f;
            if (faktorVOcich < 0.0f)
                faktorVOcich = 0.0f;
        }
    }
    // LookAround's turn
    if (look_around_want != look_around)
    {
        if (look_around_want > look_around)
        {
            look_around += time * 0.008;
            if (look_around > look_around_want)
                look_around = look_around_want;
        }
        else
        {
            look_around -= time * 0.008;
            if (look_around < look_around_want)
                look_around = look_around_want;
        }
    }
    if (car)
    {
        if (unknown_20)
            return;
        if (car->IsVehicleUnstable())
        {
            if (faktorStability > 0.0f)
            {
                faktorStability -= time * 0.01f;
                if (faktorStability < 0.0f)
                    faktorStability = 0.0f;
            }
        }
        else if (faktorStability < 1.0f)
        {
            faktorStability += time * 0.003f;
            if (faktorStability > 1.0f)
                faktorStability = 1.0f;
        }
        if (mode_changed)
            faktorStability = 0.0f;
        switch (mode)
        {
        // "C Beh. Hellboy", "C Behind Emeth", "C Behind3": behind the car, turned by LookAround. The y of the
        // direction and the position and direction are filtered with the car's speed; WantPosCar puts the camera at
        // the point it looks at minus look.
        case CAMERA_C_BEHIND_HELLBOY:
        case CAMERA_C_BEHIND_EMETH:
        case CAMERA_C_BEHIND3:
            {
                static float oldDirY;
                float speed = car->speed / 15.0f;
                float k = dt * 40.0f;
                // Two statements: VC6 multiplies a flat chain of the five factors by faktorStability first
                float f = k * p[CAMV_DIR_FILTER] * speed * speed;
                f *= faktorStability;
                if (f > 1.0f)
                    f = 1.0f;
                if (mode_changed)
                {
                    f = 1.0f;
                    oldDirY = 0.0f;
                }
                dir = car->GetDir() * (p[CAMV_C_HEADING] * 10.0f);
                if (!mode_changed && car->speed > 0.001f)
                    dir += car->GetVelocity() * (p[CAMV_C_VELOCITY] * 10.0f) / car->speed;
                dir.y = (dir.y * f + (1.0f - f) * oldDirY) * faktorStability;
                oldDirY = dir.y;
                dir.Normalize();
                float c = (float)cos(look_around * PI);
                float s = (float)sin(look_around * PI);
                dir *= p[CAMV_C_ALONG] * 20.0f;
                dir -= S_vector(0.0f, p[CAMV_C_DOWN] * 10.0f, 0.0f);
                S_vector pos;
                pos = -dir + S_vector(0.0f, p[CAMV_C_HEIGHT] * 10.0f, 0.0f);
                dir *= f;
                pos *= k * p[CAMV_POS_FILTER];
                dir += oldDir * (1.0f - f);
                pos += oldPos * (1.0f - k * p[CAMV_POS_FILTER]);
                oldPos = pos;
                oldDir = dir;
                dir.Normalize();
                dir *= p[CAMV_C_DISTANCE] * 10.0f;
                S_vector side(car->GetDir().z, 0.0f, -car->GetDir().x);
                side.Normalize();
                if (c >= 0.0f)
                {
                    look = dir * (1.0f - fabs(s));
                    look += side * p[CAMV_C_SIDE] * 10.0f * s;
                }
                else
                {
                    look = car->GetDir() * p[CAMV_C_SIDE] * 10.0f * (fabs(s) - 1.0f);
                    look += side * p[CAMV_C_SIDE] * 10.0f * s;
                }
                float max_y = -(car->GetVehicleSize(false).y - p[CAMV_C_HEIGHT] * 10.0f);
                if (look.y > max_y)
                    look.y = max_y;
                WantPosCar(car->unknown_398 + S_vector(0.0f, p[CAMV_C_HEIGHT] * 10.0f, 0.0f), look);
            }
            break;
        // Only the heights of the direction and the position are filtered; the camera turns round the car with
        // LookAround
        case CAMERA_C_AROUND:
            {
                static float oldDirY;
                float speed = car->speed / 15.0f;
                float t = dt * 40.0f;
                // Grouped: VC6 multiplies a flat chain of the factors by faktorStability first
                float k = (t * p[CAMV_DIR_FILTER] * speed * speed) * faktorStability;
                if (k > 1.0f)
                    k = 1.0f;
                if (mode_changed)
                {
                    k = 1.0f;
                    oldDirY = 0.0f;
                }
                dir = car->GetDir() * (p[CAMV_C_HEADING] * 10.0f);
                oldDirY = dir.y;
                dir.Normalize();
                float angle = look_around * PI;
                float c = (float)cos(angle);
                float s = (float)sin(angle);
                S_vector perp(dir.z, 0.0f, -dir.x);
                dir *= c;
                dir += perp * s;
                dir *= p[CAMV_C_ALONG] * 20.0f;
                dir -= S_vector(0.0f, p[CAMV_C_DOWN] * 10.0f, 0.0f);
                S_vector pos;
                pos = -dir + S_vector(0.0f, p[CAMV_C_HEIGHT] * 10.0f, 0.0f);
                dir.y *= k;
                pos.y = (pos.y * t) * p[CAMV_POS_FILTER];
                dir.y += (1.0f - k) * oldDir.y;
                pos.y += (1.0f - t * p[CAMV_POS_FILTER]) * oldPos.y;
                oldDir = dir;
                oldPos = pos;
                dir.Normalize();
                dir *= p[CAMV_C_DISTANCE] * 10.0f;
                S_vector side(car->GetDir().z, 0.0f, -car->GetDir().x);
                side.Normalize();
                look = dir * (1.0f - fabs(s));
                look += side * p[CAMV_C_SIDE] * 10.0f * s;
                WantPosCar(car->unknown_398 + S_vector(0.0f, p[CAMV_C_HEIGHT] * 10.0f, 0.0f), look);
            }
            break;
        // As the three before, the direction turned towards the velocity at speed (forward or backward by unknown_c64)
        case CAMERA_C_BEHIND_HELLBOY_11:
            {
                static float oldDirY;
                float s = car->speed / 15.0f;
                float t = dt * 40.0f;
                float k = t * p[CAMV_DIR_FILTER] * s * s;
                k *= faktorStability;
                if (k > 1.0f)
                    k = 1.0f;
                if (mode_changed)
                {
                    k = 1.0f;
                    oldDirY = 0.0f;
                }
                dir = car->GetDir() * (p[CAMV_C_HEADING] * 10.0f);
                if (!mode_changed && car->speed > 0.001f)
                    dir += car->GetVelocity() * (p[CAMV_C_VELOCITY] * 10.0f) / car->speed;
                // Three statements, as PS2's three stores
                dir.y *= k;
                dir.y += oldDirY * (1.0f - k);
                dir.y *= faktorStability;
                oldDirY = dir.y;
                dir.Normalize();
                if (s > 1.0f)
                    s = 1.0f;
                dir *= s;
                dir += car->GetDir() * (car->speed > 0.4f ? (car->unknown_c64 < PI / 2 ? 1 : -1) : 0.0f) *
                    (1.0f - s);
                dir.Normalize();
                float c = (float)cos(look_around * PI);
                float sn = (float)sin(look_around * PI);
                // PS2 computes it here and never reads it; VC6 drops it
                S_vector right0(dir.z, 0.0f, -dir.x);
                dir *= p[CAMV_C_ALONG] * 20.0f;
                dir -= S_vector(0.0f, p[CAMV_C_DOWN] * 10.0f, 0.0f);
                S_vector pos;
                pos = -dir + S_vector(0.0f, p[CAMV_C_HEIGHT] * 10.0f, 0.0f);
                dir *= k;
                pos *= t * p[CAMV_POS_FILTER];
                dir += oldDir * (1.0f - k);
                pos += oldPos * (1.0f - t * p[CAMV_POS_FILTER]);
                oldPos = pos;
                oldDir = dir;
                dir.Normalize();
                dir *= p[CAMV_C_DISTANCE] * 10.0f;
                S_vector right(car->GetDir().z, 0.0f, -car->GetDir().x);
                right.Normalize();
                if (c >= 0.0f)
                {
                    look = dir * (1.0f - fabs(sn));
                    look += right * p[CAMV_C_SIDE] * 10.0f * sn;
                }
                else
                {
                    look = car->GetDir() * p[CAMV_C_SIDE] * 10.0f * (fabs(sn) - 1.0f);
                    look += right * p[CAMV_C_SIDE] * 10.0f * sn;
                }
                float max_y = -(car->GetVehicleSize(false).y - p[CAMV_C_HEIGHT] * 10.0f);
                if (look.y > max_y)
                    look.y = max_y;
                WantPosCar(car->unknown_398 + S_vector(0.0f, p[CAMV_C_HEIGHT] * 10.0f, 0.0f), look);
            }
            break;
        // "C Chase" (PS2's is another camera, with C_Vehicle::GetWheelCamPos; the PC's has C Over's code)
        case CAMERA_C_CHASE:
            dir = S_vector(sin(look_around * PI), 0.0f, cos(look_around * PI));
            camera->SetDir(dir, 0.0f);
            camera->Update();
            break;
        // "C In": at the driver's head, tilted by the wheels' vectors (filtered)
        case CAMERA_C_IN:
            {
                dir = S_vector(sin(look_around * PI), -0.03f, cos(look_around * PI));
                S_vector sum = car->wheels[0]->unknown_7c;
                for (int i = car->num_wheels - 1; i > 0; i--)
                    sum += car->wheels[i]->unknown_7c;
                sum = sum.RotateByMatrix(car->GetInvModelMatrix(false));
                sum *= 3e-6f;
                // PS2's names and order: oldSpeed (guard init$2496) first, oldS (init$2499) second
                static S_vector oldSpeed;
                static S_vector oldS;
                sum *= 0.1f;
                sum += oldS * 0.9f;
                sum.y = 1.0f;
                dir.y += sum.z;
                camera->SetDir1(dir, sum);
                oldS = sum;
                sum.x = 0.0f;
                sum.y = 0.0f;
                camera->SetPos(car->GetHeadPos() - sum);
                camera->Update();
                oldSpeed = car->GetVelocity();
            }
            break;
        // "C Over" (PS2's goes on with g_collision::TestSphereB and SetMode(-1, true); the PC's doesn't)
        case CAMERA_C_OVER:
            dir = S_vector(sin(look_around * PI), 0.0f, cos(look_around * PI));
            camera->SetDir(dir, 0.0f);
            camera->Update();
            break;
        // 50 above the car, looking down
        case CAMERA_C_ABOVE:
            camera->SetPos(car->GetPos() + S_vector(0.0f, 50.0f, 0.0f));
            camera->SetDir1(S_vector(0.0f, -1.0f, 0.0f), car->GetDir());
            camera->Update();
            break;
        // "C Racing": from the race camera whose range of gates holds the car's last one (PS2 also ticks an
        // S_auto_hide_machine when it switches cameras; the PC doesn't)
        case CAMERA_C_RACING:
            {
                if (!race_cameras.size())
                    return;
                if (!activeRaceCam)
                    activeRaceCam = race_cameras[0];
                if (!CRacing)
                    return;
                int gate = -1;
                for (uint i = 0; i < CRacing->num_opponents; i++)
                {
                    if (CRacing->opponents[i]->car == car)
                    {
                        gate = CRacing->opponents[i]->place->gate;
                        break;
                    }
                }
                if (gate <= activeRaceCam->gate_from || gate > activeRaceCam->gate_to)
                {
                    for (uint j = 0; j < race_cameras.size(); j++)
                    {
                        if (gate > race_cameras[j]->gate_from && gate <= race_cameras[j]->gate_to)
                        {
                            activeRaceCam = race_cameras[j];
                            break;
                        }
                    }
                }
                S_vector pos = activeRaceCam->frame->GetWorldPos();
                S_vector d = car->GetPos() - pos;
                camera->SetPos(pos);
                camera->SetDir(d, 0.0f);
                camera->Update();
            }
            break;
        }
    }
    else
    {
        // The function's start tested it already
        if (!player)
            return;
        // The camera drops while the player crouches
        if (player->crouching)
        {
            poklesVDrepu += dt * 2.3f;
            if (poklesVDrepu > 0.7f)
                poklesVDrepu = 0.7f;
        }
        else
        {
            poklesVDrepu -= dt * 2.8f;
            if (poklesVDrepu < 0.0f)
                poklesVDrepu = 0.0f;
        }
        switch (mode)
        {
        // "C Dialog": beside the dialog's two humans, on the side dialogSide picks
        case CAMERA_C_DIALOG:
            {
                if (!camera)
                    break;
                if (!dialogLookFrom)
                    break;
                if (!dialogLookTo)
                    break;
                S_vector from = dialogLookFrom->GetPos() + S_vector(0.0f, 1.7f, 0.0f);
                S_vector to = dialogLookTo->GetPos() + S_vector(0.0f, 1.7f, 0.0f);
                S_vector axis;
                axis = to - from;
                axis.Normalize();
                S_vector side;
                side = S_vector(axis.z, 0.0f, -axis.x);
                side.Normalize();
                S_vector target = from * 0.7f + to * 0.3f;
                target = target * 0.5f + (from + axis * 0.5f) * 0.5f;
                S_vector pos = from - axis * 0.3f + side * 0.5f * dialogSide + S_vector(0.0f, 0.3f, 0.0f);
                S_vector d;
                d = pos - target;
                camera->SetPos(pos);
                camera->SetDir(-d, 0.0f);
                camera->Update();
            }
            break;
        // "CP Sniper": along the player's direction tilted by tilt, swaying less while he crouches
        case CAMERA_CP_SNIPER:
            {
                static float kl = 0.0f;
                static float kl2 = 0.0f;
                kl += dt * 2.5f;
                kl2 += dt * 3.0f;
                if (kl > 2 * PI)
                    kl -= 2 * PI;
                if (kl2 > 2 * PI)
                    kl2 -= 2 * PI;
                look = player->GetDir();
                S_vector pos = player->GetPos() + S_vector(0.0f, EYE_HEIGHT - poklesVDrepu, 0.0f);
                float amp = 0.0015f * (1.0 - poklesVDrepu * 0.5) * (1.0f + unknown_8c * 2.0f);
                look.x += look.z * amp * (float)sin(kl2);
                look.z += look.x * amp * (float)sin(kl2);
                look *= (float)sin(tilt * PI);
                look.y = (float)cos(tilt * PI);
                look.y += amp * (float)sin(kl) * 2.0f;
                if (swing)
                {
                    float roll = (float)sin(naklon * PI) * swing_amount;
                    camera->SetPos(pos);
                    camera->SetDir1(look, S_vector(0.0f, 1.0f, roll));
                    camera->Update();
                    if (swing_frame)
                    {
                        act_mission->scene->backdrop_sector->SetDir1(S_vector(0.0f, -roll, 1.0f),
                            S_vector(0.0f, 1.0f, roll));
                        act_mission->scene->backdrop_sector->Update();
                        swing_frame->SetDir1(S_vector(0.0f, -roll, 1.0f), S_vector(0.0f, 1.0f, roll));
                        swing_frame->Update();
                    }
                }
                else
                {
                    camera->SetPos(pos);
                    camera->SetDir(look, 0.0f);
                    camera->Update();
                }
            }
            break;
        // "P In": from the player's neck, or above his position without one; then as the sniper camera without the
        // sway
        case CAMERA_P_IN:
            {
                I3D_frame *frm = player->frame->FindChildFrame("neck", ENUMF_ALL);
                if (!frm)
                    look = player->GetDir();
                else
                    look = frm->GetWorldDir();
                S_vector pos;
                if (!frm)
                    pos = player->GetPos() + S_vector(0.0f, 1.75f, 0.0f);
                else
                    pos = frm->GetWorldPos() + S_vector(0.0f, 0.2f, 0.0f);
                look *= (float)sin(tilt * PI);
                look.y = (float)cos(tilt * PI);
                if (swing)
                {
                    float roll = (float)sin(naklon * PI) * swing_amount;
                    camera->SetPos(pos);
                    camera->SetDir1(look, S_vector(0.0f, 1.0f, roll));
                    camera->Update();
                    if (swing_frame)
                    {
                        act_mission->scene->backdrop_sector->SetDir1(S_vector(0.0f, -roll, 1.0f),
                            S_vector(0.0f, 1.0f, roll));
                        act_mission->scene->backdrop_sector->Update();
                        swing_frame->SetDir1(S_vector(0.0f, -roll, 1.0f), S_vector(0.0f, 1.0f, roll));
                        swing_frame->Update();
                    }
                }
                else
                {
                    camera->SetPos(pos);
                    camera->SetDir(look, 0.0f);
                    camera->Update();
                }
            }
            break;
        // "C RailwayIn": above the frame's position, along the player's direction tilted by tilt
        case CAMERA_C_RAILWAY_IN:
            {
                look = player->GetDir();
                S_vector pos = player->frame->GetPos() + S_vector(0.0f, 1.85f, 0.0f);
                look *= (float)sin(tilt * PI);
                look.y = (float)cos(tilt * PI);
                camera->SetDir(look, 0.0f);
                camera->SetPos(pos);
                camera->Update();
            }
            break;
        // "P Behind" (camera 23): C RailwayIn lower
        case CAMERA_P_BEHIND_23:
            {
                look = player->GetDir();
                S_vector pos = player->frame->GetPos() + S_vector(0.0f, 1.45f, 0.0f);
                look *= (float)sin(tilt * PI);
                look.y = (float)cos(tilt * PI);
                camera->SetDir(look, 0.0f);
                camera->SetPos(pos);
                camera->Update();
            }
            break;
        // "P Behind": behind and above the player, along his direction tilted by tilt; the eyes' position and
        // direction for the move into the head
        case CAMERA_P_BEHIND:
            {
                (S_vector &)up = S_vector(0.0f, 1.0f, 0.0f);
                float s = (float)sin(tilt * PI);
                float c = (float)cos(tilt * PI);
                dir = player->GetDir();
                S_vector target = dir * (p[CAMV_P_AHEAD] * 10.0f - 5.0f) +
                    S_vector(0.0f, p[CAMV_P_HEIGHT] * 5.0f - poklesVDrepu, 0.0f);
                S_vector behind = target + dir * c * p[CAMV_P_BEHIND] * -5.0f +
                    S_vector(0.0f, s * p[CAMV_P_ABOVE] * 5.0f, 0.0f);
                S_vector d = dir * s;
                d.y = c;
                S_vector eye = behind + d * (p[CAMV_P_ALONG] * 10.0f - 5.0f);
                I3D_frame *neck = player->frame->FindChildFrame("neck", ENUMF_ALL);
                camInDir = player->GetDir();
                if (!neck)
                    camInPos = player->GetPos() + S_vector(0.0f, EYE_HEIGHT, 0.0f);
                else
                    camInPos = neck->GetWorldPos() + S_vector(0.0f, 0.2f, 0.0f);
                camInDir *= (float)sin(tilt * PI);
                camInDir.y = (float)cos(tilt * PI);
                WantPos(target + player->GetPos(), eye + player->GetPos(), d, dt);
            }
            break;
        // P Behind turned by the mouse: its last FILTERING_SIZE movements averaged with weights that fall with their age
        // turn cameraDirRot and tilt cameraDirUpDown; the player turns with the camera
        case CAMERA_P_MOUSE:
            {
                static float cameraDirRot = 0.0f;
                static float cameraDirUpDown = 0.0f;
                filteringBuffer[filteringLastPos][0] = (float)igraph->Mouse_rx();
                filteringBuffer[filteringLastPos][1] = (float)igraph->Mouse_ry();
                filteringBuffer[filteringLastPos][2] = time * 0.001f;
                filteringLastPos++;
                if (filteringLastPos >= FILTERING_SIZE)
                    filteringLastPos = 0;
                float sumX = 0.0f, sumY = 0.0f, sumTime = 0.0f, weight = 1.0f, sumWeight = 0.0f;
                int i = filteringLastPos - 1;
                while (i-- != filteringLastPos)
                {
                    if (i < 0)
                        i = FILTERING_SIZE - 1;
                    sumX += weight * filteringBuffer[i][0];
                    sumY += weight * filteringBuffer[i][1];
                    sumTime += filteringBuffer[i][2];
                    sumWeight += weight;
                    weight *= (float)pow(mafiaLoadSave.settings.unknown_c, mafiaLoadSave.settings.unknown_8 * sumTime *
                        100.0f);
                }
                cameraDirRot += (sumX / sumWeight) * mafiaLoadSave.settings.aiming_sensitivity_x * 0.02f;
                cameraDirUpDown += (sumY / sumWeight) * mafiaLoadSave.settings.aiming_sensitivity_y * 0.007f;
                if (cameraDirRot > 2 * PI)
                    cameraDirRot -= 2 * PI;
                if (cameraDirRot < 0.0f)
                    cameraDirRot += 2 * PI;
                if (cameraDirUpDown > 0.99f)
                    cameraDirUpDown = 0.99f;
                if (cameraDirUpDown < 0.01f)
                    cameraDirUpDown = 0.01f;
                (S_vector &)up = S_vector(0.0f, 1.0f, 0.0f);
                float s = (float)sin(cameraDirUpDown * PI);
                float c = (float)cos(cameraDirUpDown * PI);
                dir = S_vector((float)sin(cameraDirRot), 0.0f, (float)cos(cameraDirRot));
                S_vector side(dir.z, 0.0f, -dir.x);
                dir *= (float)cos(look_around * PI);
                dir += side * (float)sin(look_around * PI);
                S_vector target;
                target = dir * (p[CAMV_P_AHEAD] * 10.0f - 5.0f) +
                    S_vector(0.0f, p[CAMV_P_HEIGHT] * 5.0f - poklesVDrepu, 0.0f);
                S_vector behind = target + dir * c * p[CAMV_P_BEHIND] * -5.0f +
                    S_vector(0.0f, s * p[CAMV_P_ABOVE] * 5.0f, 0.0f);
                S_vector d = dir * s;
                d.y = c;
                S_vector eye = behind + d * (p[CAMV_P_ALONG] * 10.0f - 5.0f);
                I3D_frame *neck = player->frame->FindChildFrame("neck", ENUMF_ALL);
                camInDir = S_vector((float)sin(cameraDirRot), 0.0f, (float)cos(cameraDirRot));
                if (!neck)
                    camInPos = player->GetPos() + S_vector(0.0f, EYE_HEIGHT, 0.0f);
                else
                    camInPos = neck->GetWorldPos() + S_vector(0.0f, 0.2f, 0.0f);
                camInDir *= (float)sin(tilt * PI);
                camInDir.y = (float)cos(tilt * PI);
                mode_changed = true;
                WantPos(target + player->GetPos(), eye + player->GetPos(), d, dt);
                S_vector cam_dir = camera->GetDir();
                cam_dir.y = 0.0f;
                cam_dir.Normalize();
                player->dir = cam_dir;
            }
            break;
        }
    }
    mode_changed = false;
}

// Puts the camera at target - look, or where the collision test from target back along look stops short of it, and
// turns it along look, or with free_look lets the player look around (PS2 name; PS2 scales look.y by a third
// argument where the PC keeps look.y at -5 or more). The car cameras of Tick call it last.
// FUNCTION: GAME 0x5f2100
void G_Camera::WantPosCar(const S_vector &target, const S_vector &look)
{
    S_vector dir = look;
    if (dir.y < -5.0f)
        dir *= -5.0f / dir.y;
    // Assigned, not initialized: the original computes it into a temporary and copies it (PS2 the same)
    S_vector pos;
    pos = target - dir;
    gifCollision.TestSphereB(target, -dir, 0.35f, &pos, NULL, COLF_ALL, 0);
    camera->SetPos(pos);
    if (free_look)
    {
        dir.Normalize();
        // Both assigned too: initialized, VC6 builds them in place and the frame shrinks by 8 bytes
        S_vector right;
        right = dir.Cross(car->unknown_c54);
        right.Normalize();
        S_vector up;
        up = right.Cross(dir);
        FreeLookBase(controls.GetValue(CTRL_AIM, 0), controls.GetValue(CTRL_AIM + CTRL_SECONDARY, 0), dir, up, right);
    }
    else
        camera->SetDir(dir, 0.0f);
    camera->Update();
}

// The camera on foot (PS2 name): from pos stepped along look, pushed out of the walls, as an offset from target
// blended with last frame's (oldPos, oldDir, oldDist), then placed at target plus the offset (camBehindPos) blended
// towards camInPos, swung or not, and turned along look (or by the mouse's free look)
// FUNCTION: GAME 0x5f2580
void G_Camera::WantPos(const S_vector &target, const S_vector &pos, const S_vector &look, float time)
{
    S_vector mid = pos + look * 2.0f;
    float dist = 1.0f;
    mid *= 0.5f;
    mid += target * 0.5f;
    S_vector front = pos + look * 0.33f;
    // Unused (PS2 computes it too)
    S_vector delta = front - mid;
    // PC only, as the 1.6 (PS2 1.4): PS2 tests from the head always
    if (look.y < 0.0f)
    {
        S_vector head = player->GetWorldPos() + S_vector(0.0f, 1.6f, 0.0f);
        gifCollision.TestSphereB(head, mid - head, 0.29f, &mid, NULL, COLF_UNKNOWN_2 | COLF_UNKNOWN_100,
            COLF2_UNKNOWN_100000);
    }
    gifCollision.TestSphereB(mid, front - mid, 0.29f, &front, NULL, COLF_UNKNOWN_2 | COLF_UNKNOWN_100,
        COLF2_UNKNOWN_100000);
    // PS2's also goes back along look (- look * 0.05f), with COLF2_UNKNOWN_100000 for the last flags, and PS2 tests
    // from the head to camInPos after it
    S_vector cam_pos = front + S_vector(0.0f, 0.1f, 0.0f);
    gifCollision.TestSphereB(front, cam_pos - front, 0.29f, &cam_pos, NULL, COLF_ALL, 0);
    S_vector dir = look;
    cam_pos -= target;
    // Read once, before the test, into a register of its own (edi, look's), PS2 the same (s1): a local
    float *p = params;
    // PS2 also skips it while mafiaIndicators has its 0x20
    if (!mode_changed)
    {
        time *= 40.0f;
        if (time > 1.0f)
            time = 1.0f;
        dir *= p[CAMV_DIR_FILTER] * time;
        cam_pos *= p[CAMV_POS_FILTER] * time;
        dir += oldDir * (1.0f - p[CAMV_DIR_FILTER] * time);
        cam_pos += oldPos * (1.0f - p[CAMV_POS_FILTER] * time);
        // PS2's square (mul.s $f20, $f20, $f20); dist is 1 here, so VC6 folds it to p[CAMV_DIR_FILTER] + oldDist * (1 -
        // p[CAMV_DIR_FILTER])
        dist = dist * dist * p[CAMV_DIR_FILTER] + oldDist * (1.0f - p[CAMV_DIR_FILTER]);
    }
    if (!unknown_20)
    {
        if (swing)
        {
            // The blend reads the sum's temporary, not camBehindPos (the other branch and PS2 read the global): VC6
            // drops behind. Open: the original shares the product's temporary with the sum's ([esp + 0x40]) and has
            // pp at [esp + 0x58]; ours has them at [esp + 0x58] and [esp + 0x64] (the up vectors follow)
            S_vector behind = camBehindPos = cam_pos + target;
            S_vector pp = behind * (1.0f - faktorVOcich) + camInPos * faktorVOcich;
            S_vector d = player->GetDir();
            // Direct initialization: with = S_vector(...) the second argument of swing_frame's SetDir1 goes out of line
            S_vector side(d.x, 1.0f, d.z);
            camera->SetPos(pp + side * unknown_8c * 0.2f);
            float roll = swing_amount * (float)sin(naklon * PI);
            camera->SetDir1(dir, S_vector(0.0f, 1.0f, roll));
            camera->Update();
            if (swing_frame)
            {
                act_mission->scene->backdrop_sector->SetDir1(S_vector(0.0f, -roll, 1.0f), S_vector(0.0f, 1.0f, roll));
                act_mission->scene->backdrop_sector->Update();
                swing_frame->SetDir1(S_vector(0.0f, -roll, 1.0f), S_vector(0.0f, 1.0f, roll));
                swing_frame->Update();
            }
        }
        else
        {
            // The operators and SetPos out of line (the budget); side is unused here, but its constructor is a call
            camBehindPos = cam_pos + target;
            S_vector pp = camBehindPos * (1.0f - faktorVOcich) + camInPos * faktorVOcich;
            S_vector d = player->GetDir();
            S_vector side(d.x, 1.0f, d.z);
            camera->SetPos(pp);
            if (free_look)
                FreeLook(controls.GetValue(CTRL_AIM, 0), controls.GetValue(CTRL_AIM + CTRL_SECONDARY, 0));
            else
                camera->SetDir(dir, 0.0f);
            camera->Update();
        }
    }
    oldPos = cam_pos;
    oldDir = dir;
    oldDist = dist;
}

// PS2's cases in PS2's order, two of them (C Over, C In) as PS2's; "C Chase" places the camera between the first two
// wheels where PS2's in-cockpit case does, without PS2's test, and the death camera links the camera to the scene
// without PS2's test for a wall behind it. The two cases on another frame are each written out, as PS2 has them.
// Open: the original shares the LinkTo of the cases on another frame with the default's (the case loads link and
// jumps into it), where ours schedules the camera's load first and keeps them apart; the sniper's and the death
// camera's scratch registers come one later in the rotation (eax for edx), and "C Chase"'s sum adds x the other way.
// FUNCTION: GAME 0x5f2ea0
void G_Camera::Begin()
{
    if (!camera)
        return;
    unknown_14 = 0;
    int i;
    for (i = 0; i < NUM_CAMERAS; i++)
    {
        if (cameraParams[i].id == mode)
        {
            params = cameraParams[i].values;
            flags = cameraParams[i].flags;
            unknown_22 = (flags & CAMPF_UNKNOWN_1) != 0;
            unknown_14 = cameraParams[i].next;
            if (car && car->frame)
            {
                car->frame->SetOn(!(flags & CAMPF_UNKNOWN_2));
                car->unknown_2119 = (flags & CAMPF_UNKNOWN_2) != 0;
            }
            if (!(flags & CAMPF_EYES))
            {
                if (player && player->GetType() == ACTOR_PLAYER)
                    ((C_player *)player)->FadeReset();
                needLookFromEyes = 0.0f;
            }
            break;
        }
    }
    if (i >= NUM_CAMERAS)
    {
        if (player && player->GetType() == ACTOR_PLAYER)
            ((C_player *)player)->FadeReset();
        needLookFromEyes = 0.0f;
    }
    switch (mode)
    {
    case CAMERA_C_OVER:
        if (car)
        {
            camera->LinkTo(car->frame, 0);
            camera->SetPos(car->GetWheelCamPos(0, S_vector(0.5f, 0.0f, -1.0f)));
            camera->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
        }
        break;
    case CAMERA_C_IN:
        if (car)
        {
            camera->LinkTo(car->frame, 0);
            camera->SetPos(car->GetHeadPos());
            camera->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
        }
        rotInAct = 0.0f;
        break;
    case CAMERA_C_CHASE:
        if (car)
        {
            camera->LinkTo(car->frame, 0);
            camera->SetPos((car->GetWheelCamPos(0, S_vector(0.0f, 0.0f, 0.0f)) +
                car->GetWheelCamPos(1, S_vector(0.0f, 0.0f, 0.0f))) * 0.5f + S_vector(0.0f, 0.2f, 0.0f));
            camera->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
        }
        break;
    case CAMERA_C_RAILWAY_IN:
        camera->LinkTo(link, I3DLINK_UPDMATRIX);
        break;
    case CAMERA_P_BEHIND_23:
        camera->LinkTo(link, I3DLINK_UPDMATRIX);
        break;
    case CAMERA_CP_SNIPER:
        mafiaIndicators.unknown_40a4 &= ~INDF_CROSSHAIR;
        mafiaIndicators.unknown_40a4 |= INDF_SNIPER;
        break;
    case CAMERA_DEATH:
        cameraInDeath = camera->GetWorldPos();
        interpInDeath = -1.0f;
        destInDeath = cameraInDeath + camera->GetWorldDir();
        camera->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
        break;
    default:
        camera->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
    }
    camera->Update();
    mode_changed = true;
}

// PS2's without its car's frame test (and an empty loop PS2 has at the start)
// FUNCTION: GAME 0x5f3450
void G_Camera::End()
{
    if (!camera)
        return;
    switch (mode)
    {
    case CAMERA_CP_SNIPER:
        mafiaIndicators.unknown_40a4 &= ~INDF_SNIPER;
        mafiaIndicators.unknown_40a4 |= INDF_CROSSHAIR;
        break;
    case CAMERA_C_RAILWAY_IN:
        camera->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
        break;
    case CAMERA_P_BEHIND_23:
        camera->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
        break;
    }
    camera->Update();
    if (car)
    {
        car->frame->SetOn(true);
        car->unknown_2119 = false;
    }
}

// The mouse's free look (PS2 name): turns the camera by the mouse's movement (PS2's 800 steps of pi / 500 per unit),
// times the profile's sensitivity and inverted with it (PS2 has neither), up and down to just short of straight.
// VC6's choices it hangs on: dy as an if/else (a ?: of the two factors loads the factor first, and keeps dy
// in ebp, this spilled), z's square first under the sqrt (VC6 computes the left term last), and the last two products
// in double (with (float) casts on sin and cos the last one doesn't pop r)
// FUNCTION: GAME 0x5f3520
void G_Camera::FreeLook(float x, float y)
{
    int dx = (int)(mafiaLoadSave.settings.aiming_sensitivity_x * x * 800.0f);
    int dy;
    if (mafiaLoadSave.settings.mouse_inverted)
        dy = (int)(mafiaLoadSave.settings.aiming_sensitivity_y * y * -800.0f);
    else
        dy = (int)(mafiaLoadSave.settings.aiming_sensitivity_y * y * 800.0f);
    if (dx || dy)
    {
        S_vector dir = camera->GetDir();
        S_vector new_dir = dir;
        float r = (float)sqrt(dir.z * dir.z + dir.x * dir.x);
        float a = (float)atan2(-dir.x, dir.z);
        if (dx)
            a -= dx * PI / 500.0f;
        if (dy)
        {
            float b = (float)atan2(dir.y, r);
            b -= dy * PI / 500.0f;
            if (b > PI)
                b -= PI / 2.0f;
            // Just short of straight up and down: the original's constant (1.5676548f) is this product folded (PI *
            // 0.499f folds to it too; PS2 has the literal)
            b = Max(-(PI / 2 * 0.998f), Min(PI / 2 * 0.998f, b));
            new_dir.y = (float)sin(b);
            r = (float)cos(b);
        }
        new_dir.x = r * -sin(a);
        new_dir.z = r * cos(a);
        camera->SetDir(new_dir, 0.0f);
    }
    camera->Update();
}

// Turns the camera from dir by the free look's angles, which x and y (the controls' values) turn first unless a menu
// runs: the yaw kept within 0 to 2 pi, the pitch within +-1.5 (PS2 name; PS2's turns by 3 x and 2 y, without the
// profile's sensitivities and the inverted mouse). up and right are unused, on PS2 too.
// FUNCTION: GAME 0x5f3810
void G_Camera::FreeLookBase(float x, float y, S_vector &dir, S_vector &up, S_vector &right)
{
    if (!(mafiaIndicators.unknown_40a4 & INDF_MENU))
    {
        // PS2's factor order; any order compiles alike. Each branch's update is hoisted above the branch.
        if (x < 0.0f)
        {
            freeLookX -= 3.0f * x * mafiaLoadSave.settings.aiming_sensitivity_x;
            if (freeLookX > 2 * PI)
                freeLookX -= 2 * PI;
        }
        else
        {
            freeLookX -= 3.0f * x * mafiaLoadSave.settings.aiming_sensitivity_x;
            if (freeLookX < 0.0f)
                freeLookX += 2 * PI;
        }
        if (mafiaLoadSave.settings.mouse_inverted)
        {
            if (y > 0.0f)
            {
                freeLookZ += 2.0f * y * mafiaLoadSave.settings.aiming_sensitivity_y;
                if (freeLookZ > 1.5f)
                    freeLookZ = 1.5f;
            }
            else
            {
                freeLookZ += 2.0f * y * mafiaLoadSave.settings.aiming_sensitivity_y;
                if (freeLookZ < -1.5f)
                    freeLookZ = -1.5f;
            }
        }
        else
        {
            if (y < 0.0f)
            {
                freeLookZ -= 2.0f * y * mafiaLoadSave.settings.aiming_sensitivity_y;
                if (freeLookZ > 1.5f)
                    freeLookZ = 1.5f;
            }
            else
            {
                freeLookZ -= 2.0f * y * mafiaLoadSave.settings.aiming_sensitivity_y;
                if (freeLookZ < -1.5f)
                    freeLookZ = -1.5f;
            }
        }
    }
    float yaw = (float)atan2(dir.x, dir.z) - freeLookX;
    float pitch = (float)asin(dir.y) + freeLookZ;
    float sy = (float)sin(yaw);
    float sp = (float)sin(pitch);
    float cy = (float)cos(yaw);
    float cp = (float)cos(pitch);
    // Assigned from temporaries, as in WantPosCar (initialized, the frame is 0xc bytes smaller); PS2 copies them too
    S_vector d, u;
    d = S_vector(sy * cp, sp, cy * cp);
    u = S_vector(-sy * sp, cp, -cy * sp);
    camera->SetDir1(d, u);
    camera->Update();
}

#pragma auto_inline(off)

// STUB: GAME 0x5f39f0
void G_Camera::LockAt(const S_vector &pos, const S_vector &dir)
{
    StubLog("G_Camera::LockAt");
}

// STUB: GAME 0x5f3fd0
void G_Camera::SetCutscene(bool on)
{
    StubLog("G_Camera::SetCutscene");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x5f4220
void G_Camera::PopMode(bool clear)
{
    if (!push_pop.size())
        return;
    SetMode(push_pop.back().mode, true);
    if (camera)
    {
        camera->SetFOV(push_pop.back().fov);
        camera->SetRange(push_pop.back().ncp, push_pop.back().fcp);
        camera->Update();
    }
    if (clear)
        mode_changed = false;
    push_pop.pop_back();
}

#pragma auto_inline(off)

// STUB: GAME 0x5f42e0
void G_Camera::DialogLook(C_human *from, C_human *to)
{
    StubLog("G_Camera::DialogLook");
}

// STUB: GAME 0x5f4370
void G_Camera::DialogStart(C_human **humans, uint count)
{
    StubLog("G_Camera::DialogStart");
}

// STUB: GAME 0x5f4650
void G_Camera::DialogStop()
{
    StubLog("G_Camera::DialogStop");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x5f4690
bool G_Camera::IsInDialog()
{
    return mode == CAMERA_C_DIALOG;
}

#pragma auto_inline(off)

// STUB: GAME 0x5f4ab0
void G_Camera::SetSniperMode(bool on)
{
    StubLog("G_Camera::SetSniperMode");
}

#pragma auto_inline(on)



