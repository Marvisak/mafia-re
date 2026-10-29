// The game of the loaded mission (PS2 game.cpp; the PC's file is the one at 0x5a0760-0x5bcc0c, docs/game-files.md).
// Written so far: Init, which main calls, and Done and DoneIntern, which C_mission::Close calls, with the file's static
// helpers they call and the functions of the game they call that the main menu's run reaches (the shadows' models,
// ClearDiffData, the semaphores, the city's music areas, the road blocks, the taxis' stops, the telephones, the traffic
// signs, the scream areas, the weather, the inter-city boxes, GlobalProgramRun, and the Clear functions); the stacks'
// Clear functions, which the destructor calls (the constructor and the destructor are game.h's), IShowSetVolume, which
// the profiles call, ScoreSetOn, LightnessClear and EntityBlock, which C_cutscene::Close calls. Stubs: what the run
// doesn't reach (PrepareBaseBlocks and the inter-city boxes' helper, both behind the base blocks, which stay closed
// while C_cache_base_block::Open is a stub; ChangeActState, InvalidateActor, BoxPlacementDel, RemoveDeathHumanFade).
// Open: the consoles' and file 12's globals (0x647a74, 0x647ab0, 0x63d6d4) are here for now; Init, Done and DoneIntern
// aren't matched yet.

#include "mission/game.h"
#include "mission/Mission.h"
#include "mission/C_cutscene.h"
#include "mission/C_effects.h"
#include "mission/C_program.h"
#include "mission/m_cache.h"
#include "actors/C_actor.h"
#include "actors/C_animations.h"
#include "actors/C_cartridge.h"
#include "actors/C_dog.h"
#include "actors/C_door.h"
#include "actors/C_human.h"
#include "actors/C_player.h"
#include "actors/C_public_physics.h"
#include "actors/C_stone.h"
#include "ai/C_person_traffic.h"
#include "ai/jp_ai_entity.h"
#include "ai/jp_ai_main.h"
#include "ai/pathweb.h"
#include "ai/road_web.h"
#include "collision/g_colls.h"
#include "input/C_ff_manager.h"
#include "ui/console.h"
#include "ui/g_dabing.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_func.h"
#include "ui/g_textdb.h"
#include "ui/racing.h"
#include "vehicles/C_car.h"
#include "vehicles/C_trf_c.h"
#include "vehicles/C_Vehicle.h"
#include "I3D_cache.h"
#include "stub.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "ISound/ISound.h"

// The two consoles of the game's screen (PS2 has three; the names are ours), constructed in this order (the file's first
// initializer, 0x5a0760, is console_1's)
// GLOBAL: GAME 0x647ab0
C_console console_1;
// GLOBAL: GAME 0x647a74
C_console console_2;

// File 12's (docs/game-files.md): its class's constructor counts up, the destructor down (the name is ours)
// GLOBAL: GAME 0x63d6d4
int unknown_63d6d4;

// A phase of the semaphores' lights (the names are ours)
struct S_semaphore_lights
{
    uint red : 1;
    uint orange : 1;
    uint green : 1;
};

// The lights in each of the ten phases: red for five, green for four, then orange; the two directions are five phases
// apart (the name is ours)
// GLOBAL: GAME 0x625c4c
static const S_semaphore_lights semaphore_lights[10] = {
    {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0},
    {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
    {0, 1, 0},
};

// The holes' models, whose dUra_fiRe Init copies (PC only, the name is ours)
// GLOBAL: GAME 0x636ff0
const char *hole_model_names[3] = {
    // STRING: GAME 0x63715c
    "hole1.i3d",
    // STRING: GAME 0x637150
    "hole2.i3d",
    // STRING: GAME 0x637144
    "hole3.i3d",
};

// The file's static helpers, defined after Init (where VC6 puts a static function it doesn't inline: after its first
// caller)
static void _show_progress_(float f);
static void PrepareBaseBlocks();
static void AddInterCityBox(tDynamicCollObject **colls, S_matrix *matrices, S_matrix *inv_matrices, int *n,
    I3D_frame **frames, S_vector pos, S_vector axis, float angle, S_vector scale);

// The shadows' tick (PS2 name; PS2's is empty): at each period it sorts the next of the two lists by the distance from a
// point ahead of the camera, and puts the shadows' models on the nearest ones, as many as the video options allow
inline void C_game::TickShadows(uint time)
{
    if (!unknown_2b0.Tick(time))
        return;
    if (!shadow_temp.size())
        return;
    I3D_frame *own = NULL;
    std::vector<S_shadow_item> *list;
    bool humans = unknown_2b8;
    if (!humans)
    {
        list = &shadow_items_2a0;
        if (player && player->car)
            own = player->car->frame;
    }
    else
    {
        list = &shadow_items;
        if (player)
            own = player->frame;
    }
    unknown_2b8 = !humans;
    if (!list->size())
        return;
    float ahead = 3.0f;
    I3D_frame *cam = act_mission->scene->current_camera;
    S_vector pos = cam->GetWorldPos() + cam->GetWorldDir() * ahead;
    int i;
    for (i = list->size(); i--;)
    {
        if ((*list)[i].frame != own)
            (*list)[i].dist = ((*list)[i].frame->GetWorldPos() - pos).Magnitude();
        else
            (*list)[i].dist = 0.0f;
    }
    qsort(&(*list)[0], list->size(), sizeof(S_shadow_item),
        (int (__cdecl *)(const void *, const void *))CompareShadows);
    for (i = list->size(); i--;)
    {
        S_shadow_item &s = (*list)[i];
        if (i >= act_mission->options.GetShadows())
        {
            if (s.on)
            {
                s.model->LinkTo(NULL, 0);
                s.model->Close();
                s.frame->Update();
                s.on = false;
                if (s.unknown_8)
                    s.unknown_8->SetOn(true);
            }
        }
        else if (!s.on)
        {
            SetupShadow(&s);
            s.model->LinkTo(s.frame, 0);
            s.frame->Update();
            s.on = true;
            if (s.unknown_8)
                s.unknown_8->SetOn(false);
        }
    }
}

// The traffic generators' choice (PS2 name): at each period the one whose range the camera is in, else the nearest of
// those without a range, becomes the active one
inline void C_game::SetActiveTrafficP(uint time)
{
    if (!unknown_18c.Tick(time))
        return;
    I3D_frame *cam = act_mission->scene->current_camera;
    S_vector pos = cam->GetWorldPos();
    float best = 1e30f;
    bool in_range = false;
    C_traffic_generator *gen = NULL;
    std::vector<C_actor *> &actors = act_mission->actors;
    for (int i = actors.size(); i--;)
    {
        if (actors[i]->type != ACTOR_TRAFFIC_GENERATOR)
            continue;
        C_traffic_generator *g = (C_traffic_generator *)actors[i];
        I3D_frame *frm = g->frame;
        g->active = false;
        float dist = S_vector(frm->GetWorldPos().x - pos.x, frm->GetWorldPos().y - pos.y,
            frm->GetWorldPos().z - pos.z).Magnitude();
        if (!g->unknown_70)
        {
            if (dist < g->range)
            {
                gen = g;
                in_range = true;
            }
        }
        else if (!in_range && dist < best)
        {
            best = dist;
            gen = g;
        }
    }
    if (gen)
    {
        gen->SetActState(ACTSTATE_0);
        gen->active = true;
    }
}

// The animations' tracks, loaded before they are needed (PS2 name): each animation's .tck, a ? in the name standing for
// both sides
inline void C_game::PreloadAnims()
{
    timeGetTime();
    for (S_anim_file *a = _anim_info_; a->id != 0xffff; a++)
    {
        const char *name;
        if (strchr(a->name, '?'))
        {
            char buf[128];
            strcpy(buf, a->name);
            char *side = strchr(buf, '?');
            *side = 'L';
            S_hlp::PreloadAnim(buf);
            *side = 'R';
            name = buf;
        }
        else
            name = a->name;
        S_hlp::PreloadAnim(name);
    }
    if (!C_animations_machine_de_luxe::base_tracks)
        C_animations_machine_de_luxe::base_tracks = C_animations_machine_de_luxe::tracks.size();
}

// PS2 has two bools, loads the mission's collisions itself and preloads more; false if it was done already
// FUNCTION: GAME 0x5a0810
bool C_game::Init()
{
    if (initialized)
        return false;
    unknown_3665 = false;
    unknown_3668 = 0;
    weather_on = act_mission->scene->GetWeatherSystemParam(WS_PARAM_ON) != 0;
    weather_count = act_mission->scene->GetWeatherSystemParam(WS_PARAM_COUNT);
    unknown_3638 = -1;
    if (isound)
    {
        isound->SetVolume(0.0f);
        act_mission->scene->Render();
        act_mission->restore_volume = true;
    }
    stop_sounds.clear();
    unknown_2e4.clear();
    unknown_3674.clear();
    score_on = false;
    ScoreSetOn(false);
    score = 0;
    unknown_3644 = false;
    unknown_3664 = true;
    shvestky.unknown_ac = true;
    unknown_2fd4 = 7000;
    unknown_164 = false;
    unknown_140.clear();
    AIManager.police.ZacatekSmeny();
    AIManager.groups.Init();
    AIManager.follow.Init();
    AIManager.fight.Init();
    _show_progress_(0.0f);
    C_player::SetUnknown6478b4(0);
    C_player::unknown_6478b0 = false;
    if (unknown_363c)
    {
        C_Vehicle::v_light_sector.clear();
        C_Vehicle::v_sound_sector.clear();
        S_hlp::cbEnumSectors(act_mission->scene->primary_sector);
    }
    C_public_physics::unknown_63d78c.clear();
    strcpy(unknown_2ef4, "");
    unknown_2f14 = 0;
    unknown_2ed8 = false;
    unknown_48 = 0;
    unknown_29e8 = 0;
    unknown_2b0c = 0;
    unknown_2b10 = 0;
    unknown_2b14 = 0;
    unknown_2d44 = false;
    unknown_2d48 = 0;
    unknown_2d50 = 0;
    unknown_2d58 = 0;
    unknown_2d60 = 0;
    unknown_2edc = 0;
    unknown_2ee4.clear();
    unknown_168.clear();
    unknown_178 = -1;
    i_shows.clear();
    i_show_fades.clear();
    unknown_2f38 = 0;
    unknown_30c = 0;
    unknown_310 = 0;
    unknown_304.time = 0;
    unknown_304.period = 813;
    enemy_talks.clear();
    find_actors.clear();
    unknown_2f9c = true;
    unknown_2fc4.clear();
    npc_kill_events.clear();
    C_actor::posscoll_frame.clear();
    shvestky.GameInit();
    C_traffic_generator::EnableTaxiDriver(false);
    C_traffic_generator::PlayerInTaxi(NULL);
    C_traffic_generator::unknown_63d728 = true;
    sound_arena.Init();
    LightnessClear();
    unknown_160 = 0;
    invisible_actors.clear();
    mafiaIndicators.RadarClear();
    act_mission->GlobalProgramGameInit();
    // STRING: GAME 0x6376e0
    act_mission->GlobalProgramRun("GameInitStart");
    mafiaIndicators.GameInit();
    unknown_2e80.clear();
    unknown_2e90.clear();
    box_placement_temp.clear();
    act_mission->effects->LoadStaticParticles();
    act_mission->effects->Mission_Start();
    act_mission->web_path->DeRiciConns();
    ff_manager.GameInit();
    if (ff_manager.human_effects)
        C_human::InitFFEffects();
    if (ff_manager.car_effects)
        C_car::InitFFEffects();
    unknown_2adc.clear();
    unknown_2aec.clear();
    unknown_2ad9 = false;
    unknown_63d6d4 = 0;
    blood_manager.GameInit();
    frame_placements.clear();
    unknown_10.clear();
    unknown_20.clear();
    int fast_find = driver->GetState(RS_FAST_FIND);
    driver->SetState(RS_FAST_FIND, 0);
    srand(time(NULL));
    console_1.Clear();
    console_1.r = 0xff;
    console_1.g = 0xff;
    console_1.b = 0xff;
    console_1.x = .01f;
    console_1.y = .89f;
    console_1.sx = .035f;
    console_1.sy = .028f;
    console_1.upwards = true;
    console_1.right_aligned = false;
    console_1.reversed = false;
    console_1.lines = 20;
    console_1.time = 10000;
    ConsoleReg(&console_1, CONREG_RENDER);
    console_2.Clear();
    console_2.r = 0x80;
    console_2.g = 0xff;
    console_2.b = 0x80;
    console_2.x = .01f;
    console_2.y = .45f;
    console_2.sx = .038f;
    console_2.sy = .028f;
    console_2.upwards = false;
    console_2.right_aligned = false;
    console_2.reversed = false;
    console_2.lines = 20;
    ConsoleReg(&console_2, CONREG_CLEAR | CONREG_RENDER);
    AIManager.segments.Load();
    ff_manager.unknown_74 = false;
    if (unknown_363c)
    {
        C_car::LoadTyres();
        C_car::LoadMaterials();
        C_car::LoadParticles();
    }
    DelAllHydrants();
    DabingInit();
    using_object.ClearObjects();
    using_object.ClearInvalidItems();
    driver_men.clear();
    wheel_anim_time = 0.0f;
    wheel_anim_time_formula = 0.0f;
    I3D_animation_set *set;
    // STRING: GAME 0x6376cc
    anim_cache.Create("AutoRidicVolant.I3D", &set, NULL, NULL);
    if (set)
    {
        wheel_anim_time = (float)set->GetEndTime();
        set->Release();
    }
    // STRING: GAME 0x6376b8
    anim_cache.Create("FormuleVolant.I3D", &set, NULL, NULL);
    if (set)
    {
        wheel_anim_time_formula = (float)set->GetEndTime();
        set->Release();
    }
    if (unknown_363c && unknown_2f9c)
    {
        // STRING: GAME 0x63769c
        AddShadowModel("models\\ShadowManHIGH.i3d");
        // STRING: GAME 0x637680
        AddShadowModel("models\\ShadowGirlHIGH.i3d");
        // STRING: GAME 0x637668
        AddShadowModel("models\\ShadowMan.i3d");
        // STRING: GAME 0x637650
        AddShadowModel("models\\ShadowGirl.i3d");
        // STRING: GAME 0x63763c
        AddShadowModel("models\\alfaS.i3d");
        // STRING: GAME 0x637624
        AddShadowModel("models\\buick_coupS.i3d");
        // STRING: GAME 0x63760c
        AddShadowModel("models\\car_shadow.i3d");
        // STRING: GAME 0x6375f4
        AddShadowModel("models\\deliveryS.i3d");
        // STRING: GAME 0x6375dc
        AddShadowModel("models\\fordAcabS.i3d");
        // STRING: GAME 0x6375c4
        AddShadowModel("models\\fordAtudS.i3d");
        // STRING: GAME 0x6375ac
        AddShadowModel("models\\fordor_bigS.i3d");
        // STRING: GAME 0x637598
        AddShadowModel("models\\fordorS.i3d");
        // STRING: GAME 0x637580
        AddShadowModel("models\\fordTrunS.i3d");
        // STRING: GAME 0x63756c
        AddShadowModel("models\\hudCoupS.i3d");
        // STRING: GAME 0x637554
        AddShadowModel("models\\chryslerS.i3d");
        // STRING: GAME 0x637540
        AddShadowModel("models\\mercedS.i3d");
        // STRING: GAME 0x63752c
        AddShadowModel("models\\phantomS.i3d");
        // STRING: GAME 0x637518
        AddShadowModel("models\\pickupS.i3d");
        // STRING: GAME 0x637504
        AddShadowModel("models\\pierceS.i3d");
        // STRING: GAME 0x6374f0
        AddShadowModel("models\\truckAS.i3d");
        // STRING: GAME 0x6374dc
        AddShadowModel("models\\truckBS.i3d");
        // STRING: GAME 0x6374c8
        AddShadowModel("models\\bugattiS.i3d");
        // STRING: GAME 0x6374ac
        AddShadowModel("models\\duesenRacingS.i3d");
        // STRING: GAME 0x637498
        AddShadowModel("models\\milerS.i3d");
        // STRING: GAME 0x637484
        AddShadowModel("models\\Fv8SHDW.i3d");
        // STRING: GAME 0x637470
        AddShadowModel("models\\xedosS.i3d");
        // STRING: GAME 0x63745c
        AddShadowModel("models\\broukS.i3d");
        // STRING: GAME 0x637448
        AddShadowModel("models\\toyotaS.i3d");
        // STRING: GAME 0x637430
        AddShadowModel("models\\cord_sedanhs.i3d");
        // STRING: GAME 0x637418
        AddShadowModel("models\\flamer00s.i3d");
        // STRING: GAME 0x637400
        AddShadowModel("models\\fordapicks.i3d");
        // STRING: GAME 0x6373ec
        AddShadowModel("models\\fordthd.i3d");
        // STRING: GAME 0x6373d4
        AddShadowModel("models\\fordtrons.i3d");
        // STRING: GAME 0x6373bc
        AddShadowModel("models\\hotrodp2s.i3d");
        // STRING: GAME 0x6373a4
        AddShadowModel("models\\hotrodp3s.i3d");
        // STRING: GAME 0x63738c
        AddShadowModel("models\\hotrodp4s.i3d");
        // STRING: GAME 0x637374
        AddShadowModel("models\\hotrodp5s.i3d");
        // STRING: GAME 0x637358
        AddShadowModel("models\\chevroletm6hs.i3d");
        // STRING: GAME 0x637340
        AddShadowModel("models\\hotrodp6sh.i3d");
        // STRING: GAME 0x637328
        AddShadowModel("models\\deliverys.i3d");
        // STRING: GAME 0x637314
        AddShadowModel("models\\thunders.i3d");
        // STRING: GAME 0x637300
        AddShadowModel("models\\fordths.i3d");
        // STRING: GAME 0x6372e8
        AddShadowModel("models\\hotrodp6s.i3d");
    }
    shadow_items.clear();
    shadow_items_2a0.clear();
    _show_progress_(.1f);

    // The player: the last of the players that isn't ...
    std::vector<C_actor *> &actors = act_mission->actors;
    player = NULL;
    int i;
    for (i = actors.size(); i--;)
    {
        if (actors[i]->type == ACTOR_PLAYER && !(actors[i]->unknown_4 & 0x10))
        {
            player = (C_human *)actors[i];
            break;
        }
    }
    for (i = actors.size(); i--;)
    {
        if (actors[i]->type == ACTOR_CAR)
            mafiaIndicators.RadarAddCar((C_car *)actors[i], (uint)-1);
    }
    I3D_camera *cam = act_mission->scene->current_camera;
    unknown_2a10 = S_vector(0.0f, 0.0f, 0.0f);
    if (player)
    {
        I3D_frame *frm = player->frame;
        if (frm)
            unknown_2a10 = frm->GetWorldPos();
    }
    else if (cam)
        unknown_2a10 = cam->GetWorldPos();
    unknown_2a0c = 0;
    // The camera behind the player
    if (player && cam)
    {
        I3D_frame *frm = player->frame;
        S_vector pos = frm->GetPos();
        S_vector dir = frm->GetDir();
        cam->SetDir(dir, 0.0f);
        S_vector p = pos - (dir + dir);
        p.y += 2.0f;
        cam->SetPos(p);
        cam->Update();
    }
    unknown_e8 = player;
    don_ocas = new C_don_ocas;
    C_traffic_generator::SetVisible(true);
    unknown_10c = false;
    C_traffic_generator::ClearElements();
    for (i = actors.size(); i--;)
    {
        if (actors[i]->type == ACTOR_DOOR)
            ((C_door *)actors[i])->ResetUnknown110();
    }

    // The actors' GameInit, the loading bar moving from 0.1 to 0.7 over them, twenty times at most
    float step = 0.0f;
    if (actors.size())
        step = .6f / actors.size();
    int every = actors.size() > 20 ? actors.size() / 20 : actors.size();
    int count = 0;
    if (unknown_363c)
    {
        memset(holes, 0, sizeof(holes));
        memset(unknown_2fd8, 0, sizeof(unknown_2fd8));
    }
    for (i = 0; i < (int)actors.size(); i++)
    {
        if (act_mission->FrmActBuff_IsActiveActor(actors[i]))
            actors[i]->SetActState(ACTSTATE_0);
        else
            actors[i]->GameInit();
        if (actors[i]->type == ACTOR_CAR)
            ((C_car *)actors[i])->CalmVehicle(20);
        if (++count >= every)
        {
            count = 0;
            _show_progress_(i * step + .1f);
        }
    }
    unknown_ec.clear();
    unknown_fc.clear();
    unknown_110.clear();
    for (i = actors.size(); i--;)
        unknown_ec.push_back(actors[i]);
    if (unknown_10c)
        ChangeActState();
    InterCity_GameInit();
    unknown_18c.time = 0;
    unknown_18c.period = 6000;
    SetActiveTrafficP(unknown_18c.period + 1);
    fires.clear();
    camera.Reset();
    camera.SetCamera(act_mission->scene->current_camera);
    camera.SetPlayer(player);
    if (unknown_2f9c)
        PreloadAnims();
    C_dog::LoadTCK();
    sledovacka.Init();
    glass.Init(5);
    debris.Init(10.0f);
    unknown_2ad8 = false;
    compass.Activate(NULL);
    unknown_130.clear();
    unknown_19c = 0;
    unknown_1a0 = 5;
    LoadSemaphores();
    TickSemaphores();
    unknown_194.time = 0;
    unknown_194.period = 3000;
    LoadRoadBlocks();
    LoadTelephones();
    CreateSignalMarkIDX();
    LoadTaxiPoints();
    unknown_204 = 0;
    LoadCityMusicAreas();
    unknown_2fb4 = 0;
    unknown_2fb0 = -1;
    unknown_2fb8 = -1;
    unknown_2fbc = -1;
    unknown_2fc0 = false;
    LoadScreamAreas();
    unknown_362c = -1;
    unknown_3628 = 1;
    unknown_2b0.time = 0;
    unknown_2b0.period = 823;
    unknown_2b8 = true;
    TickShadows(unknown_2b0.period + 1);
    _show_progress_(.7f);

    if (unknown_363c)
    {
        // The blood's and the dirt's textures, each with its alpha (the HUD's literals for the first)
        if (GetIGraph()->CreateITexture(
                "2blood.bmp",
                "2blood+.bmp", 4, &unknown_29bc[0]))
            unknown_29bc[0] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x6372b8
                "2flek.bmp",
                // STRING: GAME 0x6372c4
                "2flek+.bmp", 4, &unknown_29bc[1]))
            unknown_29bc[1] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x63729c
                "2dirbet.bmp",
                // STRING: GAME 0x6372a8
                "2dirbet+.bmp", 4, &unknown_29bc[2]))
            unknown_29bc[2] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637280
                "2dirdre.bmp",
                // STRING: GAME 0x63728c
                "2dirdre+.bmp", 4, &unknown_29bc[3]))
            unknown_29bc[3] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637264
                "2dirple.bmp",
                // STRING: GAME 0x637270
                "2dirple+.bmp", 4, &unknown_29bc[4]))
            unknown_29bc[4] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637254
                "2dirsklo.bmp",
                "2dirsklo.bmp", 4, &unknown_29bc[5]))
            unknown_29bc[5] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637230
                "2bfootprint.bmp",
                // STRING: GAME 0x637240
                "2bfootprint+.bmp", 4, &unknown_29bc[6]))
            unknown_29bc[6] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637214
                "2blood3.bmp",
                // STRING: GAME 0x637220
                "2blood3+.bmp", 4, &unknown_29bc[7]))
            unknown_29bc[7] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x6371f4
                "2dogfoot.bmp",
                // STRING: GAME 0x637204
                "2dogfoot+.bmp", 4, &unknown_29bc[8]))
            unknown_29bc[8] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x6371e8
                "skrob.tga", NULL, 2, &unknown_29bc[9]))
            unknown_29bc[9] = NULL;
    }
    _show_progress_(.75f);

    if (unknown_363c)
    {
        // The projectors of the blood and the holes
        unknown_29e4 = 0;
        for (i = 100; i--;)
        {
            S_unknown_75c &p = unknown_75c[i];
            p.in_collision = false;
            p.frame = driver->CreateVisual(I3D_VISUAL_PROJECTOR);
            if (p.frame)
            {
                I3D_projector *proj = (I3D_projector *)p.frame;
                proj->SetOn(false);
                proj->SetMode((PROJECTOR_MODE)1);
                proj->SetOrthogonal(true);
            }
        }
        for (i = 100; i--;)
        {
            S_unknown_2fd8 &p = unknown_2fd8[i];
            p.unknown_4 = 0;
            p.unknown_8 = 0;
            p.frame = driver->CreateVisual(I3D_VISUAL_PROJECTOR);
            if (p.frame)
            {
                I3D_projector *proj = (I3D_projector *)p.frame;
                proj->SetOn(false);
                proj->SetMode((PROJECTOR_MODE)1);
                proj->SetOrthogonal(true);
            }
        }
        _show_progress_(.78f);

        // The stones
        num_stones = 0;
        memset(stones, 0, sizeof(stones));
        if (unknown_2f9c)
        {
            I3D_model *mod = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            if (mod)
            {
                // STRING: GAME 0x6371dc
                if (model_cache.Open(mod, "2stone.i3d", 0, NULL, NULL, NULL) == I3D_OK && mod->NumChildren() &&
                    mod->GetChild(0)->GetType() == FRAME_VISUAL)
                {
                    I3D_frame *vis = mod->GetChild(0);
                    for (i = 0; i < 32; i++)
                    {
                        I3D_frame *obj = driver->CreateVisual(I3D_VISUAL_OBJECT);
                        if (!obj)
                            continue;
                        obj->Duplicate(vis);
                        C_stone *stone = new C_stone;
                        if (!stone->Init(obj))
                            stone->Release();
                        else
                        {
                            obj->SetOn(false);
                            stone->GameInit();
                            stones[i] = stone;
                        }
                    }
                }
                mod->Release();
            }
        }
        _show_progress_(.8f);

        // The cartridge cases
        num_cartridges = 0;
        memset(cartridges, 0, sizeof(cartridges));
        if (unknown_2f9c)
        {
            I3D_model *mod = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            if (mod)
            {
                // STRING: GAME 0x6371cc
                if (model_cache.Open(mod, "2nabojnice.i3d", 0, NULL, NULL, NULL) == I3D_OK)
                {
                    // STRING: GAME 0x6371c4
                    I3D_frame *line = mod->FindChildFrame("Line01", ENUMF_ALL);
                    if (line)
                    {
                        for (i = 0; i < 100; i++)
                        {
                            I3D_frame *obj = driver->CreateVisual(I3D_VISUAL_OBJECT);
                            if (!obj)
                                continue;
                            obj->Duplicate(line);
                            C_cartridge *c = new C_cartridge;
                            if (!c->Init(obj))
                                c->Release();
                            else
                            {
                                c->GameInit();
                                cartridges[i] = c;
                            }
                        }
                    }
                }
                mod->Release();
            }
        }
        num_cartridges_b = 0;
        memset(cartridges_b, 0, sizeof(cartridges_b));
        if (unknown_2f9c)
        {
            I3D_model *mod = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            if (mod)
            {
                // STRING: GAME 0x6371b4
                if (model_cache.Open(mod, "2nabojniceb.i3d", 0, NULL, NULL, NULL) == I3D_OK)
                {
                    I3D_frame *line = mod->FindChildFrame("Line01", ENUMF_ALL);
                    if (line)
                    {
                        for (i = 0; i < 30; i++)
                        {
                            I3D_frame *obj = driver->CreateVisual(I3D_VISUAL_OBJECT);
                            if (!obj)
                                continue;
                            obj->Duplicate(line);
                            C_cartridge *c = new C_cartridge;
                            if (!c->Init(obj))
                                c->Release();
                            else
                            {
                                c->GameInit();
                                cartridges_b[i] = c;
                            }
                        }
                    }
                }
                mod->Release();
            }
        }
        _show_progress_(.83f);

        // The holes: a dUra_fiRe of each model, and the copies that are shown
        hole_models.clear();
        for (const char **name = hole_model_names; name < hole_model_names + 3; name++)
        {
            I3D_model *mod = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            const char *n = *name;
            if (model_cache.Open(mod, n, 0, NULL, NULL, NULL) == I3D_OK)
            {
                // STRING: GAME 0x6371a8
                I3D_frame *rect = mod->FindChildFrame("Rectangle01", ENUMF_ALL);
                if (rect)
                {
                    I3D_frame *obj = driver->CreateVisual(I3D_VISUAL_OBJECT);
                    if (obj)
                    {
                        obj->Duplicate(rect);
                        // STRING: GAME 0x63719c
                        obj->SetName("dUra_fiRe");
                        obj->LinkTo(NULL, 0);
                        hole_models.push_back(obj);
                    }
                }
            }
            mod->Release();
        }
        num_holes = 0;
        memset(holes, 0, sizeof(holes));
        for (i = 0; i < 100; i++)
        {
            holes[i] = driver->CreateVisual(I3D_VISUAL_OBJECT);
            if (holes[i])
            {
                holes[i]->SetName("dUra_fiRe");
                holes[i]->LinkTo(NULL, 0);
            }
        }
        _show_progress_(.86f);

        // The sounds
        for (i = 0; i < 3; i++)
            C_cartridge::sounds[i] = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        for (i = 0; i < 10; i++)
            unknown_2a20[i] = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        for (i = 0; i < 10; i++)
        {
            S_unknown_2a48 &l = unknown_2a48[i];
            l.unknown_4 = -1;
            l.light = (I3D_light *)driver->CreateFrame(FRAME_LIGHT);
            if (l.light)
            {
                l.light->SetLightType((I3D_LIGHTTYPE)1);
                l.light->SetDiffuse(1.0f, 1.0f, 1.0f);
                l.light->SetDiffusePower(1.0f);
                l.light->SetMode(0x11);
                l.light->SetOn(false);
            }
        }
        for (i = 0; i < 3; i++)
        {
            S_unknown_2ac0 &s = unknown_2ac0[i];
            s.unknown_4 = -1;
            s.sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
            if (s.sound)
            {
                sound_table.ConstructSound(s.sound, 0xbf);
                s.sound->SetLoop(false);
                s.sound->SetOn(false);
            }
        }

        // The sheet and the money
        unknown_365c = NULL;
        sheet = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        if (sheet)
        {
            // STRING: GAME 0x636164
            if (!model_cache.Open(sheet, "9sheet.i3d", 0, NULL, NULL, NULL))
            {
                C_actor::DebustizeFrame(sheet);
                C_actor::NoShadowFrame(sheet);
            }
            else
            {
                sheet->Release();
                sheet = NULL;
            }
        }
        money = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        if (money)
        {
            // STRING: GAME 0x636158
            if (!model_cache.Open(money, "9money.i3d", 0, NULL, NULL, NULL))
            {
                C_actor::DebustizeFrame(money);
                C_actor::NoShadowFrame(money);
            }
            else
            {
                money->Release();
                money = NULL;
            }
        }
    }
    rmorf.Mission_Start();
    driver->SetState(RS_FAST_FIND, fast_find);
    mafiaIndicators.SetUnknown40a4(4);
    if (player)
    {
        if (player->health > player->max_health)
            player->health = player->max_health;
        mafiaIndicators.PlayerSetLives((int)(player->health / player->max_health * 100.0f));
    }
    else
        mafiaIndicators.PlayerSetLives(100);
    mafiaIndicators.PlayerSetAccuracy(1.0f);
    if (player)
        ((C_player *)player)->UpdateControls();
    camera.SetModeChanged();
    if (CRacing && unknown_2f9c)
        CRacing->GameInit();
    mafiaLoadSave.GarageGameInit();
    _show_progress_(.9f);
    SetWBuffer(true);
    // STRING: GAME 0x637190
    act_mission->GlobalProgramRun("GameInitEnd");
    if (unknown_10c)
        ChangeActState();
    act_mission->SetUnknown78(10);
    unknown_2f6c = 0;
    unknown_2f70 = 0;
    unknown_2f50 = NULL;
    unknown_2f54[1] = NULL;
    unknown_2f54[0] = NULL;
    act_mission->UpdateSettings();
    _show_progress_(1.0f);
    if (mafiaLoadSave.progress)
    {
        delete mafiaLoadSave.progress;
        mafiaLoadSave.progress = NULL;
    }
    if (!unknown_2f9c && isound)
        isound->SetVolume(mafiaLoadSave.settings.sound_volume);
    if (base_cache.IsOpen())
    {
        base_cache.VisibleAllFrames();
        camera.Tick(1);
        base_cache.Tick(0xfffffff);
        act_mission->FinishTimeTransparentObjects();
        PrepareBaseBlocks();
    }
    initialized = true;
    OsefujPocasi(false, (uint)-1);
    if (player)
    {
        ((C_player *)player)->unknown_ab8 = 1.0f;
        ((C_player *)player)->unknown_abc = 1.0f;
        ((C_player *)player)->SetAlpha(1.0f);
    }
    death_human_fades.clear();
    unknown_3684 = false;
    act_mission->FrmActBuff_ClearAll();
    igraph->ResetRenderProps();
    igraph->Clear(0, 1.0f, 3);
    igraph->Present();
    unknown_363c = false;
    unknown_363d = true;
    // The bank's mission: the Duesenberg (deuseJco) the player came in stays put
    // STRING: GAME 0x637180
    if (mafiaLoadSave.GetMission() == 0x208 && !_stricmp(act_mission->GetMissionName(), "MISE19-BANKA"))
    {
        std::vector<C_actor *> &global = C_actor::GetGlobalActors();
        for (i = global.size(); i--;)
        {
            if (global[i]->GetType() == ACTOR_CAR && global[i]->GetFrame() &&
                !strcmp(global[i]->GetFrame()->GetName(),
                    // STRING: GAME 0x637174
                    "deuseJco"))
            {
                global[i]->SetActState(ACTSTATE_2);
                global[i]->GetFrame()->SetOn(false);
                break;
            }
        }
    }
    if (base_cache.IsOpen())
    {
        // STRING: GAME 0x637168
        I3D_frame *frm = act_mission->GetScene()->FindFrame("occluder198", ENUMF_ALL);
        if (frm)
            frm->SetOn(false);
    }
    return true;
}

// Moves the mission's loading screen to that fraction (PS2 name): mafiaLoadSave's, which C_mission::Open created
// FUNCTION: GAME 0x5a3960
static void _show_progress_(float f)
{
    if (mafiaLoadSave.progress)
    {
        mafiaLoadSave.progress->SetProgressGameInit(f);
        mafiaLoadSave.progress->Present();
    }
}

#pragma auto_inline(off)

// Called at Init's end when the city's base blocks are open (the name is ours)
// STUB: GAME 0x5a3990
static void PrepareBaseBlocks()
{
    StubLog("PrepareBaseBlocks (0x5a3990)");
}

#pragma auto_inline(on)

// The sectors' lights (PS2's S_hlp, the name of Init's local struct)
// FUNCTION: GAME 0x5a3b90
void C_game::S_hlp::cbEnumSectors(I3D_frame *frm)
{
    if (frm->GetType() == FRAME_SECTOR)
        C_Vehicle::AddSectorForProjectorLights((I3D_sector *)frm);
    for (int i = frm->NumChildren(); i--;)
        cbEnumSectors(frm->GetChild(i));
}

// An animation's track, its .tck file (the name is ours)
// FUNCTION: GAME 0x5a3bd0
void C_game::S_hlp::PreloadAnim(const char *name)
{
    char buf[128];
    strcpy(buf, name);
    char *ext = strchr(buf, '.');
    if (ext)
        *ext = 0;
    // STRING: GAME 0x632294
    strcat(buf, ".tck");
    C_animations_machine_de_luxe::LoadTrack(buf);
}

// PS2 name: what the mission's end undoes of Init, before DoneIntern; false if Init hasn't run
// FUNCTION: GAME 0x5a3c60
bool C_game::Done()
{
    if (!initialized)
        return false;
    StopAllSounds();
    ClearDeathHumanFade();
    // A last frame with "please wait" (PS2's PleaseWait)
    igraph->ResetRenderProps();
    igraph->Clear(0, 1.0f, 3);
    igraph->BeginScene();
    igraph->SetState((DX_STATE)0, 1);
    mafiaIndicators.OutText(textDatabase.GetText(0x581), mafiaIndicators.screen_scale.x * 780.0f + levyOkraj.x,
        mafiaIndicators.screen_scale.y * 545.0f, mafiaIndicators.screen_scale.x * 0.0f, mafiaIndicators.screen_scale.y * 40.0f, (uint)-1, 4, 1, NULL);
    igraph->EndScene();
    igraph->Present();
    if (cutscene.IsPlaying())
    {
        cutscene.Close();
        camera.SetCutscene(false);
    }
    mafiaIndicators.RadarClear();
    mafiaIndicators.SubtitlesClear();
    mafiaIndicators.ConsoleClear();
    mafiaIndicators.MapEnable(false);
    mafiaIndicators.GameDone();
    mafiaLoadSave.ObjectivesClear();
    stop_sounds.clear();
    unknown_2e4.clear();
    DelAllHydrants();
    unknown_2ed8 = true;
    LightnessClear();
    int i;
    for (i = unknown_168.size(); i--;)
    {
        if (unknown_168[i])
            unknown_168[i]->SetOn(true);
    }
    unknown_168.clear();
    // STRING: GAME 0x637738
    act_mission->GlobalProgramRun("GameDoneStart");
    if (camera.mode == CAMERA_CP_SNIPER)
        camera.SetSniperMode(false);
    camera.Link(NULL);
    camera.GameDone();
    camera.SetCamera(NULL);
    I3D_camera *cam = act_mission->scene->current_camera;
    if (cam)
    {
        cam->LinkTo(act_mission->scene->primary_sector, 1);
        cam->Update();
    }
    camera.Reset();
    {
        // The traffic's cars
        std::vector<C_traffic_car *> cars;
        for (i = C_actor::global_actors.size(); i--;)
        {
            if (C_actor::global_actors[i]->type == ACTOR_TRAFFIC_CAR)
                cars.push_back((C_traffic_car *)C_actor::global_actors[i]);
        }
        for (i = cars.size(); i--;)
            cars[i]->TakATedSiVasKupim_Vsecky();
    }
    shvestky.GameDone();
    unknown_2ee4.clear();
    // The cars in the garage leave the game
    for (i = 0; i < (int)unknown_140.size();)
    {
        if (unknown_140[i]->type == ACTOR_CAR && mafiaLoadSave.IsCarIn((C_car *)unknown_140[i]))
        {
            unknown_140[i] = unknown_140.back();
            unknown_140.erase(unknown_140.end() - 1);
            continue;
        }
        i++;
    }
    {
        std::vector<C_car *> cars;
        if (NewMissionActors[0] && NewMissionActors[0]->type == ACTOR_CAR)
            cars.push_back((C_car *)NewMissionActors[0]);
        mafiaLoadSave.GarageGameDone(&cars);
    }
    blood_manager.GameDone();
    if (player)
        ((C_player *)player)->SetSniperMode(false);
    if (CRacing && unknown_2f9c)
    {
        CRacing->GameDone();
        if (CRacing->unknown_1)
        {
            delete CRacing;
            CRacing = NULL;
        }
    }
    act_mission->effects->Mission_Done();
    ClearDiffData();
    ff_manager.GameDone();
    if (ff_manager.human_effects)
        C_human::DoneFFEffects();
    if (ff_manager.car_effects)
        C_car::DoneFFEffects();
    cutscene.SetState(CUTSCENE_STATE_0);
    C_animations_machine_de_luxe::ClearNoBaseTracks();
    for (i = driver_men.size(); i--;)
    {
        if (driver_men[i].model)
        {
            driver_men[i].model->StopAnimation(driver_men[i].anim);
            C_human::HideLegs(driver_men[i].model, false);
        }
    }
    driver_men.clear();
    FreeSemaphores();
    city_music_areas.clear();
    ConsoleUnReg(&console_1);
    console_1.Clear();
    ConsoleUnReg(&console_2);
    console_2.Clear();
    for (i = shadow_items.size(); i--;)
    {
        S_shadow_item &s = shadow_items[i];
        if (s.model)
        {
            s.model->LinkTo(NULL, 0);
            s.model->Close();
            s.model->Release();
            s.frame->Update();
            if (s.name)
                delete[] s.name;
        }
    }
    shadow_items.clear();
    for (i = shadow_items_2a0.size(); i--;)
    {
        S_shadow_item &s = shadow_items_2a0[i];
        if (s.model)
        {
            s.model->LinkTo(NULL, 0);
            s.model->Close();
            s.model->Release();
            s.frame->Update();
            if (s.name)
                delete[] s.name;
        }
    }
    shadow_items_2a0.clear();
    for (i = fires.size(); i--;)
    {
        if (fires[i])
            delete fires[i];
    }
    fires.clear();
    using_object.ClearObjects();
    using_object.ClearInvalidItems();
    ClearTemporaryActors();
    unknown_130.clear();
    std::vector<C_actor *> &actors = act_mission->actors;
    C_traffic_generator::FreeDynCollElements();
    InterCity_GameDoneInGameDone();
    // Kept actors stay as they are
    for (i = actors.size(); i--;)
    {
        C_actor *act = actors[i];
        if (!act->unknown_66 || act->type != ACTOR_CAR)
            act->GameDone();
        else if (act->act_state != ACTSTATE_2)
        {
            if (!act_mission->FrmActBuff_StoreActiveActor(actors[i]))
                actors[i]->GameDone();
        }
        else
            act->GameDone();
    }
    C_traffic_generator::ClearElements();
    if (don_ocas)
    {
        don_ocas->Release();
        don_ocas = NULL;
    }
    unknown_ec.clear();
    unknown_fc.clear();
    unknown_110.clear();
    act_mission->DelNotMeActors();
    AIManager.police.KonecSmeny();
    AIManager.follow.Destroy();
    AIManager.fight.Destroy();
    AIManager.groups.Destroy();
    AIManager.segments.Clear();
    act_mission->ClearGameAnimModels();
    player = NULL;
    glass.Done();
    debris.Done();
    sledovacka.Done();
    npc_kill_events.clear();
    ClearEffects();
    compass.Activate(NULL);
    DabingDone();
    unknown_1d4.clear();
    for (i = 100; i--;)
    {
        S_unknown_75c &p = unknown_75c[i];
        if (p.in_collision)
            gifCollision.DynErase(&p.coll);
        if (p.frame)
            p.frame->LinkTo(NULL, 0);
    }
    for (i = 100; i--;)
    {
        if (unknown_2fd8[i].frame)
            unknown_2fd8[i].frame->LinkTo(NULL, 0);
    }
    for (i = 0; i < 100; i++)
    {
        if (cartridges[i] && cartridges[i]->frame)
            cartridges[i]->frame->LinkTo(NULL, 0);
    }
    for (i = 0; i < 30; i++)
    {
        if (cartridges_b[i] && cartridges_b[i]->frame)
            cartridges_b[i]->frame->LinkTo(NULL, 0);
    }
    for (i = 0; i < 100; i++)
    {
        if (holes[i])
            holes[i]->LinkTo(NULL, 0);
    }
    for (i = 0; i < 3; i++)
    {
        if (C_cartridge::sounds[i])
            C_cartridge::sounds[i]->SetOn(false);
    }
    for (i = 0; i < 10; i++)
    {
        if (unknown_2a20[i])
            unknown_2a20[i]->SetOn(false);
    }
    for (i = 0; i < 10; i++)
    {
        if (unknown_2a48[i].light)
        {
            unknown_2a48[i].light->LinkTo(NULL, 0);
            unknown_2a48[i].light->SetOn(false);
        }
    }
    for (i = 0; i < 3; i++)
    {
        if (unknown_2ac0[i].sound)
            unknown_2ac0[i].sound->SetOn(false);
    }
    rmorf.Mission_Done();
    mafiaIndicators.SetUnknown40a4(0);
    for (i = unknown_2e80.size(); i--;)
        BoxPlacementDel(unknown_2e80[i]->frame);
    for (i = unknown_2e90.size(); i--;)
        delete unknown_2e90[i];
    unknown_2e80.clear();
    unknown_2e90.clear();
    box_placement_temp.clear();
    for (i = unknown_20.size(); i--;)
    {
        unknown_20[i]->LinkTo(NULL, 0);
        unknown_20[i]->Release();
    }
    unknown_20.clear();
    for (i = unknown_10.size(); i--;)
        unknown_10[i]->SetOn(true);
    unknown_10.clear();
    // The frames' saved state back
    for (i = frame_placements.size(); i--;)
    {
        S_frame_placement &s = frame_placements[i];
        s.frame->SetOn(s.on);
        s.frame->SetPos(s.pos);
        I3D_frame *frm = s.frame;
        frm->m_Rot = s.rot;
        frm->m_Rot.Normalize();
        frm->m_FrmFlags = (frm->m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
        s.frame->SetScale(s.scale);
        s.frame->LinkTo(s.parent, 0);
        s.frame->Update();
    }
    frame_placements.clear();
    gifCollision.GameDone();
    road_blocks.clear();
    telephones.clear();
    taxi_points.clear();
    unknown_1e4.clear();
    if (base_cache.IsOpen())
        base_cache.VisibleAllFrames();
    // STRING: GAME 0x63772c
    act_mission->GlobalProgramRun("GameDoneEnd");
    act_mission->GlobalProgramGameDone();
    DestroyIShowAll();
    if (unknown_2f50)
    {
        unknown_2f50->Release();
        unknown_2f50 = NULL;
    }
    for (i = 0; i < 2; i++)
    {
        if (unknown_2f54[i])
        {
            unknown_2f54[i]->Release();
            unknown_2f54[i] = NULL;
        }
    }
    act_mission->RestoreSettings();
    C_Vehicle::v_sound_sector.clear();
    for (i = unknown_140.size(); i--;)
    {
        act_mission->FrmActBuff_RemoveActiveActor(unknown_140[i]);
        if (unknown_140[i]->type == ACTOR_CAR)
            unknown_140[i]->own_frame = false;
        act_mission->DelActor(unknown_140[i]);
    }
    unknown_140.clear();
    // The cars the player left in the garage
restart:
    for (i = 0; i < (int)actors.size(); i++)
    {
        if (actors[i]->type == ACTOR_CAR && ((C_car *)actors[i])->unknown_220c)
        {
            act_mission->FrmActBuff_RemoveActiveActor(actors[i]);
            act_mission->DelActor(actors[i]);
            goto restart;
        }
    }
    if (sheet)
        sheet->LinkTo(NULL, 0);
    if (unknown_365c)
        unknown_365c->LinkTo(NULL, 0);
    if (money)
        money->LinkTo(NULL, 0);
    act_mission->scene->SetWeatherSystemParam(WS_PARAM_ON, weather_on);
    act_mission->scene->SetWeatherSystemParam(WS_PARAM_COUNT, weather_count);
    for (i = unknown_3674.size(); i--;)
        delete unknown_3674[i];
    unknown_3674.clear();
    initialized = false;
    return true;
}

// PS2 name: what the mission's end releases of Init's, after Done; once per Init
// FUNCTION: GAME 0x5a4d20
void C_game::DoneIntern()
{
    if (!unknown_363d)
        return;
    unknown_363d = false;
    C_Vehicle::v_light_sector.clear();
    int i;
    for (i = shadow_temp.size(); i--;)
    {
        shadow_temp[i].model->LinkTo(NULL, 0);
        shadow_temp[i].model->Close();
        shadow_temp[i].model->Release();
        if (shadow_temp[i].name)
        {
            delete[] shadow_temp[i].name;
            shadow_temp[i].name = NULL;
        }
    }
    shadow_temp.clear();
    for (i = 10; i--;)
    {
        if (unknown_29bc[i])
            unknown_29bc[i]->Release();
    }
    for (i = 100; i--;)
    {
        S_unknown_75c &p = unknown_75c[i];
        if (p.in_collision)
            gifCollision.DynErase(&p.coll);
        if (p.frame)
        {
            p.frame->LinkTo(NULL, 0);
            p.frame->Release();
        }
    }
    for (i = 100; i--;)
    {
        S_unknown_2fd8 &p = unknown_2fd8[i];
        if (p.frame)
        {
            p.frame->LinkTo(NULL, 0);
            p.frame->Release();
        }
        p.unknown_4 = 0;
    }
    for (i = 0; i < 32; i++)
    {
        if (stones[i])
        {
            stones[i]->GameDone();
            stones[i]->Release();
        }
    }
    for (i = 0; i < 100; i++)
    {
        if (cartridges[i])
        {
            I3D_frame *frm = cartridges[i]->frame;
            cartridges[i]->Release();
            if (frm)
            {
                frm->LinkTo(NULL, 0);
                frm->Release();
            }
        }
    }
    for (i = 0; i < 30; i++)
    {
        if (cartridges_b[i])
        {
            I3D_frame *frm = cartridges_b[i]->frame;
            cartridges_b[i]->Release();
            if (frm)
            {
                frm->LinkTo(NULL, 0);
                frm->Release();
            }
        }
    }
    for (i = 0; i < 100; i++)
    {
        if (holes[i])
        {
            holes[i]->Release();
            holes[i] = NULL;
        }
    }
    for (i = hole_models.size(); i--;)
    {
        if (hole_models[i])
            hole_models[i]->Release();
    }
    hole_models.clear();
    for (i = 0; i < 3; i++)
    {
        if (C_cartridge::sounds[i])
        {
            C_cartridge::sounds[i]->Release();
            C_cartridge::sounds[i] = NULL;
        }
    }
    for (i = 0; i < 10; i++)
    {
        if (unknown_2a20[i])
        {
            unknown_2a20[i]->Release();
            unknown_2a20[i] = NULL;
        }
    }
    for (i = 0; i < 10; i++)
    {
        if (unknown_2a48[i].light)
        {
            unknown_2a48[i].light->Release();
            unknown_2a48[i].light = NULL;
        }
    }
    for (i = 0; i < 3; i++)
    {
        if (unknown_2ac0[i].sound)
        {
            unknown_2ac0[i].sound->Release();
            unknown_2ac0[i].sound = NULL;
        }
    }
    C_traffic_generator::unknown_632638 = 1.0f;
    C_traffic_generator::unknown_63263c = 1.0f;
    C_traffic_car::unknown_63d7fc = 1.0f;
    C_traffic_car::unknown_63d804 = 1.0f;
    if (sheet)
    {
        sheet->Release();
        sheet = NULL;
    }
    if (unknown_365c)
    {
        unknown_365c->Release();
        unknown_365c = NULL;
    }
    if (money)
    {
        money->Release();
        money = NULL;
    }
}


#pragma auto_inline(off)

// STUB: GAME 0x5a5070
void C_game::ChangeActState()
{
    StubLog("C_game::ChangeActState");
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x5a79a0
void C_game::RemoveTemporaryActor(C_actor *act)
{
    for (int i = unknown_130.size(); i--;)
    {
        if (unknown_130[i] == act)
            return;
    }
    act->SetActState(ACTSTATE_2);
    unknown_130.push_back(act);
}

// FUNCTION: GAME 0x5a7d40
void C_game::InternTempActDel(C_actor *act)
{
    int i;
    for (i = unknown_130.size(); i--;)
    {
        if (unknown_130[i] == act)
            return;
    }
    for (i = temporary_actors.size(); i--;)
    {
        if (temporary_actors[i] == act)
        {
            temporary_actors[i] = temporary_actors.back();
            temporary_actors.pop_back();
            break;
        }
    }
    for (i = unknown_ec.size(); i--;)
    {
        if (unknown_ec[i] == act)
        {
            unknown_ec[i] = unknown_ec.back();
            unknown_ec.pop_back();
            break;
        }
    }
    for (i = unknown_fc.size(); i--;)
    {
        if (unknown_fc[i] == act)
        {
            unknown_fc[i] = unknown_fc.back();
            unknown_fc.pop_back();
            break;
        }
    }
}

// The temporary actors but the next mission's, each taken from the game's lists, ended and released (PS2 name)
// FUNCTION: GAME 0x5a7f60
void C_game::ClearTemporaryActors()
{
    for (int i = temporary_actors.size(); i--;)
    {
        if (temporary_actors[i] == NewMissionActors[0] || temporary_actors[i] == NewMissionActors[1])
            continue;
        InvalidateActor(temporary_actors[i], false);
        temporary_actors[i]->GameDone();
        temporary_actors[i]->Release();
    }
    temporary_actors.clear();
}

// The sounds the game plays: stopped, unlinked and released (PS2 name)
// FUNCTION: GAME 0x5acf00
void C_game::ClearEffects()
{
    for (int i = sound_play.size(); i--;)
    {
        sound_play[i].sound->SetOn(false, true);
        sound_play[i].sound->LinkTo(NULL, 0);
        sound_play[i].sound->Release();
    }
    sound_play.clear();
}

// FUNCTION: GAME 0x5acfb0
int __cdecl C_game::CompareShadows(const S_shadow_item *a, const S_shadow_item *b)
{
    if (a->dist == b->dist)
        return 0;
    if (a->dist < b->dist)
        return -1;
    return 1;
}

// FUNCTION: GAME 0x5acfe0
void C_game::AddShadowModel(const char *name)
{
    S_shadow_temp t;
    t.model = (I3D_shadow *)driver->CreateFrame(FRAME_SHADOW);
    t.model->Open(name, 0, NULL, NULL);
    const char *file = strrchr(name, '\\');
    if (!file)
        file = name;
    else
        file++;
    t.name = new char[strlen(file) + 1];
    strcpy(t.name, file);
    shadow_temp.push_back(t);
}

// A frame's shadow copied from the shadow model of its name, or else of its index
// FUNCTION: GAME 0x5ad210
void C_game::SetupShadow(S_shadow_item *item)
{
    if (item->name)
    {
        for (int i = shadow_temp.size(); i--;)
        {
            if (shadow_temp[i].name && !_stricmp(shadow_temp[i].name, item->name))
            {
                item->model->Duplicate(shadow_temp[i].model);
                return;
            }
        }
    }
    else
        item->model->Duplicate(shadow_temp[item->unknown_14].model);
}

// The semaphores: the models of semafor.i3d, with their lights; the direction the model faces chooses its phase
// FUNCTION: GAME 0x5ad900
I3DENUMRET __stdcall C_game::S_hlp::cbFindSemaphores(I3D_frame *frm, uint user)
{
    if (frm->GetType() != FRAME_MODEL)
        return I3DENUMRET_OK;
    const char *name = ((I3D_model *)frm)->file_name ? ((I3D_model *)frm)->file_name : "";
    const char *file = strrchr(name, '\\');
    file = file ? file + 1 : name;
    // STRING: GAME 0x637798
    if (_stricmp(file, "semafor.i3d"))
        return I3DENUMRET_OK;
    S_semaphore s;
    // STRING: GAME 0x637794
    s.red = frm->FindChildFrame("RED", ENUMF_ALL);
    // STRING: GAME 0x63778c
    s.orange = frm->FindChildFrame("ORANGE", ENUMF_ALL);
    // STRING: GAME 0x637784
    s.green = frm->FindChildFrame("GREEN", ENUMF_ALL);
    S_vector dir = frm->GetWorldDir();
    if (fabs(dir.x) > fabs(dir.z))
        s.phase = &act_mission->game->unknown_19c;
    else
        s.phase = &act_mission->game->unknown_1a0;
    ((std::vector<S_semaphore> *)user)->push_back(s);
    return I3DENUMRET_OK;
}

// The semaphores of the mission's scene (PS2 name)
// FUNCTION: GAME 0x5add10
void C_game::LoadSemaphores()
{
    semaphores.clear();
    act_mission->scene->primary_sector->EnumFrames(S_hlp::cbFindSemaphores, (uint)&semaphores, ENUMF_MODEL, NULL);
}

// The lights all on again (PS2 name)
// FUNCTION: GAME 0x5add70
void C_game::FreeSemaphores()
{
    for (int i = semaphores.size(); i--;)
    {
        if (semaphores[i].red)
            semaphores[i].red->SetOn(true);
        if (semaphores[i].orange)
            semaphores[i].orange->SetOn(true);
        if (semaphores[i].green)
            semaphores[i].green->SetOn(true);
    }
    semaphores.clear();
}

// Each semaphore's lights for its direction's phase (PS2 name)
// FUNCTION: GAME 0x5ade20
void C_game::TickSemaphores()
{
    for (int i = semaphores.size(); i--;)
    {
        if (semaphores[i].red)
            semaphores[i].red->SetOn(semaphore_lights[*semaphores[i].phase].red);
        if (semaphores[i].orange)
            semaphores[i].orange->SetOn(semaphore_lights[*semaphores[i].phase].orange);
        if (semaphores[i].green)
            semaphores[i].green->SetOn(semaphore_lights[*semaphores[i].phase].green);
    }
}

// The areas of the city's music (PS2 name)
// FUNCTION: GAME 0x5adee0
void C_game::LoadCityMusicAreas()
{
    city_music_areas.clear();
    act_mission->scene->primary_sector->EnumFrames(S_hlp::cbCityMusic, (uint)&city_music_areas, ENUMF_AREA,
        NULL);
}

// The areas named city_music_ and more
// FUNCTION: GAME 0x5adf30
I3DENUMRET __stdcall C_game::S_hlp::cbCityMusic(I3D_frame *frm, uint user)
{
    if (frm->GetType() == FRAME_AREA)
    {
        const char *name = frm->GetName();
        // STRING: GAME 0x6377a4
        if (strlen(name) > 12 && !_strnicmp(name, "city_music_", 11))
            ((std::vector<I3D_frame *> *)user)->push_back(frm);
    }
    return I3DENUMRET_OK;
}

// What the game added to the mission (PS2 name): its actors taken out of the game's lists and deleted from the mission,
// its frames deleted from the scene (a frame's actor too, and a light from its sectors' lists), its scripts ended and
// deleted
// FUNCTION: GAME 0x5af860
void C_game::ClearDiffData()
{
    int i, j;
    for (i = unknown_2aec.size(); i--;)
    {
        for (j = unknown_ec.size(); j--;)
        {
            if (unknown_ec[j] == unknown_2aec[i])
            {
                unknown_ec[j] = unknown_ec.back();
                unknown_ec.pop_back();
                break;
            }
        }
        for (j = unknown_fc.size(); j--;)
        {
            if (unknown_fc[j] == unknown_2aec[i])
            {
                unknown_fc[j] = unknown_fc.back();
                unknown_fc.pop_back();
                break;
            }
        }
        for (j = temporary_actors.size(); j--;)
        {
            if (temporary_actors[j] == unknown_2aec[i])
            {
                temporary_actors[j] = temporary_actors.back();
                temporary_actors.pop_back();
                break;
            }
        }
        InvalidateActor(unknown_2aec[i], false);
        unknown_2aec[i]->GameDone();
        act_mission->DelActor(unknown_2aec[i]);
    }
    unknown_2aec.clear();
    for (i = unknown_2adc.size(); i--;)
    {
        C_actor **act = GetFrameActor(unknown_2adc[i]);
        if (act && *act)
        {
            for (j = unknown_ec.size(); j--;)
            {
                if (unknown_ec[j] == *act)
                {
                    unknown_ec[j] = unknown_ec.back();
                    unknown_ec.pop_back();
                    break;
                }
            }
            for (j = unknown_fc.size(); j--;)
            {
                if (unknown_fc[j] == *act)
                {
                    unknown_fc[j] = unknown_fc.back();
                    unknown_fc.pop_back();
                    break;
                }
            }
            C_actor *temporary = NULL;
            for (j = temporary_actors.size(); j--;)
            {
                if (temporary_actors[j] == *act)
                {
                    temporary = temporary_actors[j];
                    temporary_actors[j] = temporary_actors.back();
                    temporary_actors.pop_back();
                    break;
                }
            }
            C_actor *a = *act;
            InvalidateActor(a, false);
            a->GameDone();
            if (!act_mission->DelActor(a) && temporary)
                temporary->Release();
        }
        S_hlp::cbFreeFramePlacement(unknown_2adc[i], frame_placements);
        I3D_frame *frm = unknown_2adc[i];
        if (frm->GetType() == FRAME_LIGHT)
        {
            I3D_light *lt = (I3D_light *)frm;
            I3D_sector *const *sectors = &lt->sectors[0];
            for (j = lt->sectors.size(); j--;)
                sectors[j]->DeleteLight(lt);
        }
        act_mission->scene->DeleteFrame(unknown_2adc[i]);
    }
    unknown_2adc.clear();
    for (i = unknown_2afc.size(); i--;)
    {
        unknown_2afc[i]->Done();
        delete unknown_2afc[i];
    }
    unknown_2afc.clear();
}

// FUNCTION: GAME 0x5afca0
void C_game::S_hlp::cbFreeFramePlacement(I3D_frame *frm, std::vector<S_frame_placement> &list)
{
    for (int i = list.size(); i--;)
    {
        if (list[i].frame == frm)
        {
            list[i] = list.back();
            list.pop_back();
            break;
        }
    }
    for (int j = frm->NumChildren(); j--;)
        cbFreeFramePlacement(frm->GetChild(j), list);
}

// Runs the game's own script of that name (its CallSubroutine): false without one
// FUNCTION: GAME 0x5afd50
bool C_game::GlobalProgramRun(const char *name)
{
    for (int i = unknown_2afc.size(); i--;)
    {
        if (!_stricmp(unknown_2afc[i]->name, name))
        {
            unknown_2afc[i]->CallSubroutine();
            return true;
        }
    }
    return false;
}

#pragma auto_inline(off)

// STUB: GAME 0x5afe40
void C_game::InvalidateActor(C_actor *act, bool)
{
    StubLog("C_game::InvalidateActor");
}

#pragma auto_inline(on)

// A frame going away: its placements and its subtree's taken from the list (PS2 name)
// FUNCTION: GAME 0x5b0210
void C_game::InvalidateFrame(I3D_frame *frm)
{
    if (frm)
        S_hlp::cbDelFrame(frm, frame_placements);
}

// FUNCTION: GAME 0x5b0230
void C_game::S_hlp::cbDelFrame(I3D_frame *frm, std::vector<S_frame_placement> &list)
{
    for (int i = list.size(); i--;)
    {
        if (list[i].frame == frm)
        {
            list[i] = list.back();
            list.pop_back();
        }
    }
    for (I3D_frame *child = frm->m_pChild; child; child = child->m_pNext)
    {
        if (child->m_pChild)
            cbDelFrame(child, list);
    }
}

// FUNCTION: GAME 0x5b1da0
void C_game::InventoryStackClear()
{
    for (int i = inventory_stack.size(); i--; )
        delete inventory_stack[i];
    inventory_stack.clear();
}

// FUNCTION: GAME 0x5b2320
void C_game::FloatStackClear()
{
    float_stack.clear();
}

// FUNCTION: GAME 0x5b2590
void C_game::CharacterStackClear()
{
    character_stack.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x5b2ee0
void C_game::BoxPlacementDel(I3D_frame *frm)
{
    StubLog("C_game::BoxPlacementDel");
}

#pragma auto_inline(on)

// Deletes the lightness entries (PS2 name)
// FUNCTION: GAME 0x5b5780
void C_game::LightnessClear()
{
    for (int i = lightness.size(); i--;)
        delete lightness[i];
    lightness.clear();
}

// The videos closed and released (PS2 name), and the lists emptied
// FUNCTION: GAME 0x5b63b0
void C_game::DestroyIShowAll()
{
    for (int i = i_shows.size(); i--;)
    {
        if (i_shows[i].show)
        {
            delete[] i_shows[i].unknown_c;
            i_shows[i].show->Close();
            i_shows[i].show->Release();
        }
    }
    i_shows.clear();
    i_show_fades.clear();
    unknown_2fc4.clear();
}

// From the last video to the first
// FUNCTION: GAME 0x5b6600
void C_game::IShowSetVolume()
{
    for (int i = i_shows.size(); i--; )
        i_shows[i].show->SetVolume(mafiaLoadSave.settings.music_volume * i_shows[i].volume);
}

// The road blocks, the dummies ROADBLOCK* (PS2 name)
// FUNCTION: GAME 0x5b69d0
void C_game::LoadRoadBlocks()
{
    road_blocks.clear();
    act_mission->scene->primary_sector->EnumFrames(S_hlp::cbRoadBlock, (uint)&road_blocks, ENUMF_WILDMASK | ENUMF_DUMMY,
        // STRING: GAME 0x6377c0
        "ROADBLOCK*");
}

// A road block at the roads' node nearest to the dummy, where there is one
// FUNCTION: GAME 0x5b6a50
I3DENUMRET __stdcall C_game::S_hlp::cbRoadBlock(I3D_frame *frm, uint user)
{
    S_RoadBlock rb;
    rb.unknown_2 = false;
    rb.node = act_mission->roads->GetNearNode(frm->GetWorldPos());
    if (rb.node != 0xffff)
        ((std::vector<S_RoadBlock> *)user)->push_back(rb);
    return I3DENUMRET_OK;
}

// The taxis' stops, the dummies taxi_point* (PS2 name)
// FUNCTION: GAME 0x5b6b10
void C_game::LoadTaxiPoints()
{
    taxi_points.clear();
    act_mission->scene->primary_sector->EnumFrames(S_hlp::cbFindTaxiPoints, (uint)&taxi_points,
        ENUMF_WILDMASK | ENUMF_DUMMY,
        // STRING: GAME 0x6377cc
        "taxi_point*");
}

// FUNCTION: GAME 0x5b6b60
I3DENUMRET __stdcall C_game::S_hlp::cbFindTaxiPoints(I3D_frame *frm, uint user)
{
    ((std::vector<I3D_frame *> *)user)->push_back(frm);
    return I3DENUMRET_OK;
}

// The telephones, the models of alarm01.i3d and PHbox01.i3d: the scene's, and the base blocks' while those are open
// (PS2 name)
// FUNCTION: GAME 0x5b6e00
void C_game::LoadTelephones()
{
    telephones.clear();
    act_mission->scene->primary_sector->EnumFrames(S_hlp::cbTelephones, (uint)&telephones, ENUMF_MODEL, NULL);
    if (!base_cache.IsOpen())
        return;
    ushort alarm = 0xffff, box = 0xffff;
    int i;
    for (i = base_cache.model_sets.size(); i--;)
    {
        // STRING: GAME 0x6377e4
        if (!_stricmp(base_cache.model_sets[i].name, "alarm01.i3d"))
            alarm = i;
        // STRING: GAME 0x6377d8
        if (!_stricmp(base_cache.model_sets[i].name, "PHbox01.i3d"))
            box = i;
    }
    for (i = base_cache.blocks.size(); i--;)
    {
        for (int j = base_cache.blocks[i].models.size(); j--;)
        {
            if (base_cache.blocks[i].models[j].model == alarm)
            {
                S_Telephone t;
                t.frame = NULL;
                t.cache_model = &base_cache.blocks[i].models[j];
                t.phone_box = false;
                telephones.push_back(t);
            }
            else if (base_cache.blocks[i].models[j].model == box)
            {
                S_Telephone t;
                t.frame = NULL;
                t.cache_model = &base_cache.blocks[i].models[j];
                t.phone_box = true;
                telephones.push_back(t);
            }
        }
    }
}

// FUNCTION: GAME 0x5b71b0
I3DENUMRET __stdcall C_game::S_hlp::cbTelephones(I3D_frame *frm, uint user)
{
    const char *name = ((I3D_model *)frm)->file_name ? ((I3D_model *)frm)->file_name : "";
    const char *file = strrchr(name, '\\');
    file = file ? file + 1 : name;
    if (!_stricmp(file, "alarm01.i3d"))
    {
        S_Telephone t;
        t.frame = frm;
        t.cache_model = NULL;
        t.phone_box = false;
        ((std::vector<S_Telephone> *)user)->push_back(t);
    }
    else if (!_stricmp(file, "PHbox01.i3d"))
    {
        S_Telephone t;
        t.frame = frm;
        t.cache_model = NULL;
        t.phone_box = true;
        ((std::vector<S_Telephone> *)user)->push_back(t);
    }
    return I3DENUMRET_OK;
}

// The base blocks' model sets of the traffic signs, dopravni1B.I3D to dopravni8B.I3D (PS2 name)
// FUNCTION: GAME 0x5b7860
void C_game::CreateSignalMarkIDX()
{
    unknown_1e4.clear();
    if (!base_cache.IsOpen())
        return;
    for (ushort i = base_cache.model_sets.size(); i--;)
    {
        // STRING: GAME 0x637860
        if (!_stricmp(base_cache.model_sets[i].name, "dopravni1B.I3D"))
            unknown_1e4.push_back(i);
        // STRING: GAME 0x637850
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni2B.I3D"))
            unknown_1e4.push_back(i);
        // STRING: GAME 0x637840
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni3B.I3D"))
            unknown_1e4.push_back(i);
        // STRING: GAME 0x637830
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni4B.I3D"))
            unknown_1e4.push_back(i);
        // STRING: GAME 0x637820
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni5B.I3D"))
            unknown_1e4.push_back(i);
        // STRING: GAME 0x637810
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni6B.I3D"))
            unknown_1e4.push_back(i);
        // STRING: GAME 0x637800
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni7B.I3D"))
            unknown_1e4.push_back(i);
        // STRING: GAME 0x6377f0
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni8B.I3D"))
            unknown_1e4.push_back(i);
    }
}

// FUNCTION: GAME 0x5b7ed0
void C_game::EntityBlock(bool block, bool unknown)
{
    int i;
    for (i = unknown_ec.size(); i--;)
    {
        if (unknown_ec[i]->type == ACTOR_ENTITY)
        {
            if (block)
                ((C_entity *)unknown_ec[i])->SedniNaPrdelASledujKino(unknown);
            else
                ((C_entity *)unknown_ec[i])->VzbudSeUzSouTitulky();
        }
    }
    for (i = unknown_fc.size(); i--;)
    {
        if (unknown_fc[i]->type == ACTOR_ENTITY)
        {
            if (block)
                ((C_entity *)unknown_fc[i])->SedniNaPrdelASledujKino(unknown);
            else
                ((C_entity *)unknown_fc[i])->VzbudSeUzSouTitulky();
        }
    }
}

// Every actor's sounds off (PS2 name)
// FUNCTION: GAME 0x5b83f0
void C_game::StopAllSounds()
{
    for (int i = C_actor::global_actors.size(); i--;)
        C_actor::global_actors[i]->SoundsChange(0);
}

// The areas where the enemies scream (PS2 name)
// FUNCTION: GAME 0x5b90b0
void C_game::LoadScreamAreas()
{
    unknown_3618.clear();
    act_mission->scene->primary_sector->EnumFrames(S_hlp::cbScreamArea, (uint)&unknown_3618, ENUMF_AREA, NULL);
}

// The areas named enm_scrm_ and more, each with the hexadecimal number in its name's parentheses, 0 without one
// FUNCTION: GAME 0x5b9110
I3DENUMRET __stdcall C_game::S_hlp::cbScreamArea(I3D_frame *frm, uint user)
{
    if (frm->GetType() == FRAME_AREA)
    {
        const char *name = frm->GetName();
        // STRING: GAME 0x637870
        if (strlen(name) > 9 && !_strnicmp(name, "enm_scrm_", 9))
        {
            S_unknown_3618 a;
            a.area = frm;
            a.id = 0;
            char buf[256];
            strcpy(buf, frm->GetName());
            char *s = strchr(buf, '(');
            if (s)
            {
                s++;
                char *e = strchr(s, ')');
                if (e)
                    *e = 0;
                a.id = (ushort)strtoul(s, &e, 16);
            }
            ((std::vector<S_unknown_3618> *)user)->push_back(a);
        }
    }
    return I3DENUMRET_OK;
}

// The weather (PS2 name: Czech for "deal with the weather"). With read, the scene's parameter param (WS_PARAM_ON or
// WS_PARAM_COUNT) is taken as the mission's own first; then, while the mission has weather, its count by the "In Game
// Effects" option: a twentieth at Low, an eighth at Medium, all of it at High (the parameters' names are ours)
// FUNCTION: GAME 0x5b9540
void C_game::OsefujPocasi(bool read, uint param)
{
    if (!initialized)
        return;
    if (read)
    {
        switch (param)
        {
        case WS_PARAM_ON:
            weather_on = act_mission->scene->GetWeatherSystemParam(WS_PARAM_ON) != 0;
            break;
        case WS_PARAM_COUNT:
            weather_count = act_mission->scene->GetWeatherSystemParam(WS_PARAM_COUNT);
            break;
        }
        OsefujPocasi(false, (uint)-1);
        return;
    }
    if (!weather_on)
        return;
    switch (act_mission->options.game_effects)
    {
    case 0:
        act_mission->scene->SetWeatherSystemParam(WS_PARAM_ON, 1);
        act_mission->scene->SetWeatherSystemParam(WS_PARAM_COUNT, weather_count / 20);
        break;
    case 1:
        act_mission->scene->SetWeatherSystemParam(WS_PARAM_ON, 1);
        act_mission->scene->SetWeatherSystemParam(WS_PARAM_COUNT, weather_count / 8);
        break;
    case 2:
        act_mission->scene->SetWeatherSystemParam(WS_PARAM_ON, 1);
        act_mission->scene->SetWeatherSystemParam(WS_PARAM_COUNT, weather_count);
        break;
    }
}

// The hydrants' water: the particles destroyed, the frames released (PS2 name)
// FUNCTION: GAME 0x5b9ef0
void C_game::DelAllHydrants()
{
    for (int i = hydrants.size(); i--;)
    {
        act_mission->effects->DestroyParticle(hydrants[i].particle);
        if (hydrants[i].frame)
        {
            hydrants[i].frame->SetOn(false);
            hydrants[i].frame->Release();
        }
    }
    hydrants.clear();
}

// The HUD's bonus counter shows the score, or nothing (PS2 name)
// FUNCTION: GAME 0x5b9fa0
void C_game::ScoreSetOn(bool on)
{
    score_on = on;
    if (on)
        mafiaIndicators.BonusSetCount(score);
    else
        mafiaIndicators.BonusSetCount(0x7fffffff);
}

// PS2 name
// FUNCTION: GAME 0x5ba310
void C_game::ClearDeathHumanFade()
{
    for (int i = death_human_fades.size(); i--;)
        RemoveDeathHumanFade(i, false);
    death_human_fades.clear();
    unknown_3684 = false;
}

#pragma auto_inline(off)

// STUB: GAME 0x5baa40
void C_game::RemoveDeathHumanFade(int index, bool)
{
    StubLog("C_game::RemoveDeathHumanFade");
}

#pragma auto_inline(on)

// The roads out of the city, closed by boxes of collision while the city's base blocks are open: nine, and four more in
// the mission mise16-mesto (PS2's is C_mission's and another function)
// FUNCTION: GAME 0x5bab80
void C_game::InterCity_GameInit()
{
    unknown_3698 = NULL;
    unknown_369c = NULL;
    unknown_36a0 = NULL;
    unknown_36a8 = NULL;
    unknown_36a4 = 0;
    if (!base_cache.IsOpen())
        return;
    // STRING: GAME 0x63787c
    bool city = !_stricmp(act_mission->GetMissionName(), "mise16-mesto");
    unknown_36a4 = city ? 13 : 9;
    unknown_3698 = new tDynamicCollObject *[unknown_36a4];
    unknown_369c = new S_matrix[unknown_36a4];
    unknown_36a0 = new S_matrix[unknown_36a4];
    unknown_36a8 = new I3D_frame *[unknown_36a4];
    int n = 0;
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(561.601f, -0.364f, 13.733f), S_vector(-0.061f, -0.991f, -0.12f), 65.83f, S_vector(22.15f, 2.955f, 0.358f));
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(-764.324f, 5.911f, 466.544f), S_vector(0.0f, -1.0f, 0.0f), 179.445f, S_vector(1.604f, 3.066f, 0.347f));
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(-775.669f, 5.664f, 466.665f), S_vector(0.0f, -1.0f, 0.0f), 177.698f, S_vector(3.134f, 3.642f, 0.158f));
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(-780.123f, 5.041f, 466.292f), S_vector(0.0f, -1.0f, 0.0f), 179.445f, S_vector(3.138f, 3.066f, 0.347f));
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(-742.653f, 5.641f, 467.539f), S_vector(0.007f, 0.969f, 0.246f), 179.66f, S_vector(1.546f, 1.0f, 2.371f));
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(-735.396f, 5.641f, 462.545f), S_vector(0.245f, 0.942f, 0.23f), 94.16f, S_vector(1.546f, 1.0f, 2.371f));
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(-735.561f, 5.715f, 450.99f), S_vector(0.185f, 0.968f, 0.17f), 92.645f, S_vector(2.127f, 1.0f, 2.371f));
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(-735.349f, 6.378f, 450.0f), S_vector(0.185f, 0.968f, 0.17f), 92.645f, S_vector(0.28f, 1.0f, 2.371f));
    AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
        S_vector(-735.319f, 6.378f, 451.956f), S_vector(0.185f, 0.968f, 0.17f), 92.645f, S_vector(0.28f, 1.0f, 2.371f));
    if (city)
    {
        AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
            S_vector(-338.627f, 14.89f, 188.401f), S_vector(0.0f, -1.0f, 0.0f), 90.0f, S_vector(1.036f, 5.0f, 12.43f));
        AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
            S_vector(-351.433f, 13.947f, 188.491f), S_vector(0.0f, -1.0f, 0.0f), 90.0f, S_vector(1.0f, 5.0f, 13.34f));
        AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
            S_vector(-351.433f, 14.919f, 153.997f), S_vector(0.0f, -1.0f, 0.0f), 90.0f, S_vector(1.0f, 5.0f, 13.34f));
        AddInterCityBox(unknown_3698, unknown_369c, unknown_36a0, &n, unknown_36a8,
            S_vector(-338.627f, 15.861f, 153.906f), S_vector(0.0f, -1.0f, 0.0f), 90.0f, S_vector(1.036f, 5.0f, 12.43f));
    }
}

#pragma auto_inline(off)

// A box of collision at pos, turned by angle degrees around axis and scaled by scale, as a dummy frame of the unit box
// and a collision object of its matrix and inverse, added to the collisions as the n-th (the name is ours)
// STUB: GAME 0x5bb570
static void AddInterCityBox(tDynamicCollObject **colls, S_matrix *matrices, S_matrix *inv_matrices, int *n,
    I3D_frame **frames, S_vector pos, S_vector axis, float angle, S_vector scale)
{
    StubLog("AddInterCityBox");
}

#pragma auto_inline(on)

// What InterCity_GameInit created: the collisions taken out and deleted, the frames released, the arrays deleted (PS2
// name, PS2's C_mission's)
// FUNCTION: GAME 0x5bbb20
void C_game::InterCity_GameDoneInGameDone()
{
    if (!unknown_3698)
        return;
    for (int i = unknown_36a4; i--;)
    {
        if (unknown_3698[i])
        {
            gifCollision.DynErase(unknown_3698[i]);
            delete unknown_3698[i];
        }
        if (unknown_36a8[i])
            unknown_36a8[i]->Release();
    }
    unknown_36a4 = 0;
    if (unknown_3698)
        delete[] unknown_3698;
    unknown_3698 = NULL;
    if (unknown_369c)
        delete[] unknown_369c;
    unknown_369c = NULL;
    if (unknown_36a0)
        delete[] unknown_36a0;
    unknown_36a0 = NULL;
    if (unknown_36a8)
        delete[] unknown_36a8;
    unknown_36a8 = NULL;
}
