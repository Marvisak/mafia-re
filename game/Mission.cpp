// The mission (PS2 Mission.cpp), the file at 0x53efb0-0x54913c (docs/game-files.md). Written so far: the constructor,
// the destructor, Init and UnInit; the rest of what main and InitSystem call is stubs.

#include "Mission.h"
#include "game.h"
#include "tshift.h"
#include "pathweb.h"
#include "road_web.h"
#include "C_effects.h"
#include "I3D_cache.h"
#include "stub.h"

#include "I3D/I3D.h"

// The stores come in the members' order, the vectors' among them: an initializer list
// FUNCTION: GAME 0x53f010
C_mission::C_mission() :
    unknown_0(0),
    unknown_4(0),
    unknown_c(false),
    scene(NULL),
    tshift(NULL),
    unknown_68(NULL)
{
}

// FUNCTION: GAME 0x53f110
C_mission::~C_mission()
{
    UnInit();
}

// PS2's also creates the game's members inline (its C_game constructor is inlined), and clears a flag (0xd4c) the PC's
// mission doesn't have
// FUNCTION: GAME 0x53f230
void C_mission::Init(I3D_driver *driver)
{
    if (scene)
        return;
    scene = (I3D_scene *)driver->CreateFrame(FRAME_SCENE);
    if (!tshift)
        tshift = new C_tshift;
    if (!web_path)
        web_path = new C_web_path(50);
    if (!roads)
        roads = new C_roads;
    if (!effects)
        effects = new C_effects;
    t_transparent.clear();
    if (!game)
    {
        game = new C_game;
        game->mission = this;
    }
}

// The caches' Clear inlined, as in CloseSystem. PS2's also deletes the arrays at its 0x60.
// FUNCTION: GAME 0x53f3b0
void C_mission::UnInit()
{
    sound_cache.Clear();
    model_cache.Clear();
    anim_cache.Clear();
    Close();
    if (tshift)
    {
        delete tshift;
        tshift = NULL;
    }
    if (web_path)
    {
        delete web_path;
        web_path = NULL;
    }
    if (roads)
    {
        delete roads;
        roads = NULL;
    }
    if (effects)
    {
        delete effects;
        effects = NULL;
    }
    if (game)
    {
        delete game;
        game = NULL;
    }
    if (scene)
    {
        Close();
        scene->Release();
        scene = NULL;
    }
    if (unknown_68)
    {
        delete[] unknown_68;
        unknown_68 = NULL;
    }
}

#pragma auto_inline(off)

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
