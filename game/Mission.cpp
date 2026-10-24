// The mission (PS2 Mission.cpp), the file at 0x53efb0-0x54913c (docs/game-files.md). Not written yet: only what main
// needs to link.

#include "Mission.h"

#include "ls3df.h"

#pragma auto_inline(off)

// STUB: GAME 0x5405e0
void C_mission::Close()
{
    dbgPrintf("STUB: C_mission::Close");
}

// STUB: GAME 0x5409d0
int C_mission::Open(const char *name, bool, uint, bool)
{
    dbgPrintf("STUB: C_mission::Open %s", name);
    return 1;
}

// STUB: GAME 0x545b20
void S_mission_options::SetLevelOfDetail(float level)
{
    dbgPrintf("STUB: S_mission_options::SetLevelOfDetail");
    level_of_detail = level;
}

// STUB: GAME 0x545b60
void S_mission_options::SetDrawDistance(float distance)
{
    dbgPrintf("STUB: S_mission_options::SetDrawDistance");
    draw_distance = distance;
}

// STUB: GAME 0x545ba0
void S_mission_options::SetGameEffects(int level)
{
    dbgPrintf("STUB: S_mission_options::SetGameEffects");
    game_effects = level;
}

// STUB: GAME 0x545bd0
void S_mission_options::SetGameSounds(int level)
{
    dbgPrintf("STUB: S_mission_options::SetGameSounds");
    game_sounds = level;
}

// STUB: GAME 0x545c00
void S_mission_options::SetShadows(int index)
{
    dbgPrintf("STUB: S_mission_options::SetShadows");
    shadows = index;
}

// STUB: GAME 0x545c60
int S_mission_options::GetShadows()
{
    dbgPrintf("STUB: S_mission_options::GetShadows");
    return shadows;
}

// STUB: GAME 0x545cb0
void S_mission_options::SetParticles(int level)
{
    dbgPrintf("STUB: S_mission_options::SetParticles");
    particles = level;
}

#pragma auto_inline(on)
