// The mission (PS2 Mission.cpp), the file at 0x53efb0-0x54913c (docs/game-files.md). Written so far: the names' order
// of OpenBin's map, the constructor, the destructor, Init, UnInit, CreateActor, AddActor, DelActors, Close,
// ShowProgress, Open with its two helpers, OpenBin with its frames' callback, the video options' methods and
// FrmActBuff_ClearAll; what C_game's Init and Done call: ClearGameAnimModels, DelNotMeActors, the GlobalProgram
// functions, UpdateSettings, RestoreSettings and FrmActBuff_IsActiveActor; the rest is stubs, Tick among them.

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
#include "actors/C_actor_shot.h"
#include "actors/C_bottle.h"
#include "actors/C_box_new.h"
#include "actors/C_bridge.h"
#include "actors/c_clock.h"
#include "actors/C_detector.h"
#include "actors/C_dog.h"
#include "actors/C_door.h"
#include "actors/C_dyn_object.h"
#include "actors/C_irenka.h"
#include "actors/C_player.h"
#include "actors/C_public_physics.h"
#include "actors/C_pumper.h"
#include "actors/C_scatter.h"
#include "vehicles/C_Vehicle.h"
#include "vehicles/C_airplane.h"
#include "vehicles/C_car_ex.h"
#include "vehicles/C_rail_generator.h"
#include "vehicles/C_railway.h"
#include "vehicles/C_trf_c.h"
#include "vehicles/C_turnout.h"
#include "vehicles/C_wagon.h"
#include "ai/C_person_traffic.h"
#include "ai/jp_ai_entity.h"
#include "camera/c_racecam.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_func.h"
#include "ui/g_indicators.h"
#include "ui/Load_progress.h"
#include "C_chunk.h"
#include "C_fstr.h"
#include "I3D_cache.h"
#include "Winstuff.h"
#include "stub.h"

#include <string.h>
#include <map>

#include "I3D/I3D.h"
#include "ISound/ISound.h"

// Whether the mission has a cache.bin: Open clears it, sets it to 1 when it finds the file and to 2 after OpenBin, and
// the Tick reads it (PS2 name, the same values in PS2's Open)
// GLOBAL: GAME 0x646db0
int _can_show_progress_;

// The order of OpenBin's map of the frames by name (PC only): the names' strcmp
// FUNCTION: GAME 0x53efb0
bool operator<(const C_str &s1, const C_str &s2)
{
    return strcmp(s1, s2) < 0;
}

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

// The flagged models leave the list, the last one taking each one's place, as PS2's
// FUNCTION: GAME 0x53f770
void C_mission::ClearGameAnimModels()
{
    for (uint i = 0; i < anim_models.size();)
    {
        if (anim_models[i].unknown_4)
        {
            anim_models[i] = anim_models.back();
            anim_models.pop_back();
        }
        else
            i++;
    }
}

// An actor of the class for its type, NULL for a type without one; OpenBin initializes it with its frame. PS2's in the
// same order, without the three PC only classes (C_dyn_object, C_seagull and C_scatter), and with its local: the NULL
// is loaded before the switch, which the other types return, and VC6 copies the return into each case. The
// constructors are other files', so each new has an unwind state of its own, in the cases' order.
// FUNCTION: GAME 0x53f7d0
C_actor *C_mission::CreateActor(int type)
{
    C_actor *actor = NULL;
    switch (type)
    {
    case ACTOR_DETECTOR:
        actor = new C_detector;
        break;
    case ACTOR_PLAYER:
        actor = new C_player;
        break;
    case ACTOR_CAR:
        actor = new C_car;
        break;
    case ACTOR_CAR_EX:
        actor = new C_car_ex;
        break;
    case ACTOR_BOTTLE:
        actor = new C_bottle;
        break;
    case ACTOR_RAILWAY:
        actor = new C_railway;
        break;
    case ACTOR_DYN_OBJECT:
        actor = new C_dyn_object;
        break;
    case ACTOR_DOOR:
        actor = new C_door;
        break;
    case ACTOR_TRAFFIC_CAR:
        actor = new C_traffic_car;
        break;
    case ACTOR_ACTOR:
        actor = new C_actor;
        break;
    case ACTOR_AIRPLANE:
        actor = new C_airplane;
        break;
    case ACTOR_TRAFFIC_GENERATOR:
        actor = new C_traffic_generator;
        break;
    case ACTOR_BRIDGE:
        actor = new C_bridge;
        break;
    case ACTOR_DOG:
        actor = new C_dog;
        break;
    case ACTOR_SEAGULL:
        actor = new C_seagull;
        break;
    case ACTOR_TURNOUT:
        actor = new C_turnout;
        break;
    case ACTOR_PUMPER:
        actor = new C_pumper;
        break;
    case ACTOR_ENTITY:
        actor = new C_entity;
        break;
    case ACTOR_RACECAM:
        actor = new C_racecam;
        break;
    case ACTOR_SCATTER:
        actor = new C_scatter;
        break;
    case ACTOR_WAGON:
        actor = new C_wagon;
        break;
    case ACTOR_IRENKA:
        actor = new C_irenka;
        break;
    case ACTOR_PUBLIC_PHYSICS:
        actor = new C_public_physics;
        break;
    case ACTOR_ACTOR_SHOT:
        actor = new C_actor_shot;
        break;
    case ACTOR_CLOCK:
        actor = new C_clock;
        break;
    case ACTOR_BOX_NEW:
        actor = new C_box_new;
        break;
    case ACTOR_RAIL_GENERATOR:
        actor = new C_rail_generator;
        break;
    }
    return actor;
}

// Each list searched from its last actor, as AddActorToGlobal does, and PS2's second search through a flag
// FUNCTION: GAME 0x53fea0
void C_mission::AddActor(C_actor *actor, bool add_unknown_44)
{
    for (int i = actors.size(); i--;)
    {
        if (actor == actors[i])
            return;
    }
    actors.push_back(actor);
    if (add_unknown_44)
    {
        bool add = true;
        for (int i = unknown_44.size(); i--;)
        {
            if (actor == unknown_44[i])
            {
                add = false;
                break;
            }
        }
        if (add)
            unknown_44.push_back(actor);
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x540080
bool C_mission::RemoveActor(C_actor *actor)
{
    StubLog("C_mission::RemoveActor");
    return false;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x540130
bool C_mission::DelActor(C_actor *actor)
{
    StubLog("C_mission::DelActor");
    return true;
}

#pragma auto_inline(on)

// From the last actor, as PS2's: each one's frame forgets it first, through a pointer to NULL, which SetFrameActor frees
// FUNCTION: GAME 0x540240
void C_mission::DelActors()
{
    for (int i = actors.size(); i--;)
    {
        I3D_frame *frm = actors[i]->frame;
        if (frm)
        {
            C_actor **p = GetFrameActor(frm);
            if (p && *p)
            {
                *p = NULL;
                SetFrameActor(frm, p);
            }
        }
        actors[i]->Release();
    }
    actors.clear();
    unknown_44.clear();
}

// The actors of unknown_44 leave the mission as DelActors' do, each looked for from the last actor, as PS2's
// FUNCTION: GAME 0x540300
void C_mission::DelNotMeActors()
{
    for (int i = unknown_44.size(); i--;)
    {
        for (int j = actors.size(); j--;)
        {
            if (actors[j] == unknown_44[i])
            {
                I3D_frame *frm = actors[j]->frame;
                if (frm)
                {
                    C_actor **p = GetFrameActor(frm);
                    if (p && *p)
                    {
                        *p = NULL;
                        SetFrameActor(frm, p);
                    }
                }
                actors[j]->Release();
                actors.erase(&actors[j]);
                break;
            }
        }
    }
    unknown_44.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x540420
bool C_mission::IsOwnActor(C_actor *actor)
{
    StubLog("C_mission::IsOwnActor");
    return false;
}

#pragma auto_inline(on)

// From the last script, as PS2's
// FUNCTION: GAME 0x540520
void C_mission::GlobalProgramGameInit()
{
    for (int i = programs.size(); i--;)
        programs[i]->Init();
}

// FUNCTION: GAME 0x540550
void C_mission::GlobalProgramGameDone()
{
    for (int i = programs.size(); i--;)
        programs[i]->Done();
}

// The game's scripts first: the mission's of that name runs only where the game has none (PS2's the same)
// FUNCTION: GAME 0x540580
void C_mission::GlobalProgramRun(const char *name)
{
    if (game->GlobalProgramRun(name))
        return;
    for (int i = programs.size(); i--;)
    {
        if (!_strcmpi(programs[i]->name, name))
        {
            programs[i]->CallSubroutine();
            return;
        }
    }
}

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

// The loading bar at the file's position: the whole bar, or its first fifth while a cache.bin is to be loaded after the
// file (PS2 name and code)
// FUNCTION: GAME 0x540950
void ShowProgress(C_chunk *chunk)
{
    float progress;
    int pos = dtaSeek(chunk->handle, 0, SEEK_CUR);
    int size = chunk->file_size;
    if (size)
        progress = (float)pos / size;
    else
        progress = 1.0f;
    switch (_can_show_progress_)
    {
    case 1:
        progress *= .2f;
        break;
    case 2:
        progress = progress * .8f + .2f;
        break;
    }
    cbLoadScene(CBM_MISSION_PROGRESS, *(uint *)&progress, LOAD_PROGRESS_STEP, NULL);
}

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

// The chunks of a mission's scene2.bin (the names are ours)
#define CT_SCENE 0x4c53                 // the file's
#define CT_VERSION 0x1                  // its version, the major and the minor number
#define CT_CAMERA_FOV 0x3010
#define CT_CAMERA_RANGE 0x3011          // the far range, which the options keep
#define CT_CAMERA_ORTHOGONAL 0x3012
#define CT_CAMERA_ORTHO_SCALE 0x3013
#define CT_BG_COLOR 0x3200              // read and ignored: the background stays black (PS2 the same)
#define CT_BACKDROP_RANGE 0x3211
#define CT_OBJECTS 0x4000
#define CT_TSHIFTS 0xae02
#define CT_ACTORS 0xae20
#define CT_WEB 0xae40
#define CT_ROADS 0xae41
#define CT_SCRIPTS 0xae50
#define CT_WEATHER_PARAMS 0xaef0
#define CT_WEATHER_DUMMIES 0xaef1
#define CT_MISSION_DATA 0xafff          // the mission's first two dwords

// In CT_OBJECTS: an object, a frame of the scene, which the chunk creates or changes
#define CT_OBJECT 0x4010
#define CT_NAME 0x10                    // the frame's, and a light's sound's or parent's name
#define CT_POS 0x20
#define CT_ROT 0x22
#define CT_WORLD_POS 0x2c               // the position where the parent isn't found
#define CT_SCALE 0x2d
#define CT_MODEL 0x2012                 // a model's file
#define CT_FRAME_TYPE 0x4011            // a frame to create, I3D_FRAME_TYPE
#define CT_VISUAL_TYPE 0x4012           // the visual's type, for FRAME_VISUAL
#define CT_PARENT 0x4020                // a chunk with the parent's name
#define CT_HIDDEN 0x4033                // switched off (but a sound)
#define CT_LIGHT 0x4040
#define CT_DUMMY_BBOX 0x4050
#define CT_SOUND 0x4060
#define CT_OCCLUDER 0x4083              // the occluder's vertices and faces
#define CT_CHANGE_VISUAL_TYPE 0x4090    // a visual of another type takes the frame's place
#define CT_LIGHTMAP 0x40a0              // the lightmap's data, from version 3
#define CT_UNKNOWN_B100 0xb100
#define CT_LENSFLARE 0xb110             // the lens flare's parameters, from version 5
#define CT_AREA 0xb151                  // the area's vertices and faces
#define CT_SECTOR_SOUND_DATA 0xb400     // the sector's sound data, I3D_SECTOR_SOUND_DATA_INDEX
#define CT_SECTOR_SOUND_DATA2 0xb401

// In CT_LIGHT
#define CT_LIGHT_TYPE 0x4041
#define CT_LIGHT_POWER 0x4042
#define CT_LIGHT_COLOR 0x26
#define CT_LIGHT_CONE 0x4043
#define CT_LIGHT_RANGE 0x4044
#define CT_LIGHT_MODE 0x4045
#define CT_LIGHT_SECTOR 0x4046          // a sector it lights

// In CT_SOUND (CT_NAME is its file)
#define CT_SOUND_TYPE 0x4061
#define CT_SOUND_VOLUME 0x4062
#define CT_SOUND_OUTVOL 0x4063
#define CT_SOUND_CONE 0x4064
#define CT_SOUND_RANGE 0x4065           // the near and far range and the middle one
#define CT_SOUND_LOOP 0x4066
#define CT_SOUND_UNKNOWN_4067 0x4067
#define CT_SOUND_RANGE_VOL 0x4068       // and the middle range's volume
#define CT_SOUND_SECTOR 0xb200          // a sector it sounds in
#define CT_SOUND_AREA 0xb201            // an area it sounds in
#define CT_SOUND_FREQUENCY 0xb800

// In CT_LENSFLARE
#define CT_LENSFLARE_SIZE 0xb112
#define CT_LENSFLARE_INTENSITY 0xb113
#define CT_LENSFLARE_TEST_INTERVAL 0xb114
#define CT_LENSFLARE_FADE_IN 0xb115
#define CT_LENSFLARE_FADE_OUT 0xb116
#define CT_LENSFLARE_RANGE 0xb117

// In CT_TSHIFTS
#define CT_TSHIFT 0xae03

// In CT_ACTORS: an actor and its frame
#define CT_ACTOR 0xae21
#define CT_ACTOR_TYPE 0xae22
#define CT_ACTOR_FRAME 0xae23
#define CT_ACTOR_DATA 0xae24            // what the actor's LoadData reads

// In CT_SCRIPTS
#define CT_SCRIPT 0xae51

// What an object's chunks set, which its end applies (the names are ours)
#define OBJF_POS 0x1
#define OBJF_ROT 0x2
#define OBJF_SCALE 0x4
#define OBJF_PARENT 0x10                // the parent is found: linked to it
#define OBJF_LIGHT 0x20
#define OBJF_SOUND 0x40
#define OBJF_OCCLUDER 0x200
#define OBJF_VISUAL 0x400               // the visual changed: another type, its lightmap or its lens flare
#define OBJF_HIDDEN 0x10000
#define OBJF_CREATED 0x20000            // added to the scene: the scene finds its sector
#define OBJF_AREA 0x100000
#define OBJF_SECTOR 0x200000

// A light or a sound whose sector or area comes later in the file, by name (PS2 name, the members' names are ours)
struct S_sound_light_sector
{
    I3D_frame *frm;
    char *name;                         // deleted with the entry
};

// The scene's frames by name (PC only): what the chunks find their frames in
typedef std::map<C_str, I3D_frame *> t_frame_map;

static I3DENUMRET __stdcall cbFrameMap(I3D_frame *frm, uint context);

// The chunk's data, which it then leaves (PS2 names: PS2 has them out of line, the quaternion's components in another
// order)
static inline S_vector ReadVector3(C_chunk &chunk)
{
    S_vector v;
    chunk.Read(&v, sizeof(v));
    chunk.Descend();
    return v;
}

static inline S_quat ReadQuatOld(C_chunk &chunk)
{
    S_quat q;
    chunk.Read(&q, sizeof(q));
    chunk.Descend();
    return q;
}

static inline void ReadBBoxOld(C_chunk &chunk, I3D_bbox &bbox)
{
    chunk.Read(&bbox, sizeof(bbox));
    chunk.Descend();
}

// A copy of a string, deleted with delete[] (the name is ours; PS2 copies the names the same way)
static inline char *CopyString(const char *s)
{
    char *copy = new char[strlen(s) + 1];
    strcpy(copy, s);
    return copy;
}

// The scene2.bin of a mission's folder: the scene's camera, then its chunks, the objects in the order of the file, a
// frame's chunks setting it up or creating it, and finally the lights and sounds whose sector or area came later.
// PS2's has the frames found by FindFrame and more chunks (the inter-city missions' among them); the PC's finds them in
// a map of its own, which the models' frames are added to.
// The cases are in PS2's order, which its compare chains give (reversed): VC6 lays the case bodies out by value but
// spends the inline budget in source order, and in that order the original's budget runs out at the model's chunk, as
// ours does. A single value is read through the chunk's ReadDword and ReadFloat, whose nested Read and Descend the
// budget leaves out of line (PS2's the same).
// Open: the frame (0x2004 bytes, the original's 0x2008) and most stack slots; which later calls are inlined (the
// original calls accessors of the frames by reference, GetType, GetParent, NumChildren, GetName and the visual's type,
// which LS3DF's header doesn't have or returns by value; the light's mode read inlined; the occluder's and area's
// arrays constructed by the vector constructor iterator, the S_vector constructor out of line); the strings' copy
// loop's registers; the lens flare's values pass through the FPU into its members, as inline setters' parameters do.
// FUNCTION: GAME 0x541030
int C_mission::OpenBin(const char *dir, uint flags)
{
    C_str name;
    C_chunk chunk;
    short version = 0, version_minor = 0;
    std::vector<S_sound_light_sector> sectors;
    std::vector<S_sound_light_sector> areas;
    // STRING: GAME 0x636220
    if (!chunk.ROpen(C_fstr("%s\\scene2.bin", dir)))
        return OPENBIN_NOFILE;
    t_frame_map frames;
    scene->EnumFrames(cbFrameMap, (uint)&frames, ENUMF_ALL | ENUMF_BACKDROP, NULL);
    frames[scene->primary_sector->GetName()] = scene->primary_sector;
    frames[scene->backdrop_sector->GetName()] = scene->backdrop_sector;
    frames[scene->omni_sector->GetName()] = scene->omni_sector;
    I3D_camera *cam = scene->current_camera;
    if (!cam)
    {
        cam = (I3D_camera *)driver->CreateFrame(FRAME_CAMERA);
        // STRING: GAME 0x636214
        cam->SetName("GameCamera");
        scene->SetActiveCamera(cam);
        cam->SetDir(S_vector(-1.0f, -.1f, -1.0f), 0.0f);
        cam->SetPos(S_vector(10.0f, 5.0f, 10.0f));
        cam->SetRange(.05f, 250.0f);
        options.far_range = 250.0f;
        cam->Update();
        cam->Release();
    }
    cam->SetOrthogonal(false);
    cam->SetRange(.05f, 250.0f);
    options.far_range = 250.0f;
    mafiaIndicators.SetCameraFOV(cam, PI / 2);
    cam->Update();
    ushort type;
    if (!chunk.RAscend(type))
        goto fail;
    if (type == CT_SCENE)
    {
        while (chunk.Size())
        {
            if (!chunk.RAscend(type))
                goto fail;
            ShowProgress(&chunk);
            switch (type)
            {
            case CT_VERSION:
                chunk.Read(&version, sizeof(version));
                chunk.Read(&version_minor, sizeof(version_minor));
                break;
            case CT_MISSION_DATA:
                chunk.Read(this, 8);
                break;
            case CT_BG_COLOR:
            {
                S_vector color;
                chunk.Read(&color, sizeof(color));
                color = S_vector(0.0f, 0.0f, 0.0f);
                scene->bg_color = color;
                break;
            }
            case CT_CAMERA_FOV:
            case CT_CAMERA_RANGE:
            case CT_CAMERA_ORTHO_SCALE:
            {
                float value;
                chunk.Read(&value, sizeof(value));
                switch (type)
                {
                case CT_CAMERA_FOV:
                    mafiaIndicators.SetCameraFOV(cam, value);
                    break;
                case CT_CAMERA_RANGE:
                    cam->SetRange(.05f, value);
                    options.far_range = value;
                    break;
                case CT_CAMERA_ORTHO_SCALE:
                    cam->SetOrthoScale(value);
                    break;
                }
                break;
            }
            case CT_CAMERA_ORTHOGONAL:
                cam->SetOrthogonal(true);
                break;
            case CT_BACKDROP_RANGE:
            {
                float range[2];
                chunk.Read(range, sizeof(range));
                if (range[1] < 3000.0f)
                    range[1] = 3000.0f;
                scene->SetBackdropRange(range[0], range[1]);
                break;
            }
            case CT_OBJECTS:
                while (chunk.Size())
                {
                    ShowProgress(&chunk);
                    if (chunk.Size() && !chunk.RAscend(type))
                        goto fail;
                    ShowProgress(&chunk);
                    if (type == CT_OBJECT)
                    {
                        int frame_type = FRAME_NULL;
                        I3D_frame *frm = NULL;
                        uint obj_flags = 0;
                        S_vector scale(1.0f, 1.0f, 1.0f);
                        bool world_pos_read = false;
                        // PS2's too, unused
                        std::vector<uint> unused;
                        int visual_type;
                        S_vector pos, world_pos;
                        S_quat rot;
                        I3D_frame *parent;
                        while (chunk.Size())
                        {
                            if (!chunk.RAscend(type))
                                goto fail;
                            switch (type)
                            {
                            case CT_FRAME_TYPE:
                                frame_type = chunk.ReadDword();
                                break;
                            case CT_VISUAL_TYPE:
                                visual_type = chunk.ReadDword();
                                break;
                            case CT_NAME:
                                chunk.ReadString(name);
                                if (!frame_type)
                                    frm = frames[name];
                                else if (!frames[name])
                                {
                                    if (frame_type == FRAME_VISUAL)
                                        frm = driver->CreateVisual((I3D_VISUAL_TYPE)visual_type);
                                    else
                                        frm = driver->CreateFrame((I3D_FRAME_TYPE)frame_type);
                                    if (frm)
                                    {
                                        frm->SetName(name);
                                        obj_flags |= OBJF_CREATED;
                                        scene->AddFrame(frm);
                                        frm->Release();
                                        frames[frm->GetName()] = frm;
                                    }
                                }
                                break;
                            case CT_POS:
                                pos = ReadVector3(chunk);
                                obj_flags |= OBJF_POS;
                                break;
                            case CT_WORLD_POS:
                                world_pos = ReadVector3(chunk);
                                world_pos_read = true;
                                break;
                            case CT_ROT:
                                rot = ReadQuatOld(chunk);
                                obj_flags |= OBJF_ROT;
                                break;
                            case CT_SCALE:
                                scale = ReadVector3(chunk);
                                obj_flags |= OBJF_SCALE;
                                break;
                            // The light leaves its sectors first: CT_LIGHT_SECTOR gives them
                            case CT_LIGHT:
                                if (frm && frm->GetType() == FRAME_LIGHT)
                                {
                                    I3D_light *light = (I3D_light *)frm;
                                    while (light->sectors.size())
                                        light->sectors[0]->DeleteLight(light);
                                    while (chunk.Size())
                                    {
                                        if (!chunk.RAscend(type))
                                            goto fail;
                                        switch (type)
                                        {
                                        case CT_LIGHT_TYPE:
                                        {
                                            light->SetLightType((I3D_LIGHTTYPE)chunk.ReadDword());
                                            break;
                                        }
                                        case CT_LIGHT_COLOR:
                                            light->SetDiffuse(ReadVector3(chunk));
                                            break;
                                        case CT_LIGHT_POWER:
                                        {
                                            light->SetDiffusePower(chunk.ReadFloat());
                                            break;
                                        }
                                        case CT_LIGHT_CONE:
                                        {
                                            float cone[2];
                                            chunk.Read(cone, sizeof(cone));
                                            chunk.Descend();
                                            light->SetCone(cone[0], cone[1]);
                                            break;
                                        }
                                        case CT_LIGHT_RANGE:
                                        {
                                            float range[2];
                                            chunk.Read(range, sizeof(range));
                                            chunk.Descend();
                                            if (range[0] <= MRG_ZERO)
                                                range[0] = .1f;
                                            if (range[1] <= range[0])
                                                range[1] = range[0] + .1f;
                                            light->SetRange(range[0], range[1]);
                                            break;
                                        }
                                        // Files before 5.10 lack a mode bit
                                        case CT_LIGHT_MODE:
                                        {
                                            uint mode;
                                            chunk.Read(&mode, sizeof(mode));
                                            chunk.Descend();
                                            if (version * 100 + version_minor < 510)
                                                mode |= 0x40;
                                            light->SetMode(mode);
                                            break;
                                        }
                                        case CT_LIGHT_SECTOR:
                                        {
                                            chunk.ReadString(name);
                                            I3D_frame *sct = frames[name];
                                            if (sct && sct->GetType() == FRAME_SECTOR)
                                                ((I3D_sector *)sct)->AddLight(light);
                                            else
                                            {
                                                S_sound_light_sector s;
                                                s.frm = light;
                                                s.name = CopyString(name);
                                                sectors.push_back(s);
                                            }
                                            break;
                                        }
                                        default:
                                            chunk.Descend();
                                        }
                                    }
                                    obj_flags |= OBJF_LIGHT;
                                }
                                chunk.Descend();
                                break;
                            // The sound is switched off while it is set up, and its file opened through the cache
                            case CT_SOUND:
                                if (frm && frm->GetType() == FRAME_SOUND)
                                {
                                    I3D_sound *sound = (I3D_sound *)frm;
                                    sound->SetOn(false);
                                    sound->SetLoop(false);
                                    while (chunk.Size())
                                    {
                                        if (!chunk.RAscend(type))
                                            goto fail;
                                        switch (type)
                                        {
                                        case CT_NAME:
                                            chunk.ReadString(name);
                                            sound_cache.Open(sound, name, 0, NULL, NULL, NULL);
                                            break;
                                        case CT_SOUND_TYPE:
                                        {
                                            sound->SetSoundType((I3D_SOUNDTYPE)chunk.ReadDword());
                                            break;
                                        }
                                        case CT_SOUND_VOLUME:
                                        {
                                            sound->SetVolume(chunk.ReadFloat());
                                            break;
                                        }
                                        case CT_SOUND_OUTVOL:
                                        {
                                            sound->SetOutVol(chunk.ReadFloat());
                                            break;
                                        }
                                        case CT_SOUND_CONE:
                                        {
                                            float cone[2];
                                            chunk.Read(cone, sizeof(cone));
                                            chunk.Descend();
                                            sound->SetCone(cone[0], cone[1]);
                                            break;
                                        }
                                        case CT_SOUND_RANGE:
                                        {
                                            float range[3];
                                            chunk.Read(range, sizeof(range));
                                            chunk.Descend();
                                            sound->SetRange(range[0], range[1], range[2], 1.0f);
                                            break;
                                        }
                                        case CT_SOUND_RANGE_VOL:
                                        {
                                            float range[4];
                                            chunk.Read(&range[0], sizeof(float));
                                            chunk.Read(&range[1], sizeof(float));
                                            chunk.Read(&range[2], sizeof(float));
                                            chunk.Read(&range[3], sizeof(float));
                                            chunk.Descend();
                                            sound->SetRange(range[0], range[1], range[2], range[3]);
                                            break;
                                        }
                                        case CT_SOUND_LOOP:
                                            sound->SetLoop(true);
                                            chunk.Descend();
                                            break;
                                        // PS2 sets a flag of its own
                                        case CT_SOUND_UNKNOWN_4067:
                                            chunk.Descend();
                                            break;
                                        case CT_SOUND_FREQUENCY:
                                        {
                                            sound->SetFrequency(chunk.ReadFloat());
                                            break;
                                        }
                                        case CT_SOUND_SECTOR:
                                        {
                                            chunk.ReadString(name);
                                            I3D_frame *sct = frames[name];
                                            if (sct && sct->GetType() == FRAME_SECTOR)
                                                ((I3D_sector *)sct)->AddSound(sound);
                                            else
                                            {
                                                S_sound_light_sector s;
                                                s.frm = sound;
                                                s.name = CopyString(name);
                                                sectors.push_back(s);
                                            }
                                            break;
                                        }
                                        case CT_SOUND_AREA:
                                        {
                                            chunk.ReadString(name);
                                            I3D_frame *area = frames[name];
                                            if (area && area->GetType() == FRAME_AREA)
                                                ((I3D_area *)area)->AddSound(sound);
                                            else
                                            {
                                                S_sound_light_sector s;
                                                s.frm = sound;
                                                s.name = CopyString(name);
                                                areas.push_back(s);
                                            }
                                            break;
                                        }
                                        default:
                                            chunk.Descend();
                                        }
                                    }
                                    obj_flags |= OBJF_SOUND;
                                    sound->SetOn(true);
                                }
                                chunk.Descend();
                                break;
                            // Each chunk's type is the index of its data; the first's (0) sets index 11 too
                            case CT_SECTOR_SOUND_DATA:
                                if (frm && frm->GetType() == FRAME_SECTOR)
                                {
                                    while (chunk.Size())
                                    {
                                        ushort index;
                                        chunk.RAscend(index);
                                        uint data;
                                        chunk.Read(&data, sizeof(data));
                                        if (!index)
                                            ((I3D_sector *)frm)->SetSoundData((I3D_SECTOR_SOUND_DATA_INDEX)11, data);
                                        ((I3D_sector *)frm)->SetSoundData((I3D_SECTOR_SOUND_DATA_INDEX)index, data);
                                        chunk.Descend();
                                    }
                                    obj_flags |= OBJF_SECTOR;
                                }
                                chunk.Descend();
                                break;
                            // The same, with 12 bytes of data for indices 20 and 21, passed by address
                            case CT_SECTOR_SOUND_DATA2:
                                if (frm && frm->GetType() == FRAME_SECTOR)
                                {
                                    while (chunk.Size())
                                    {
                                        ushort index;
                                        chunk.RAscend(index);
                                        uint data;
                                        S_vector v;
                                        if (index >= 20 && index <= 21)
                                        {
                                            chunk.Read(&v, sizeof(v));
                                            ((I3D_sector *)frm)->SetSoundData((I3D_SECTOR_SOUND_DATA_INDEX)index,
                                                (uint)&v);
                                        }
                                        else
                                        {
                                            chunk.Read(&data, sizeof(data));
                                            ((I3D_sector *)frm)->SetSoundData((I3D_SECTOR_SOUND_DATA_INDEX)index, data);
                                        }
                                        chunk.Descend();
                                    }
                                    obj_flags |= OBJF_SECTOR;
                                }
                                chunk.Descend();
                                break;
                            case CT_UNKNOWN_B100:
                                chunk.Descend();
                                break;
                            case CT_LENSFLARE:
                                if (frm && version >= 5)
                                {
                                    I3D_lensflare *lf = (I3D_lensflare *)frm;
                                    ushort lf_type;
                                    obj_flags |= OBJF_VISUAL;
                                    while (chunk.Size())
                                    {
                                        if (chunk.Size())
                                            chunk.RAscend(lf_type);
                                        switch (lf_type)
                                        {
                                        case CT_LENSFLARE_SIZE:
                                        {
                                            lf->size = chunk.ReadFloat();
                                            break;
                                        }
                                        case CT_LENSFLARE_INTENSITY:
                                        {
                                            lf->intensity = chunk.ReadFloat();
                                            break;
                                        }
                                        case CT_LENSFLARE_TEST_INTERVAL:
                                        {
                                            lf->test_interval = (uint)chunk.ReadFloat();
                                            break;
                                        }
                                        case CT_LENSFLARE_FADE_IN:
                                        {
                                            lf->fade_in = chunk.ReadFloat();
                                            break;
                                        }
                                        case CT_LENSFLARE_FADE_OUT:
                                        {
                                            lf->fade_out = chunk.ReadFloat();
                                            break;
                                        }
                                        case CT_LENSFLARE_RANGE:
                                        {
                                            lf->range = chunk.ReadFloat();
                                            break;
                                        }
                                        }
                                    }
                                }
                                chunk.Descend();
                                break;
                            case CT_PARENT:
                                if (frm)
                                {
                                    if (!chunk.RAscend(type))
                                        goto fail;
                                    if (type == CT_NAME)
                                    {
                                        chunk.ReadString(name);
                                        parent = frames[name];
                                        if (parent)
                                            obj_flags |= OBJF_PARENT;
                                        else if (world_pos_read)
                                            pos = world_pos;
                                    }
                                    else
                                        chunk.Descend();
                                }
                                chunk.Descend();
                                break;
                            // The file's name without its extension: an empty one removes the model. The model's frames
                            // are added to the map.
                            case CT_MODEL:
                                if (frm && frm->GetType() == FRAME_MODEL)
                                {
                                    chunk.ReadString(name);
                                    if (!name[0])
                                    {
                                        frm->m_RefCount++;
                                        scene->DeleteFrame(frm);
                                        frm->Release();
                                        frm = NULL;
                                        break;
                                    }
                                    int i;
                                    for (i = name.Size(); i; i--)
                                    {
                                        if (name[i - 1] == '.')
                                            break;
                                    }
                                    if (i)
                                        name[i - 1] = 0;
                                    name = (const char *)name;
                                    // STRING: GAME 0x63620c
                                    name += ".i3d";
                                    if (model_cache.Open((I3D_model *)frm, name, 0, NULL, NULL, NULL) >= I3D_OK)
                                        frm->EnumFrames(cbFrameMap, (uint)&frames, ENUMF_ALL, NULL);
                                    break;
                                }
                                chunk.Descend();
                                break;
                            case CT_DUMMY_BBOX:
                            {
                                I3D_bbox bbox;
                                ReadBBoxOld(chunk, bbox);
                                if (frm && frm->GetType() == FRAME_DUMMY)
                                    ((I3D_dummy *)frm)->bbox = bbox;
                                break;
                            }
                            // A visual of the new type takes the frame's place: its copy, linked where the frame was,
                            // with its children
                            case CT_CHANGE_VISUAL_TYPE:
                            {
                                int new_type = chunk.ReadDword();
                                if (frm && frm->GetType() == FRAME_VISUAL &&
                                    ((I3D_visual *)frm)->GetVisualType() != new_type)
                                {
                                    I3D_visual *vis = driver->CreateVisual((I3D_VISUAL_TYPE)new_type);
                                    if (vis)
                                    {
                                        if (vis->Duplicate(frm) >= I3D_OK)
                                        {
                                            vis->LinkTo(frm->m_pParent, 0);
                                            while (frm->NumChildren())
                                                frm->GetChild(0)->LinkTo(vis, 0);
                                            vis->Update();
                                            frm->LinkTo(NULL, 0);
                                            frames[frm->GetName()] = vis;
                                            I3D_frame *owner = vis;
                                            while ((owner = owner->m_pParent) != NULL)
                                            {
                                                if (owner->GetType() == FRAME_MODEL)
                                                {
                                                    ((I3D_model *)owner)->DeleteFrame(frm);
                                                    ((I3D_model *)owner)->AddFrame(vis);
                                                    break;
                                                }
                                            }
                                            if (!owner)
                                            {
                                                scene->DeleteFrame(frm);
                                                scene->AddFrame(vis);
                                            }
                                            frm = vis;
                                            obj_flags |= OBJF_VISUAL;
                                        }
                                        vis->Release();
                                    }
                                }
                                break;
                            }
                            case CT_LIGHTMAP:
                                if (version >= 3 && frm && frm->GetType() == FRAME_VISUAL &&
                                    ((I3D_visual *)frm)->GetVisualType() == I3D_VISUAL_LIT_OBJECT)
                                {
                                    ((I3D_lit_object *)frm)->Load(chunk.GetHandle());
                                    obj_flags |= OBJF_VISUAL;
                                }
                                chunk.Descend();
                                break;
                            case CT_OCCLUDER:
                                if (frm && frm->GetType() == FRAME_OCCLUDER)
                                {
                                    uint num_verts = 0;
                                    chunk.Read(&num_verts, sizeof(num_verts));
                                    S_vector *verts = new S_vector[num_verts];
                                    chunk.Read(verts, num_verts * sizeof(S_vector));
                                    uint num_faces = 0;
                                    chunk.Read(&num_faces, sizeof(num_faces));
                                    ushort *faces = new ushort[num_faces * 3];
                                    chunk.Read(faces, num_faces * 3 * sizeof(ushort));
                                    ((I3D_occluder *)frm)->Build(verts, num_verts, faces, num_faces, sizeof(S_vector));
                                    delete[] verts;
                                    delete[] faces;
                                    obj_flags |= OBJF_OCCLUDER;
                                }
                                chunk.Descend();
                                break;
                            case CT_AREA:
                                if (frm && frm->GetType() == FRAME_AREA)
                                {
                                    uint num_verts = 0;
                                    chunk.Read(&num_verts, sizeof(num_verts));
                                    S_vector *verts = new S_vector[num_verts];
                                    chunk.Read(verts, num_verts * sizeof(S_vector));
                                    uint num_faces = 0;
                                    chunk.Read(&num_faces, sizeof(num_faces));
                                    ushort *faces = new ushort[num_faces * 3];
                                    chunk.Read(faces, num_faces * 3 * sizeof(ushort));
                                    ((I3D_area *)frm)->Build(verts, num_verts, faces, num_faces, sizeof(S_vector));
                                    delete[] verts;
                                    delete[] faces;
                                    obj_flags |= OBJF_AREA;
                                }
                                chunk.Descend();
                                break;
                            case CT_HIDDEN:
                                if (!frm || frm->GetType() != FRAME_SOUND)
                                    obj_flags |= OBJF_HIDDEN;
                                chunk.Descend();
                                break;
                            default:
                                chunk.Descend();
                            }
                        }
                        if (frm)
                        {
                            if (obj_flags & OBJF_PARENT && parent)
                                frm->LinkTo(parent, 0);
                            if (obj_flags & OBJF_POS)
                                frm->SetPos(pos);
                            if (obj_flags & OBJF_ROT)
                                frm->SetRot(rot);
                            if (obj_flags & OBJF_SCALE)
                                frm->SetScale(scale);
                            if (obj_flags & OBJF_HIDDEN)
                                frm->SetOn(false);
                            frm->Update();
                            if (frm->GetType() == FRAME_MODEL && ((I3D_model *)frm)->GetAnimationSet(0))
                            {
                                S_anim_model m;
                                m.model = (I3D_model *)frm;
                                m.unknown_4 = false;
                                anim_models.push_back(m);
                            }
                            if (obj_flags & OBJF_CREATED && !(obj_flags & OBJF_PARENT))
                                scene->SetFrameSectorPos(frm, frm->GetWorldPos());
                        }
                    }
                    chunk.Descend();
                }
                break;
            // The lightmapped and plain objects' meshes the time shift moves: the frame's name, then its numbers
            case CT_TSHIFTS:
                while (chunk.Size())
                {
                    if (!chunk.RAscend(type))
                        goto fail;
                    if (type == CT_TSHIFT)
                    {
                        ushort a;
                        int b, c;
                        int len;
                        char frame_name[256];
                        chunk.Read(&a, sizeof(a));
                        chunk.Read(&b, sizeof(b));
                        chunk.Read(&c, sizeof(c));
                        chunk.Read(&len, sizeof(len));
                        chunk.Read(frame_name, len);
                        frame_name[len] = 0;
                        I3D_frame *frm = frames[frame_name];
                        if (frm && frm->GetType() == FRAME_VISUAL)
                        {
                            I3D_VISUAL_TYPE vt = ((I3D_visual *)frm)->GetVisualType();
                            if ((vt == I3D_VISUAL_LIT_OBJECT || vt == I3D_VISUAL_OBJECT) &&
                                ((I3D_object *)frm)->GetMesh())
                            {
                                S_tshift_elem elem;
                                elem.Init((I3D_visual *)frm, ((I3D_object *)frm)->GetMesh(), b, c, a);
                                GetTShift()->Add(elem);
                            }
                        }
                    }
                    chunk.Descend();
                }
                break;
            // Each actor's chunk has its frame's name, then its type, which creates it, then its data
            case CT_ACTORS:
                while (chunk.Size())
                {
                    if (!chunk.RAscend(type))
                        goto fail;
                    if (type == CT_ACTOR)
                    {
                        C_actor *actor = NULL;
                        I3D_frame *frm = NULL;
                        while (chunk.Size())
                        {
                            if (!chunk.RAscend(type))
                                goto fail;
                            switch (type)
                            {
                            // A frame that has an actor already gets no other
                            case CT_ACTOR_FRAME:
                            {
                                chunk.ReadString(name);
                                frm = frames[name];
                                actor = NULL;
                                C_actor **p;
                                if (frm && (p = GetFrameActor(frm)) != NULL && *p)
                                    frm = NULL;
                                break;
                            }
                            case CT_ACTOR_TYPE:
                            {
                                int actor_type = chunk.ReadDword();
                                if (frm)
                                {
                                    actor = CreateActor(actor_type);
                                    if (actor)
                                    {
                                        if (actor->Init(frm))
                                        {
                                            actor->SetUnknown66();
                                            AddActor(actor, false);
                                            C_actor **p = GetFrameActor(frm);
                                            if (!p)
                                                p = new C_actor *;
                                            *p = actor;
                                            SetFrameActor(frm, p);
                                        }
                                        else
                                        {
                                            actor->Release();
                                            actor = NULL;
                                        }
                                    }
                                }
                                break;
                            }
                            case CT_ACTOR_DATA:
                                if (actor && chunk.Size())
                                    actor->LoadData(&chunk);
                                chunk.Descend();
                                break;
                            }
                        }
                    }
                    chunk.Descend();
                }
                break;
            case CT_WEATHER_PARAMS:
                while (chunk.Size())
                {
                    int param;
                    uint value;
                    chunk.Read(&param, sizeof(param));
                    chunk.Read(&value, sizeof(value));
                    scene->SetWeatherSystemParam((WS_PARAM)param, value);
                }
                break;
            case CT_WEATHER_DUMMIES:
            {
                int count;
                chunk.Read(&count, sizeof(count));
                for (int i = count; i; i--)
                {
                    int len;
                    chunk.Read(&len, sizeof(len));
                    if (!len)
                        continue;
                    char *s = new char[len];
                    chunk.Read(s, len);
                    I3D_frame *frm = frames[s];
                    if (frm && frm->GetType() == FRAME_DUMMY)
                        scene->WeatherSystemAddDummy((I3D_dummy *)frm);
                    delete[] s;
                }
                break;
            }
            case CT_WEB:
                web_path->LoadNodes(&chunk);
                break;
            case CT_ROADS:
                roads->LoadData(&chunk);
                break;
            case CT_SCRIPTS:
                while (chunk.Size())
                {
                    if (!chunk.RAscend(type))
                        goto fail;
                    if (type == CT_SCRIPT)
                    {
                        C_program *prog = new C_program;
                        prog->LoadSourceCode(&chunk);
                        programs.push_back(prog);
                    }
                    chunk.Descend();
                }
                break;
            }
            chunk.Descend();
        }
    }
    chunk.Close();
    {
        int i;
        for (i = sectors.size(); i--;)
        {
            if (sectors[i].frm->GetType() == FRAME_LIGHT)
            {
                I3D_frame *sct = frames[sectors[i].name];
                if (sct && sct->GetType() == FRAME_SECTOR)
                    ((I3D_sector *)sct)->AddLight((I3D_light *)sectors[i].frm);
            }
            else if (sectors[i].frm->GetType() == FRAME_SOUND)
            {
                I3D_frame *sct = frames[sectors[i].name];
                if (sct && sct->GetType() == FRAME_SECTOR)
                    ((I3D_sector *)sct)->AddSound((I3D_sound *)sectors[i].frm);
            }
            delete[] sectors[i].name;
        }
        for (i = areas.size(); i--;)
        {
            if (areas[i].frm->GetType() == FRAME_SOUND)
            {
                I3D_frame *area = frames[areas[i].name];
                if (area && area->GetType() == FRAME_AREA)
                    ((I3D_area *)area)->AddSound((I3D_sound *)areas[i].frm);
            }
            delete[] areas[i].name;
        }
    }
    return OPENBIN_OK;
fail:
    {
        int i;
        for (i = sectors.size(); i--;)
            delete[] sectors[i].name;
        for (i = areas.size(); i--;)
            delete[] areas[i].name;
    }
    chunk.Close();
    return OPENBIN_CORRUPTED;
}

// Adds a frame to the map by its name (PC only, the name is ours)
// FUNCTION: GAME 0x5455e0
static I3DENUMRET __stdcall cbFrameMap(I3D_frame *frm, uint context)
{
    (*(t_frame_map *)context)[frm->GetName()] = frm;
    return I3DENUMRET_OK;
}

#pragma auto_inline(off)

// STUB: GAME 0x5459b0
void C_mission::FinishTimeTransparentObjects()
{
    StubLog("C_mission::FinishTimeTransparentObjects");
}

#pragma auto_inline(on)

// With the city's blocks open the camera sees the options' part of its range, and the level of detail can go below 1
// (PS2's the same). The level's bits are read through its address, which keeps it in memory from its copy on, as the
// original's (I3DFloatAsInt's copy leaves it on the FPU stack)
// FUNCTION: GAME 0x545a30
void C_mission::UpdateSettings()
{
    I3D_camera *cam = scene->current_camera;
    if (cam)
    {
        if (base_cache.IsOpen())
            cam->SetRange(.05f, options.far_range * options.draw_distance);
        else
            cam->SetRange(.05f, options.far_range);
        cam->Update();
    }
    float lod = options.level_of_detail;
    if (!base_cache.IsOpen() && lod < 1.0f)
        lod = 1.0f;
    driver->SetState(RS_LOD_QUALITY, *(int *)&lod);
    game->OsefujPocasi(false, (uint)-1);
}

// FUNCTION: GAME 0x545ad0
void C_mission::RestoreSettings()
{
    I3D_camera *cam = scene->current_camera;
    if (cam)
    {
        cam->SetRange(.05f, options.far_range);
        cam->Update();
    }
    driver->SetState(RS_LOD_QUALITY, I3DFloatAsInt(1.0f));
}

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

// STUB: GAME 0x545ce0
bool C_mission::FrmActBuff_StoreActor(C_actor *actor, bool flag)
{
    StubLog("C_mission::FrmActBuff_StoreActor");
    // Kept, so that the callers don't release it
    return true;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x546280
C_actor *C_mission::FrmActBuff_GetActor(const char *name, int type)
{
    StubLog("C_mission::FrmActBuff_GetActor %s", name);
    return NULL;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5464d0
bool C_mission::FrmActBuff_StoreActiveActor(C_actor *actor)
{
    StubLog("C_mission::FrmActBuff_StoreActiveActor");
    return false;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5466e0
void C_mission::FrmActBuff_RemoveActiveActor(C_actor *actor)
{
    StubLog("C_mission::FrmActBuff_RemoveActiveActor");
}

#pragma auto_inline(on)

// Whether the actor was kept active between missions: it leaves the list (the last one taking its place) and is reborn, a
// car reset with its tank full first (PS2's the same)
// FUNCTION: GAME 0x546770
bool C_mission::FrmActBuff_IsActiveActor(C_actor *actor)
{
    for (int i = active_actors.size(); i--;)
    {
        if (active_actors[i] == actor)
        {
            if (active_actors[i]->type == ACTOR_CAR)
            {
                ((C_car *)active_actors[i])->Reset(0.0f, true);
                ((C_car *)active_actors[i])->SetFuel(((C_car *)active_actors[i])->GetUnknown1fa0());
            }
            active_actors[i]->Rebirth();
            active_actors[i] = active_actors.back();
            active_actors.pop_back();
            return true;
        }
    }
    return false;
}

// Each buffer from its last entry, as PS2's. An entry's name is deleted before the entry is read again: the original
// reloads the array after the delete, then keeps the entry's address
// FUNCTION: GAME 0x546b70
void C_mission::FrmActBuff_ClearAll()
{
    int i;
    for (i = frame_actor_buffer.size(); i--;)
    {
        delete[] frame_actor_buffer[i].name;
        S_frame_actor_buffer &b = frame_actor_buffer[i];
        if (b.actor)
        {
            b.actor->GameDone();
            b.actor->Release();
            if (b.release_frame && b.frame)
                b.frame->Release();
        }
        else if (b.frame)
            b.frame->Release();
    }
    frame_actor_buffer.clear();
    for (i = active_actors.size(); i--;)
        active_actors[i]->GameDone();
    active_actors.clear();
    for (i = actor_sound_buffer.size(); i--;)
        actor_sound_buffer[i].sound->Release();
    actor_sound_buffer.clear();
}
