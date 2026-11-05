// The game of the loaded mission (PS2 game.cpp; the PC's file is the one at 0x5a0760-0x5bcc0c, docs/game-files.md).
// Written so far: Init, which main calls, and Done and DoneIntern, which C_mission::Close calls, with the file's static
// helpers they call and the functions of the game they call that the main menu's run reaches (the shadows' models,
// ClearDiffData, the semaphores, the city's music areas, the road blocks, the taxis' stops, the telephones, the traffic
// signs, the scream areas, the weather, the inter-city boxes, GlobalProgramRun, and the Clear functions); the stacks'
// Clear functions, which the destructor calls (the constructor and the destructor are game.h's), IShowSetVolume, which
// the profiles call, ScoreSetOn, LightnessClear and EntityBlock, which C_recorder::Close calls, and ScoreSet, which the
// scripts call; Tick, which C_mission::Tick runs every frame of the main menu, with PS2's functions it inlines (its
// TickEffects, ShootLightTick, LightCacheTick, TickHydrant, TickCityMusic and TickDriverMan, inline here, defined before
// it), TickShoot, the shots in flight, and DoubleKiss, the guns on the player's car (dead code on the PC), which Tick
// calls every frame, CreateIShow, OpenIShow, GetIShow, IShowStoreVolume and IShowLoop, the streams the city's music and
// the scripts play, CityMusicOff, which the scripts call, and ChangeActState, which Init and Tick call once an actor's
// state has changed. Stubs: what the run doesn't reach
// (PrepareBaseBlocks and the inter-city boxes' helper, both behind the base blocks, which stay closed while
// C_cache_base_block::Open is a stub; InvalidateActor, BoxPlacementDel, RemoveDeathHumanFade),
// the other functions the scripts call (C_program::Process, the streams' others), what DoubleKiss calls (NewShoot,
// NewCartidge), and what Tick calls out of line when their lists or flags are set (FindActorsVect,
// TickDeadHumanForTick, DeathActorProcess, _RemoveTemporaryActor, ItemBlurTick, TickScreamAreas, TickCarSoundManager
// and LightnessTick).
// Open: the consoles' globals (0x647a74, 0x647ab0) are here for now; Init, Done, DoneIntern and Tick aren't matched yet.
// Tick's remaining differences are mostly registers and the inline budget: the original calls TickEffects' size() and
// the second clear()'s std::copy out of line, but inlines the second clear()'s erase and the city music's i_shows.size().
// The speed (player_speed) is the Magnitude of a named difference, whose squares the original sums x, z, y (a nested
// Square() most likely, which the engine's Magnitude doesn't have), and the city music reads the area's two letters
// through a pointer it advances (add eax, 11), where ours folds the offset. TickShoot's remaining differences are
// registers and its sorted frame (frm at 0xc8, the original's 0x2c), and the order of the x87 loads in two Normalize
// copies and in the hole's position.

#include "mission/game.h"
#include "mission/Mission.h"
#include "mission/C_recorder.h"
#include "mission/C_effects.h"
#include "mission/C_program.h"
#include "mission/m_cache.h"
#include "actors/C_actor.h"
#include "actors/C_animations.h"
#include "actors/C_cartridge.h"
#include "actors/C_crash_obj.h"
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
#include "g_material.h"
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
    if (!shadow_timer.Tick(time))
        return;
    if (!shadow_temp.size())
        return;
    I3D_frame *own = NULL;
    std::vector<S_shadow_item> *list;
    bool humans = human_shadows_next;
    if (humans)
    {
        list = &shadow_items;
        if (player)
            own = player->frame;
    }
    else
    {
        list = &car_shadow_items;
        if (player && player->car)
            own = player->car->frame;
    }
    human_shadows_next = !humans;
    if (!list->size())
        return;
    float ahead = 3.0f;
    I3D_frame *cam = act_mission->scene->current_camera;
    S_vector pos = cam->GetWorldPos() + cam->GetWorldDir() * ahead;
    int i;
    for (i = list->size(); i--;)
    {
        if ((*list)[i].frame == own)
            (*list)[i].dist = 0.0f;
        else
            (*list)[i].dist = ((*list)[i].frame->GetWorldPos() - pos).Square();
    }
    qsort(&(*list)[0], list->size(), sizeof(S_shadow_item),
        (int (__cdecl *)(const void *, const void *))CompareShadows);
    for (i = list->size(); i--;)
    {
        if (i < act_mission->options.GetShadows())
        {
            if (!(*list)[i].on)
            {
                SetupShadow(&(*list)[i]);
                (*list)[i].model->LinkTo((*list)[i].frame, 0);
                (*list)[i].frame->Update();
                (*list)[i].on = true;
                if ((*list)[i].replaced_frame)
                    (*list)[i].replaced_frame->SetOn(false);
            }
        }
        else if ((*list)[i].on)
        {
            (*list)[i].model->LinkTo(NULL, 0);
            (*list)[i].model->Close();
            (*list)[i].frame->Update();
            (*list)[i].on = false;
            if ((*list)[i].replaced_frame)
                (*list)[i].replaced_frame->SetOn(true);
        }
    }
}

// The traffic generators' choice (PS2 name): at each period the one whose range the camera is in, else the nearest of
// those without a range, becomes the active one
inline void C_game::SetActiveTrafficP(uint time)
{
    if (!traffic_timer.Tick(time))
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
        float dist = (frm->GetWorldPos() - pos).Magnitude();
        if (g->no_range)
        {
            if (!in_range && dist < best)
            {
                best = dist;
                gen = g;
            }
        }
        else if (dist < g->range)
        {
            gen = g;
            in_range = true;
        }
    }
    if (gen)
    {
        gen->SetActState(ACTSTATE_ACTIVE);
        gen->active = true;
    }
}

// The animations' tracks, loaded before they are needed (PS2 name): each animation's .tck, a ? in the name standing for
// both sides
inline void C_game::PreloadAnims()
{
    timeGetTime();
    for (S_anim_info *a = _anim_info_; a->id != 0xffff; a++)
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
    mission_passed = false;
    car_sounds = 0;
    weather_on = act_mission->scene->GetWeatherSystemParam(WS_PARAM_ON) != 0;
    weather_count = act_mission->scene->GetWeatherSystemParam(WS_PARAM_COUNT);
    weather_buffer = -1;
    if (isound)
    {
        isound->SetVolume(0.0f);
        act_mission->scene->Render();
        act_mission->restore_volume = true;
    }
    stop_sounds.clear();
    paused_i_shows.clear();
    free_shadow_names.clear();
    score_on = false;
    ScoreSetOn(false);
    score = 0;
    camera_rot_repair = false;
    heal_cheat = true;
    shvestky.unknown_ac = true;
    death_fade_time = 7000;
    actor_lightness = false;
    unknown_140.clear();
    AIManager.police.ZacatekSmeny();
    AIManager.groups.Init();
    AIManager.follow.Init();
    AIManager.fight.Init();
    _show_progress_(0.0f);
    SetPlayerNaviPoint(NULL);
    C_player::can_car_damage = false;
    if (intern_load)
    {
        C_Vehicle::v_light_sector.clear();
        C_Vehicle::v_sound_sector.clear();
        S_hlp::cbEnumSectors(act_mission->scene->primary_sector);
    }
    C_public_physics::unknown_63d78c.clear();
    strcpy(cheat_keys, "");
    cheats = 0;
    game_done = false;
    mission_end = 0;
    growing_projectors = 0;
    game_time = 0;
    game_frames = 0;
    init_frames = 0;
    night_mission = false;
    timeout_program = NULL;
    player_fire_program = NULL;
    player_horn_program = NULL;
    player_wfall_program = NULL;
    race6_state = RACE6_STATE_NONE;
    death_actors.clear();
    light_cache.clear();
    light_cache_time = -1;
    i_shows.clear();
    i_show_fades.clear();
    next_i_show_id = 0;
    next_sound_number = 0;
    sounds_fading = 0;
    sound_play_timer.time = 0;
    sound_play_timer.period = 813;
    enemy_talks.clear();
    find_actors.clear();
    anim_preload = true;
    film_music_streams.clear();
    npc_kill_events.clear();
    C_actor::posscoll_frame.clear();
    shvestky.GameInit();
    C_traffic_generator::EnableTaxiDriver(false);
    C_traffic_generator::PlayerInTaxi(NULL);
    C_traffic_generator::global_valid_scream = true;
    sound_arena.Init();
    LightnessClear();
    lightness_next = 0;
    invisible_actors.clear();
    mafiaIndicators.RadarClear();
    act_mission->GlobalProgramGameInit();
    // STRING: GAME 0x6376e0
    act_mission->GlobalProgramRun("GameInitStart");
    mafiaIndicators.GameInit();
    boxes.clear();
    box_dests.clear();
    box_placement_temp.clear();
    act_mission->effects->LoadStaticParticles();
    act_mission->effects->Mission_Start();
    act_mission->web_path->DeRiciConns();
    ff_manager.GameInit();
    if (ff_manager.human_effects)
        C_human::InitFFEffects();
    if (ff_manager.car_effects)
        C_car::InitFFEffects();
    diff_frames.clear();
    diff_actors.clear();
    clear_diff_data = false;
    C_crash_obj::count = 0;
    blood_manager.GameInit();
    frame_placements.clear();
    hidden_frames.clear();
    created_frames.clear();
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
    if (intern_load)
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
    if (intern_load && anim_preload)
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
    car_shadow_items.clear();
    _show_progress_(.1f);

    // The player: the last of the players that isn't a cutscene's
    std::vector<C_actor *> &actors = act_mission->actors;
    player = NULL;
    int i;
    for (i = actors.size(); i--;)
    {
        if (actors[i]->type == ACTOR_PLAYER && !(actors[i]->act_flags & ACTF_CUTSCENE))
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
    player_pos = S_vector(0.0f, 0.0f, 0.0f);
    if (player)
    {
        I3D_frame *frm = player->frame;
        if (frm)
            player_pos = frm->GetWorldPos();
    }
    else if (cam)
        player_pos = cam->GetWorldPos();
    player_speed = 0;
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
    camera_human = player;
    don_ocas = new C_don_ocas;
    C_traffic_generator::SetVisible(true);
    act_state_changed = false;
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
    if (intern_load)
    {
        memset(holes, 0, sizeof(holes));
        memset(scratches, 0, sizeof(scratches));
    }
    for (i = 0; i < (int)actors.size(); i++)
    {
        if (act_mission->FrmActBuff_IsActiveActor(actors[i]))
            actors[i]->SetActState(ACTSTATE_ACTIVE);
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
    active_actors.clear();
    inactive_actors.clear();
    dead_humans.clear();
    for (i = actors.size(); i--;)
        active_actors.push_back(actors[i]);
    if (act_state_changed)
        ChangeActState();
    InterCity_GameInit();
    traffic_timer.time = 0;
    traffic_timer.period = 6000;
    SetActiveTrafficP(traffic_timer.period + 1);
    fires.clear();
    camera.Reset();
    camera.SetCamera(act_mission->scene->current_camera);
    camera.SetPlayer(player);
    if (anim_preload)
        PreloadAnims();
    C_dog::LoadTCK();
    sledovacka.Init();
    glass.Init(5);
    debris.Init(10.0f);
    game_failed = false;
    compass.Activate(NULL);
    removed_actors.clear();
    semaphore_phase_x = 0;
    semaphore_phase_z = 5;
    LoadSemaphores();
    TickSemaphores();
    semaphore_timer.time = 0;
    semaphore_timer.period = 3000;
    LoadRoadBlocks();
    LoadTelephones();
    CreateSignalMarkIDX();
    LoadTaxiPoints();
    taxi_passenger = NULL;
    LoadCityMusicAreas();
    city_music_area = 0;
    city_music_time = -1;
    city_music = -1;
    city_music_old = -1;
    city_music_paused = false;
    LoadScreamAreas();
    scream_areas_time = -1;
    camera_scream_areas = 1;
    shadow_timer.time = 0;
    shadow_timer.period = 823;
    human_shadows_next = true;
    TickShadows(shadow_timer.period + 1);
    _show_progress_(.7f);

    if (intern_load)
    {
        // The blood's and the dirt's textures, each with its alpha (the HUD's literals for the first)
        if (GetIGraph()->CreateITexture(
                "2blood.bmp",
                "2blood+.bmp", 4, &track_textures[0]))
            track_textures[0] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x6372b8
                "2flek.bmp",
                // STRING: GAME 0x6372c4
                "2flek+.bmp", 4, &track_textures[1]))
            track_textures[1] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x63729c
                "2dirbet.bmp",
                // STRING: GAME 0x6372a8
                "2dirbet+.bmp", 4, &track_textures[2]))
            track_textures[2] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637280
                "2dirdre.bmp",
                // STRING: GAME 0x63728c
                "2dirdre+.bmp", 4, &track_textures[3]))
            track_textures[3] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637264
                "2dirple.bmp",
                // STRING: GAME 0x637270
                "2dirple+.bmp", 4, &track_textures[4]))
            track_textures[4] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637254
                "2dirsklo.bmp",
                "2dirsklo.bmp", 4, &track_textures[5]))
            track_textures[5] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637230
                "2bfootprint.bmp",
                // STRING: GAME 0x637240
                "2bfootprint+.bmp", 4, &track_textures[6]))
            track_textures[6] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x637214
                "2blood3.bmp",
                // STRING: GAME 0x637220
                "2blood3+.bmp", 4, &track_textures[7]))
            track_textures[7] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x6371f4
                "2dogfoot.bmp",
                // STRING: GAME 0x637204
                "2dogfoot+.bmp", 4, &track_textures[8]))
            track_textures[8] = NULL;
        if (GetIGraph()->CreateITexture(
                // STRING: GAME 0x6371e8
                "skrob.tga", NULL, 2, &track_textures[9]))
            track_textures[9] = NULL;
    }
    _show_progress_(.75f);

    if (intern_load)
    {
        // The projectors of the blood and the holes
        track_projector_next = 0;
        for (i = 100; i--;)
        {
            S_track_projector &p = track_projectors[i];
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
            S_scratch &p = scratches[i];
            p.car = NULL;
            p.car_frame = NULL;
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
        if (anim_preload)
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
        if (anim_preload)
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
        if (anim_preload)
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
            shoot_sounds[i] = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        for (i = 0; i < 10; i++)
        {
            S_shoot_light &l = shoot_lights[i];
            l.time = -1;
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
            S_delayed_sound &s = delayed_sounds[i];
            s.time = -1;
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
    mafiaIndicators.SetFlags(4);
    if (player)
    {
        if (player->properties[HUMAN_PROPERTY_ENERGY] > player->max_properties[HUMAN_PROPERTY_ENERGY])
            player->properties[HUMAN_PROPERTY_ENERGY] = player->max_properties[HUMAN_PROPERTY_ENERGY];
        mafiaIndicators.PlayerSetLives(
            (int)(player->properties[HUMAN_PROPERTY_ENERGY] / player->max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f));
    }
    else
        mafiaIndicators.PlayerSetLives(100);
    mafiaIndicators.PlayerSetAccuracy(1.0f);
    if (player)
        ((C_player *)player)->UpdateControls();
    camera.SetModeChanged();
    if (CRacing && anim_preload)
        CRacing->GameInit();
    mafiaLoadSave.GarageGameInit();
    _show_progress_(.9f);
    SetWBuffer(true);
    // STRING: GAME 0x637190
    act_mission->GlobalProgramRun("GameInitEnd");
    if (act_state_changed)
        ChangeActState();
    act_mission->SetSkipFrames(10);
    kiss_car = NULL;
    kiss_target = NULL;
    kiss_frame = NULL;
    unknown_2f54[1] = NULL;
    unknown_2f54[0] = NULL;
    act_mission->UpdateSettings();
    _show_progress_(1.0f);
    if (mafiaLoadSave.progress)
    {
        delete mafiaLoadSave.progress;
        mafiaLoadSave.progress = NULL;
    }
    if (!anim_preload && isound)
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
        ((C_player *)player)->fade_alpha = 1.0f;
        ((C_player *)player)->fade_target = 1.0f;
        ((C_player *)player)->SetAlpha(1.0f);
    }
    death_human_fades.clear();
    item_blur = false;
    act_mission->FrmActBuff_ClearAll();
    igraph->ResetRenderProps();
    igraph->Clear(0, 1.0f, 3);
    igraph->Present();
    intern_load = false;
    intern_loaded = true;
    // The bank's mission: the Duesenberg (deuseJco) the player came in stays put
    // STRING: GAME 0x637180
    if (mafiaLoadSave.SaveGameGetSelectedNumber() == 0x208 &&!_stricmp(act_mission->GetMissionName(), "MISE19-BANKA"))
    {
        std::vector<C_actor *> &global = C_actor::GetGlobalActors();
        for (i = global.size(); i--;)
        {
            if (global[i]->GetType() == ACTOR_CAR && global[i]->GetFrame() &&
                !strcmp(global[i]->GetFrame()->GetName(),
                    // STRING: GAME 0x637174
                    "deuseJco"))
            {
                global[i]->SetActState(ACTSTATE_NONE);
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

// Called at Init's end when the city's base blocks are open (the name is ours)
// STUB: GAME 0x5a3990
static void PrepareBaseBlocks()
{
    StubLog("PrepareBaseBlocks (0x5a3990)");
}

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
    paused_i_shows.clear();
    DelAllHydrants();
    game_done = true;
    LightnessClear();
    int i;
    for (i = light_cache.size(); i--;)
    {
        if (light_cache[i])
            light_cache[i]->SetOn(true);
    }
    light_cache.clear();
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
    death_actors.clear();
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
    if (CRacing && anim_preload)
    {
        CRacing->GameDone();
        if (CRacing->mission_race)
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
    for (i = car_shadow_items.size(); i--;)
    {
        S_shadow_item &s = car_shadow_items[i];
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
    car_shadow_items.clear();
    for (i = fires.size(); i--;)
    {
        if (fires[i])
            delete fires[i];
    }
    fires.clear();
    using_object.ClearObjects();
    using_object.ClearInvalidItems();
    ClearTemporaryActors();
    removed_actors.clear();
    std::vector<C_actor *> &actors = act_mission->actors;
    C_traffic_generator::FreeDynCollElements();
    InterCity_GameDoneInGameDone();
    // Kept actors stay as they are
    for (i = actors.size(); i--;)
    {
        C_actor *act = actors[i];
        if (!act->from_mission || act->type != ACTOR_CAR)
            act->GameDone();
        else if (act->act_state != ACTSTATE_NONE)
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
    active_actors.clear();
    inactive_actors.clear();
    dead_humans.clear();
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
    shoots.clear();
    for (i = 100; i--;)
    {
        S_track_projector &p = track_projectors[i];
        if (p.in_collision)
            gifCollision.DynErase(&p.coll);
        if (p.frame)
            p.frame->LinkTo(NULL, 0);
    }
    for (i = 100; i--;)
    {
        if (scratches[i].frame)
            scratches[i].frame->LinkTo(NULL, 0);
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
        if (shoot_sounds[i])
            shoot_sounds[i]->SetOn(false);
    }
    for (i = 0; i < 10; i++)
    {
        if (shoot_lights[i].light)
        {
            shoot_lights[i].light->LinkTo(NULL, 0);
            shoot_lights[i].light->SetOn(false);
        }
    }
    for (i = 0; i < 3; i++)
    {
        if (delayed_sounds[i].sound)
            delayed_sounds[i].sound->SetOn(false);
    }
    rmorf.Mission_Done();
    mafiaIndicators.SetFlags(0);
    for (i = boxes.size(); i--;)
        BoxPlacementDel(boxes[i]->frame);
    for (i = box_dests.size(); i--;)
        delete box_dests[i];
    boxes.clear();
    box_dests.clear();
    box_placement_temp.clear();
    for (i = created_frames.size(); i--;)
    {
        created_frames[i]->LinkTo(NULL, 0);
        created_frames[i]->Release();
    }
    created_frames.clear();
    for (i = hidden_frames.size(); i--;)
        hidden_frames[i]->SetOn(true);
    hidden_frames.clear();
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
    signal_mark_idx.clear();
    if (base_cache.IsOpen())
        base_cache.VisibleAllFrames();
    // STRING: GAME 0x63772c
    act_mission->GlobalProgramRun("GameDoneEnd");
    act_mission->GlobalProgramGameDone();
    DestroyIShowAll();
    if (kiss_frame)
    {
        kiss_frame->Release();
        kiss_frame = NULL;
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
        if (actors[i]->type == ACTOR_CAR && ((C_car *)actors[i])->temporary)
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
    for (i = free_shadow_names.size(); i--;)
        delete free_shadow_names[i];
    free_shadow_names.clear();
    initialized = false;
    return true;
}

// PS2 name: what the mission's end releases of Init's, after Done; once per Init
// FUNCTION: GAME 0x5a4d20
void C_game::DoneIntern()
{
    if (!intern_loaded)
        return;
    intern_loaded = false;
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
        if (track_textures[i])
            track_textures[i]->Release();
    }
    for (i = 100; i--;)
    {
        S_track_projector &p = track_projectors[i];
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
        S_scratch &p = scratches[i];
        if (p.frame)
        {
            p.frame->LinkTo(NULL, 0);
            p.frame->Release();
        }
        p.car = NULL;
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
        if (shoot_sounds[i])
        {
            shoot_sounds[i]->Release();
            shoot_sounds[i] = NULL;
        }
    }
    for (i = 0; i < 10; i++)
    {
        if (shoot_lights[i].light)
        {
            shoot_lights[i].light->Release();
            shoot_lights[i].light = NULL;
        }
    }
    for (i = 0; i < 3; i++)
    {
        if (delayed_sounds[i].sound)
        {
            delayed_sounds[i].sound->Release();
            delayed_sounds[i].sound = NULL;
        }
    }
    C_traffic_generator::ratio_all = 1.0f;
    C_traffic_generator::ratio_police = 1.0f;
    C_traffic_car::elems_coefff = 1.0f;
    C_traffic_car::police_coefff = 1.0f;
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


// The sounds of sound_play (PS2 name): a sound that has stopped, or gone back to its start without looping, is
// released; a fading one has its volume moved by its fade, up to its volume or down to silence, where it is turned
// off. While none fades the list is checked only at sound_play_timer's periods.
inline void C_game::TickEffects(uint time)
{
    if (!sounds_fading && !sound_play_timer.Tick(time))
        return;
    for (uint i = 0; i < sound_play.size();)
    {
        if (sound_play[i].sound->IsPlaying(false))
        {
            if (!sound_play[i].sound->loop)
            {
                if (sound_play[i].time > sound_play[i].sound->GetCurrTime())
                    goto stopped;
                sound_play[i].time = sound_play[i].sound->GetCurrTime();
            }
            if (sound_play[i].fading)
            {
                if (sound_play[i].fade > 0.0f)
                {
                    float vol = time * sound_play[i].fade + sound_play[i].sound->GetVolume();
                    if (vol >= sound_play[i].volume)
                    {
                        vol = sound_play[i].volume;
                        sound_play[i].fading = false;
                        sounds_fading--;
                    }
                    sound_play[i].sound->SetVolume(vol);
                }
                else
                {
                    float vol = time * sound_play[i].fade + sound_play[i].sound->GetVolume();
                    if (vol <= 0.0f)
                    {
                        sound_play[i].sound->SetOn(false, true);
                        sounds_fading--;
                    }
                    else
                        sound_play[i].sound->SetVolume(vol);
                }
            }
            i++;
            continue;
        }
stopped:
        sound_play[i].sound->SetOn(false, true);
        sound_play[i].sound->LinkTo(NULL, 0);
        sound_play[i].sound->Release();
        sound_play[i] = sound_play.back();
        sound_play.pop_back();
    }
}

// The shots' lights (PS2 name): each goes off once its time is up, out of its sector; a flickering one gets a random
// power until its last 500 ms, in which it fades
inline void C_game::ShootLightTick(uint time)
{
    for (int i = 10; i--;)
    {
        S_shoot_light &l = shoot_lights[i];
        if (l.time > 0)
        {
            l.time -= time;
            if (l.time <= 0)
            {
                I3D_frame *parent = l.light->GetParent();
                if (parent && parent->GetType() == FRAME_SECTOR)
                    ((I3D_sector *)parent)->DeleteLight(l.light);
                l.light->LinkTo(NULL, 0);
                l.light->SetOn(false);
            }
            else if (l.flags & 1)
            {
                if (l.time > 500)
                    l.light->SetDiffusePower(rnd() * .4f + .8f);
                else
                    l.light->SetDiffusePower(l.time * .002f);
            }
        }
    }
}

// The lights of light_cache (PS2 name): about every two seconds, each turned on while the camera is within its range
// and the camera's own, 15 m more
inline void C_game::LightCacheTick(uint time)
{
    if (!light_cache.size())
        return;
    if (light_cache_time > 0)
    {
        light_cache_time -= time;
        return;
    }
    light_cache_time = 2023;
    I3D_camera *cam = act_mission->GetScene()->GetActiveCamera();
    float range = cam->fcp;
    S_vector pos = cam->GetWorldPos();
    for (int i = light_cache.size(); i--;)
    {
        S_vector dir = light_cache[i]->GetWorldPos() - pos;
        I3D_light *lt = (I3D_light *)light_cache[i];
        float r = (lt->far_range + range + 15.0f) * (lt->far_range + range + 15.0f);
        if (dir.Square() < r)
            lt->SetOn(true);
        else
            lt->SetOn(false);
    }
}

// The hydrants (PS2 name): one whose particle has ended, or 150 m from the camera, taken from the list; the others'
// particles stop while the line up from them hits something, a car if anything with an actor (PauseParticle's bool
// is the emitter's emitting: PS2's toggles the emitter's flag 4 instead, whatever the bool)
inline void C_game::TickHydrant(uint time)
{
    S_vector pos = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    for (uint i = 0; i < hydrants.size();)
    {
        if (!act_mission->GetEffects()->IsParticleAlive(hydrants[i].particle))
        {
            if (hydrants[i].frame)
            {
                hydrants[i].frame->SetOn(false);
                hydrants[i].frame->Release();
            }
        }
        else if ((hydrants[i].pos - pos).Square() > 22500.0f)
        {
            if (hydrants[i].frame)
            {
                hydrants[i].frame->SetOn(false);
                hydrants[i].frame->Release();
            }
            act_mission->GetEffects()->DestroyParticle(hydrants[i].particle);
        }
        else
        {
            g_collision_header *c = (g_collision_header *)gifCollision.TestLineV(hydrants[i].pos,
                S_vector(0.0f, 1.0f, 0.0f), NULL, NULL, 2, (uint)-1);
            if (c && c->owner && c->owner->type != ACTOR_CAR)
                c = NULL;
            if (c)
            {
                if (!hydrants[i].blocked)
                {
                    hydrants[i].blocked = true;
                    act_mission->GetEffects()->PauseParticle(hydrants[i].particle, false);
                }
            }
            else if (hydrants[i].blocked)
            {
                hydrants[i].blocked = false;
                act_mission->GetEffects()->PauseParticle(hydrants[i].particle, true);
            }
            i++;
            continue;
        }
        hydrants[i] = hydrants.back();
        hydrants.pop_back();
    }
}

// The city's music (PS2 name): every ten seconds, once no other video is playing, the music of the city_music_ area
// the camera is in, its name's two letters after the prefix, faded in over the one before
inline void C_game::TickCityMusic(uint time)
{
    if (city_music_time < 0)
    {
        city_music_time = 10000;
        if (!city_music_areas.size())
            return;
        int i;
        for (i = i_shows.size(); i--;)
        {
            if (i_shows[i].id != city_music && i_shows[i].id != city_music_old && i_shows[i].show)
            {
                i_shows[i].show->GetEvent();
                if (!i_shows[i].show->GetEvent())
                    return;
            }
        }
        I3D_frame *cam = act_mission->GetScene()->GetActiveCamera();
        if (!cam)
            return;
        S_vector pos = cam->GetWorldPos();
        for (i = city_music_areas.size(); i--;)
        {
            if (((I3D_area *)city_music_areas[i])->CheckPoint(pos))
            {
                I3D_frame *area = city_music_areas[i];
                if (!area || area == city_music_area)
                    return;
                city_music_area = area;
                // The two letters after city_music_
                const char *name = area->GetName() + 11;
                char music[3];
                music[0] = name[0];
                music[1] = name[1];
                music[2] = 0;
                char file[128];
                // STRING: GAME 0x637748
                sprintf(file, "sounds\\music\\city_music_%s.ogg", music);
                for (int j = i_shows.size(); j--;)
                {
                    if (i_shows[j].id == city_music)
                    {
                        if (i_shows[j].name && !strcmp(file, i_shows[j].name))
                            return;
                        break;
                    }
                }
                if (city_music_old > -1)
                    DestroyIShow(city_music_old);
                city_music_old = city_music;
                if (GetIShow(city_music_old))
                {
                    IShowLoop(city_music_old, false);
                    IShowFade(city_music_old, .5f, 0.0f, 5000);
                }
                city_music = CreateIShow(2);
                OpenIShow(file, city_music);
                IShow *show = GetIShow(city_music);
                if (show)
                {
                    show->Play();
                    IShowLoop(city_music, true);
                    IShowFade(city_music, 0.0f, .5f, 5000);
                }
                return;
            }
        }
    }
    else
        city_music_time -= time;
}

// The drivers' steering wheels (PS2 name), with the game effects on: each animation's time by its car's steering
inline void C_game::TickDriverMan()
{
    if (act_mission->GetOptions().game_effects < 1)
        return;
    for (int i = driver_men.size(); i--;)
    {
        driver_men[i].model->SetAnimTime(driver_men[i].anim,
            (int)((driver_men[i].car->steer / driver_men[i].car->steer_max + 1.0f) * .5f *
            (driver_men[i].formula ? wheel_anim_time_formula : wheel_anim_time)));
        driver_men[i].model->Tick(0);
    }
}

// The actors whose state changed moved between the two lists (PS2 name): each actor out of ACTSTATE_ACTIVE from
// active_actors, each out of ACTSTATE_INACTIVE from inactive_actors, told by ChangeState and put into the other list
// unless it is in ACTSTATE_NONE, the list's last actor in its place
// FUNCTION: GAME 0x5a5070
void C_game::ChangeActState()
{
    act_state_changed = false;
    uint i;
    for (i = 0; i < active_actors.size();)
    {
        if (active_actors[i]->GetActState() != ACTSTATE_ACTIVE)
        {
            active_actors[i]->ChangeState();
            if (active_actors[i]->GetActState() != ACTSTATE_NONE)
                inactive_actors.push_back(active_actors[i]);
            active_actors[i] = active_actors.back();
            active_actors.pop_back();
        }
        else
            i++;
    }
    for (i = 0; i < inactive_actors.size();)
    {
        if (inactive_actors[i]->GetActState() != ACTSTATE_INACTIVE)
        {
            inactive_actors[i]->ChangeState();
            if (inactive_actors[i]->GetActState() != ACTSTATE_NONE)
                active_actors.push_back(inactive_actors[i]);
            inactive_actors[i] = inactive_actors.back();
            inactive_actors.pop_back();
        }
        else
            i++;
    }
}

// A frame of the game (PS2 name), which C_mission::Tick runs once Init has: the failure menus, the actors, the
// camera, the sounds, lights, videos and the rest of what the game keeps going
// FUNCTION: GAME 0x5a51c0
void C_game::Tick(uint time)
{
    if (!initialized)
        return;

    // The weather's particles a script asked for (weather_buffer), or the weather off
    if (weather_buffer > 0)
    {
        act_mission->GetScene()->WeatherSystemReset();
        act_mission->GetScene()->SetWeatherSystemParam(WS_PARAM_ON, 1);
        act_mission->GetScene()->SetWeatherSystemParam(WS_PARAM_COUNT, weather_buffer);
        weather_buffer = 0;
    }
    else if (!weather_buffer)
    {
        act_mission->GetScene()->SetWeatherSystemParam(WS_PARAM_ON, 0);
        weather_buffer = -1;
    }

    // The mission failed, or the player dead: once the screen has faded out, the failure's menu
    if (!CRacing)
    {
        if (game_failed && mafiaIndicators.FadeInOutGetState())
        {
            if (mafiaIndicators.FadeInOutIsDark())
            {
                act_mission->GetGame()->StopAllSounds();
                switch (GM_Menu::ExecuteMenu(new GM_GameFailed(0x5b3f63), false, true))
                {
                case GAMEFAILED_RESULT_EXIT:
                    mission_end = 1;
                    break;
                case GAMEFAILED_RESULT_4FC:
                    mission_end = 2;
                    break;
                case MAINMENU_LOAD_GAME:
                case GAMEFAILED_RESULT_RETRY:
                    mission_end = 3;
                    break;
                }
                mafiaIndicators.FadeInOut(false, 0, 0);
            }
            else
                mafiaIndicators.FadeInOut(true, 1000, 0);
        }
        if (player && player->dead)
        {
            if (death_fade_time < 0)
            {
                if (mafiaIndicators.FadeInOutGetState())
                {
                    if (mafiaIndicators.FadeInOutIsDark())
                    {
                        act_mission->GetGame()->StopAllSounds();
                        switch (GM_Menu::ExecuteMenu(new GM_GameFailed(0xd55162), false, true))
                        {
                        case GAMEFAILED_RESULT_EXIT:
                            mission_end = 1;
                            break;
                        case GAMEFAILED_RESULT_4FC:
                            mission_end = 2;
                            break;
                        case MAINMENU_LOAD_GAME:
                        case GAMEFAILED_RESULT_RETRY:
                            mission_end = 3;
                            break;
                        }
                        mafiaIndicators.FadeInOut(false, 0, 0);
                    }
                    else
                        mafiaIndicators.FadeInOut(true, 1000, 0);
                }
            }
            else
                death_fade_time -= time;
        }
    }

    if (find_actors.size())
        FindActorsVect();

    game_time += time;
    game_frames++;
    init_frames++;
    act_mission->render_number = act_mission->GetScene()->m_RenderNumber;

    // How fast the player, or else the camera, moves
    I3D_frame *frm = act_mission->GetScene()->GetActiveCamera();
    if (player)
        frm = player->frame;
    S_vector pos = frm->GetWorldPos();
    S_vector d = pos - player_pos;
    player_speed = d.Magnitude();
    if (time > 0)
        player_speed = 1000.0f / time * player_speed;
    else
        player_speed *= 1000.0f;
    player_pos = pos;

    if (C_crash_obj::count > 20)
        C_crash_obj::DestroyOuterMostCrashObject();

    for (uint i = 0; i < active_actors.size(); i++)
        active_actors[i]->Tick(time);
    if (dead_humans.size())
        TickDeadHumanForTick(time);
    C_traffic_generator::TickElements(time);
    if (death_actors.size())
        DeathActorProcess();
    if (clear_diff_data)
    {
        clear_diff_data = false;
        ClearDiffData();
    }

    // The invisible actors, at most every sixth frame: a sleeping one woken, an active one put back once its model or
    // visual was rendered in the last frame
    for (int j = invisible_actors.size(); j--;)
    {
        if (game_frames - invisible_actors[j].frame <= 5)
            continue;
        C_actor *act = invisible_actors[j].actor;
        switch (act->act_state)
        {
        case ACTSTATE_ACTIVE:
            if (!act->rendered)
            {
                act->SetActState(ACTSTATE_INACTIVE);
                invisible_actors[j].frame = game_frames;
            }
            break;
        case ACTSTATE_INACTIVE:
            I3D_frame *f = act->frame;
            if (f)
            {
                bool seen = true;
                if (f->GetType() == FRAME_MODEL)
                    seen = f->m_RenderNumber == act_mission->GetScene()->m_RenderNumber;
                else if (f->GetType() == FRAME_VISUAL)
                    seen = f->m_RenderNumber == act_mission->GetScene()->m_RenderNumber;
                if (seen)
                {
                    act->SetActState(ACTSTATE_ACTIVE);
                    invisible_actors[j].frame = game_frames;
                }
            }
            break;
        }
    }
    if (act_state_changed)
        ChangeActState();

    // The camera, turned to the four sides in the first frames (the scene's textures loaded), then the rotation a
    // script set
    camera.tilt = player ? player->tilt - ((C_player *)player)->recoil * .1f : .5f;
    camera.Tick(time);
    if (init_frames < 7)
    {
        I3D_frame *cam = act_mission->GetScene()->GetActiveCamera();
        switch (init_frames)
        {
        case 0:
        case 1:
        case 2:
            cam->SetDir(S_vector(1.0f, 0.0f, 0.0f), 0.0f);
            break;
        case 3:
            cam->SetDir(S_vector(-1.0f, 0.0f, 0.0f), 0.0f);
            break;
        case 4:
            cam->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
            break;
        case 5:
            cam->SetDir(S_vector(0.0f, 0.0f, -1.0f), 0.0f);
            break;
        case 6:
            if (camera_rot_repair)
            {
                camera_rot_repair = false;
                cam->SetRot(camera_rot);
            }
            break;
        }
        cam->Update();
    }

    if (removed_actors.size())
    {
        for (int k = removed_actors.size(); k--;)
            _RemoveTemporaryActor(removed_actors[k]);
        removed_actors.clear();
    }

    DabingTick(time);
    SetActiveTrafficP(time);
    TickShoot(time);
    glass.Tick(time);
    debris.Tick(time);
    TickEffects(time);
    TickShadows(time);
    rmorf.Tick(time);
    sledovacka.Tick(time);
    act_mission->GetEffects()->ClientTick(time);
    blood_manager.Tick(time);
    ff_manager.Tick(time);
    shvestky.Tick(time);
    ShootLightTick(time);
    LightCacheTick(time);
    AIManager.police.Tick(time);
    AIManager.groups.Tick();
    AIManager.follow.Update();
    AIManager.fight.Tick();
    TickHydrant(time);
    if (item_blur)
        ItemBlurTick(time);
    if (scream_areas.size())
    {
        if (scream_areas_time < 0)
        {
            scream_areas_time = 513;
            TickScreamAreas();
        }
        else
            scream_areas_time -= time;
    }
    if (act_state_changed)
        ChangeActState();
    if (removed_actors.size())
    {
        for (int k = removed_actors.size(); k--;)
            _RemoveTemporaryActor(removed_actors[k]);
        removed_actors.clear();
    }

    // The cars' sounds, alternately 200 and 800 ms apart
    if (car_sounds > 0)
    {
        car_sounds_time -= time;
        if (car_sounds_time < 0)
        {
            car_sounds_short = !car_sounds_short;
            car_sounds_time = car_sounds_short ? 200 : 800;
            TickCarSoundManager();
        }
    }

    // The videos' fades (IShowFade's), each taken from the list at its end, a video faded out stopped
    for (uint m = 0; m < i_show_fades.size();)
    {
        i_show_fades[m].time += time;
        if (i_show_fades[m].time >= i_show_fades[m].length)
        {
            IShow *show = GetIShow(i_show_fades[m].id);
            if (show)
            {
                float to = i_show_fades[m].to;
                if (to < .005f && to < i_show_fades[m].from)
                {
                    show->Stop();
                    if (i_show_fades[m].id == city_music_old)
                    {
                        DestroyIShow(city_music_old);
                        city_music_old = -1;
                    }
                }
                else
                {
                    IShowStoreVolume(i_show_fades[m].id, to);
                    show->SetVolume(mafiaLoadSave.settings.music_volume * to);
                }
            }
            i_show_fades[m] = i_show_fades.back();
            i_show_fades.pop_back();
        }
        else
        {
            IShow *show = GetIShow(i_show_fades[m].id);
            if (show)
            {
                float vol = (float)i_show_fades[m].time / (float)i_show_fades[m].length *
                    (i_show_fades[m].to - i_show_fades[m].from) + i_show_fades[m].from;
                show->SetVolume(mafiaLoadSave.settings.music_volume * vol);
                IShowStoreVolume(i_show_fades[m].id, vol);
            }
            m++;
        }
    }

    // The looping videos started again at their ends
    for (int n = i_shows.size(); n--;)
    {
        if (i_shows[n].flags & ISHOWF_LOOP)
        {
            if (i_shows[n].show->GetEvent() == 1)
            {
                i_shows[n].show->SetCurrentPos(0);
                i_shows[n].show->Play();
            }
        }
    }

    if (actor_lightness && lightness.size() && act_mission->GetOptions().game_effects >= 2)
        LightnessTick(time);

    // The semaphores' phases, by semaphore_timer's periods
    if (semaphore_timer.Tick(time))
    {
        if (++semaphore_phase_x > 9)
            semaphore_phase_x = 0;
        if (++semaphore_phase_z > 9)
            semaphore_phase_z = 0;
        TickSemaphores();
    }

    if (!city_music_paused)
        TickCityMusic(time);

    // The fires, each deleted once it has burnt out
    for (uint f = 0; f < fires.size();)
    {
        if (fires[f]->Tick(time))
            f++;
        else
        {
            delete fires[f];
            fires[f] = fires.back();
            fires.pop_back();
        }
    }

    if (CRacing && anim_preload)
    {
        bool run = true;
        if (CRacing->mission_race)
            run =race6_state != 0;
        if (run)
            CRacing->Tick(time);
    }

    TickDriverMan();

    // The blood's and the holes' projectors growing to their full size
    if (growing_projectors)
    {
        for (int p = 100; p--;)
        {
            S_track_projector &h = track_projectors[p];
            if (h.in_collision && h.growing)
            {
                S_vector scale = h.frame->GetScale();
                bool grown = false;
                scale.x /= h.scale;
                scale.y /= h.scale;
                h.scale += time * .0003f;
                if (h.scale >= 1.0f)
                {
                    h.scale = 1.0f;
                    h.growing = false;
                    grown = true;
                    growing_projectors--;
                }
                scale.x *= h.scale;
                scale.y *= h.scale;
                h.frame->SetScale(scale);
                h.frame->Update();
                if (grown)
                    ((I3D_projector *)track_projectors[track_projector_next].frame)->SetProjectorLock(I3DLOCK_RECEIVERS);
            }
        }
    }

    // Mission 6's race run (RACING_MISSION6_START): its result into the script's register
    if (race6_state == RACE6_STATE_STARTED)
    {
        race6_state = RACE6_STATE_RUNNING;
        car_sounds = 3;
        car_sounds_short = true;
        car_sounds_time = 0;
        uint result = RacingMisse06ExecuteLoop();
        if (race6_result)
        {
            *race6_result = (float)result;
            race6_result = NULL;
        }
        race6_state = RACE6_STATE_END;
    }

    // The sounds turned on once their time is up
    for (int q = 3; q--;)
    {
        S_delayed_sound &s = delayed_sounds[q];
        if (s.time >= 0)
        {
            s.time -= time;
            if (s.time < 0 && s.sound)
                s.sound->SetOn(true);
        }
    }

    DoubleKiss(time);
    gifCollision.DynOptimizeTick();
    console_1.Tick(time);
}

// The guns on the player's car (PS2 name; nothing of the PC's sets GAMEF_DOUBLEKISS or creates kiss_frame, so it
// returns at once): kiss_frame, linked to the car, is turned to its target every 20 ms, the nearest enemy
// (ACTOR_ENTITY) or other car more than 5 m and less than 40 m away (a car's squared distance counting 1.2 times),
// looked for again 100 ms after none was found and given up once it is dead or more than 50 m away. The two guns shoot
// at it every 250 ms, and every turn pushes the car back. The tests nest as PS2's block layout shows.
// FUNCTION: GAME 0x5a6d60
void C_game::DoubleKiss(uint time)
{
    if (cheats & GAMEF_DOUBLEKISS)
    {
        if (kiss_frame && player)
        {
            C_car *car = player->car;
            if (car && car->type == ACTOR_CAR)
            {
                if (kiss_car)
                {
                    kiss_find_time -= time;
                    kiss_shoot_time -= time;
                    if (kiss_find_time < 0)
                    {
                        kiss_find_time = 20;
                        if (kiss_target)
                        {
                            // Turned to the target: a human's chest, a random point of a car
                            S_vector pos = kiss_frame->GetWorldPos();
                            S_vector dir = kiss_target->frame->GetWorldPos();
                            switch (kiss_target->type)
                            {
                            case ACTOR_PLAYER:
                            case ACTOR_ENTITY:
                                dir.y += 1.3f;
                                break;
                            case ACTOR_CAR:
                                {
                                    S_vector v(rnd() - .5f, rnd(), rnd());
                                    v *= kiss_target->frame->GetMatrix();
                                    dir = v;
                                }
                                break;
                            }
                            dir -= pos;
                            float dist2 = dir.Square();
                            bool lost = kiss_target->GetDead();
                            if (!lost && kiss_target->type == ACTOR_ENTITY &&
                                ((C_human *)kiss_target)->properties[HUMAN_PROPERTY_ENERGY] < .01f)
                                lost = true;
                            if (dist2 > 2500.0f || lost)
                            {
                                kiss_target = NULL;
                                return;
                            }
                            dir.Normalize();
                            kiss_frame->SetWorldDir(dir, 0.0f);
                            kiss_frame->Update();

                            // Both guns' cartridges and shots
                            if (kiss_shoot_time < 0 && dist2 < 2500.0f)
                            {
                                kiss_shoot_time = 250;
                                for (int i = 2; i--;)
                                {
                                    S_vector shot_dir = dir;
                                    shot_dir.Normalize();
                                    if (kiss_cartridges[i])
                                        NewCartidge(kiss_cartridges[i], shot_dir, false, false);
                                    if (kiss_guns[i])
                                    {
                                        shot_dir *= 100.0f;
                                        NewShoot(NULL, kiss_guns[i]->GetWorldPos(), shot_dir, 100.0f, 12, kiss_guns[i],
                                            1);
                                    }
                                }
                            }

                            // The car pushed back, against the frame's direction. pos and dir are used again, as PS2
                            // stores them where they were (and the original's frame shares their areas).
                            dir = kiss_frame->GetWorldPos();
                            pos = kiss_frame->GetWorldDir();
                            pos *= -50.0f;
                            kiss_car->ExternalImpuls(pos, dir, 1, S_vector(1.0f, 1.0f, 1.0f));
                        }
                        else
                        {
                            // The nearest target looked for. Through the reference VC6 keeps the vector's array in a
                            // register across the loop, as the original does; the car's sum of squares is
                            // parenthesized so, which alone gives the original's order of the squares (y, z, x).
                            S_vector pos = kiss_frame->GetWorldPos();
                            float best = 1600.0f;
                            std::vector<C_actor *> &actors = C_actor::GetGlobalActors();
                            for (int i = actors.size(); i--;)
                            {
                                if (!actors[i]->frame)
                                    continue;
                                switch (actors[i]->type)
                                {
                                case ACTOR_CAR:
                                    if (actors[i] != player->car && actors[i]->act_state != ACTSTATE_NONE)
                                    {
                                        S_vector d = actors[i]->frame->GetWorldPos();
                                        d -= pos;
                                        float dist2 = ((d.z * d.z + d.y * d.y) + d.x * d.x) * 1.2f;
                                        if (dist2 > 25.0f && dist2 < best)
                                        {
                                            best = dist2;
                                            kiss_target = actors[i];
                                        }
                                    }
                                    break;
                                case ACTOR_ENTITY:
                                    if (actors[i]->act_state == ACTSTATE_ACTIVE)
                                    {
                                        S_vector d = actors[i]->frame->GetWorldPos();
                                        d -= pos;
                                        float dist2 = d.Square();
                                        if (dist2 > 25.0f && dist2 < best)
                                        {
                                            best = dist2;
                                            kiss_target = actors[i];
                                        }
                                    }
                                    break;
                                }
                            }
                            if (!kiss_target)
                                kiss_find_time = 100;
                        }
                    }
                }
                else
                {
                    // The frame put on the car's roof, facing forward
                    kiss_car = car;
                    kiss_frame->LinkTo(car->frame, 0);
                    kiss_frame->SetPos(S_vector(0.0f,
                        kiss_car->GetVehicleSize(false).y - kiss_car->GetVehiclePivotPos(false).y, 0.0f));
                    kiss_frame->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
                    kiss_frame->Update();
                    kiss_shoot_time = 0;
                    kiss_find_time = 0;
                    kiss_target = NULL;
                }
            }
            else if (kiss_car)
            {
                kiss_car = NULL;
                kiss_target = NULL;
                kiss_frame->LinkTo(NULL, 0);
            }
        }
    }
    else if (kiss_car)
    {
        kiss_car = NULL;
        kiss_target = NULL;
        if (kiss_frame)
            kiss_frame->LinkTo(NULL, 0);
    }
}

// STUB: GAME 0x5a77c0
void C_game::AddTemporaryActor(C_actor *act)
{
    StubLog("C_game::AddTemporaryActor");
}

// FUNCTION: GAME 0x5a79a0
void C_game::RemoveTemporaryActor(C_actor *act)
{
    for (int i = removed_actors.size(); i--;)
    {
        if (removed_actors[i] == act)
            return;
    }
    act->SetActState(ACTSTATE_NONE);
    removed_actors.push_back(act);
}

// STUB: GAME 0x5a7b70
void C_game::InternTempActAdd(C_actor *act)
{
    StubLog("C_game::InternTempActAdd");
}

// FUNCTION: GAME 0x5a7d40
void C_game::InternTempActDel(C_actor *act)
{
    int i;
    for (i = removed_actors.size(); i--;)
    {
        if (removed_actors[i] == act)
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
    for (i = active_actors.size(); i--;)
    {
        if (active_actors[i] == act)
        {
            active_actors[i] = active_actors.back();
            active_actors.pop_back();
            break;
        }
    }
    for (i = inactive_actors.size(); i--;)
    {
        if (inactive_actors[i] == act)
        {
            inactive_actors[i] = inactive_actors.back();
            inactive_actors.pop_back();
            break;
        }
    }
}

// STUB: GAME 0x5a7ec0
void C_game::_RemoveTemporaryActor(C_actor *act)
{
    StubLog("C_game::_RemoveTemporaryActor");
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

// FUNCTION: GAME 0x5a7ff0
void C_game::UpdateControls()
{
    if (initialized)
    {
        std::vector<C_actor *> &actors = act_mission->GetActors();
        for (int i = actors.size(); i--;)
            if (actors[i]->GetType() == ACTOR_PLAYER)
                ((C_player *)actors[i])->UpdateControls();
    }
}

// The original's code between this and SetTrafficVisible (0x5a8030-0x5a846f) is more of the game's

// STUB: GAME 0x5a8470
void C_game::SetTrafficVisible(bool visible)
{
    StubLog("C_game::SetTrafficVisible");
}

// STUB: GAME 0x5a84a0
bool C_game::NewShoot(C_actor *actor, S_vector pos, S_vector dir, float power, int unknown1, I3D_frame *frm,
    int unknown2)
{
    StubLog("C_game::NewShoot");
    return true;
}

// The shots in flight (PS2 name), each moved along its direction by 0.3 a ms: one that hits nothing goes on in the next
// frame; one that hits an actor's collision hits the actor (its Hit), whose false lets the shot through, the actor's
// collisions left out of the test, which runs again for the same shot; one that hit or reached the end of its range
// leaves its marks (a hole in what it hit, a track and the material's particle where it ended, broken glass, a crash
// model, the control signals it destroys) and its sound, and is taken from the list.
// FUNCTION: GAME 0x5a8e70
void C_game::TickShoot(uint time)
{
    int i = 0;
    std::vector<tDynamicCollObject *> colls;
    bool move = true;
    float len2;
    S_vector old_pos, new_pos, delta;
    S_vector hit_pos, hit_normal;
    I3D_COLLISION col;
    while (i < shoots.size())
    {
        // The shot moved, up to the end of its range
        if (move)
        {
            len2 = shoots[i].dir.Square();
            S_vector dir = shoots[i].dir;
            dir.Normalize();
            float step = time * .3;
            new_pos = shoots[i].curr_pos + dir * step;
            old_pos = shoots[i].curr_pos;
            delta = new_pos - old_pos;
            shoots[i].curr_pos = new_pos;
            if (len2 < (shoots[i].curr_pos - shoots[i].pos).Square())
            {
                new_pos = shoots[i].curr_pos = shoots[i].pos + shoots[i].dir;
                shoots[i].curr_pos = new_pos;
                delta = shoots[i].curr_pos - old_pos;
            }
        }

        // The moving objects' collisions along the move, but the shooter's
        if (!colls.size())
        {
            C_actor *shooter = shoots[i].actor;
            if (shooter && (shooter->type == ACTOR_PLAYER || shooter->type == ACTOR_ENTITY))
            {
                for (int k = ((C_human *)shooter)->num_colls; k--;)
                    colls.push_back(((C_human *)shooter)->colls[k]);
            }
        }
        if (colls.size())
            gifCollision.DynSetNextCollOwnerMult(&colls[0], colls.size(), NULL);
        tDynamicCollObject *hit = gifCollision.TestLineHDynamic(old_pos, delta, &hit_pos, &hit_normal, 1);
        gifCollision.DynSetNextCollOwner(NULL, NULL);
        S_vector diff = new_pos - shoots[i].pos;
        bool end = diff.Square() + 1.0f > len2;
        if (!hit && !end)
        {
            i++;
        }
        else
        {
            // The actor hit, with the shot's power by the distance
            bool ok = true;
            bool make_hole = false;
            I3D_frame *frm = NULL;
            C_actor *actor = NULL;
            if (hit && hit->owner)
            {
                float power = shoots[i].power;
                float dist = diff.Magnitude();
                if (dist > shoots[i].range)
                    power = 0.0f;
                else if (3.0f * dist > shoots[i].range)
                    power *= 1.0f - (dist - shoots[i].range * .33f) / (shoots[i].range * .66f);
                actor = hit->owner;
                if (actor && actor != shoots[i].actor)
                {
                    if (actor->type == ACTOR_PLAYER || actor->type == ACTOR_ENTITY)
                    {
                        bool can = !end || (new_pos - hit_pos).Square() > 1.0f;
                        if (can && shoots[i].actor &&
                            (shoots[i].actor->type == ACTOR_PLAYER || shoots[i].actor->type == ACTOR_ENTITY))
                            can = C_human::Can_Hit_FilterHumanType((C_human *)shoots[i].actor, (C_human *)actor);
                        if (can)
                        {
                            int bone = ((C_human *)actor)->TestShotCollision(old_pos, delta,
                                (new_pos - old_pos).Square());
                            if (bone)
                            {
                                S_vector d = shoots[i].dir;
                                d.Normalize();
                                ok |= actor->Hit(HIT_SHOT, d, hit_pos, hit_normal, power, shoots[i].actor, bone, NULL);
                                if (act_mission->GetOptions().particles >= 1)
                                    act_mission->GetEffects()->CreateParticle(SHOTPARTICLE_BODY, hit_pos, hit_normal, 0,
                                        NULL);
                            }
                            else
                                ok = false;
                        }
                    }
                    else if (actor->type == ACTOR_DON_OCAS)
                    {
                        S_vector d = shoots[i].dir;
                        d.Normalize();
                        ok = actor->Hit(HIT_SHOT, delta, old_pos, hit_pos, power, shoots[i].actor, 0, hit->frame);
                        if (ok && act_mission->GetOptions().particles >= 1)
                            act_mission->GetEffects()->CreateParticle(SHOTPARTICLE_BODY, hit_pos, hit_normal, 0, NULL);
                    }
                    else if (actor->type == ACTOR_SEAGULL)
                        ok = actor->Hit(HIT_SHOT, delta, old_pos, hit_pos, power, shoots[i].actor, 0, hit->frame);
                    else if (actor->type == ACTOR_RAILWAY)
                    {
                        S_vector d = shoots[i].dir;
                        d.Normalize();
                        ok = actor->Hit(HIT_SHOT, d, hit_pos, hit_normal, power, shoots[i].actor, 0, hit->frame);
                    }
                    else if (actor->type == ACTOR_DETECTOR && hit->frame && hit->frame->GetType() == FRAME_DUMMY)
                    {
                        S_vector d = shoots[i].dir;
                        d.Normalize();
                        ok = actor->Hit(HIT_SHOT, d, hit_pos, hit_normal, power, shoots[i].actor, 0, hit->frame);
                    }
                    else
                    {
                        frm = act_mission->scene->TestColHierarchy(old_pos, delta, hit->frame, col, 0, NULL, 0, NULL);
                        if (frm)
                        {
                            S_vector d = shoots[i].dir;
                            d.Normalize();
                            hit_normal = col.normal;
                            S_vector n = delta;
                            n.Normalize();
                            hit_pos = old_pos + n * col.dist;
                            I3D_frame *f = frm;
                            // A hole: the frame it is on
                            if (!strcmp("dUra_fiRe", frm->GetName()))
                                f = frm->GetParent();
                            ok = actor->Hit(HIT_SHOT, d, hit_pos, hit_normal, power, shoots[i].actor, 0, f);
                            make_hole = ok;
                            if (actor->type == ACTOR_DOG)
                                make_hole = false;
                        }
                        else
                            ok = false;
                    }
                }
            }
            if (!ok)
            {
                colls.push_back(hit);
                move = false;
                continue;
            }

            // A track where the shot ended in the level, or a hole in the actor it hit
            if (!shoots[i].frame && holes[num_holes])
            {
                if (!hit && shoots[i].hit)
                {
                    if (act_mission->GetOptions().game_effects >= 1)
                    {
                        int tex = materialList[shoots[i].material].hole;
                        if (tex >= 0 && tex < 10)
                        {
                            S_vector pos = shoots[i].hit_pos + shoots[i].hit_normal * .005f;
                            S_vector normal = -shoots[i].hit_normal;
                            S_vector size(.04f, .04f, .01f);
                            I3D_frame *f = shoots[i].coll->flags & (COLLHF_UNKNOWN_40 | COLLHF_UNKNOWN_80) ?
                                shoots[i].coll->frame : NULL;
                            CreateTrackEffect(pos, normal, rnd() * (2.0f * PI), size, .002f, tex, false, 1.0f, false,
                                false, f);
                        }
                    }
                    int particle = materialList[shoots[i].material].particle;
                    if (particle >= 0 && act_mission->GetOptions().particles >= 1)
                        act_mission->GetEffects()->CreateParticle(particle, shoots[i].hit_pos, shoots[i].hit_normal, 0,
                            NULL);
                }
                else if (make_hole)
                {
                    if (strcmp("dUra_fiRe", frm->GetName()) && act_mission->GetOptions().game_effects >= 1 &&
                        hole_models.size())
                    {
                        I3D_frame *f = frm;
                        if (!strcmp("dUra_fiRe", frm->GetName()))
                            f = frm->GetParent();
                        int mat = actor->GetMaterialID(f);
                        if (mat >= 0)
                        {
                            uint model = materialList[mat].hole;
                            if (model >= hole_models.size())
                                model = 0;
                            holes[num_holes]->LinkTo(NULL, 0);
                            holes[num_holes]->Duplicate(hole_models[model]);
                            S_vector d = delta;
                            d.Normalize();
                            S_vector pos = old_pos + d * col.dist;
                            pos += col.normal * .02f;
                            holes[num_holes]->LinkTo(act_mission->scene->primary_sector, 0);
                            holes[num_holes]->SetPos(pos);
                            holes[num_holes]->SetDir(col.normal, 0.0f);
                            holes[num_holes]->Update();
                            holes[num_holes]->LinkTo(frm, 1);
                            if (++num_holes >= 100)
                                num_holes = 0;
                        }
                    }
                }
            }

            // The level's collision hit: control signals destroyed, glass broken, a crash model
            if (!hit && shoots[i].coll && (shoots[i].coll->flags & COLLHF_CONTROL_SIGNAL) &&
                !(shoots[i].coll->flags & COLLHF_DISABLED))
            {
                S_vector d = shoots[i].dir;
                d.Normalize();
                d.y = (float)fabs(d.y);
                d *= 10.0f;
                DestroyControlSignal(shoots[i].coll, shoots[i].hit_pos, shoots[i].pos, d, (C_human *)shoots[i].actor);
            }
            if (!hit && shoots[i].frame && shoots[i].coll && !(shoots[i].coll->flags & COLLHF_DISABLED))
            {
                if (shoots[i].coll->material == MAT_GLASS)
                {
                    shoots[i].coll->flags |= COLLHF_DISABLED;
                    S_vector d = shoots[i].dir;
                    d.Normalize();
                    // STRING: GAME 0x6323b8
                    glass.BreakGlass(shoots[i].frame, &d, "lustr1.wav", true);
                }
                if (shoots[i].coll->material == MAT_CRASH)
                {
                    S_vector d = delta;
                    d.Normalize();
                    d *= 5.0f;
                    float power = shoots[i].power;
                    float dist = diff.Magnitude();
                    if (dist > shoots[i].range)
                        power = 0.0f;
                    else if (3.0f * dist > shoots[i].range)
                        power *= 1.0f - (dist - shoots[i].range * .33f) / (shoots[i].range * .66f);
                    CreateCrashModel(shoots[i].frame, old_pos, d, power);
                }
            }

            // The sound of what was hit
            if (hit && actor)
            {
                switch (actor->type)
                {
                case ACTOR_CAR:
                    shoots[i].sound = SHOTSOUND_CAR;
                    break;
                case ACTOR_PLAYER:
                case ACTOR_ENTITY:
                    shoots[i].sound = SHOTSOUND_BODY;
                    break;
                case ACTOR_DROP_OUT:
                case ACTOR_CRASH_OBJ:
                case ACTOR_PUBLIC_PHYSICS:
                case ACTOR_BOX_NEW:
                    {
                        int mat = ((C_PhThing *)actor)->material;
                        if (mat > -1 && mat < numOfMaterials)
                            shoots[i].sound = materialList[mat].shot_sound;
                        else
                            shoots[i].sound = -1;
                    }
                    break;
                default:
                    shoots[i].sound = -1;
                }
            }
            if (shoots[i].sound > -1)
            {
                for (int k = 0; k < 10; k++)
                {
                    if (shoot_sounds[k] && !shoot_sounds[k]->IsPlaying(false))
                    {
                        if (sound_table.ConstructSound(shoot_sounds[k], shoots[i].sound))
                        {
                            shoot_sounds[k]->SetPos(new_pos);
                            act_mission->scene->SetFrameSector(shoot_sounds[k]);
                            shoot_sounds[k]->Update();
                            shoot_sounds[k]->SetOn(true, true);
                        }
                        break;
                    }
                }
            }
            shoots[i] = shoots.back();
            shoots.pop_back();
            if (!end)
                i++;
        }
        colls.clear();
        move = true;
    }
}

// STUB: GAME 0x5aae10
void C_game::NewExplosion(C_actor *actor, S_vector &pos, float unknown1, float unknown2, bool unknown3, bool unknown4,
    bool unknown5, int unknown6)
{
    StubLog("C_game::NewExplosion");
}

// STUB: GAME 0x5abfb0
void C_game::NewCartidge(I3D_frame *&frm, S_vector &dir, bool unknown1, bool unknown2)
{
    StubLog("C_game::NewCartidge");
}

// STUB: GAME 0x5aca70
int C_game::Play3DSound(I3D_frame *frm, const char *name, I3D_SOUNDTYPE type, S_vector pos, float near_range,
    float far_range, float volume, bool fade_in, float fade, bool unknown3)
{
    StubLog("C_game::Play3DSound %s", name);
    return -1;
}

// STUB: GAME 0x5acc00
void C_game::Stop3DSound(int sound, bool fade, float time)
{
    StubLog("C_game::Stop3DSound");
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
        item->model->Duplicate(shadow_temp[item->model_index].model);
}

// STUB: GAME 0x5ad2a0
int C_game::AddFrameForShadow(I3D_frame *frm, E_shadow_type type1, E_shadow_type type2, I3D_frame *replaced_frame,
    char *name)
{
    StubLog("C_game::AddFrameForShadow");
    return 0;
}

// STUB: GAME 0x5ad7c0
void C_game::DelFrameForShadow(I3D_frame *frm, E_shadow_type type)
{
    StubLog("C_game::DelFrameForShadow");
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
        s.phase = &act_mission->game->semaphore_phase_x;
    else
        s.phase = &act_mission->game->semaphore_phase_z;
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

// The city's music faded out from its volume over 5 s, as TickCityMusic does it when the area changes (PS2 name):
// the music fading out before destroyed, the one playing forgotten
// FUNCTION: GAME 0x5ae130
void C_game::CityMusicOff()
{
    city_music_area = NULL;
    if (city_music_old > -1)
        DestroyIShow(city_music_old);
    city_music_old = city_music;
    IShow *show = GetIShow(city_music_old);
    if (show)
    {
        IShowLoop(city_music_old, false);
        IShowFade(city_music_old, show->GetVolume(), 0.0f, 5000);
    }
    city_music = -1;
}

// STUB: GAME 0x5ae1b0
void C_game::CityMusicPaused(bool paused)
{
    StubLog("C_game::CityMusicPaused");
}

// STUB: GAME 0x5ae1d0
void C_game::SetFilmMusic(int music)
{
    StubLog("C_game::SetFilmMusic");
}

// STUB: GAME 0x5ae390
int C_game::GetFilmMusic(int index)
{
    StubLog("C_game::GetFilmMusic");
    return -1;
}

// STUB: GAME 0x5ae3d0
void C_game::StreamDestroyAll()
{
    StubLog("C_game::StreamDestroyAll");
}

// STUB: GAME 0x5ae480
void C_game::LoadDiffData(const char *name)
{
    StubLog("C_game::LoadDiffData %s", name);
}

// What the game added to the mission (PS2 name): its actors taken out of the game's lists and deleted from the mission,
// its frames deleted from the scene (a frame's actor too, and a light from its sectors' lists), its scripts ended and
// deleted
// FUNCTION: GAME 0x5af860
void C_game::ClearDiffData()
{
    int i, j;
    for (i = diff_actors.size(); i--;)
    {
        for (j = active_actors.size(); j--;)
        {
            if (active_actors[j] == diff_actors[i])
            {
                active_actors[j] = active_actors.back();
                active_actors.pop_back();
                break;
            }
        }
        for (j = inactive_actors.size(); j--;)
        {
            if (inactive_actors[j] == diff_actors[i])
            {
                inactive_actors[j] = inactive_actors.back();
                inactive_actors.pop_back();
                break;
            }
        }
        for (j = temporary_actors.size(); j--;)
        {
            if (temporary_actors[j] == diff_actors[i])
            {
                temporary_actors[j] = temporary_actors.back();
                temporary_actors.pop_back();
                break;
            }
        }
        InvalidateActor(diff_actors[i], false);
        diff_actors[i]->GameDone();
        act_mission->DelActor(diff_actors[i]);
    }
    diff_actors.clear();
    for (i = diff_frames.size(); i--;)
    {
        C_actor **act = GetFrameActor(diff_frames[i]);
        if (act && *act)
        {
            for (j = active_actors.size(); j--;)
            {
                if (active_actors[j] == *act)
                {
                    active_actors[j] = active_actors.back();
                    active_actors.pop_back();
                    break;
                }
            }
            for (j = inactive_actors.size(); j--;)
            {
                if (inactive_actors[j] == *act)
                {
                    inactive_actors[j] = inactive_actors.back();
                    inactive_actors.pop_back();
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
        S_hlp::cbFreeFramePlacement(diff_frames[i], frame_placements);
        I3D_frame *frm = diff_frames[i];
        if (frm->GetType() == FRAME_LIGHT)
        {
            I3D_light *lt = (I3D_light *)frm;
            I3D_sector *const *sectors = &lt->sectors[0];
            for (j = lt->sectors.size(); j--;)
                sectors[j]->DeleteLight(lt);
        }
        act_mission->scene->DeleteFrame(diff_frames[i]);
    }
    diff_frames.clear();
    for (i = diff_programs.size(); i--;)
    {
        diff_programs[i]->Done();
        delete diff_programs[i];
    }
    diff_programs.clear();
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
    for (int i = diff_programs.size(); i--;)
    {
        if (!_stricmp(diff_programs[i]->name, name))
        {
            diff_programs[i]->CallSubroutine();
            return true;
        }
    }
    return false;
}

// STUB: GAME 0x5afe40
void C_game::InvalidateActor(C_actor *act, bool)
{
    StubLog("C_game::InvalidateActor");
}

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

// STUB: GAME 0x5b0300
void C_game::AddFramePlacement(I3D_frame *frm)
{
    StubLog("C_game::AddFramePlacement");
}

// STUB: GAME 0x5b06e0
void C_game::AddNPCKillEvent(C_program *prog, int unknown, char *label)
{
    StubLog("C_game::AddNPCKillEvent");
}

// STUB: GAME 0x5b0b60
void C_game::CreateTrackEffect(S_vector &pos, S_vector &normal, float angle, S_vector &size, float unknown1,
    int texture, bool unknown2, float unknown3, bool unknown4, bool unknown5, I3D_frame *frm)
{
    StubLog("C_game::CreateTrackEffect");
}

// STUB: GAME 0x5b1d70
void C_game::ClearProjectorsForFrame(I3D_frame *frm)
{
    StubLog("C_game::ClearProjectorsForFrame");
}

// FUNCTION: GAME 0x5b1da0
void C_game::InventoryStackClear()
{
    for (int i = inventory_stack.size(); i--; )
        delete inventory_stack[i];
    inventory_stack.clear();
}

// STUB: GAME 0x5b1e10
void C_game::InventoryStackPush(C_human *human)
{
    StubLog("C_game::InventoryStackPush");
}

// STUB: GAME 0x5b20b0
void C_game::InventoryStackPop(C_human *human)
{
    StubLog("C_game::InventoryStackPop");
}

// FUNCTION: GAME 0x5b2320
void C_game::FloatStackClear()
{
    float_stack.clear();
}

// STUB: GAME 0x5b2350
void C_game::FloatStackPush(float f)
{
    StubLog("C_game::FloatStackPush");
}

// STUB: GAME 0x5b2530
void C_game::FloatStackPop(float *f)
{
    StubLog("C_game::FloatStackPop");
}

// FUNCTION: GAME 0x5b2590
void C_game::CharacterStackClear()
{
    character_stack.clear();
}

// STUB: GAME 0x5b25c0
void C_game::CharacterStackPush(C_human *human)
{
    StubLog("C_game::CharacterStackPush");
}

// STUB: GAME 0x5b2740
void C_game::CharacterStackPop(C_human *human)
{
    StubLog("C_game::CharacterStackPop");
}

// STUB: GAME 0x5b2a00
void C_game::BoxPlacementAdd(I3D_frame *frm, int unknown)
{
    StubLog("C_game::BoxPlacementAdd");
}

// STUB: GAME 0x5b2ee0
void C_game::BoxPlacementDel(I3D_frame *frm)
{
    StubLog("C_game::BoxPlacementDel");
}

// STUB: GAME 0x5b2fc0
void C_game::BoxPlacementRecompile()
{
    StubLog("C_game::BoxPlacementRecompile");
}

// STUB: GAME 0x5b3220
void C_game::BoxPlacementDestAdd(I3D_frame *frm, int unknown)
{
    StubLog("C_game::BoxPlacementDestAdd");
}

// STUB: GAME 0x5b3a90
void C_game::BoxPlacement_DelInfo(I3D_frame *frm)
{
    StubLog("C_game::BoxPlacement_DelInfo");
}

// STUB: GAME 0x5b4190
int C_game::BoxPlacement_GetBox(int index)
{
    StubLog("C_game::BoxPlacement_GetBox");
    return 0;
}

// STUB: GAME 0x5b41d0
int C_game::BoxPlacement_GetBoxDest(int index)
{
    StubLog("C_game::BoxPlacement_GetBoxDest");
    return 0;
}

// STUB: GAME 0x5b4210
void C_game::BoxPlacement_MoveBoxToDest(int index)
{
    StubLog("C_game::BoxPlacement_MoveBoxToDest");
}

// STUB: GAME 0x5b42e0
void C_game::BoxRecompileBoxForCar(C_car *car, bool unknown)
{
    StubLog("C_game::BoxRecompileBoxForCar");
}

// STUB: GAME 0x5b44e0
int C_game::BoxPlacementPoPiciNaZkurvenemAute(C_car *car, int unknown)
{
    StubLog("C_game::BoxPlacementPoPiciNaZkurvenemAute");
    return 0;
}

// STUB: GAME 0x5b4560
void C_game::DeathActorAdd(C_actor *act)
{
    StubLog("C_game::DeathActorAdd");
}

// STUB: GAME 0x5b4720
void C_game::DeathActorProcess()
{
    StubLog("C_game::DeathActorProcess");
}

// STUB: GAME 0x5b4790
C_actor *C_game::GetActorFromGUID(uint id)
{
    StubLog("C_game::GetActorFromGUID");
    return NULL;
}

// STUB: GAME 0x5b4d10
bool C_game::SaveGameLoad(void **buf)
{
    StubLog("C_game::SaveGameLoad");
    return false;
}

// STUB: GAME 0x5b5410
bool C_game::SaveGameLoadBefore(void **buf)
{
    StubLog("C_game::SaveGameLoadBefore");
    return false;
}

// STUB: GAME 0x5b47d0
uint C_game::SaveGameGetSize(bool full)
{
    StubLog("C_game::SaveGameGetSize");
    return 0;
}

// STUB: GAME 0x5b4920
bool C_game::SaveGameSave(void **buf, bool full)
{
    StubLog("C_game::SaveGameSave");
    return true;
}

// Deletes the lightness entries (PS2 name)
// FUNCTION: GAME 0x5b5780
void C_game::LightnessClear()
{
    for (int i = lightness.size(); i--;)
        delete lightness[i];
    lightness.clear();
}

// STUB: GAME 0x5b57e0
void C_game::LightnessTick(uint time)
{
    StubLog("C_game::LightnessTick");
}

// STUB: GAME 0x5b5c80
void C_game::UseLightCacheSystem()
{
    StubLog("C_game::UseLightCacheSystem");
}

// STUB: GAME 0x5b5d40
void C_game::RecomputeLightCache()
{
    StubLog("C_game::RecomputeLightCache");
}

// A new stream of IGraph's in the list, at the music's volume: its id, -1 without sound or if IGraph makes none
// FUNCTION: GAME 0x5b5e90
int C_game::CreateIShow(uint flags)
{
    if (setting.disable_sound)
        return -1;
    S_i_show s;
    s.id = next_i_show_id++;
    s.flags = flags;
    s.name = NULL;
    s.show = igraph->CreateIShow();
    s.volume = 1.0f;
    if (s.show)
    {
        i_shows.push_back(s);
        s.show->SetVolume(mafiaLoadSave.settings.music_volume * s.volume);
        return s.id;
    }
    return -1;
}

// Another stream playing the file is destroyed first; the stream of the id keeps the name, and an .ogg it doesn't find
// is opened as an .mp3 instead
// FUNCTION: GAME 0x5b6110
bool C_game::OpenIShow(const char *name, int id)
{
    if (!name)
        return false;
    int i;
    for (i = i_shows.size(); i--;)
    {
        if (i_shows[i].id != id && i_shows[i].name && !_stricmp(i_shows[i].name, name))
        {
            DestroyIShow(i_shows[i].id);
            break;
        }
    }
    for (i = i_shows.size(); i--;)
    {
        if (i_shows[i].id == id)
        {
            if (i_shows[i].name)
                delete[] i_shows[i].name;
            i_shows[i].name = new char[strlen(name) + 1];
            strcpy(i_shows[i].name, name);
            // STRING: GAME 0x6377b8
            if (i_shows[i].show->Open(name, 0) == I3DERR_NOFILE && strstr(name, ".ogg"))
            {
                char *mp3 = new char[strlen(name) + 1];
                strcpy(mp3, name);
                char *ext = strstr(mp3, ".ogg");
                ext[1] = 'm';
                ext[2] = 'p';
                ext[3] = '3';
                i_shows[i].show->Open(mp3, 0);
                delete[] mp3;
            }
            return true;
        }
    }
    return false;
}

// STUB: GAME 0x5b62c0
void C_game::DestroyIShow(int show)
{
    StubLog("C_game::DestroyIShow");
}

// The videos closed and released (PS2 name), and the lists emptied
// FUNCTION: GAME 0x5b63b0
void C_game::DestroyIShowAll()
{
    for (int i = i_shows.size(); i--;)
    {
        if (i_shows[i].show)
        {
            delete[] i_shows[i].name;
            i_shows[i].show->Close();
            i_shows[i].show->Release();
        }
    }
    i_shows.clear();
    i_show_fades.clear();
    film_music_streams.clear();
}

// The stream of the id, NULL for none (PS2 name), from the last one
// FUNCTION: GAME 0x5b64c0
IShow *C_game::GetIShow(int id)
{
    for (int i = i_shows.size(); i--;)
    {
        if (i_shows[i].id == id)
            return i_shows[i].show;
    }
    return NULL;
}

// The stream's own volume, which IShowSetVolume scales by the music's (PS2 name), from the last one
// FUNCTION: GAME 0x5b6520
void C_game::IShowStoreVolume(int show, float volume)
{
    for (int i = i_shows.size(); i--;)
    {
        if (i_shows[i].id == show)
        {
            i_shows[i].volume = volume;
            return;
        }
    }
}

// Whether Tick starts the stream again at its end (PS2 name): PS2's tells its stream too, the PC's IShow has no loop
// FUNCTION: GAME 0x5b6580
void C_game::IShowLoop(int show, bool loop)
{
    for (int i = i_shows.size(); i--;)
    {
        if (i_shows[i].id == show)
        {
            if (loop)
                i_shows[i].flags |= ISHOWF_LOOP;
            else
                i_shows[i].flags &= ~ISHOWF_LOOP;
            return;
        }
    }
}

// From the last video to the first
// FUNCTION: GAME 0x5b6600
void C_game::IShowSetVolume()
{
    for (int i = i_shows.size(); i--; )
        i_shows[i].show->SetVolume(mafiaLoadSave.settings.music_volume * i_shows[i].volume);
}

// STUB: GAME 0x5b6660
void C_game::IShowFadeDownAll(int time)
{
    StubLog("C_game::IShowFadeDownAll");
}

// STUB: GAME 0x5b6710
void C_game::IShowFade(int id, float from, float to, int time)
{
    StubLog("C_game::IShowFade");
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
    rb.active = false;
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

// STUB: GAME 0x5b6d10
bool C_game::IsValidTaxiPassenger()
{
    StubLog("C_game::IsValidTaxiPassenger");
    return false;
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

// STUB: GAME 0x5b7500
bool C_game::IsTelephoneExist(I3D_frame *frm)
{
    StubLog("C_game::IsTelephoneExist");
    return false;
}

// STUB: GAME 0x5b7560
void C_game::AddHumanTalk(C_human *human, int chanel)
{
    StubLog("C_game::AddHumanTalk");
}

// STUB: GAME 0x5b7730
void C_game::DelHumanTalk(C_human *human, int chanel)
{
    StubLog("C_game::DelHumanTalk");
}

// STUB: GAME 0x5b77c0
void C_game::DeathHumanTalk(C_human *human)
{
    StubLog("C_game::DeathHumanTalk");
}

// The base blocks' model sets of the traffic signs, dopravni1B.I3D to dopravni8B.I3D (PS2 name)
// FUNCTION: GAME 0x5b7860
void C_game::CreateSignalMarkIDX()
{
    signal_mark_idx.clear();
    if (!base_cache.IsOpen())
        return;
    for (ushort i = base_cache.model_sets.size(); i--;)
    {
        // STRING: GAME 0x637860
        if (!_stricmp(base_cache.model_sets[i].name, "dopravni1B.I3D"))
            signal_mark_idx.push_back(i);
        // STRING: GAME 0x637850
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni2B.I3D"))
            signal_mark_idx.push_back(i);
        // STRING: GAME 0x637840
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni3B.I3D"))
            signal_mark_idx.push_back(i);
        // STRING: GAME 0x637830
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni4B.I3D"))
            signal_mark_idx.push_back(i);
        // STRING: GAME 0x637820
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni5B.I3D"))
            signal_mark_idx.push_back(i);
        // STRING: GAME 0x637810
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni6B.I3D"))
            signal_mark_idx.push_back(i);
        // STRING: GAME 0x637800
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni7B.I3D"))
            signal_mark_idx.push_back(i);
        // STRING: GAME 0x6377f0
        else if (!_stricmp(base_cache.model_sets[i].name, "dopravni8B.I3D"))
            signal_mark_idx.push_back(i);
    }
}

// STUB: GAME 0x5b7be0
void C_game::DestroyControlSignal(g_collision_header *coll, S_vector &pos, S_vector &from, S_vector &dir,
    C_human *human)
{
    StubLog("C_game::DestroyControlSignal");
}

// FUNCTION: GAME 0x5b7ed0
void C_game::EntityBlock(bool block, bool unknown)
{
    int i;
    for (i = active_actors.size(); i--;)
    {
        if (active_actors[i]->type == ACTOR_ENTITY)
        {
            if (block)
                ((C_entity *)active_actors[i])->SedniNaPrdelASledujKino(unknown);
            else
                ((C_entity *)active_actors[i])->VzbudSeUzSouTitulky();
        }
    }
    for (i = inactive_actors.size(); i--;)
    {
        if (inactive_actors[i]->type == ACTOR_ENTITY)
        {
            if (block)
                ((C_entity *)inactive_actors[i])->SedniNaPrdelASledujKino(unknown);
            else
                ((C_entity *)inactive_actors[i])->VzbudSeUzSouTitulky();
        }
    }
}

// STUB: GAME 0x5b7f70
void C_game::AddInvisibleActor(C_actor *act)
{
    StubLog("C_game::AddInvisibleActor");
}

// STUB: GAME 0x5b8150
void C_game::FindActorsVect()
{
    StubLog("C_game::FindActorsVect");
}

// STUB: GAME 0x5b81e0
void C_game::AddCutsceneParticle(int particle)
{
    StubLog("C_game::AddCutsceneParticle");
}

// STUB: GAME 0x5b8390
void C_game::DestroyCutsceneParticles()
{
    StubLog("C_game::DestroyCutsceneParticles");
}

// Every actor's sounds off (PS2 name)
// FUNCTION: GAME 0x5b83f0
void C_game::StopAllSounds()
{
    for (int i = C_actor::global_actors.size(); i--;)
        C_actor::global_actors[i]->SoundsChange(0);
}

// STUB: GAME 0x5b8e70
void C_game::AddCreatedFrameToVector(I3D_frame *frm)
{
    StubLog("C_game::AddCreatedFrameToVector");
}

// STUB: GAME 0x5b9040
bool C_game::DelCreatedFrameToVector(I3D_frame *frm)
{
    StubLog("C_game::DelCreatedFrameToVector");
    return false;
}

// The areas where the enemies scream (PS2 name)
// FUNCTION: GAME 0x5b90b0
void C_game::LoadScreamAreas()
{
    scream_areas.clear();
    act_mission->scene->primary_sector->EnumFrames(S_hlp::cbScreamArea, (uint)&scream_areas, ENUMF_AREA, NULL);
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
            S_scream_area a;
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
            ((std::vector<S_scream_area> *)user)->push_back(a);
        }
    }
    return I3DENUMRET_OK;
}

// STUB: GAME 0x5b9390
void C_game::TickScreamAreas()
{
    StubLog("C_game::TickScreamAreas");
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

// STUB: GAME 0x5b98c0
void C_game::TickDeadHumanForTick(uint time)
{
    StubLog("C_game::TickDeadHumanForTick");
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
// FUNCTION: GAME 0x5b9fe0
void C_game::ScoreSet(int value)
{
    score = value;
    if (score_on)
        mafiaIndicators.BonusSetCount(score);
}

// STUB: GAME 0x5ba010
void C_game::SetCameraRotRepair()
{
    StubLog("C_game::SetCameraRotRepair");
}

// STUB: GAME 0x5ba080
void C_game::ClearCartridges()
{
    StubLog("C_game::ClearCartridges");
}

// STUB: GAME 0x5ba110
void C_game::TickCarSoundManager()
{
    StubLog("C_game::TickCarSoundManager");
}

// PS2 name
// FUNCTION: GAME 0x5ba310
void C_game::ClearDeathHumanFade()
{
    for (int i = death_human_fades.size(); i--;)
        RemoveDeathHumanFade(i, false);
    death_human_fades.clear();
    item_blur = false;
}

// STUB: GAME 0x5ba390
void C_game::ItemBlurTick(uint time)
{
    StubLog("C_game::ItemBlurTick");
}

// STUB: GAME 0x5baa40
void C_game::RemoveDeathHumanFade(int index, bool)
{
    StubLog("C_game::RemoveDeathHumanFade");
}

// STUB: GAME 0x5bab20
void C_game::RecPrepareHumans()
{
    StubLog("C_game::RecPrepareHumans");
}

// The roads out of the city, closed by boxes of collision while the city's base blocks are open: nine, and four more in
// the mission mise16-mesto (PS2's is C_mission's and another function)
// FUNCTION: GAME 0x5bab80
void C_game::InterCity_GameInit()
{
    inter_city_colls = NULL;
    inter_city_matrices = NULL;
    inter_city_inv_matrices = NULL;
    inter_city_frames = NULL;
    num_inter_city = 0;
    if (!base_cache.IsOpen())
        return;
    // STRING: GAME 0x63787c
    bool city = !_stricmp(act_mission->GetMissionName(), "mise16-mesto");
    num_inter_city = city ? 13 : 9;
    inter_city_colls = new tDynamicCollObject *[num_inter_city];
    inter_city_matrices = new S_matrix[num_inter_city];
    inter_city_inv_matrices = new S_matrix[num_inter_city];
    inter_city_frames = new I3D_frame *[num_inter_city];
    int n = 0;
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(561.601f, -0.364f, 13.733f), S_vector(-0.061f, -0.991f, -0.12f), 65.83f, S_vector(22.15f, 2.955f, 0.358f));
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(-764.324f, 5.911f, 466.544f), S_vector(0.0f, -1.0f, 0.0f), 179.445f, S_vector(1.604f, 3.066f, 0.347f));
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(-775.669f, 5.664f, 466.665f), S_vector(0.0f, -1.0f, 0.0f), 177.698f, S_vector(3.134f, 3.642f, 0.158f));
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(-780.123f, 5.041f, 466.292f), S_vector(0.0f, -1.0f, 0.0f), 179.445f, S_vector(3.138f, 3.066f, 0.347f));
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(-742.653f, 5.641f, 467.539f), S_vector(0.007f, 0.969f, 0.246f), 179.66f, S_vector(1.546f, 1.0f, 2.371f));
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(-735.396f, 5.641f, 462.545f), S_vector(0.245f, 0.942f, 0.23f), 94.16f, S_vector(1.546f, 1.0f, 2.371f));
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(-735.561f, 5.715f, 450.99f), S_vector(0.185f, 0.968f, 0.17f), 92.645f, S_vector(2.127f, 1.0f, 2.371f));
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(-735.349f, 6.378f, 450.0f), S_vector(0.185f, 0.968f, 0.17f), 92.645f, S_vector(0.28f, 1.0f, 2.371f));
    AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
        S_vector(-735.319f, 6.378f, 451.956f), S_vector(0.185f, 0.968f, 0.17f), 92.645f, S_vector(0.28f, 1.0f, 2.371f));
    if (city)
    {
        AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
            S_vector(-338.627f, 14.89f, 188.401f), S_vector(0.0f, -1.0f, 0.0f), 90.0f, S_vector(1.036f, 5.0f, 12.43f));
        AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
            S_vector(-351.433f, 13.947f, 188.491f), S_vector(0.0f, -1.0f, 0.0f), 90.0f, S_vector(1.0f, 5.0f, 13.34f));
        AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
            S_vector(-351.433f, 14.919f, 153.997f), S_vector(0.0f, -1.0f, 0.0f), 90.0f, S_vector(1.0f, 5.0f, 13.34f));
        AddInterCityBox(inter_city_colls, inter_city_matrices, inter_city_inv_matrices, &n, inter_city_frames,
            S_vector(-338.627f, 15.861f, 153.906f), S_vector(0.0f, -1.0f, 0.0f), 90.0f, S_vector(1.036f, 5.0f, 12.43f));
    }
}

// A box of collision at pos, turned by angle degrees around axis and scaled by scale, as a dummy frame of the unit box
// and a collision object of its matrix and inverse, added to the collisions as the n-th (the name is ours)
// STUB: GAME 0x5bb570
static void AddInterCityBox(tDynamicCollObject **colls, S_matrix *matrices, S_matrix *inv_matrices, int *n,
    I3D_frame **frames, S_vector pos, S_vector axis, float angle, S_vector scale)
{
    StubLog("AddInterCityBox");
}

// What InterCity_GameInit created: the collisions taken out and deleted, the frames released, the arrays deleted (PS2
// name, PS2's C_mission's)
// FUNCTION: GAME 0x5bbb20
void C_game::InterCity_GameDoneInGameDone()
{
    if (!inter_city_colls)
        return;
    for (int i = num_inter_city; i--;)
    {
        if (inter_city_colls[i])
        {
            gifCollision.DynErase(inter_city_colls[i]);
            delete inter_city_colls[i];
        }
        if (inter_city_frames[i])
            inter_city_frames[i]->Release();
    }
    num_inter_city = 0;
    if (inter_city_colls)
        delete[] inter_city_colls;
    inter_city_colls = NULL;
    if (inter_city_matrices)
        delete[] inter_city_matrices;
    inter_city_matrices = NULL;
    if (inter_city_inv_matrices)
        delete[] inter_city_inv_matrices;
    inter_city_inv_matrices = NULL;
    if (inter_city_frames)
        delete[] inter_city_frames;
    inter_city_frames = NULL;
}

// STUB: GAME 0x5accb0
int C_game::Play3DSound(I3D_sound *sound)
{
    StubLog("C_game::Play3DSound(I3D_sound *)");
    return 0;
}

// STUB: GAME 0x5afdb0
void C_game::InvalidateColl(tDynamicCollObject *coll)
{
    StubLog("C_game::InvalidateColl");
}

// STUB: GAME 0x5b5450
void C_game::LightnessAdd(C_actor *act, I3D_frame *frm, ushort unknown, S_vector &pos)
{
    StubLog("C_game::LightnessAdd");
}

// STUB: GAME 0x5b56f0
void C_game::LightnessDel(I3D_frame *frm)
{
    StubLog("C_game::LightnessDel");
}

// STUB: GAME 0x5ac580
void C_game::NewGrenade(int type, S_vector &pos, S_vector &dir, bool unknown, I3D_frame *frm)
{
    StubLog("C_game::NewGrenade");
}

// STUB: GAME 0x5b06c0
void C_game::WFall_Player()
{
    StubLog("C_game::WFall_Player");
}

// STUB: GAME 0x5b1370
void C_game::CreateTrackEffect(S_vector &pos, S_vector &normal, S_vector &dir, S_vector &size, float unknown1,
    int texture, bool unknown2, float unknown3, bool unknown4, bool unknown5, I3D_frame *frm)
{
    StubLog("C_game::CreateTrackEffect(direction)");
}

// STUB: GAME 0x5ad640
void C_game::SetFrameForShadow(I3D_frame *frm, E_shadow_type type1, E_shadow_type type2)
{
    StubLog("C_game::SetFrameForShadow");
}

// STUB: GAME 0x5b33f0
int C_game::BoxPlacementDestSetOU(bool on, I3D_frame *frm)
{
    StubLog("C_game::BoxPlacementDestSetOU");
    return 0;
}

// STUB: GAME 0x5b3b10
void C_game::BoxPlacement_RestoreInfo(I3D_frame *frm)
{
    StubLog("C_game::BoxPlacement_RestoreInfo");
}

// STUB: GAME 0x5b3c00
bool C_game::BoxPlacement_GetStartPoint(I3D_frame *frm, S_vector &pos)
{
    StubLog("C_game::BoxPlacement_GetStartPoint");
    return false;
}

// STUB: GAME 0x5b3c80
void C_game::BoxPlacementGetNextDest(I3D_frame *frm, I3D_frame *dest)
{
    StubLog("C_game::BoxPlacementGetNextDest");
}

// STUB: GAME 0x5b4010
I3D_frame *C_game::BoxPlacementGetEnemyDest(C_human *human, int index)
{
    StubLog("C_game::BoxPlacementGetEnemyDest");
    return NULL;
}

// STUB: GAME 0x5b5b40
void C_game::AddShootLight(S_vector &pos, float range, int time, ushort unknown)
{
    StubLog("C_game::AddShootLight");
}

// STUB: GAME 0x5b9690
void C_game::AddDeadHumanForTick(C_human *human)
{
    StubLog("C_game::AddDeadHumanForTick");
}
