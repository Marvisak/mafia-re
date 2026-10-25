// The mission (PS2 Mission.cpp), the file at 0x53efb0-0x54913c (docs/game-files.md). Written so far: the constructor,
// the destructor, Init, UnInit and the video options' methods; the rest of what main and InitSystem call is stubs.

#include "mission/Mission.h"
#include "mission/game.h"
#include "mission/tshift.h"
#include "ai/pathweb.h"
#include "ai/road_web.h"
#include "mission/C_effects.h"
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

#pragma auto_inline(on)

// An else if on the parameter: the lower bound's branch returns at once, and the upper bound compares the parameter, where
// SetDrawDistance's reads the member back
// FUNCTION: GAME 0x545b20
void S_mission_options::SetLevelOfDetail(float level)
{
    level_of_detail = level;
    if (level < 0.4f)
        level_of_detail = 0.4f;
    else if (level > 2.0f)
        level_of_detail = 2.0f;
}

// FUNCTION: GAME 0x545b60
void S_mission_options::SetDrawDistance(float distance)
{
    draw_distance = distance;
    if (draw_distance < 0.3f)
        draw_distance = 0.3f;
    if (draw_distance > 1.0f)
        draw_distance = 1.0f;
}

// FUNCTION: GAME 0x545ba0
void S_mission_options::SetGameEffects(int level)
{
    game_effects = level;
    if (game_effects < 0)
        game_effects = 0;
    if (game_effects > 2)
        game_effects = 2;
}

// FUNCTION: GAME 0x545bd0
void S_mission_options::SetGameSounds(int level)
{
    game_sounds = level;
    if (game_sounds < 0)
        game_sounds = 0;
    if (game_sounds > 2)
        game_sounds = 2;
}

// FUNCTION: GAME 0x545c00
void S_mission_options::SetShadows(int index)
{
    int levels[6] = {0, 1, 3, 6, 9, 15};
    shadows = index;
    if (shadows < 0)
        shadows = 0;
    if (shadows > 5)
        shadows = 5;
    shadows = levels[shadows];
}

// FUNCTION: GAME 0x545c60
int S_mission_options::GetShadows()
{
    int levels[6] = {0, 1, 3, 6, 9, 15};
    for (int i = 0; i < 6; i++)
    {
        if (shadows == levels[i])
            return i;
    }
    return 2;
}

// FUNCTION: GAME 0x545cb0
void S_mission_options::SetParticles(int level)
{
    particles = level;
    if (particles < 0)
        particles = 0;
    if (particles > 2)
        particles = 2;
}
