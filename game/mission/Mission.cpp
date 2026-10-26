// The mission (PS2 Mission.cpp), the file at 0x53efb0-0x54913c (docs/game-files.md). Written so far: the constructor,
// the destructor, Init, UnInit, Close, Open with its two helpers and the video options' methods; the rest of what main
// and InitSystem call is stubs, OpenBin among them.

#include "mission/Mission.h"
#include "mission/game.h"
#include "mission/tshift.h"
#include "ai/pathweb.h"
#include "ai/road_web.h"
#include "mission/C_effects.h"
#include "mission/C_program.h"
#include "mission/C_cutscene.h"
#include "mission/m_cache.h"
#include "actors/C_actor.h"
#include "vehicles/C_Vehicle.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_func.h"
#include "ui/Load_progress.h"
#include "C_chunk.h"
#include "C_fstr.h"
#include "I3D_cache.h"
#include "Winstuff.h"
#include "stub.h"

#include <string.h>

#include "I3D/I3D.h"
#include "ISound/ISound.h"

// Whether the mission has a cache.bin: Open clears it, sets it to 1 when it finds the file and to 2 after OpenBin, and
// the Tick reads it (PS2 name, the same values in PS2's Open)
// GLOBAL: GAME 0x646db0
int _can_show_progress_;

// The stores come in the members' order, the vectors' among them: an initializer list
// FUNCTION: GAME 0x53f010
C_mission::C_mission() :
    unknown_0(0),
    unknown_4(0),
    unknown_c(false),
    scene(NULL),
    tshift(NULL),
    mission_name(NULL)
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
    if (mission_name)
    {
        delete[] mission_name;
        mission_name = NULL;
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

// STUB: GAME 0x5407e0
void C_mission::Tick(int time)
{
    StubLog("C_mission::Tick");
}

#pragma auto_inline(on)

static void __stdcall __SceneErrFnc(LS3D_CALLBACK_MESSAGE msg, uint prm1, uint prm2, void *context);
static void cbSectorBitFill(I3D_frame *frm);

// Replaces a string with a copy of another (the name is ours). PS2's Open copies its two names with this code twice,
// and as an inline call it gives Open the original's budget: its chunk destructors' calls out of line
static inline void CopyString(char *&dst, const char *src)
{
    if (dst)
    {
        delete[] dst;
        dst = NULL;
    }
    dst = new char[strlen(src) + 1];
    strcpy(dst, src);
}

// PS2's also copies the name into a buffer of its own first (with a prefix under a flag), opens its scripts' file
// (OpenFQLFile), shows the loading screen itself (InitProgressScreen, whose code the PC's load callback has at the
// load's start), reads the actors of a fourth file and the map after cache.bin's test; the PC's mutes the sound around
// OpenBin. The chunk files are closed through the chunk's inline Close. The inline calls of CopyString, SetScene and
// ROpen give the original's budget for the chunk's destructors: the one at the early return calls Close and the stack's
// destructor out of line (the last ROpen is the inline call still to come there), the one at the end inlines both but
// Descend. The scene in a local of its own gives the scratch registers' rotation from there on.
// Open: the inlined strcpy addresses [ecx + edx] where ours has [edx + ecx]; no spelling of the copy moved it.
// FUNCTION: GAME 0x5409d0
int C_mission::Open(const char *name, bool log, uint flags, bool progress)
{
    game->ResetUnknown363c();
    unknown_8 = 0;
    Close();
    unknown_78 = 0;
    C_actor::global_refs = 0;
    if (!WaitForCD())
        return MISSION_OPEN_NO_CD;
    C_Vehicle::SetScene(scene);
    CopyString(mission_name, name);
    if (progress && !mafiaLoadSave.progress)
        mafiaLoadSave.progress = new GM_LoadProgress(0, .7f, NULL);
    // STRING: GAME 0x6361f4
    scene->Open(C_fstr("missions\\%s\\scene.i3d", name), (log ? I3DLOAD_LOG : 0) | I3DLOAD_PROGRESS | I3DLOAD_ERRORS,
        __SceneErrFnc, NULL);
    scene->WeatherSystemReset();
    scene->SetWeatherSystemParam(WS_PARAM_ON, 0);
    I3D_scene *act_scene = act_mission->scene;
    cbSectorBitFill(act_scene->primary_sector);
    C_chunk chunk;
    // STRING: GAME 0x6361dc
    if (chunk.ROpen(C_fstr("missions\\%s\\check.bin", name)))
    {
        web_path->LoadNodes(&chunk);
        chunk.Close();
    }
    // STRING: GAME 0x6361c4
    if (chunk.ROpen(C_fstr("missions\\%s\\road.bin", name)))
    {
        roads->LoadData(&chunk);
        chunk.Close();
    }
    // STRING: GAME 0x6361a0
    C_fstr dir("missions\\%s", name);
    _can_show_progress_ = 0;
    // STRING: GAME 0x6361ac
    if (chunk.ROpen(C_fstr("missions\\%s\\cache.bin", name)))
    {
        chunk.Close();
        _can_show_progress_ = 1;
    }
    if (isound)
    {
        isound->SetVolume(0.0f);
        act_mission->scene->Render();
    }
    restore_volume = true;
    int ret = OpenBin(dir, flags);
    if (isound)
        isound->SetVolume(mafiaLoadSave.settings.sound_volume);
    if (_can_show_progress_ == 1)
        _can_show_progress_ = 2;
    if (ret)
        return MISSION_OPEN_FAILED;
    unknown_c = true;
    if (chunk.ROpen(C_fstr("missions\\%s\\cache.bin", name)))
    {
        base_cache.Open(&chunk, scene);
        chunk.Close();
    }
    cbLoadScene(CBM_MISSION_PROGRESS, I3DFloatAsInt(1.0f), LOAD_PROGRESS_END, NULL);
    return MISSION_OPEN_OK;
}

// What the scene's loader reports goes to the loading screen, but for its errors and log (PS2 name; PS2's tests CBM_LOG
// first). Defined before Open in PS2's order; VC6 puts a static function after its first caller either way.
// FUNCTION: GAME 0x540fd0
static void __stdcall __SceneErrFnc(LS3D_CALLBACK_MESSAGE msg, uint prm1, uint prm2, void *context)
{
    if (msg != CBM_ERROR && msg != CBM_LOG)
        cbLoadScene(msg, prm1, prm2, context);
}

// Sets every group bit of the sectors in the hierarchy (PS2 C_mission::S_hlp::cbSectorBitFill, the static function of a
// struct of Open's; PS2's Open has its first two levels inlined)
// FUNCTION: GAME 0x540ff0
static void cbSectorBitFill(I3D_frame *frm)
{
    if (frm->GetType() == FRAME_SECTOR)
        ((I3D_sector *)frm)->SetGroupBits((uint)-1);
    for (int i = frm->NumChildren(); i--;)
        cbSectorBitFill(frm->GetChild(i));
}

#pragma auto_inline(off)

// Succeeds, so that a run goes on to the menus
// STUB: GAME 0x541030
int C_mission::OpenBin(const char *dir, uint flags)
{
    StubLog("C_mission::OpenBin %s", dir);
    return 0;
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
