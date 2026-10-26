// The mission (PS2 Mission.cpp), the file at 0x53efb0-0x54913c (docs/game-files.md). Written so far: the constructor,
// the destructor, Init, UnInit, Close and the video options' methods; the rest of what main and InitSystem call is
// stubs.

#include "mission/Mission.h"
#include "mission/game.h"
#include "mission/tshift.h"
#include "ai/pathweb.h"
#include "ai/road_web.h"
#include "mission/C_effects.h"
#include "mission/C_program.h"
#include "mission/C_cutscene.h"
#include "mission/m_cache.h"
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

// Close's helpers where the original has them: Close has no unwinding, so a stub defined before it changes nothing

#pragma auto_inline(off)

// STUB: GAME 0x540240
void C_mission::DelActors()
{
    StubLog("C_mission::DelActors");
}

#pragma auto_inline(on)

// PS2's in another order (its caches after the scripts and the scene, its anim cache only down to a count of its own),
// with PS2's own (its screen cleared, its second block cache, its frames' reference buffer, two arrays deleted) and
// without the cutscene's Close. The scripts are deleted through their virtual destructor, which tests the pointer itself.
// FUNCTION: GAME 0x5405e0
void C_mission::Close()
{
    if (!unknown_c)
        return;
    if (game)
    {
        game->Done();
        game->DoneIntern();
    }
    FrmActBuff_ClearAll();
    cutscene.Close();
    if (tshift)
        tshift->elems.clear();
    if (web_path)
        web_path->ClearNode();
    if (roads)
        roads->Clear();
    DelActors();
    anim_models.clear();
    base_cache.Close();
    sound_cache.Clear();
    model_cache.Clear();
    anim_cache.Clear();
    int i;
    for (i = programs.size(); i--;)
        delete programs[i];
    programs.clear();
    for (i = t_transparent.size(); i--;)
    {
        if (t_transparent[i].object)
            t_transparent[i].object->Release();
    }
    t_transparent.clear();
    scene->Close();
    unknown_c = false;
}

#pragma auto_inline(off)

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
    if (game_effects < OPTION_LOW)
        game_effects = OPTION_LOW;
    if (game_effects > OPTION_HIGH)
        game_effects = OPTION_HIGH;
}

// FUNCTION: GAME 0x545bd0
void S_mission_options::SetGameSounds(int level)
{
    game_sounds = level;
    if (game_sounds < OPTION_LOW)
        game_sounds = OPTION_LOW;
    if (game_sounds > OPTION_HIGH)
        game_sounds = OPTION_HIGH;
}

// FUNCTION: GAME 0x545c00
void S_mission_options::SetShadows(int index)
{
    int levels[NUM_SHADOWS] = {0, 1, 3, 6, 9, 15};
    shadows = index;
    if (shadows < SHADOWS_VERY_LOW)
        shadows = SHADOWS_VERY_LOW;
    if (shadows > SHADOWS_HIGHEST)
        shadows = SHADOWS_HIGHEST;
    shadows = levels[shadows];
}

// FUNCTION: GAME 0x545c60
int S_mission_options::GetShadows()
{
    int levels[NUM_SHADOWS] = {0, 1, 3, 6, 9, 15};
    for (int i = 0; i < NUM_SHADOWS; i++)
    {
        if (shadows == levels[i])
            return i;
    }
    return SHADOWS_MEDIUM;
}

// FUNCTION: GAME 0x545cb0
void S_mission_options::SetParticles(int level)
{
    particles = level;
    if (particles < OPTION_LOW)
        particles = OPTION_LOW;
    if (particles > OPTION_HIGH)
        particles = OPTION_HIGH;
}

#pragma auto_inline(off)

// STUB: GAME 0x546b70
void C_mission::FrmActBuff_ClearAll()
{
    StubLog("C_mission::FrmActBuff_ClearAll");
}

#pragma auto_inline(on)
