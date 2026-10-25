// The mission (PS2 Mission.cpp), the file at 0x53efb0-0x54913c (docs/game-files.md). Not written yet: only what main
// and InitSystem need to link.

#include "Mission.h"
#include "stub.h"

#include "I3D/I3D.h"

#pragma auto_inline(off)

// STUB: GAME 0x53f010
C_mission::C_mission()
{
    StubLog("C_mission::C_mission");
}

// STUB: GAME 0x53f110
C_mission::~C_mission()
{
    StubLog("C_mission::~C_mission");
}

// Creates only the scene, which InitSystem gives its callback. The original also creates the mission's C_tshift,
// C_web_path(50) and C_roads (PS2's names), an object of 0x38 bytes and the game, and clears a vector at 0x84.
// STUB: GAME 0x53f230
void C_mission::Init(I3D_driver *driver)
{
    StubLog("C_mission::Init");
    if (!scene)
        scene = (I3D_scene *)driver->CreateFrame(FRAME_SCENE);
}

// STUB: GAME 0x5405e0
void C_mission::Close()
{
    StubLog("C_mission::Close");
}

// STUB: GAME 0x5409d0
int C_mission::Open(const char *name, bool, uint, bool)
{
    StubLog("C_mission::Open %s", name);
    return 1;
}

// STUB: GAME 0x545b20
void S_mission_options::SetLevelOfDetail(float level)
{
    StubLog("S_mission_options::SetLevelOfDetail");
    level_of_detail = level;
}

// STUB: GAME 0x545b60
void S_mission_options::SetDrawDistance(float distance)
{
    StubLog("S_mission_options::SetDrawDistance");
    draw_distance = distance;
}

// STUB: GAME 0x545ba0
void S_mission_options::SetGameEffects(int level)
{
    StubLog("S_mission_options::SetGameEffects");
    game_effects = level;
}

// STUB: GAME 0x545bd0
void S_mission_options::SetGameSounds(int level)
{
    StubLog("S_mission_options::SetGameSounds");
    game_sounds = level;
}

// STUB: GAME 0x545c00
void S_mission_options::SetShadows(int index)
{
    StubLog("S_mission_options::SetShadows");
    shadows = index;
}

// STUB: GAME 0x545c60
int S_mission_options::GetShadows()
{
    StubLog("S_mission_options::GetShadows");
    return shadows;
}

// STUB: GAME 0x545cb0
void S_mission_options::SetParticles(int level)
{
    StubLog("S_mission_options::SetParticles");
    particles = level;
}

#pragma auto_inline(on)
