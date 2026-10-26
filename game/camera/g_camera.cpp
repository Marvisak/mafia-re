// The game's camera (PS2 g_camera.cpp; the PC's file is the one at 0x5ece70-0x5f4b5c, docs/game-files.md): its
// constructor, destructor and Reset, PS2's order. Written so far: those three, which the game's constructor and
// destructor run; Tick, which the menus run, is a stub.

#include "camera/g_camera.h"
#include "ui/g_loadsave.h"

#include "stub.h"

// The cameras (PS2's name, a static of the file), from 0x638700, after the file's other initialized data
// GLOBAL: GAME 0x638700
static S_camera_params cameraParams[17] = {
    {1, 0, {0.5f, 0.31f, 0.09f, 0.09f, 0.3f, 0.5f, 0.2f, 0.1f, 0.27f, 0.27f}, "P Behind", CAMPF_UNKNOWN_4},
    {3, 1, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "P In", CAMPF_UNKNOWN_4},
    {6, 0, {0.5f, 0.29f, 0.09f, 0.09f, 0.3f, 0.5f, 0.2f, 0.1f, 0.27f, 0.27f}, "P Behind", 0},
    {11, 14, {0.08f, 0.14f, 0.27f, 0.35f, 0.86f, 0.4f, 0.2f, 0.6f, 0.14f, 0.54f}, "C Beh. Hellboy", 0},
    {7, 8, {0.08f, 0.14f, 0.27f, 0.35f, 0.86f, 0.4f, 0.2f, 0.6f, 0.14f, 0.54f}, "C Beh. Hellboy", 0},
    {8, 9, {0.15f, 0.05f, 0.6f, 0.2f, 0.33f, 0.33f, 0.08f, 0.65f, 0.11f, 0.28f}, "C Behind Emeth", 0},
    {9, 12, {0.12f, 0.02f, 0.06f, 0.5f, 0.8f, 0.36f, 0.2f, 1.0f, 0.48f, 0.2f}, "C Behind3", 0},
    {12, 13, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Chase", CAMPF_UNKNOWN_2},
    {13, 14, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C In", 0},
    {14, 7, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Over", 0},
    {17, 0, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C RailwayIn", CAMPF_UNKNOWN_1},
    {23, 0, {0.5f, 0.21f, 0.09f, 0.09f, 0.3f, 0.5f, 0.2f, 0.1f, 0.27f, 0.27f}, "P Behind", CAMPF_UNKNOWN_1},
    {5, 0, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "CP Sniper", CAMPF_UNKNOWN_1},
    {21, 0, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C LookTo", CAMPF_UNKNOWN_8},
    {22, 0, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Lock", CAMPF_UNKNOWN_8},
    {19, 0, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Dialog", CAMPF_UNKNOWN_8},
    {20, 7, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}, "C Racing", 0},
};

// PS2's name, a static of the file
// GLOBAL: GAME 0x6bdb10
static float needLookFromEyes;

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
    unknown_4 = 0;
    unknown_8 = 0;
    unknown_c = 0;
    unknown_0 = 0;
    unknown_10 = mafiaLoadSave.settings.camera;
    unknown_14 = unknown_10;
    unknown_20 = false;
    unknown_21 = false;
    unknown_22 = false;
    unknown_24 = 5.0f;
    unknown_28 = 0.5f;
    unknown_2c = 0;
    unknown_30 = 0;
    unknown_58 = false;
    unknown_60 = 0;
    push_pop.clear();
    unknown_8c = 0;
    unknown_90 = 0.2f;
    params = cameraParams[0].values;
    unknown_94 = false;
    push_pop.clear();
    needLookFromEyes = 0.0f;
}

#pragma auto_inline(off)

// STUB: GAME 0x5ed4c0
void G_Camera::Tick(uint time)
{
    StubLog("G_Camera::Tick");
}

#pragma auto_inline(on)
