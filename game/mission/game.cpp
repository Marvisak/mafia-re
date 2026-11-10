// The game of the loaded mission (PS2 game.cpp; the PC's file is the one at 0x5a0760-0x5bcc0c, docs/game-files.md),
// written whole: the game's Init, Done and Tick, the shots, explosions, fires and grenades, the tracks, the 3D sounds,
// the shadows, the film music and the videos, the diff data, the temporary actors, the cheats, the frame placements,
// the scripts' events, the stacks the player keeps between missions, the box placements, the saved game's record, the
// lightness and the lights, the talks, the scratches on the cars, the dead humans, the hydrants, the cars' sounds, the
// item blur and the inter-city boxes, in the original's order; PS2's names but where said.
// Open: the consoles' globals (0x647a74, 0x647ab0) are here for now; the templates after the file's last function
// (0x5bbbf0-0x5bcbc0) aren't annotated yet. Init's remaining differences are the inline budget (about 60 statements'
// worth of pending inline calls before its first anim cache Create, which its early erases and the push_back of
// I3D_frame pointers, 0x546ef0, the original calls out of line, would take) and with it its frame (0xe8, the
// original's 0xc8). cbLight, cbPauseSounds and TickCarSoundManager each lack one pending inline call after their
// push_back (the original calls its insert out of line). Tick's remaining differences are mostly registers and the
// inline budget: the original calls TickEffects' size() and the second clear()'s std::copy out of line, but inlines the
// second clear()'s erase and the city music's i_shows.size(). The speed (player_speed) is the Magnitude of a named
// difference, whose squares the original sums x, z, y (a nested Square() most likely, which the engine's Magnitude
// doesn't have), and the city music reads the area's two letters through a pointer it advances (add eax, 11), where
// ours folds the offset. TickShoot's remaining differences are registers and its sorted frame (frm at 0xc8, the
// original's 0x2c), and the order of the x87 loads in two Normalize copies and in the hole's position.

#include "mission/game.h"
#include "mission/Mission.h"
#include "mission/C_recorder.h"
#include "mission/C_effects.h"
#include "mission/C_program.h"
#include "mission/m_cache.h"
#include "actors/C_actor.h"
#include "actors/C_animations.h"
#include "actors/C_bridge.h"
#include "actors/C_cartridge.h"
#include "actors/C_crash_obj.h"
#include "actors/C_detector.h"
#include "actors/C_dog.h"
#include "actors/C_door.h"
#include "actors/C_grenade.h"
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
#include "vehicles/C_railway.h"
#include "vehicles/C_trf_c.h"
#include "vehicles/C_Vehicle.h"
#include "g_material.h"
#include "I3D_cache.h"
#include "stub.h"

#include <float.h>
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
static void AddCheatCarDetector();
// NewExplosion's, defined after it
static void _explosion(C_actor *act, float dist2, C_actor *exploder, S_vector &pos, float range, float power);

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
    for (int i = 0; _anim_info_[i].id != 0xffff; i++)
    {
        const char *name;
        if (strchr(_anim_info_[i].name, '?'))
        {
            char buf[128];
            strcpy(buf, _anim_info_[i].name);
            char *side = strchr(buf, '?');
            *side = 'L';
            S_hlp::PreloadAnim(buf);
            *side = 'R';
            name = buf;
        }
        else
            name = _anim_info_[i].name;
        S_hlp::PreloadAnim(name);
    }
    if (!C_animations_machine_de_luxe::base_track)
        C_animations_machine_de_luxe::base_track = C_animations_machine_de_luxe::tracks.size();
}

// The human the camera follows: the player or an entity, the camera given him too (PS2 name)
// FUNCTION: GAME 0x5a07e0
void C_game::SetHuman(C_actor *act)
{
    if (act && (act->type == ACTOR_PLAYER || act->type == ACTOR_ENTITY))
    {
        camera_human = (C_human *)act;
        camera.SetPlayer(camera_human);
    }
}

// PS2 has two bools, loads the mission's collisions itself and preloads more; false if it was done already
// FUNCTION: GAME 0x5a0810
bool C_game::Init()
{
    if (initialized)
        return false;
    mission_passed = false;
    car_sounds = 0;
    weather_on = act_mission->GetScene()->GetWeatherSystemParam(WS_PARAM_ON) != 0;
    weather_count = act_mission->GetScene()->GetWeatherSystemParam(WS_PARAM_COUNT);
    weather_buffer = -1;
    if (isound)
    {
        isound->SetVolume(0.0f);
        act_mission->GetScene()->Render();
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
    stacks_clear = true;
    death_fade_time = 7000;
    actor_lightness = false;
    carried_actors.clear();
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
        S_hlp::cbEnumSectors(act_mission->GetScene()->primary_sector);
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
    ActiveNewMission = false;
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
    std::vector<C_actor *> &actors = act_mission->GetActors();
    player = NULL;
    int i;
    for (i = actors.size(); i--;)
    {
        if (actors[i]->GetType() == ACTOR_PLAYER && !(actors[i]->GetActFlags() & ACTF_CUTSCENE))
        {
            player = (C_human *)actors[i];
            break;
        }
    }
    for (i = actors.size(); i--;)
    {
        if (actors[i]->GetType() == ACTOR_CAR)
            mafiaIndicators.RadarAddCar((C_car *)actors[i], (uint)-1);
    }
    I3D_camera *cam = act_mission->GetScene()->GetActiveCamera();
    player_pos = S_vector(0.0f, 0.0f, 0.0f);
    if (player)
    {
        I3D_frame *frm = player->GetFrame();
        if (frm)
            player_pos = frm->GetWorldPos();
    }
    else if (cam)
        player_pos = cam->GetWorldPos();
    player_speed = 0;
    // The camera behind the player
    if (player && cam)
    {
        I3D_frame *frm = player->GetFrame();
        S_vector pos = frm->GetPos();
        S_vector dir = frm->GetDir();
        cam->SetDir(dir, 0.0f);
        cam->SetPos(pos - dir * 2.0f + S_vector(0.0f, 2.0f, 0.0f));
        cam->Update();
    }
    camera_human = player;
    don_ocas = new C_don_ocas;
    C_traffic_generator::SetVisible(true);
    act_state_changed = false;
    C_traffic_generator::ClearElements();
    for (i = actors.size(); i--;)
    {
        if (actors[i]->GetType() == ACTOR_DOOR)
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
    for (i = 0; i < actors.size(); i++)
    {
        if (!act_mission->FrmActBuff_IsActiveActor(actors[i]))
            actors[i]->GameInit();
        else
            actors[i]->SetActState(ACTSTATE_ACTIVE);
        if (actors[i]->GetType() == ACTOR_CAR)
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
    traffic_timer.Reset(6000);
    SetActiveTrafficP(traffic_timer.period + 1);
    fires.clear();
    camera.Reset();
    camera.SetCamera(act_mission->GetScene()->GetActiveCamera());
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
    shadow_timer.Reset(823);
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
                p.frame->SetOn(false);
                ((I3D_projector *)p.frame)->SetMode(PROJECTOR_MODE_BLEND);
                ((I3D_projector *)p.frame)->SetOrthogonal(true);
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
                p.frame->SetOn(false);
                ((I3D_projector *)p.frame)->SetMode(PROJECTOR_MODE_BLEND);
                ((I3D_projector *)p.frame)->SetOrthogonal(true);
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
                    if (vis)
                    {
                        for (i = 0; i < 32; i++)
                        {
                            I3D_frame *obj = driver->CreateVisual(I3D_VISUAL_OBJECT);
                            if (!obj)
                                continue;
                            obj->Duplicate(vis);
                            C_stone *stone = new C_stone;
                            if (stone->Init(obj))
                            {
                                obj->SetOn(false);
                                stone->GameInit();
                                stones[i] = stone;
                            }
                            else
                                stone->Release();
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
                            if (c->Init(obj))
                            {
                                c->GameInit();
                                cartridges[i] = c;
                            }
                            else
                                c->Release();
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
                            if (c->Init(obj))
                            {
                                c->GameInit();
                                cartridges_b[i] = c;
                            }
                            else
                                c->Release();
                        }
                    }
                }
                mod->Release();
            }
        }
        _show_progress_(.83f);

        // The holes: a dUra_fiRe of each model, and the copies that are shown
        hole_models.clear();
        for (i = 0; i < 3; i++)
        {
            I3D_model *mod = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            const char *n = hole_model_names[i];
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
            if (model_cache.Open(sheet, "9sheet.i3d", 0, NULL, NULL, NULL) != I3D_OK)
            {
                sheet->Release();
                sheet = NULL;
            }
            else
            {
                C_actor::DebustizeFrame(sheet);
                C_actor::NoShadowFrame(sheet);
            }
        }
        money = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        if (money)
        {
            // STRING: GAME 0x636158
            if (model_cache.Open(money, "9money.i3d", 0, NULL, NULL, NULL) != I3D_OK)
            {
                money->Release();
                money = NULL;
            }
            else
            {
                C_actor::DebustizeFrame(money);
                C_actor::NoShadowFrame(money);
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
        AddCheatCarDetector();
    }
    initialized = true;
    OsefujPocasi(false, (uint)-1);
    if (player)
    {
        ((C_player *)player)->fade_alpha = 1.0f;
        ((C_player *)player)->fade_target = 1.0f;
        ((C_player *)player)->SetAlpha(1.0f);
    }
    item_blurs.clear();
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

// The developers' car (the name is ours): a detector in the city, a box of 1 x 1 x 1 at -1517, 9.5, -294.7, whose script
// waits for a hit, fades the screen out and runs DAN_INTERNAL_1, which puts a Thunderbird at a place of the city (see
// C_program::Process). The detector owns its frame and is the game's temporary actor
// FUNCTION: GAME 0x5a3990
static void AddCheatCarDetector()
{
    I3D_dummy *frm = (I3D_dummy *)driver->CreateFrame(FRAME_DUMMY);
    if (!frm)
        return;
    C_detector *det = new C_detector;
    if (!det)
    {
        frm->Release();
        return;
    }
    frm->SetBBox(I3D_bbox(S_vector(-.5f, -.5f, -.5f), S_vector(.5f, .5f, .5f)));
    frm->SetPos(-1517.0f, 9.5f, -294.7f);
    frm->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
    frm->SetScale(S_vector(1.0f, 1.0f, 1.0f));
    frm->Update();
    det->Init(frm);
    // STRING: GAME 0x6376f0
    det->SetSourceCode("Detector_WaitForHit\r\nZatmySe 0, 500\r\nDan_Internal_1\r\nEnd\r\n");
    det->GameInit();
    det->own_frame = true;
    act_mission->GetGame()->InternTempActAdd(det);
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
    ItemBlurDone();
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
        camera.Unlock(false);
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
    for (i = 0; i < (int)carried_actors.size();)
    {
        if (carried_actors[i]->type == ACTOR_CAR && mafiaLoadSave.IsCarIn((C_car *)carried_actors[i]))
        {
            carried_actors[i] = carried_actors.back();
            carried_actors.erase(carried_actors.end() - 1);
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
    for (i = carried_actors.size(); i--;)
    {
        act_mission->FrmActBuff_RemoveActiveActor(carried_actors[i]);
        if (carried_actors[i]->type == ACTOR_CAR)
            carried_actors[i]->own_frame = false;
        act_mission->DelActor(carried_actors[i]);
    }
    carried_actors.clear();
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
            else if (l.flags & SHOOTLIGHTF_FLICKER)
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
        I3D_light *lt = light_cache[i];
        float r = (lt->far_range + range + 15.0f) * (lt->far_range + range + 15.0f);
        if (r > dir.Square())
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
                city_music = CreateIShow(ISHOWF_CITY_MUSIC);
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

// A temporary actor taken into the game (PS2 name): its GameInit, then the list of its state and temporary_actors,
// once
// FUNCTION: GAME 0x5a77c0
void C_game::AddTemporaryActor(C_actor *act)
{
    for (int i = temporary_actors.size(); i--;)
    {
        if (temporary_actors[i] == act)
            return;
    }
    act->GameInit();
    if (act->act_state == ACTSTATE_ACTIVE)
        active_actors.push_back(act);
    else if (act->act_state == ACTSTATE_INACTIVE)
        inactive_actors.push_back(act);
    temporary_actors.push_back(act);
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

// The same without the GameInit, for an actor already in the game (PS2 name)
// FUNCTION: GAME 0x5a7b70
void C_game::InternTempActAdd(C_actor *act)
{
    for (int i = temporary_actors.size(); i--;)
    {
        if (temporary_actors[i] == act)
            return;
    }
    if (act->act_state == ACTSTATE_ACTIVE)
        active_actors.push_back(act);
    else if (act->act_state == ACTSTATE_INACTIVE)
        inactive_actors.push_back(act);
    temporary_actors.push_back(act);
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

// A temporary actor removed for good (PS2 name; RemoveTemporaryActor's list, which Tick empties through it): taken from
// temporary_actors, out of everything that refers to it, ended and released
// FUNCTION: GAME 0x5a7ec0
void C_game::_RemoveTemporaryActor(C_actor *act)
{
    int index = -1;
    for (int i = temporary_actors.size(); i--;)
    {
        if (temporary_actors[i] == act)
        {
            index = i;
            break;
        }
    }
    if (index < 0)
        return;
    temporary_actors[index] = temporary_actors.back();
    temporary_actors.pop_back();
    InvalidateActor(act, false);
    act->GameDone();
    act->Release();
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

// A cheat's word and its number, the case of CheatKey's switch (PC only, the names are ours): the word's letters upper
// case and complemented, as CheatKey keeps the keys typed. POLIFUCK's and ROZMAZ's numbers have no case.
#define CHEAT_BOXER 4
#define CHEAT_CORNUFSE 7
#define CHEAT_POLIFUCK 8
#define CHEAT_PARNIK 0xb
#define CHEAT_JIZDA 0xc
#define CHEAT_KREFJEZIVOT 0xd
#define CHEAT_MUNICAK 0xe
#define CHEAT_ROZMAZ 0xf
#define CHEAT_CUMDAL 0x10
struct S_cheat_word
{
    char word[0x20];
    int cheat;                          // CHEAT_*
};

// The cheats' words, which CheatKey searches from the last
// GLOBAL: GAME 0x637000
static S_cheat_word cheat_words[] = {
    {{~'B', ~'O', ~'X', ~'E', ~'R'}, CHEAT_BOXER},
    {{~'P', ~'A', ~'R', ~'N', ~'I', ~'K'}, CHEAT_PARNIK},
    {{~'J', ~'I', ~'Z', ~'D', ~'A'}, CHEAT_JIZDA},
    {{~'K', ~'R', ~'E', ~'F', ~'J', ~'E', ~'Z', ~'I', ~'V', ~'O', ~'T'}, CHEAT_KREFJEZIVOT},
    {{~'M', ~'U', ~'N', ~'I', ~'C', ~'A', ~'K'}, CHEAT_MUNICAK},
    {{~'P', ~'O', ~'L', ~'I', ~'F', ~'U', ~'C', ~'K'}, CHEAT_POLIFUCK},
    {{~'C', ~'O', ~'R', ~'N', ~'U', ~'F', ~'S', ~'E'}, CHEAT_CORNUFSE},
    {{~'R', ~'O', ~'Z', ~'M', ~'A', ~'Z'}, CHEAT_ROZMAZ},
    {{~'C', ~'U', ~'M', ~'D', ~'A', ~'L'}, CHEAT_CUMDAL},
};

// A key typed, for the cheats' words (PC only, the name is ours; GameLoopSingle's): it is added to cheat_keys, which
// are cleared once they begin no word, and a word typed whole does its cheat. True where the key was a cheat's, which
// clears the controls' input: a word typed whole, or two letters or more of one.
// FUNCTION: GAME 0x5a8030
bool C_game::CheatKey(char key)
{
    if (strlen(cheat_keys) >= sizeof(cheat_keys) - 1)
        strcpy(cheat_keys, "");
    if (key >= 'a' && key <= 'z')
        key += 'A' - 'a';
    int len = strlen(cheat_keys);
    cheat_keys[len++] = ~key;
    cheat_keys[len] = 0;
    // The word the keys begin: its number where they are all of it, -2 where they are less, -1 for none
    int cheat = -1;
    for (int i = sizeof(cheat_words) / sizeof(cheat_words[0]); i--;)
    {
        int word_len = strlen(cheat_words[i].word);
        if (!memcmp(cheat_keys, cheat_words[i].word, min(len, word_len)))
        {
            cheat = word_len == len ? cheat_words[i].cheat : -2;
            break;
        }
    }
    if (cheat == -1)
    {
        strcpy(cheat_keys, "");
        return false;
    }
    if (cheat < 0)
        return strlen(cheat_keys) > 1;
    mafiaIndicators.FadeInOut(false, 500, 0);
    strcpy(cheat_keys, "");
    switch (cheat)
    {
    case CHEAT_BOXER:
        if (cheats & GAMEF_BOXER)
            cheats &= ~GAMEF_BOXER;
        else
            cheats |= GAMEF_BOXER;
        if (player)
            ((C_player *)player)->SetBoxer((bool)(cheats & GAMEF_BOXER));
        break;
    case CHEAT_CORNUFSE:
        if (cheats & GAMEF_STEAL_ALL)
            cheats &= ~GAMEF_STEAL_ALL;
        else
            cheats |= GAMEF_STEAL_ALL;
        break;
    case CHEAT_PARNIK:
        {
            if (cheats & GAMEF_TITANIC)
                cheats &= ~GAMEF_TITANIC;
            else
                cheats |= GAMEF_TITANIC;
            std::vector<C_actor *> &actors = act_mission->game->active_actors;
            for (int i = actors.size(); i--;)
            {
                if (actors[i]->type == ACTOR_BRIDGE)
                    ((C_bridge *)actors[i])->SetTitanic((bool)(cheats & GAMEF_TITANIC));
            }
        }
        break;
    case CHEAT_JIZDA:
        if (player && player->car && player->car->type == ACTOR_CAR)
            C_car::OdjebajTuStrechu((I3D_model *)player->car->frame, true);
        break;
    case CHEAT_KREFJEZIVOT:
        if (heal_cheat && player)
        {
            player->properties[HUMAN_PROPERTY_ENERGY] = player->max_properties[HUMAN_PROPERTY_ENERGY];
            player->properties[HUMAN_PROPERTY_ENERGY_HAND_L] = player->max_properties[HUMAN_PROPERTY_ENERGY_HAND_L];
            player->properties[HUMAN_PROPERTY_ENERGY_HAND_R] = player->max_properties[HUMAN_PROPERTY_ENERGY_HAND_R];
            player->properties[HUMAN_PROPERTY_ENERGY_LEG_L] = player->max_properties[HUMAN_PROPERTY_ENERGY_LEG_L];
            player->properties[HUMAN_PROPERTY_ENERGY_LEG_R] = player->max_properties[HUMAN_PROPERTY_ENERGY_LEG_R];
            mafiaIndicators.PlayerSetLives(100);
            heal_cheat = false;
        }
        break;
    case CHEAT_MUNICAK:
        if (cheats & GAMEF_INFINITE_AMMO)
            cheats &= ~GAMEF_INFINITE_AMMO;
        else
            cheats |= GAMEF_INFINITE_AMMO;
        break;
    case CHEAT_CUMDAL:
        // Only in 32 bits, with the city's blocks open: the camera sees 600 m, or the options' distance again
        if (setting.bit_depth == 32 && base_cache.open)
        {
            if (cheats & GAMEF_FAR_BLOCKS)
                cheats &= ~GAMEF_FAR_BLOCKS;
            else
                cheats |= GAMEF_FAR_BLOCKS;
            if ((bool)(cheats & GAMEF_FAR_BLOCKS))
            {
                I3D_camera *cam = act_mission->scene->GetActiveCamera();
                if (cam)
                {
                    float near_range, far_range;
                    cam->GetRange(near_range, far_range);
                    cam->SetRange(near_range, 600.0f);
                    cam->Update();
                }
            }
            else
                act_mission->options.SetDrawDistance(act_mission->options.GetDrawDistance());
            base_cache.cam_pos = S_vector(0.0f, -1000.0f, 0.0f);
            base_cache.timer = 10000;
            base_cache.Tick(1);
        }
        break;
    }
    return true;
}

// The traffic's pedestrians and cars and the trams shown or hidden (PS2 name; the script's)
// FUNCTION: GAME 0x5a8470
void C_game::SetTrafficVisible(bool visible)
{
    C_traffic_generator::SetVisible(visible);
    visible = !visible;
    C_traffic_car::Scipni(visible);
    C_railway::salina_v_kine = visible;
}

// The distance from the plane through p with the normal n (Insanity3D's, as VE_BreakageGlass.cpp's, which defines it
// for its own use): NewShoot inlines it, its first Dot too, and calls the second; its copy follows its first out-of-line
// caller, CreateTrackEffect
// FUNCTION: GAME 0x5b1330
inline float S_vector::DistanceToPlane(const S_vector &p, const S_vector &n) const
{
    return Dot(n) - p.Dot(n);
}

// A shot from pos along dir, as far as dir's length, which TickShoot moves (PS2 name): one that passes the camera
// more than 12 m from pos and nearer than 10 m to it turns on the free whizzing sounds (delayed_sounds) at the point
// it passes, each after the time the shot takes to get there (0.3 m a ms), with the in game sounds at High. With count
// above 1 (a shotgun) it is count shots of power / count each, their directions scattered around dir by up to a
// twentieth of its length up and to the side. Each shot is tested against the level's static collision: the
// collision it hits gives its end, its sound and its material, and a glass or crash collision with COLLHF_UNKNOWN_40
// or COLLHF_UNKNOWN_80 its frame. Then the muzzle's particle on frm, the sound the humans hear and the pedestrians'
// panic.
// FUNCTION: GAME 0x5a84a0
bool C_game::NewShoot(C_actor *actor, S_vector pos, S_vector dir, float power, int particle, I3D_frame *frm,
    int count)
{
    S_shoot s;
    s.pos = pos;
    s.curr_pos = pos;
    s.power = power;
    s.actor = actor;
    s.frame = NULL;
    s.range = dir.Magnitude();
    s.sound = -1;
    s.material = 0;
    S_vector n = dir;
    n.Normalize();

    // The whizzing past the camera
    if (act_mission->GetOptions().game_sounds >= OPTION_HIGH)
    {
        S_vector cam = act_mission->scene->GetActiveCamera()->GetWorldPos();
        float t = cam.DistanceToPlane(pos, n);
        if (t > 12.0f)
        {
            S_vector p = pos + n * t;
            if ((p - cam).Square() < 100.0f)
            {
                for (int i = 3; i--;)
                {
                    if (delayed_sounds[i].sound && delayed_sounds[i].time < 0 &&
                        !delayed_sounds[i].sound->IsPlaying(false))
                    {
                        delayed_sounds[i].time = (int)(t * 3.3333f);
                        delayed_sounds[i].sound->SetPos(p);
                        delayed_sounds[i].sound->Update();
                        delayed_sounds[i].sound->LinkTo(NULL, 0);
                        act_mission->scene->SetFrameSector(delayed_sounds[i].sound);
                    }
                }
            }
        }
    }

    if (count > 1)
    {
        // A shotgun's shots, scattered
        s.power /= count;
        S_vector d = n;
        S_vector up(d.z, d.y, -d.x);
        S_vector side = d.Cross(up);
        up = side.Cross(d);
        float len = dir.Magnitude();
        d *= len;
        len *= .05f;
        side *= len;
        up *= len;
        for (int i = 0; i < count; i++)
        {
            dir = d + side * (rnd() * 2.0f - 1.0f) + up * (rnd() * 2.0f - 1.0f);
            s.coll = (g_collision_header *)gifCollision.TestLineHStatic(pos, dir, &s.hit_pos, &s.hit_normal,
                COLF2_UNKNOWN_200000 | COLF2_UNKNOWN_400000);
            s.hit = s.coll ? true : false;
            if (s.coll)
            {
                s.sound = materialList[s.coll->material].shot_sound;
                s.material = s.coll->material;
                s.dir = s.hit_pos - pos;
                // Glass and crash collisions keep their frame
                int mat = s.coll->material;
                if ((mat == MAT_GLASS || mat == MAT_CRASH) &&
                    (s.coll->flags & (COLLHF_UNKNOWN_40 | COLLHF_UNKNOWN_80)))
                    s.frame = s.coll->frame;
            }
            else
                s.dir = dir;
            shoots.push_back(s);
        }
    }
    else
    {
        s.coll = (g_collision_header *)gifCollision.TestLineHStatic(pos, dir, &s.hit_pos, &s.hit_normal,
            COLF2_UNKNOWN_200000 | COLF2_UNKNOWN_400000);
        s.hit = s.coll ? true : false;
        if (s.coll)
        {
            s.sound = materialList[s.coll->material].shot_sound;
            s.material = s.coll->material;
            s.dir = s.hit_pos - pos;
            // Glass and crash collisions keep their frame
            int mat = s.coll->material;
            if ((mat == MAT_GLASS || mat == MAT_CRASH) &&
                (s.coll->flags & (COLLHF_UNKNOWN_40 | COLLHF_UNKNOWN_80)))
                s.frame = s.coll->frame;
        }
        else
            s.dir = dir;
        shoots.push_back(s);
    }

    // The muzzle's particle, the sound the humans hear, the pedestrians' panic
    if (particle > 0 && act_mission->GetOptions().particles >= OPTION_MEDIUM)
        act_mission->GetEffects()->CreateParticle(particle, S_vector(0.0f, 0.0f, 0.0f), S_vector(0.0f, 0.0f, 1.0f), 0,
            frm);
    sound_arena.SoundEmit(pos, 50.0f, 1.0f, 2, actor);
    C_traffic_generator::NewPanic(pos);
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

// An explosion of actor at pos (PS2 name; EXPLOSION's, a car's, a crash object's): with damage every other active and
// inactive actor hit (the traffic's cars and actors of ACTOR_UNKNOWN_B whatever the distance, the others within range,
// _explosion), the pedestrians' (C_traffic_generator::Explosion), the level's glass within range * .3 broken and its
// crash collisions there broken off; then its particle (big's the larger one), with the in game effects at High and
// smoke 8 to 13 smoke trails, its track, the pedestrians' panic, its sound (the sound table's) and its light.
// FUNCTION: GAME 0x5aae10
void C_game::NewExplosion(C_actor *actor, S_vector &pos, float range, float power, bool damage, bool big, bool smoke,
    int sound)
{
    float range2 = range * range;
    if (damage)
    {
        int i;
        for (i = active_actors.size(); i--;)
        {
            if (active_actors[i] == actor)
                continue;
            if (active_actors[i]->type == ACTOR_TRAFFIC_CAR || active_actors[i]->type == ACTOR_UNKNOWN_B)
                active_actors[i]->Hit(HIT_EXPLOSION, pos, pos, S_vector(range, range, range), power, actor, 0, NULL);
            else
            {
                float dist2 = (active_actors[i]->frame->GetWorldPos() - pos).Square();
                if (dist2 < range2)
                    _explosion(active_actors[i], dist2, actor, pos, range, power);
            }
        }
        for (i = inactive_actors.size(); i--;)
        {
            if (inactive_actors[i] == actor)
                continue;
            if (inactive_actors[i]->type == ACTOR_TRAFFIC_CAR || inactive_actors[i]->type == ACTOR_UNKNOWN_B)
                inactive_actors[i]->Hit(HIT_EXPLOSION, pos, pos, S_vector(range, range, range), power, actor, 0, NULL);
            else
            {
                float dist2 = (inactive_actors[i]->frame->GetWorldPos() - pos).Square();
                if (dist2 < range2)
                    _explosion(inactive_actors[i], dist2, actor, pos, range, power);
            }
        }
        C_traffic_generator::Explosion(pos, range, power);

        // The level's glass near it broken
        std::vector<g_collision_header *> colls;
        float r = range * .3f;
        gifCollision.FindStaticCollsInLocality(pos, r, MAT_GLASS, MAT_GLASS, colls);
        int j;
        for (j = colls.size(); j--;)
        {
            if (colls[j]->frame)
            {
                S_vector dir = colls[j]->frame->GetWorldPos() - pos;
                dir.Normalize();
                dir *= 2.0f;
                colls[j]->flags |= COLLHF_DISABLED;
                glass.BreakGlass(colls[j]->frame, &dir, "lustr1.wav", true);
            }
        }

        // The crash collisions near it broken off, with power less by the distance
        colls.clear();
        gifCollision.FindStaticCollsInLocality(pos, r, MAT_CRASH, MAT_CRASH, colls);
        for (j = colls.size(); j--;)
        {
            if (colls[j]->frame && !(colls[j]->flags & COLLHF_DISABLED))
            {
                S_vector dir = colls[j]->frame->GetWorldPos() - pos;
                dir.Normalize();
                dir *= 5.0f;
                float f = 1.0f - (pos - colls[j]->frame->GetWorldPos()).Magnitude() / r;
                if (f < 0.0f)
                    f = 0.0f;
                f *= power;
                CreateCrashModel(colls[j]->frame, pos, dir, f);
            }
        }
    }

    act_mission->GetEffects()->CreateParticle(big ? PARTICLE_EXPLOSION_BIG : PARTICLE_EXPLOSION, pos,
        S_vector(0.0f, 0.0f, 1.0f), 0, NULL);
    if (act_mission->GetOptions().game_effects >= OPTION_HIGH && smoke)
    {
        int n = rnd(6) + 8;
        for (int i = 0; i < n; i++)
            NewSmokeTrail(pos, 10.0f, 15.0f);
    }
    CreateTrackEffect_Grenade(pos);
    C_traffic_generator::NewPanic(pos);
    if (act_mission->GetOptions().game_sounds >= OPTION_MEDIUM)
    {
        I3D_sound *snd = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        if (snd)
        {
            snd->SetPos(pos);
            snd->Update();
            if (sound_table.ConstructSound(snd, sound))
                act_mission->GetGame()->Play3DSound(snd);
            else
                snd->Release();
        }
    }
    AddShootLight(pos, 12.0f, 100, 0);
}

// An actor in an explosion's range hit by it (PS2 name): unless the level's static collision is between the actor's
// frame and the explosion, and between each of its visuals and joints and the explosion, it is hit along the
// direction from the explosion with power less by the distance (none at range). exploder isn't used.
// FUNCTION: GAME 0x5ab680
static void _explosion(C_actor *act, float dist2, C_actor *exploder, S_vector &pos, float range, float power)
{
    S_vector dir = act->frame->GetWorldPos() - pos;
    S_vector hit_pos, hit_normal;
    int col = gifCollision.TestLineHStatic(pos, dir, &hit_pos, &hit_normal, 0);
    if (col)
    {
        I3D_frame *frm = act->frame;
        int n = frm->NumChildren();
        for (int i = 0; i < n; i++)
        {
            I3D_frame *child = frm->GetChild(i);
            if (child->GetType() == FRAME_VISUAL || child->GetType() == FRAME_JOINT)
            {
                col = gifCollision.TestLineHStatic(pos, child->GetWorldPos() - pos, &hit_pos, &hit_normal, 0);
                if (!col)
                    break;
            }
        }
        if (col)
            return;
    }
    float f = 1.0f - (float)sqrt(dist2) / range;
    if (f < 0.0f)
        f = 0.0f;
    dir.Normalize();
    act->Hit(HIT_EXPLOSION, dir, pos, pos, f * power, NULL, 0, NULL);
}

// A piece thrown from an explosion at pos (PS2 name; NewExplosion's smoke trails): the game's next stone (num_stones,
// 2stone.i3d) unless it flies already, thrown upwards in a random direction at a random speed from min_speed to
// max_speed, as a temporary actor with a reference more on it and its frame
// FUNCTION: GAME 0x5ab8f0
void C_game::NewSmokeTrail(S_vector &pos, float min_speed, float max_speed)
{
    if (!stones[num_stones])
        return;
    for (int i = temporary_actors.size(); i--;)
    {
        if (temporary_actors[i] == stones[num_stones])
            return;
    }
    S_vector dir(rnd() * 2.0f - 1.0f, rnd() + .3f, rnd() * 2.0f - 1.0f);
    if (dir.y < .4f)
        dir.y += .4f;
    dir.Normalize();
    dir *= rnd() * (max_speed - min_speed) + min_speed;
    AddTemporaryActor(stones[num_stones]);
    stones[num_stones]->ref++;
    stones[num_stones]->frame->m_RefCount++;
    stones[num_stones]->frame->SetOn(true);
    stones[num_stones]->SetPos(pos);
    stones[num_stones]->AbsoluteImpulse(dir);
    if (++num_stones >= MAX_STONES)
        num_stones = 0;
}

// count small fires around pos, with the particles at High (PS2 name): each where a line from half a random direction
// (upwards, from .1 to 2.1 long) away from pos along that direction hits the level, or else where the line from its
// end down and five times as far does; none where neither hits
// FUNCTION: GAME 0x5abb90
void C_game::NewSmallFires(S_vector &pos, int count)
{
    if (act_mission->GetOptions().particles >= OPTION_HIGH)
    {
        while (count--)
        {
            S_vector dir(rnd() * 2.0f - 1.0f, .5f, rnd() * 2.0f - 1.0f);
            dir.Normalize();
            dir *= rnd() * 2.0f + .1f;
            S_vector from = pos + dir * .5f;
            S_vector hit_pos, hit_normal;
            if (!gifCollision.TestLineV(from, dir, &hit_pos, &hit_normal, COLF_ALL, 0))
            {
                // Missed: from the end of the line, down and farther
                from += dir;
                dir.y *= -1.0f;
                dir *= 5.0f;
                if (!gifCollision.TestLineV(from, dir, &hit_pos, &hit_normal, COLF_ALL, 0))
                    continue;
            }
            act_mission->GetEffects()->CreateParticle(PARTICLE_SMALL_FIRE, hit_pos, hit_normal, 0, NULL);
        }
    }
}

// A fire at pos, in frm's space where frm isn't NULL (PS2 name): a C_fire (fires), its light for time ms, its track,
// its sound and the pedestrians' panic
// FUNCTION: GAME 0x5abe10
void C_game::NewFire(S_vector &pos, I3D_frame *frm, int time, float range, float damage, bool hurts)
{
    S_vector p = pos;
    if (frm)
        p *= frm->GetMatrix();
    C_fire *fire = new C_fire;
    fire->Init(pos, frm, time, range, damage, hurts);
    AddShootLight(p, 10.0f, time, SHOOTLIGHTF_FLICKER);
    fires.push_back(fire);
    CreateTrackEffect_Grenade(p);
    if (act_mission->GetOptions().game_sounds >= OPTION_MEDIUM)
    {
        I3D_sound *snd = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        if (snd)
        {
            snd->SetPos(p);
            snd->Update();
            if (sound_table.ConstructSound(snd, SOUND_FIRE))
                act_mission->GetGame()->Play3DSound(snd);
            else
                snd->Release();
        }
    }
    C_traffic_generator::NewPanic(p);
}

// A cartridge case thrown out at the frame (PS2 name; the parameters' names are ours): DoubleKiss's guns and C_human's
// shots throw one out of the weapon, a shotgun's shell (cartridges_b's) for a close-range weapon; the revolver's
// cylinder drops its six along dir. The next case of the two arrays (they wrap) taken into the game's ticks at the
// frame's position, thrown along dir where it drops, else the frame's direction, a little at random. Not with the
// game's effects low.
// FUNCTION: GAME 0x5abfb0
void C_game::NewCartidge(I3D_frame *&frm, S_vector &dir, bool shell, bool drop)
{
    if (act_mission->GetOptions().game_effects < OPTION_MEDIUM)
        return;
    if (shell) {
        C_cartridge *&cartridge = cartridges_b[num_cartridges_b];
        if (!cartridge)
            return;
        AddTemporaryActor(cartridge);
        cartridges_b[num_cartridges_b]->SetShotgunShell(shell);
        cartridges_b[num_cartridges_b]->ref++;
        cartridges_b[num_cartridges_b]->SetPos(frm->GetWorldPos());
        cartridges_b[num_cartridges_b]->SetDir(dir);
        S_vector v;
        if (drop)
            v = dir;
        else
            v = frm->GetWorldDir();
        S_vector r;
        r.x = rnd();
        r.y = rnd();
        r.z = rnd();
        v += r * .3f;
        v *= 3.0f;
        cartridges_b[num_cartridges_b]->AbsoluteImpulse(v);
        if (++num_cartridges_b >= 30)
            num_cartridges_b = 0;
    } else {
        C_cartridge *&cartridge = cartridges[num_cartridges];
        if (!cartridge)
            return;
        AddTemporaryActor(cartridge);
        cartridges[num_cartridges]->SetShotgunShell(shell);
        cartridges[num_cartridges]->ref++;
        cartridges[num_cartridges]->SetPos(frm->GetWorldPos());
        cartridges[num_cartridges]->SetDir(dir);
        S_vector v;
        if (drop)
            v = dir;
        else
            v = frm->GetWorldDir();
        S_vector r;
        r.x = rnd();
        r.y = rnd();
        r.z = rnd();
        v += r * .3f;
        v *= 3.0f;
        cartridges[num_cartridges]->AbsoluteImpulse(v);
        if (++num_cartridges >= 100)
            num_cartridges = 0;
    }
}

// A grenade of the type thrown from pos (PS2 name; the parameters' names are ours): the model frm, or else the type's
// own, loaded through the model cache (NULL for another type); put at pos, upright and unscaled, into its sector, and a
// new C_grenade of it taken into the game's ticks. Thrown, it flies along dir, and its type sets it to go off: the
// grenade 5 s after the throw and the cheat Magnum's shot at its first hit, exploding, the incendiary 5 s after the
// throw and the Molotov cocktail at its first hit, setting a fire. The grenade, NULL if it fails. It reaches the game
// through act_mission, as the actors do. The ifs nest to one return: the failed Init's grenade is NULL there.
// Open: the original calls the inlined Open's C_str conversion out of line, as one more pending inline call anywhere
// after it gives, but then ours moves the folders' loop out of line, where the original keeps it in place; and the
// registers (the type in ebp, reloaded after the Open, ours eax; the name in its dead parameter's slot, ours ebp).
// FUNCTION: GAME 0x5ac580
C_grenade *C_game::NewGrenade(int type, S_vector &pos, S_vector &dir, bool thrown, I3D_frame *frm)
{
    if (!frm) {
        const char *name;
        switch (type) {
        case NEWGRENADE_GRENADE:
        case NEWGRENADE_SHOT:
            // STRING: GAME 0x637774
            name = "2grenade.i3d";
            break;
        case NEWGRENADE_INCENDIARY:
        case NEWGRENADE_MOLOTOV:
            // STRING: GAME 0x637768
            name = "2mol.i3d";
            break;
        default:
            return NULL;
        }
        frm = driver->CreateFrame(FRAME_MODEL);
        if (!frm)
            return NULL;
        if (model_cache.Open((I3D_model *)frm, name, 0, NULL, NULL, NULL) != I3D_OK) {
            frm->Release();
            return NULL;
        }
    }
    C_actor::DebustizeFrame(frm);
    C_actor::NoShadowFrame(frm);
    frm->SetPos(pos);
    frm->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
    frm->SetScale(S_vector(1.0f, 1.0f, 1.0f));
    frm->Update();
    act_mission->scene->SetFrameSector(frm);
    C_grenade *grenade = new C_grenade;
    if (grenade) {
        if (grenade->Init(frm)) {
            act_mission->game->AddTemporaryActor(grenade);
            if (thrown) {
                grenade->AbsoluteImpulse(dir);
                switch (type) {
                case NEWGRENADE_GRENADE:
                    grenade->timer = true;
                    grenade->fuse_time = 5000;
                    grenade->explode = true;
                    grenade->explosion_radius = 15.0f;
                    grenade->explosion_power = 400.0f;
                    break;
                case NEWGRENADE_SHOT:
                    grenade->impact = true;
                    grenade->explode = true;
                    grenade->explosion_radius = 15.0f;
                    grenade->explosion_power = 400.0f;
                    break;
                case NEWGRENADE_INCENDIARY:
                    grenade->timer = true;
                    grenade->fuse_time = 5000;
                    grenade->SetFire(true, 5000, 2.5f, 50.0f);
                    break;
                case NEWGRENADE_MOLOTOV:
                    grenade->impact = true;
                    grenade->SetFire(true, 5000, 2.5f, 50.0f);
                    break;
                }
            }
        } else {
            grenade->Release();
            grenade = NULL;
        }
    }
    return grenade;
}

// A sound of the sound cache played at the frame (in the primary sector without one) at pos, with its type, ranges and
// volume (PS2 name, the vector by value, PS2's by reference): its number, which Stop3DSound takes, -1 if it can't be
// created or opened. Its cone is all around, its volume falls to 0.3 at 0.4 of the way from the near range to the far
// one. With fade_in it starts silent and fades in to its volume by fade a ms (S_sound_play's).
// FUNCTION: GAME 0x5aca70
int C_game::Play3DSound(I3D_frame *frm, const char *name, I3D_SOUNDTYPE type, S_vector pos, float near_range,
    float far_range, float volume, bool fade_in, float fade, bool loop)
{
    I3D_sound *snd = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (!snd)
        return -1;
    if (sound_cache.Open(snd, name, 0, NULL, NULL, NULL) != I3D_OK)
    {
        snd->Release();
        return -1;
    }
    snd->SetPos(pos);
    snd->LinkTo(frm ? frm : act_mission->GetScene()->primary_sector, 0);
    snd->SetSoundType(type);
    snd->SetRange(near_range, far_range, .4f, .3f);
    snd->SetCone(PI * 2, PI * 2);
    snd->SetOutVol(0.0f);
    snd->SetVolume(volume);
    snd->Update();
    snd->SetOn(true);
    snd->SetLoop(loop);
    S_sound_play sp;
    sp.sound = snd;
    sp.number = next_sound_number++;
    sp.fading = fade_in;
    sp.time = 0;
    if (fade_in)
    {
        sp.fade = fade;
        sp.volume = volume;
        snd->SetVolume(0.0f);
        sounds_fading++;
    }
    sound_play.push_back(sp);
    return sound_play.back().number;
}

// A sound Play3DSound played, by its number, stopped (PS2 name): with fade it fades out by speed a ms, else it stops at
// once (TickEffects releases it)
// FUNCTION: GAME 0x5acc00
void C_game::Stop3DSound(int sound, bool fade, float speed)
{
    for (int i = sound_play.size(); i--;)
    {
        if (sound_play[i].number == sound)
        {
            sound_play[i].fading = fade;
            if (fade)
            {
                sound_play[i].fade = -speed;
                sounds_fading++;
            }
            else
                sound_play[i].sound->SetOn(false, true);
            return;
        }
    }
}

// The sound played as it is set up, released once it ends (PS2 name): its number, which Stop3DSound takes
// FUNCTION: GAME 0x5accb0
int C_game::Play3DSound(I3D_sound *sound)
{
    sound->SetOn(true);
    S_sound_play sp;
    sp.sound = sound;
    sp.number = next_sound_number++;
    sp.fading = false;
    sp.time = 0;
    sound_play.push_back(sp);
    return sp.number;
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
inline void C_game::SetupShadow(S_shadow_item *item)
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

// A shadow for the frame (the name is ours): a copy of the shadow model of type2's index, or of the model of the name
// where one is given, in the cars' list if type1 isn't 0, else the humans', which TickShadows puts on the frame once it
// is among the nearest. replaced_frame is the frame it turns off while it is on, on until then. The name goes into one
// of the spare buffers where one is left and it fits. The shadow (the human keeps it), 0 if the frame has one in that
// list already, or for no frame or a model that isn't there
// FUNCTION: GAME 0x5ad2a0
int C_game::AddFrameForShadow(I3D_frame *frm, E_shadow_type type1, E_shadow_type type2, I3D_frame *replaced_frame,
    char *name)
{
    if ((uint)type2 >= shadow_temp.size())
        return 0;
    if (!frm)
        return 0;
    std::vector<S_shadow_item> *list;
    if (!type1)
        list = &shadow_items;
    else
        list = &car_shadow_items;
    for (int i = list->size(); i--;)
    {
        if ((*list)[i].frame == frm)
            return 0;
    }
    S_shadow_item item;
    item.frame = frm;
    if (name)
    {
        if (strlen(name) < SHADOW_NAME_SIZE - 1 && free_shadow_names.size())
        {
            item.name = free_shadow_names.back();
            free_shadow_names.pop_back();
        }
        else
            item.name = new char[Max(SHADOW_NAME_SIZE, (int)strlen(name) + 1)];
        strcpy(item.name, name);
    }
    else
        item.name = NULL;
    item.replaced_frame = replaced_frame;
    item.on = false;
    item.model = (I3D_shadow *)driver->CreateFrame(FRAME_SHADOW);
    if (!item.model)
        return 0;
    item.model_index = type2;
    if (!type1)
        shadow_items.push_back(item);
    else
        car_shadow_items.push_back(item);
    if (item.replaced_frame)
        item.replaced_frame->SetOn(true);
    return (int)item.model;
}

// The frame's shadow made a copy of the shadow model of type2's index (PC only, PS2's is empty): put on the frame again at
// once where it is on. False if type2 isn't a shadow model's index or the frame has no shadow in type1's list.
// FUNCTION: GAME 0x5ad640
bool C_game::SetFrameForShadow(I3D_frame *frm, E_shadow_type type1, E_shadow_type type2)
{
    if ((uint)type2 >= shadow_temp.size())
        return false;
    std::vector<S_shadow_item> *list = &shadow_items;
    if (type1)
        list = &car_shadow_items;
    for (int i = list->size(); i--;)
    {
        if ((*list)[i].frame == frm)
        {
            (*list)[i].model_index = type2;
            if ((*list)[i].on)
            {
                (*list)[i].model->LinkTo(NULL, 0);
                (*list)[i].model->Close();
                (*list)[i].frame->Update();
                SetupShadow(&(*list)[i]);
                (*list)[i].model->LinkTo((*list)[i].frame, 0);
                (*list)[i].frame->Update();
            }
            return true;
        }
    }
    return false;
}

// The frame's shadow taken from type's list (the cars' if it isn't 0, else the humans'; the name is ours): the shadow off
// and released, its name's buffer kept for another, the replaced frame on again. False if the frame has none.
// FUNCTION: GAME 0x5ad7c0
bool C_game::DelFrameForShadow(I3D_frame *frm, E_shadow_type type)
{
    std::vector<S_shadow_item> *list;
    if (!type)
        list = &shadow_items;
    else
        list = &car_shadow_items;
    for (int i = list->size(); i--;)
    {
        if ((*list)[i].frame == frm)
        {
            if ((*list)[i].model)
            {
                (*list)[i].model->LinkTo(NULL, 0);
                (*list)[i].model->Close();
                (*list)[i].model->Release();
                (*list)[i].model = NULL;
                if ((*list)[i].name)
                {
                    free_shadow_names.push_back((*list)[i].name);
                    (*list)[i].name = NULL;
                }
                (*list)[i].frame->Update();
                if ((*list)[i].replaced_frame)
                    (*list)[i].replaced_frame->SetOn(false);
            }
            (*list)[i] = list->back();
            list->pop_back();
            return true;
        }
    }
    return false;
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

// The city's music paused, its areas not checked (PS2 name): pausing fades it out as CityMusicOff does
// FUNCTION: GAME 0x5ae1b0
void C_game::CityMusicPaused(bool paused)
{
    city_music_paused = paused;
    if (paused)
        CityMusicOff();
}

// A stream of the film music (an IShow id) added to film_music_streams, unless it is there (PS2 name, PS2's is empty)
// FUNCTION: GAME 0x5ae1d0
void C_game::SetFilmMusic(int music)
{
    for (int i = film_music_streams.size(); i--;)
    {
        if (film_music_streams[i] == music)
            return;
    }
    film_music_streams.push_back(music);
}

// The film music's stream of the index, -1 for none (PS2 name, PS2's returns -1)
// FUNCTION: GAME 0x5ae390
int C_game::GetFilmMusic(int index)
{
    if (film_music_streams.size() > (uint)index && index >= 0)
        return film_music_streams[index];
    return -1;
}

// The film music's streams destroyed, from the last, and forgotten (RECUNLOAD's; the name is ours)
// FUNCTION: GAME 0x5ae3d0
void C_game::StreamDestroyAll()
{
    for (int i = film_music_streams.size(); i--;)
        DestroyIShow(film_music_streams[i]);
    film_music_streams.clear();
}

// The holes (the shots' marks) in the frame's subtree unlinked, as the frame goes away (PS2 name, PS2's is empty)
// FUNCTION: GAME 0x5ae430
void C_game::ClearFrameFromFireHole(I3D_frame *frm)
{
    for (int i = 0; i < 100; i++)
    {
        if (!holes[i])
            continue;
        for (I3D_frame *p = holes[i]->m_pParent; p; p = p->m_pParent)
        {
            if (p == frm)
            {
                holes[i]->LinkTo(NULL, 0);
                break;
            }
        }
    }
}

// The diff data's chunk file (the names are ours): its root, and the chunks in it. A frame of the scene, found by its
// name or created (its type, position, scale, rotation, name and parent, then its type's data: a light's values and
// sectors, a sound's file and values, a model's file); an actor for that frame (its type, then its LoadData's data);
// an actor for a frame of the scene by its name (the name, then its type); the game's own scripts (LoadSourceCode's).
#define CT_DIFF 0x4d2
#define CT_DIFF_FRAME 0x64
#define CT_DIFF_FRAME_ACTOR 0xc8
#define CT_DIFF_ACTOR 0x190
#define CT_DIFF_PROGRAMS 0x1f4

static C_actor **NewFrameActor(I3D_frame *frm);

// ClearDiffData, then the diff data of the name, the chunk file diff\<name> (PC only, the name is ours), whose frames,
// actors and scripts it adds to the mission and to diff_frames, diff_actors (but those of CT_DIFF_FRAME_ACTOR) and
// diff_programs, which ClearDiffData deletes again. A frame that has an actor already ends the file. A player's actor
// becomes an entity that reads a human's data. The frame's types come in the order model, sound, light (the original
// inlines the model's and the sound's cache Open and calls the light's Reads out of line), and the scale's vector, the
// name's length and the light's type and count are one variable each, as the original's frame has them.
// Open: the original has about seven more pending inline calls after the sound's type (seven calls of an empty inline
// function there give its out-of-line size() in the frame's fifth Read, its inlined Read of the sound's type and its
// out-of-line ~C_chunk at the end: 83.60%, masked 97.21%); and it puts the created frame's block after the model Open's
// first loop, where ours follows the found frame's
// FUNCTION: GAME 0x5ae480
void C_game::LoadDiffData(const char *name)
{
    I3D_frame *frm = NULL;
    ClearDiffData();
    C_chunk ck;
    char buf[256];
    int len;
    // STRING: GAME 0x6377b0
    sprintf(buf, "diff\\%s", name);
    if (!ck.OpenFile(buf, false))
        return;
    ushort type;
    if (!ck.RAscend(type))
        goto fail;
    if (type != CT_DIFF)
        ck.Descend();
    else
    {
        while (ck.Size())
        {
            if (!ck.RAscend(type))
                goto fail;
            if (type == CT_DIFF_FRAME)
            {
                int frame_type;
                S_vector pos, vec;              // vec: the scale, a light's color
                S_quat rot;
                ck.Read(&frame_type, sizeof(frame_type));
                ck.Read(&pos, sizeof(pos));
                ck.Read(&vec, sizeof(vec));
                ck.Read(&rot, sizeof(rot));
                ck.Read(&len, sizeof(len));
                ck.Read(buf, len);
                buf[len] = 0;
                bool created = true;
                frm = act_mission->GetScene()->FindFrame(buf, ENUMF_ALL);
                if (frm)
                {
                    C_actor **p = GetFrameActor(frm);
                    if (p && *p)
                        frm = NULL;
                    else
                        created = false;
                }
                else
                    frm = driver->CreateFrame((I3D_FRAME_TYPE)frame_type);
                if (!frm)
                    goto fail;
                frm->SetPos(pos);
                frm->SetScale(vec);
                frm->SetRot(rot);
                frm->SetName(buf);
                ck.Read(&len, sizeof(len));
                ck.Read(buf, len);
                buf[len] = 0;
                I3D_frame *parent = act_mission->GetScene()->FindFrame(buf, ENUMF_ALL);
                if (!parent)
                    parent = act_mission->GetScene()->primary_sector;
                frm->LinkTo(parent, 0);
                switch (frm->GetType())
                {
                case FRAME_MODEL:
                    ck.Read(&len, sizeof(len));
                    ck.Read(buf, len);
                    buf[len] = 0;
                    model_cache.Open((I3D_model *)frm, buf, 0, NULL, NULL, NULL);
                    break;
                case FRAME_SOUND:
                {
                    I3D_sound *snd = (I3D_sound *)frm;
                    ck.Read(&len, sizeof(len));
                    ck.Read(buf, len);
                    buf[len] = 0;
                    sound_cache.Open(snd, buf, 0, NULL, NULL, NULL);
                    int stype;
                    ck.Read(&stype, sizeof(stype));
                    snd->SetSoundType((I3D_SOUNDTYPE)stype);
                    bool b;
                    ck.Read(&b, sizeof(b));
                    snd->SetOn(b);
                    float f1, f2, f3, f4;
                    ck.Read(&f1, sizeof(f1));
                    ck.Read(&f2, sizeof(f2));
                    ck.Read(&f3, sizeof(f3));
                    ck.Read(&f4, sizeof(f4));
                    snd->SetRange(f1, f2, f3, f4);
                    ck.Read(&f1, sizeof(f1));
                    ck.Read(&f2, sizeof(f2));
                    snd->SetCone(f1, f2);
                    ck.Read(&f1, sizeof(f1));
                    snd->SetVolume(f1);
                    ck.Read(&f1, sizeof(f1));
                    snd->SetOutVol(f1);
                    ck.Read(&b, sizeof(b));
                    snd->SetLoop(b);
                    ck.Read(&f1, sizeof(f1));
                    snd->SetFrequency(f1);
                    break;
                }
                case FRAME_LIGHT:
                {
                    I3D_light *lt = (I3D_light *)frm;
                    int n;
                    ck.Read(&n, sizeof(n));
                    lt->SetLightType((I3D_LIGHTTYPE)n);
                    ck.Read(&vec, sizeof(vec));
                    lt->SetDiffuse(vec);
                    float f1, f2;
                    ck.Read(&f1, sizeof(f1));
                    lt->SetDiffusePower(f1);
                    bool on;
                    ck.Read(&on, sizeof(on));
                    lt->SetOn(on);
                    ck.Read(&f1, sizeof(f1));
                    ck.Read(&f2, sizeof(f2));
                    lt->SetRange(f1, f2);
                    ck.Read(&f1, sizeof(f1));
                    ck.Read(&f2, sizeof(f2));
                    lt->SetCone(f1, f2);
                    uint mode;
                    ck.Read(&mode, sizeof(mode));
                    lt->SetMode(mode);
                    // The sectors it lights, by their names
                    ck.Read(&n, sizeof(n));
                    while (n--)
                    {
                        int name_len;
                        ck.Read(&name_len, sizeof(name_len));
                        char *sector_name = new char[name_len + 1];
                        ck.Read(sector_name, name_len + 1);
                        I3D_sector *sct = (I3D_sector *)act_mission->GetScene()->FindFrame(sector_name, ENUMF_SECTOR);
                        if (sct)
                            sct->AddLight(lt);
                        delete[] sector_name;
                    }
                    lt->Update();
                    break;
                }
                }
                diff_frames.push_back(frm);
                act_mission->GetScene()->AddFrame(frm);
                if (created)
                    frm->Release();
            }
            else if (type == CT_DIFF_FRAME_ACTOR)
            {
                if (frm)
                {
                    int act_type;
                    ck.Read(&act_type, sizeof(act_type));
                    int file_type = act_type;
                    if (act_type == ACTOR_PLAYER)
                        act_type = ACTOR_ENTITY;
                    C_actor *act = act_mission->CreateActor(act_type);
                    if (act)
                    {
                        if (act->Init(frm))
                        {
                            C_actor **p = NewFrameActor(frm);
                            *p = act;
                            SetFrameActor(frm, p);
                            if (file_type == ACTOR_PLAYER)
                                ((C_human *)act)->C_human::LoadData(&ck);
                            else
                                act->LoadData(&ck);
                            act->GameInit();
                            active_actors.push_back(act);
                            act_mission->AddActor(act, true);
                            act->unknown_65 = true;
                        }
                        else
                            act->Release();
                    }
                }
            }
            else if (type == CT_DIFF_ACTOR)
            {
                ck.Read(&len, sizeof(len));
                ck.Read(buf, len);
                buf[len] = 0;
                I3D_frame *f = act_mission->GetScene()->FindFrame(buf, ENUMF_ALL);
                if (f)
                {
                    C_actor **p = GetFrameActor(f);
                    if (!p || !*p)
                    {
                        int act_type;
                        ck.Read(&act_type, sizeof(act_type));
                        C_actor *act = act_mission->CreateActor(act_type);
                        if (act && act->Init(f))
                        {
                            C_actor **q = NewFrameActor(f);
                            *q = act;
                            SetFrameActor(f, q);
                            act->GameInit();
                            active_actors.push_back(act);
                            act_mission->AddActor(act, true);
                            diff_actors.push_back(act);
                            act->unknown_65 = true;
                        }
                    }
                }
            }
            else if (type == CT_DIFF_PROGRAMS)
            {
                while (ck.Size())
                {
                    C_program *prog = new C_program;
                    prog->LoadSourceCode(&ck);
                    prog->Init();
                    diff_programs.push_back(prog);
                }
            }
            ck.Descend();
        }
        ck.Descend();
    }
fail:
    ck.Close();
}

// The frame's pointer to its actor, allocated for it where the frame has none (LoadDiffData's; the name is ours)
// FUNCTION: GAME 0x5af840
static C_actor **NewFrameActor(I3D_frame *frm)
{
    C_actor **p = GetFrameActor(frm);
    if (!p)
    {
        p = new C_actor *;
        frm->m_UserData = (uint)p;
    }
    return p;
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

// A moving collision gone: the entities whose last move ran into it forget it (PS2 name, PS2's takes a uint *)
// FUNCTION: GAME 0x5afdb0
void C_game::InvalidateColl(tDynamicCollObject *coll)
{
    int i;
    for (i = active_actors.size(); i--;)
    {
        C_human *h = (C_human *)active_actors[i];
        if (h->type == ACTOR_ENTITY && h->move_coll == (g_collision_header *)coll)
            h->move_coll = NULL;
    }
    for (i = inactive_actors.size(); i--;)
    {
        C_human *h = (C_human *)inactive_actors[i];
        if (h->type == ACTOR_ENTITY && h->move_coll == (g_collision_header *)coll)
            h->move_coll = NULL;
    }
}

// Takes an actor out of everything that refers to it (PS2 name): the actors' (the scripts' registers but with
// keep_regs, the entities', the humans' killer and cars, the physical things'), the shots', the sounds the humans hear,
// the player's car damage, the scratches, the fires on its frame, the AI's groups, fights and followers, the chases,
// a human's car seat, speech and dead tick, the possible collisions, the player, the scripts that wait for the game's
// events, the taxi's passenger and the item blur's (the parameter's name is ours)
// FUNCTION: GAME 0x5afe40
void C_game::InvalidateActor(C_actor *act, bool keep_regs)
{
    if (!act)
        return;
    int i;
    for (i = C_actor::global_actors.size(); i--;)
    {
        switch (C_actor::global_actors[i]->type)
        {
        case ACTOR_DETECTOR:
            if (!keep_regs)
                ((C_detector *)C_actor::global_actors[i])->InvalidateActorReg(act);
            break;
        case ACTOR_ENTITY:
            if (!keep_regs)
                ((C_entity *)C_actor::global_actors[i])->InvalidateActorReg(act);
            ((C_entity *)C_actor::global_actors[i])->InvalidateActor(act);
            ((C_entity *)C_actor::global_actors[i])->move_coll = NULL;
            if (((C_entity *)C_actor::global_actors[i])->killer == act)
                ((C_entity *)C_actor::global_actors[i])->killer = NULL;
            // and as a player's
        case ACTOR_PLAYER:
            if (((C_human *)C_actor::global_actors[i])->killer == act)
                ((C_human *)C_actor::global_actors[i])->killer = NULL;
            if (((C_human *)C_actor::global_actors[i])->car == act)
                ((C_human *)C_actor::global_actors[i])->car = NULL;
            if (((C_human *)C_actor::global_actors[i])->using_car == act)
                ((C_human *)C_actor::global_actors[i])->using_car = NULL;
            break;
        case ACTOR_DROP_OUT:
        case ACTOR_BOX_NEW:
        case ACTOR_CRASH_OBJ:
        case ACTOR_PUBLIC_PHYSICS:
        case ACTOR_DROP_IN_WEAPON:
            ((C_PhThing *)C_actor::global_actors[i])->InvalidateActor(act);
            break;
        }
    }
    for (i = shoots.size(); i--;)
    {
        if (shoots[i].actor == act)
            shoots[i].actor = NULL;
    }
    sound_arena.InvalidateActor(act);
    if (player && act->type == ACTOR_CAR)
        ((C_player *)player)->InvalidateCarDamage((C_car *)act);
    if (act->type == ACTOR_CAR)
        ScratchDeleteFromCar((C_car *)act);
    // The pop_back's _Destroy called out of line, as the original, only with the accessors' pending inline calls
    // after it (GetType, GetCar)
    for (uint j = 0; j < fires.size();)
    {
        if (fires[j]->frame == act->frame)
        {
            delete fires[j];
            fires[j] = fires.back();
            fires.pop_back();
        }
        else
            j++;
    }
    AIManager.groups.InvalidateActor(act);
    AIManager.fight.Invalidate(act);
    AIManager.follow.Invalidate(act);
    if (act->GetType() == ACTOR_CAR)
        sledovacka.VypicujMiToyotu(act);
    if (act->GetType() == ACTOR_PLAYER || act->GetType() == ACTOR_ENTITY)
    {
        C_human *human = (C_human *)act;
        if (human->GetCar() && human->GetCar()->GetType() == ACTOR_CAR)
            human->GetCar()->SetOwner(NULL, human->seat, 0.0f);
        DabingInvalidate(human);
        DelDeadHumanForTick(human);
    }
    C_actor::DelFrameForPossibleColl(act->frame);
    if (act == player)
        player = NULL;
    if (act->GetType() == ACTOR_DETECTOR || act->GetType() == ACTOR_ENTITY)
    {
        C_program *prog = NULL;
        if (act->GetType() == ACTOR_DETECTOR)
            prog = (C_detector *)act;
        else if (act->GetType() == ACTOR_ENTITY)
            prog = (C_entity *)act;
        if (prog == timeout_program)
            timeout_program = NULL;
        if (prog == player_fire_program)
            player_fire_program = NULL;
        if (prog == player_horn_program)
            player_horn_program = NULL;
        if (prog == player_wfall_program)
            player_wfall_program = NULL;
    }
    if (taxi_passenger == act)
        taxi_passenger = NULL;
    for (i = item_blurs.size(); i--;)
    {
        if (act == item_blurs[i].actor)
        {
            ItemBlurDelItem(i, true);
            break;
        }
    }
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

// The frame's state kept in frame_placements, which Done puts back (PS2 name): once, and not for an actor's frame
// FUNCTION: GAME 0x5b0300
void C_game::AddFramePlacement(I3D_frame *frm)
{
    if (!frm)
        return;
    C_actor **act = GetFrameActor(frm);
    if (act && *act)
        return;
    for (int i = frame_placements.size(); i--;)
    {
        if (frame_placements[i].frame == frm)
            return;
    }
    // The parent second, as PS2's (whose struct has it after the frame): its load comes first
    S_frame_placement fp;
    fp.frame = frm;
    fp.parent = frm->GetParent();
    fp.pos = frm->GetPos();
    fp.rot = frm->GetRot();
    fp.scale = frm->GetScale();
    fp.on = frm->IsOn();
    frame_placements.push_back(fp);
}

// The frame's placement taken from frame_placements (PS2 name), the last one in its place
// FUNCTION: GAME 0x5b05d0
void C_game::DelFramePlacement(I3D_frame *frm)
{
    for (int i = frame_placements.size(); i--;)
    {
        if (frame_placements[i].frame == frm)
        {
            frame_placements[i] = frame_placements.back();
            frame_placements.pop_back();
            break;
        }
    }
}

// The scripts' events the game fires (PS2 names): each in its script, if one waits for it
// FUNCTION: GAME 0x5b0660
void C_game::Timer_TimeOut()
{
    if (timeout_program)
        timeout_program->SetEvent(timeout_event);
}

// FUNCTION: GAME 0x5b0680
void C_game::PFire_Fire()
{
    if (player_fire_program)
        player_fire_program->SetEvent(player_fire_event);
}

// FUNCTION: GAME 0x5b06a0
void C_game::PHorn_Horn()
{
    if (player_horn_program)
        player_horn_program->SetEvent(player_horn_event);
}

// FUNCTION: GAME 0x5b06c0
void C_game::WFall_Player()
{
    if (player_wfall_program)
        player_wfall_program->SetEvent(player_wfall_event);
}

// SETNPCKILLEVENT's (PS2 name): the script's event fired when the NPC of that name is killed (Do_NPCKillEvent)
// FUNCTION: GAME 0x5b06e0
void C_game::AddNPCKillEvent(C_program *prog, int event, char *name)
{
    S_npc_kill_event e;
    memset(&e, 0, sizeof(e));
    npc_kill_events.push_back(e);
    npc_kill_events.back().program = prog;
    npc_kill_events.back().event = event;
    strcpy(npc_kill_events.back().name, name);
}

// An NPC's kill (PS2 name): the events of npc_kill_events of his name, without its path, fired
// FUNCTION: GAME 0x5b0750
void C_game::Do_NPCKillEvent(char *label)
{
    char *name = strrchr(label, '\\');
    if (name)
        name++;
    else
        name = label;
    for (int i = npc_kill_events.size(); i--;)
    {
        if (!_stricmp(name, npc_kill_events[i].name))
            npc_kill_events[i].program->SetEvent(npc_kill_events[i].event);
    }
}

// A driver's model for the car (PS2 name), once: the steering wheel's animation (FormuleVolant.I3D's for a formula,
// else AutoRidicVolant.I3D's) set as the model's animation of the index, which TickDriverMan moves by the car's
// steering, and the legs hidden
// FUNCTION: GAME 0x5b07e0
void C_game::AddDriverMan(I3D_model *model, C_car *car, int anim, bool formula)
{
    if (!car)
        return;
    if (!model)
        return;
    for (int i = driver_men.size(); i--;)
    {
        if (driver_men[i].model == model)
            return;
    }
    I3D_animation_set *set = NULL;
    if (formula)
        anim_cache.Create("FormuleVolant.I3D", &set, NULL, NULL);
    else
        anim_cache.Create("AutoRidicVolant.I3D", &set, NULL, NULL);
    if (!set)
        return;
    model->SetAnimation(set, anim, 1.0f, 0);
    set->Release();
    C_human::HideLegs(model, true);
    S_driver_man dm;
    dm.model = model;
    dm.car = car;
    dm.anim = anim;
    dm.formula = formula;
    driver_men.push_back(dm);
}

// The driver's model taken from driver_men (PS2 name): its animation stopped, its legs shown, the last one in its place
// FUNCTION: GAME 0x5b0a80
void C_game::DelDriverMan(I3D_model *model)
{
    for (int i = driver_men.size(); i--;)
    {
        if (driver_men[i].model == model)
        {
            driver_men[i].model->StopAnimation(driver_men[i].anim);
            C_human::HideLegs(driver_men[i].model, false);
            driver_men[i] = driver_men.back();
            driver_men.pop_back();
            break;
        }
    }
}

// A track fitted to the surface it is on (the name is ours): a line from pos along each of the axes perpendicular to
// the normal, both ways; where one hits something, the normal turns toward it and pos is moved off it to 0.707. Inline:
// both CreateTrackEffect overloads call its operators out of line, as nested calls starved by the inline calls after
// it (the scene's accessor twice), where written out in them, they inline every one.
inline void C_game::FitTrack(S_vector &pos, S_vector &normal)
{
    S_vector dirs[4];
    GetTangents(normal, dirs[0], dirs[1]);
    dirs[2] = -dirs[0];
    dirs[3] = -dirs[1];
    for (int i = 4; i--;) {
        S_vector dir = dirs[i];
        S_vector hit_pos, hit_normal;
        if (gifCollision.TestLineV(pos, dir, &hit_pos, &hit_normal, COLF_ALL, 0)) {
            normal += dir;
            float dist = pos.DistanceToPlane(hit_pos, hit_normal);
            if (dist < .707f) {
                float d = .707f - dist;
                pos += hit_normal * d;
            }
        }
    }
    normal.Normalize();
}

// A track on a surface (PS2 name; PS2's is empty; the parameters' names are ours): the next of the tracks' projectors
// (track_projector_next, which wraps at 100), its blood's collision taken out first, projects the texture of
// track_textures' index at pos along normal, turned by angle (a hole's at random), sized size with x and y varied
// together by up to size_rnd, faded by alpha and lit as the scene is at pos. With collision it puts a collision in the
// game around pos (half a meter lower), which the humans' steps through it take blood from (cbCollTester); with fit
// the track is fitted to the surface, moved off what a line along each of its axes hits, its direction turned to that;
// with grow it grows from a tenth to its size (Tick), else its receivers are locked at once. frm is the frame it is on
// (the shot's level's, for ClearProjectorsForFrame). The projector, NULL without a texture or a projector.
// FUNCTION: GAME 0x5b0b60
I3D_frame *C_game::CreateTrackEffect(S_vector &pos, S_vector &normal, float angle, S_vector &size, float size_rnd,
    int texture, bool fit, float alpha, bool collision, bool grow, I3D_frame *frm)
{
    if (texture < 0 || texture >= 10 || !track_textures[texture])
        return NULL;
    if (track_projectors[track_projector_next].in_collision) {
        gifCollision.DynErase(&track_projectors[track_projector_next].coll);
        track_projectors[track_projector_next].in_collision = false;
    }
    I3D_frame *ret = NULL;
    if (track_projectors[track_projector_next].frame) {
        if (collision) {
            track_projectors[track_projector_next].in_collision = true;
            memset(&track_projectors[track_projector_next].coll, 0, sizeof(tDynamicCollObject));
            track_projectors[track_projector_next].coll.type = DYNCOLL_UNKNOWN_48;
            track_projectors[track_projector_next].coll.flags = DYNCOLLF_UNKNOWN_200000 | DYNCOLLF_LOWER;
            ((I3D_bsphere &)track_projectors[track_projector_next].coll.bsphere).pos = pos;
            ((I3D_bsphere &)track_projectors[track_projector_next].coll.bsphere).pos.y -= .5f;
            ((I3D_bsphere &)track_projectors[track_projector_next].coll.bsphere).radius = .3f;
            track_projectors[track_projector_next].coll.frame = NULL;
            track_projectors[track_projector_next].coll.owner = NULL;
            gifCollision.DynInsert(&track_projectors[track_projector_next].coll);
        }
        if (fit)
            FitTrack(pos, normal);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetProjectorLock(I3DLOCK_NONE);
        track_projectors[track_projector_next].frame->SetOn(true);
        track_projectors[track_projector_next].frame->SetPos(pos);
        track_projectors[track_projector_next].frame->SetDir(normal, angle);
        float r = (rnd() * 2.0f - 1.0f) * size_rnd;
        track_projectors[track_projector_next].frame->SetScale(S_vector(size.x + r, size.y + r, size.z));
        track_projectors[track_projector_next].frame->Update();
        act_mission->GetScene()->SetFrameSector(track_projectors[track_projector_next].frame);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetMode(PROJECTOR_MODE_BLEND);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetOrthogonal(true);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetTexture(track_textures[texture]);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetAlpha(alpha);
        float light = act_mission->GetScene()->GetLightness(pos, NULL, 0, NULL);
        S_vector color(light, light, light);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetColor(color);
        if (grow) {
            track_projectors[track_projector_next].growing = true;
            track_projectors[track_projector_next].scale = .1f;
            growing_projectors++;
            S_vector scale = track_projectors[track_projector_next].frame->GetScale();
            scale.x *= track_projectors[track_projector_next].scale;
            scale.y *= track_projectors[track_projector_next].scale;
            track_projectors[track_projector_next].frame->SetScale(scale);
            track_projectors[track_projector_next].frame->Update();
        } else {
            track_projectors[track_projector_next].growing = false;
            ((I3D_projector *)track_projectors[track_projector_next].frame)->SetProjectorLock(I3DLOCK_RECEIVERS);
        }
        track_projectors[track_projector_next].hit_frame = frm;
        ret = track_projectors[track_projector_next].frame;
    }
    NextTrackProjector();
    return ret;
}

// The same turned by a direction instead of an angle (PS2's other overload, also empty): its local z axis along normal,
// the y axis toward dir (human_notify_callback's steps)
// FUNCTION: GAME 0x5b1370
I3D_frame *C_game::CreateTrackEffect(S_vector &pos, S_vector &normal, S_vector &dir, S_vector &size, float size_rnd,
    int texture, bool fit, float alpha, bool collision, bool grow, I3D_frame *frm)
{
    if (texture < 0 || texture >= 10 || !track_textures[texture])
        return NULL;
    if (track_projectors[track_projector_next].in_collision) {
        gifCollision.DynErase(&track_projectors[track_projector_next].coll);
        track_projectors[track_projector_next].in_collision = false;
    }
    I3D_frame *ret = NULL;
    if (track_projectors[track_projector_next].frame) {
        if (collision) {
            track_projectors[track_projector_next].in_collision = true;
            memset(&track_projectors[track_projector_next].coll, 0, sizeof(tDynamicCollObject));
            track_projectors[track_projector_next].coll.type = DYNCOLL_UNKNOWN_48;
            track_projectors[track_projector_next].coll.flags = DYNCOLLF_UNKNOWN_200000 | DYNCOLLF_LOWER;
            ((I3D_bsphere &)track_projectors[track_projector_next].coll.bsphere).pos = pos;
            ((I3D_bsphere &)track_projectors[track_projector_next].coll.bsphere).pos.y -= .5f;
            ((I3D_bsphere &)track_projectors[track_projector_next].coll.bsphere).radius = .3f;
            track_projectors[track_projector_next].coll.frame = NULL;
            track_projectors[track_projector_next].coll.owner = NULL;
            gifCollision.DynInsert(&track_projectors[track_projector_next].coll);
        }
        if (fit)
            FitTrack(pos, normal);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetProjectorLock(I3DLOCK_NONE);
        track_projectors[track_projector_next].frame->SetOn(true);
        track_projectors[track_projector_next].frame->SetPos(pos);
        track_projectors[track_projector_next].frame->SetDir1(normal, dir);
        float r = (rnd() * 2.0f - 1.0f) * size_rnd;
        track_projectors[track_projector_next].frame->SetScale(S_vector(size.x + r, size.y + r, size.z));
        track_projectors[track_projector_next].frame->Update();
        act_mission->GetScene()->SetFrameSector(track_projectors[track_projector_next].frame);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetMode(PROJECTOR_MODE_BLEND);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetOrthogonal(true);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetTexture(track_textures[texture]);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetAlpha(alpha);
        float light = act_mission->GetScene()->GetLightness(pos, NULL, 0, NULL);
        S_vector color(light, light, light);
        ((I3D_projector *)track_projectors[track_projector_next].frame)->SetColor(color);
        if (grow) {
            track_projectors[track_projector_next].growing = true;
            track_projectors[track_projector_next].scale = .1f;
            growing_projectors++;
            S_vector scale = track_projectors[track_projector_next].frame->GetScale();
            scale.x *= track_projectors[track_projector_next].scale;
            scale.y *= track_projectors[track_projector_next].scale;
            track_projectors[track_projector_next].frame->SetScale(scale);
            track_projectors[track_projector_next].frame->Update();
        } else {
            track_projectors[track_projector_next].growing = false;
            ((I3D_projector *)track_projectors[track_projector_next].frame)->SetProjectorLock(I3DLOCK_RECEIVERS);
        }
        track_projectors[track_projector_next].hit_frame = frm;
        ret = track_projectors[track_projector_next].frame;
    }
    NextTrackProjector();
    return ret;
}

// Two axes perpendicular to the normal and to each other (the name is ours): u horizontal, v completing them, the world's
// x and z for a vertical normal
// FUNCTION: GAME 0x5b1b60
void C_game::GetTangents(const S_vector &normal, S_vector &u, S_vector &v)
{
    if ((float)fabs(normal.x) < MRG_ZERO && (float)fabs(normal.z) < MRG_ZERO) {
        u.x = 1.0f;
        u.y = 0.0f;
        u.z = 0.0f;
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 1.0f;
        return;
    }
    u = normal.Cross(S_vector(0.0f, 1.0f, 0.0f));
    v = u.Cross(normal);
    u = normal.Cross(v);
}

// A grenade's track where it exploded (PS2 name): a stain (track_textures' 1) 1 m above pos, projected down and fitted to
// the ground, 0.8 m wide at random turns. The direction both builds compute from two random numbers isn't used: the
// PC's keeps only the calls.
// FUNCTION: GAME 0x5b1ca0
void C_game::CreateTrackEffect_Grenade(S_vector &pos)
{
    S_vector p = pos;
    p.y += 1.0f;
    S_vector normal(0.0f, -1.0f, 0.0f);
    S_vector size(.8f, .8f, 1.45f);
    S_vector dir(rnd() + .001f, 0.0f, rnd() + .001f);
    CreateTrackEffect(p, normal, rnd() * (2.0f * PI), size, .2f, 1, true, 1.0f, false, false, NULL);
}

// track_projector_next to the next projector, wrapping at 100 (the name is ours)
// FUNCTION: GAME 0x5b1d50
void C_game::NextTrackProjector()
{
    if (++track_projector_next >= 100)
        track_projector_next = 0;
}

// The tracks on the frame turned off (a frame the scripts take away)
// FUNCTION: GAME 0x5b1d70
void C_game::ClearProjectorsForFrame(I3D_frame *frm)
{
    for (int i = 100; i--;) {
        if (track_projectors[i].hit_frame == frm)
            track_projectors[i].frame->SetOn(false);
    }
}

// FUNCTION: GAME 0x5b1da0
void C_game::InventoryStackClear()
{
    for (int i = inventory_stack.size(); i--; )
        delete inventory_stack[i];
    inventory_stack.clear();
}

// The player's inventory kept for a later mission (PS2 name; ENTITY_PUSHINVENTORY's most likely): a copy of it but its
// cycle. The first push after Init empties the stacks first.
// FUNCTION: GAME 0x5b1e10
void C_game::InventoryStackPush(C_human *human)
{
    if (!human)
        return;
    if (stacks_clear)
    {
        stacks_clear = false;
        FloatStackClear();
        CharacterStackClear();
        InventoryStackClear();
    }
    inventory_stack.push_back(new G_Inventory(human->inventory));
}

// The last inventory pushed given back to the human and taken off the stack (PS2 name); the player's is then put into
// his car's state or out of it
// FUNCTION: GAME 0x5b20b0
void C_game::InventoryStackPop(C_human *human)
{
    if (!human || !inventory_stack.size())
        return;
    human->inventory = *inventory_stack.back();
    delete inventory_stack.back();
    inventory_stack.pop_back();
    if (GetPlayer() == human)
    {
        if (human->car && human->car->type == ACTOR_CAR && human->seat != -1)
            human->inventory.SetInCar(true, NULL);
        else
        {
            std::vector<S_GameItem> dropped;
            human->inventory.SetInCar(false, &dropped);
        }
    }
}

// FUNCTION: GAME 0x5b2320
void C_game::FloatStackClear()
{
    float_stack.clear();
}

// A float kept for a later mission (PS2 name), and the last one taken back
// FUNCTION: GAME 0x5b2350
void C_game::FloatStackPush(float f)
{
    if (stacks_clear)
    {
        stacks_clear = false;
        FloatStackClear();
        CharacterStackClear();
        InventoryStackClear();
    }
    float_stack.push_back(f);
}

// FUNCTION: GAME 0x5b2530
void C_game::FloatStackPop(float *f)
{
    if (float_stack.size())
    {
        *f = float_stack.back();
        float_stack.pop_back();
    }
}

// FUNCTION: GAME 0x5b2590
void C_game::CharacterStackClear()
{
    character_stack.clear();
}

// The human's properties kept for a later mission (PS2 name), all 16 in the order Pop takes them off in reverse
// FUNCTION: GAME 0x5b25c0
void C_game::CharacterStackPush(C_human *human)
{
    if (!human)
        return;
    if (stacks_clear)
    {
        stacks_clear = false;
        FloatStackClear();
        CharacterStackClear();
        InventoryStackClear();
    }
    character_stack.push_back(human->properties[HUMAN_PROPERTY_STRENGTH]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_SPEED]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_SIGHT]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_SHOOTING]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_REACTIONS]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_15]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_MASS]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_INTELLIGENCE]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_HEARING]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_ENERGY_LEG_R]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_ENERGY_LEG_L]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_ENERGY_HAND_R]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_ENERGY_HAND_L]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_ENERGY]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_DRIVING]);
    character_stack.push_back(human->properties[HUMAN_PROPERTY_AGGRESIVITY]);
}

// The last properties pushed taken off the stack (PS2 name): only the energies are given back, the human dies without
// energy, and the HUD shows the player's or the wingman's lives again
// FUNCTION: GAME 0x5b2740
void C_game::CharacterStackPop(C_human *human)
{
    if (!human || character_stack.size() < 16)
        return;
    character_stack.pop_back();
    character_stack.pop_back();
    human->properties[HUMAN_PROPERTY_ENERGY] = character_stack.back();
    character_stack.pop_back();
    human->properties[HUMAN_PROPERTY_ENERGY_HAND_L] = character_stack.back();
    character_stack.pop_back();
    human->properties[HUMAN_PROPERTY_ENERGY_HAND_R] = character_stack.back();
    character_stack.pop_back();
    human->properties[HUMAN_PROPERTY_ENERGY_LEG_L] = character_stack.back();
    character_stack.pop_back();
    human->properties[HUMAN_PROPERTY_ENERGY_LEG_R] = character_stack.back();
    character_stack.pop_back();
    character_stack.pop_back();
    character_stack.pop_back();
    character_stack.pop_back();
    character_stack.pop_back();
    character_stack.pop_back();
    character_stack.pop_back();
    character_stack.pop_back();
    character_stack.pop_back();
    character_stack.pop_back();
    if (human->properties[HUMAN_PROPERTY_ENERGY] < .01f)
        human->_intern_ForceDeath();
    if (act_mission->GetGame()->GetPlayer() == human)
    {
        uint lives = (int)(human->properties[HUMAN_PROPERTY_ENERGY] / human->max_properties[HUMAN_PROPERTY_ENERGY] *
            100.0f);
        if (!human->GetDead() && lives < 1)
            lives = 1;
        mafiaIndicators.PlayerSetLives(lives);
    }
    if (human->wingman > -1)
    {
        int lives = (int)(human->properties[HUMAN_PROPERTY_ENERGY] / human->max_properties[HUMAN_PROPERTY_ENERGY] *
            100.0f);
        if (!human->GetDead())
            lives = Max(lives, 1);
        mafiaIndicators.PlayerSetWingmanLives(human->wingman, lives);
    }
}

// A box the scripts place (PS2 name; FUCKINGBOX_ADD's, and a box put down again): an actor of its frame unloaded
// from the mission, a collision of its bound volume (in the collisions unless the box lies on an actor), and the id
// of its destinations. A box placed already only gets the id, if it has none.
// FUNCTION: GAME 0x5b2a00
void C_game::BoxPlacementAdd(I3D_frame *frm, int id)
{
    for (int i = boxes.size(); i--; )
        if (boxes[i]->frame == frm)
        {
            if (boxes[i]->id == BOXID_NONE)
                boxes[i]->id = id;
            return;
        }
    C_actor **p = GetFrameActor(frm);
    if (p)
    {
        C_actor *act = *p;
        if (act && act_mission->UnloadActor(act))
        {
            *p = NULL;
            SetFrameActor(frm, p);
            InternTempActAdd(act);
            RemoveTemporaryActor(act);
        }
    }
    S_box_placement *box = new S_box_placement;
    C_actor::DebustizeFrame(frm);
    bool on_actor = false;
    for (I3D_frame *f = frm->GetParent(); f; f = f->GetParent())
        if (GetFrameActor(f) && *GetFrameActor(f))
        {
            on_actor = true;
            break;
        }
    memset(&box->coll, 0, sizeof(box->coll));
    box->coll.type = DYNCOLL_UNKNOWN_40;
    box->coll.frame = frm;
    box->coll.owner = NULL;
    I3D_bvolume vol = frm->GetHRBoundVolume();
    (I3D_bbox &)box->coll.bbox = vol.bbox;
    S_vector scale = frm->GetWorldScale();
    ((I3D_bsphere &)box->coll.bsphere).radius = Max(scale.x, Max(scale.y, scale.z)) * vol.bsphere.radius;
    ((I3D_bsphere &)box->coll.bsphere).pos = vol.bsphere.pos * frm->GetMatrix();
    box->coll.flags = DYNCOLLF_UNKNOWN_1000;
    box->coll.matrix = (S_matrix *)&frm->GetMatrix();
    box->inv_matrix = frm->GetMatrix();
    S_matrix &m = box->inv_matrix;
    m.Inverse(S_matrix(m));
    box->coll.inv_matrix = &box->inv_matrix;
    if (!on_actor)
        gifCollision.DynInsert(&box->coll);
    box->frame = frm;
    box->id = id;
    box->on = true;
    box->using_object = new C_using_object::S_using_object;
    memset(box->using_object, 0, sizeof(*box->using_object));
    boxes.push_back(box);
}

// The box taken out of the placements (PS2 name): its collision and its using object gone, the last box in its place
// FUNCTION: GAME 0x5b2ee0
void C_game::BoxPlacementDel(I3D_frame *frm)
{
    for (int i = boxes.size(); i--; )
        if (boxes[i]->frame == frm)
        {
            gifCollision.DynErase(&boxes[i]->coll);
            act_mission->GetGame()->GetUsingObject().DelObject(boxes[i]->using_object);
            delete boxes[i]->using_object;
            delete boxes[i];
            boxes[i] = boxes.back();
            boxes.pop_back();
            return;
        }
}

// Which boxes the player can pick up (PS2 name; FUCKINGBOX_RECOMPILE's): those with an id, which can be taken, and
// that no other box lies on (one 0.2 to 1 m higher, less than about 0.63 m away horizontally) get their "Pick up
// box", the others lose it
// FUNCTION: GAME 0x5b2fc0
void C_game::BoxPlacementRecompile()
{
    for (int i = boxes.size(); i--; )
    {
        if (boxes[i]->id == BOXID_NONE)
            continue;
        bool covered = false;
        for (int j = boxes.size(); j--; )
        {
            if (i == j || !boxes[i]->on || !boxes[j]->on)
                continue;
            S_vector d = boxes[j]->frame->GetWorldPos() - boxes[i]->frame->GetWorldPos();
            if (d.y > .2f && d.y < 1.0f && d.x * d.x + d.z * d.z < .4f)
            {
                covered = true;
                break;
            }
        }
        if (!covered && boxes[i]->on)
        {
            boxes[i]->using_object->pos = boxes[i]->frame->GetWorldPos();
            boxes[i]->using_object->radius_2 = .5f;
            boxes[i]->using_object->matrix = NULL;
            boxes[i]->using_object->flags = 0;
            boxes[i]->using_object->frame = boxes[i]->frame;
            boxes[i]->using_object->actor = NULL;
            boxes[i]->using_object->item.id = GAMEITEM_SPECIAL_ACTION;
            boxes[i]->using_object->item.ammo_loaded = USING_ACTION_PICK_UP_BOX;
            boxes[i]->using_object->item.using_object = boxes[i]->using_object;
            boxes[i]->using_object->sector = NULL;
            act_mission->GetGame()->GetUsingObject().AddObject(boxes[i]->using_object);
        }
        else
            act_mission->GetGame()->GetUsingObject().DelObject(boxes[i]->using_object);
    }
}

// A destination of the boxes of the id (PS2 name; FUCKINGBOX_ADD_DEST's), once
// FUNCTION: GAME 0x5b3220
void C_game::BoxPlacementDestAdd(I3D_frame *frm, int id)
{
    for (int i = box_dests.size(); i--; )
        if (box_dests[i]->frame == frm)
            return;
    S_box_dest *dest = new S_box_dest;
    dest->frame = frm;
    dest->id = id;
    box_dests.push_back(dest);
}

// The player takes the box (on) or puts it down (PS2 name): "Drop box" at the last destination of the box's id while
// he carries it, where the box was saved (BoxPlacement_SaveInfo), and the box's id returned; -1 and no "Drop box" when
// he puts it down or the box has no destination. The destinations are compared with a copy of the box's id
// (FindBoxDest's parameter): VC6 doesn't share it with the id returned, which stays in memory.
// FUNCTION: GAME 0x5b33f0
int C_game::BoxPlacementDestSetOU(bool on, I3D_frame *frm)
{
    int id = -1;
    using_object.DelObject(&dest_object);
    if (!box_dests.size())
        return id;
    if (!on)
        return id;
    int index = -1;
    for (int i = boxes.size(); i--; )
        if (boxes[i]->frame == frm)
        {
            id = boxes[i]->id;
            index = i;
            break;
        }
    if (index < 0)
        return -1;
    int j = FindBoxDest(boxes[index]->id);
    if (j < 0)
        return -1;
    dest_object.pos = box_dests[j]->frame->GetWorldPos();
    dest_object.radius_2 = 1.0f;
    dest_object.matrix = NULL;
    dest_object.flags = 0;
    dest_object.frame = box_dests[j]->frame;
    dest_object.actor = NULL;
    dest_object.item.id = GAMEITEM_SPECIAL_ACTION;
    dest_object.item.ammo_loaded = USING_ACTION_DROP_BOX;
    dest_object.item.using_object = &dest_object;
    dest_object.sector = NULL;
    using_object.AddObject(&dest_object);
    BoxPlacement_SaveInfo(frm);
    return id;
}

// "Drop box" again at the last destination of the player's box's id, while he carries one (PS2 name)
// FUNCTION: GAME 0x5b3590
void C_game::BoxPlacementReposeDestPlayer()
{
    if (!act_mission->GetGame()->GetPlayer()->box)
        return;
    using_object.DelObject(&dest_object);
    int j;
    for (j = box_dests.size(); j--; )
        if (act_mission->GetGame()->GetPlayer()->box_index == box_dests[j]->id)
            break;
    if (j < 0)
        return;
    dest_object.pos = box_dests[j]->frame->GetWorldPos();
    dest_object.radius_2 = 1.0f;
    dest_object.matrix = NULL;
    dest_object.flags = 0;
    dest_object.frame = box_dests[j]->frame;
    dest_object.actor = NULL;
    dest_object.item.id = GAMEITEM_SPECIAL_ACTION;
    dest_object.item.ammo_loaded = USING_ACTION_DROP_BOX;
    dest_object.item.using_object = &dest_object;
    dest_object.sector = NULL;
    using_object.AddObject(&dest_object);
}

// Where the box is, kept for BoxPlacement_RestoreInfo and BoxPlacement_GetStartPoint (PS2 name), with its frame's
// placement for Done: its parent, local position and direction, once
// FUNCTION: GAME 0x5b36b0
void C_game::BoxPlacement_SaveInfo(I3D_frame *frm)
{
    AddFramePlacement(frm);
    for (int i = box_placement_temp.size(); i--; )
        if (box_placement_temp[i].frame == frm)
        {
            box_placement_temp[i] = box_placement_temp.back();
            box_placement_temp.pop_back();
        }
    S_box_placement_temp temp;
    temp.frame = frm;
    temp.parent = frm->GetParent();
    temp.pos = frm->GetPos();
    temp.dir = frm->GetDir();
    box_placement_temp.push_back(temp);
}

// What BoxPlacement_SaveInfo kept of the box forgotten (PS2 name; SETMODELTOCAR's, before the frame goes into a model)
// FUNCTION: GAME 0x5b3a90
void C_game::BoxPlacement_DelInfo(I3D_frame *frm)
{
    for (int i = box_placement_temp.size(); i--; )
        if (box_placement_temp[i].frame == frm)
        {
            box_placement_temp[i] = box_placement_temp.back();
            box_placement_temp.pop_back();
            return;
        }
}

// The box put back where BoxPlacement_SaveInfo found it (PS2 name), and that forgotten
// FUNCTION: GAME 0x5b3b10
void C_game::BoxPlacement_RestoreInfo(I3D_frame *frm)
{
    for (int i = box_placement_temp.size(); i--; )
        if (box_placement_temp[i].frame == frm)
        {
            frm->SetPos(box_placement_temp[i].pos);
            frm->SetDir(box_placement_temp[i].dir, 0.0f);
            frm->Update();
            frm->LinkTo(box_placement_temp[i].parent, 0);
            box_placement_temp[i] = box_placement_temp.back();
            box_placement_temp.pop_back();
            return;
        }
}

// Where the box was taken from, in world space: false if BoxPlacement_SaveInfo didn't keep it (PS2 name)
// FUNCTION: GAME 0x5b3c00
bool C_game::BoxPlacement_GetStartPoint(I3D_frame *frm, S_vector &pos)
{
    for (int i = box_placement_temp.size(); i--; )
        if (box_placement_temp[i].frame == frm)
        {
            pos = box_placement_temp[i].pos;
            pos *= box_placement_temp[i].parent->GetMatrix();
            return true;
        }
    return false;
}

// The box put down at the destination dest (PS2 name): placed as dest, which is used up, and placed again with no id.
// On a car the box rides along: placed only if the car's special dynamic collisions are on, else it can't be taken,
// out of the collisions; on another actor it isn't placed.
// FUNCTION: GAME 0x5b3c80
void C_game::BoxPlacementGetNextDest(I3D_frame *frm, I3D_frame *dest)
{
    if (!dest || !frm)
        return;
    frm->SetPos(dest->GetPos());
    frm->SetDir(dest->GetDir(), 0.0f);
    frm->Update();
    frm->LinkTo(dest->GetParent(), 0);
    int i;
    for (i = box_dests.size(); i--; )
        if (box_dests[i]->frame == dest)
        {
            box_dests.erase(box_dests.begin() + i);
            break;
        }
    C_actor *act = NULL;
    for (I3D_frame *f = dest; f; f = f->GetParent())
    {
        C_actor **p = GetFrameActor(f);
        if (p && *p)
        {
            act = *p;
            break;
        }
    }
    if (act)
    {
        if (act->GetType() == ACTOR_CAR)
        {
            BoxPlacementAdd(frm, BOXID_NONE);
            if (!((C_car *)act)->GetSpecialDynamicColls())
            {
                for (i = boxes.size(); i--; )
                    if (boxes[i]->frame == frm)
                    {
                        boxes[i]->on = false;
                        gifCollision.DynErase(&boxes[i]->coll);
                        break;
                    }
            }
            else
            {
                for (i = boxes.size(); i--; )
                    if (boxes[i]->frame == frm)
                    {
                        gifCollision.DynInsert(&boxes[i]->coll);
                        break;
                    }
            }
        }
    }
    else
        BoxPlacementAdd(frm, BOXID_NONE);
}

// Where the human takes a box of the id (PS2 name): carrying one, the last destination of the id; else the box of
// the id nearest to him by twice the squared distance plus the square of its height under 10 m (the higher ones first;
// PS2's sums the same); NULL for none
// FUNCTION: GAME 0x5b4010
I3D_frame *C_game::BoxPlacementGetEnemyDest(C_human *human, int id)
{
    if (human->box)
    {
        for (int i = box_dests.size(); i--; )
            if (box_dests[i]->id == id)
                return box_dests[i]->frame;
        return NULL;
    }
    int found = -1;
    float best = FLT_MAX;
    for (int i = boxes.size(); i--; )
    {
        if (boxes[i]->id != id)
            continue;
        S_vector pos = boxes[i]->frame->GetWorldPos();
        float h = 10.0f - pos.y;
        S_vector d = pos - human->GetFrame()->GetWorldPos();
        float dist = d.Square();
        dist += h * h;
        dist += d.Square();
        if (dist < best)
        {
            best = dist;
            found = i;
        }
    }
    if (found >= 0)
        return boxes[found]->frame;
    return NULL;
}

// How many boxes and destinations have the id (PS2 names; FUCKINGBOX_GETNUMBOX's and FUCKINGBOX_GETNUMDEST's)
// FUNCTION: GAME 0x5b4190
int C_game::BoxPlacement_GetBox(int id)
{
    int count = 0;
    for (int i = boxes.size(); i--; )
        if (boxes[i]->id == id)
            count++;
    return count;
}

// FUNCTION: GAME 0x5b41d0
int C_game::BoxPlacement_GetBoxDest(int id)
{
    int count = 0;
    for (int i = box_dests.size(); i--; )
        if (box_dests[i]->id == id)
            count++;
    return count;
}

// The first box of the id put down at the last destination of the id, as the player would (PS2 name;
// FUCKINGBOX_MOVE's)
// FUNCTION: GAME 0x5b4210
void C_game::BoxPlacement_MoveBoxToDest(int id)
{
    I3D_frame *box = NULL;
    for (int i = boxes.size(); i--; )
        if (boxes[i]->id == id)
            box = boxes[i]->frame;
    I3D_frame *dest = NULL;
    for (uint j = 0; j < box_dests.size(); j++)
        if (box_dests[j]->id == id)
            dest = box_dests[j]->frame;
    if (box && dest)
    {
        act_mission->GetGame()->BoxPlacementDestSetOU(true, box);
        act_mission->GetGame()->BoxPlacementDel(box);
        act_mission->GetGame()->BoxPlacementRecompile();
        act_mission->GetGame()->BoxPlacementGetNextDest(box, dest);
        act_mission->GetGame()->BoxPlacementDestSetOU(false, box);
        BoxPlacementReposeDestPlayer();
    }
}

// The car's special dynamic collisions turned on or off (PS2 name; CAR_SETSPECCOLL's): the boxes on it can be taken
// and are in the collisions where they are now, or neither
// FUNCTION: GAME 0x5b42e0
void C_game::BoxRecompileBoxForCar(C_car *car, bool on)
{
    if (!car)
        return;
    int i;
    I3D_frame *f;
    if (on)
    {
        for (i = boxes.size(); i--; )
            for (f = boxes[i]->frame; f; f = f->GetParent())
                if (f == car->GetFrame())
                {
                    boxes[i]->on = true;
                    I3D_bvolume vol = boxes[i]->frame->GetHRBoundVolume();
                    ((I3D_bsphere &)boxes[i]->coll.bsphere).pos = vol.bsphere.pos * boxes[i]->frame->GetMatrix();
                    boxes[i]->inv_matrix = boxes[i]->frame->GetMatrix();
                    S_matrix &m = boxes[i]->inv_matrix;
                    m.Inverse(S_matrix(m));
                    boxes[i]->coll.inv_matrix = &boxes[i]->inv_matrix;
                    gifCollision.DynInsert(&boxes[i]->coll);
                    break;
                }
    }
    else
    {
        for (i = boxes.size(); i--; )
            for (f = boxes[i]->frame; f; f = f->GetParent())
                if (f == car->GetFrame())
                {
                    boxes[i]->on = false;
                    gifCollision.DynErase(&boxes[i]->coll);
                    break;
                }
    }
    BoxPlacementRecompile();
}

// How many boxes of the id lie on the car (PS2 name, Czech and rude for that; POCZKURVENYCHBEDENCAR's)
// FUNCTION: GAME 0x5b44e0
int C_game::BoxPlacementPoPiciNaZkurvenemAute(C_car *car, int id)
{
    int count = 0;
    for (int i = boxes.size(); i--; )
    {
        if (boxes[i]->id != id)
            continue;
        for (I3D_frame *f = boxes[i]->frame; f; f = f->GetParent())
            if (GetFrameActor(f) && *GetFrameActor(f) && *GetFrameActor(f) == car)
            {
                count++;
                break;
            }
    }
    return count;
}

// An actor DeathActorProcess kills, once (PS2 name): DEATHACTOR_ADD's
// FUNCTION: GAME 0x5b4560
void C_game::DeathActorAdd(C_actor *act)
{
    for (int i = death_actors.size(); i--;)
        if (death_actors[i] == (uint)act)
            return;
    death_actors.push_back((uint &)act);
}

// The actors DeathActorAdd added killed, the humans among them (ACTOR_PLAYER, ACTOR_ENTITY), and the list emptied (PS2
// name)
// FUNCTION: GAME 0x5b4720
void C_game::DeathActorProcess()
{
    for (int i = death_actors.size(); i--;)
    {
        C_actor *act = (C_actor *)death_actors[i];
        switch (act->type)
        {
        case ACTOR_PLAYER:
        case ACTOR_ENTITY:
            ((C_human *)act)->_intern_ForceDeath();
        }
    }
    death_actors.clear();
}

// The actor of the number (C_actor::number), the last of global_actors that has it, NULL for none (PS2 name)
// FUNCTION: GAME 0x5b4790
C_actor *C_game::GetActorFromGUID(uint id)
{
    for (int i = C_actor::global_actors.size(); i--;)
        if (C_actor::global_actors[i]->number == id)
            return C_actor::global_actors[i];
    return NULL;
}

// The size of the game's record in a saved game (PS2 name): S_game_savegame, the chases', the stacks' items, the using
// objects' and with full the videos' (each name with a byte more than SaveGameSave writes)
// FUNCTION: GAME 0x5b47d0
uint C_game::SaveGameGetSize(bool full)
{
    uint size = sizeof(S_game_savegame) + sledovacka.SaveGameGetSize();
    size += character_stack.size() * sizeof(float);
    for (uint i = 0; i < inventory_stack.size(); i++)
        size += inventory_stack[i]->SaveGameGetSize();
    size += float_stack.size() * sizeof(float);
    if (full)
    {
        size += i_show_fades.size() * sizeof(S_i_show_fade);
        size += i_shows.size() * sizeof(S_i_show_savegame);
        for (int i = i_shows.size(); i--;)
            if (!(i_shows[i].flags & ISHOWF_CITY_MUSIC) && i_shows[i].name)
                size += strlen(i_shows[i].name) + 1;
    }
    size += using_object.SaveGameGetSize();
    return size;
}

// The game's record of a saved game into *buf, which it advances (PS2 name): S_game_savegame, the chases', with full
// the videos (but the city's music: a playing one with its position and its volume relative to the music's) and their
// fades, then the stacks' items and the using objects'; false if the chases' fails
// FUNCTION: GAME 0x5b4920
bool C_game::SaveGameSave(void **buf, bool full)
{
    S_game_savegame *rec = (S_game_savegame *)*buf;
    rec->version = GAME_SAVEGAME_VERSION;
    rec->game_time = game_time;
    rec->game_frames = game_frames;
    rec->mission_number = act_mission->GetMissionNumber();
    rec->timer = mafiaIndicators.show_flags & INDF_TIMER ? true : false;
    rec->num_inventories = inventory_stack.size();
    rec->num_characters = character_stack.size();
    rec->num_floats = float_stack.size();
    rec->num_i_shows = 0;
    if (full)
    {
        rec->num_i_show_fades = i_show_fades.size();
        rec->next_i_show_id = next_i_show_id;
        rec->city_music = city_music;
        rec->city_music_old = city_music_old;
    }
    else
    {
        rec->num_i_show_fades = 0;
        rec->next_i_show_id = 0;
        rec->city_music = -1;
        rec->city_music_old = -1;
    }
    rec->score_on = score_on;
    rec->score = score;
    mafiaIndicators.TimerGetInterval(&rec->timer_interval);
    mafiaIndicators.TimerGetTime(&rec->timer_hour, &rec->timer_minute, &rec->timer_second);
    *buf = (uchar *)*buf + sizeof(S_game_savegame);
    if (!sledovacka.SaveGameSave(buf))
        return false;
    if (full)
    {
        uint i;
        for (i = 0; i < i_shows.size(); i++)
        {
            if (i_shows[i].flags & ISHOWF_CITY_MUSIC)
                continue;
            rec->num_i_shows++;
            S_i_show_savegame *s = (S_i_show_savegame *)*buf;
            s->flags = i_shows[i].flags;
            s->id = i_shows[i].id;
            s->playing = false;
            s->pos = 0;
            s->unknown_11 = 0;
            s->volume = 1.0f;
            if (i_shows[i].show)
            {
                i_shows[i].show->GetEvent();
                s->playing = !i_shows[i].show->GetEvent();
                if (s->playing)
                {
                    if (mafiaLoadSave.settings.music_volume > .01f)
                    {
                        s->volume = i_shows[i].show->GetVolume();
                        s->volume /= mafiaLoadSave.settings.music_volume;
                        if (s->volume > 1.0f)
                            s->volume = 1.0f;
                    }
                    else
                        s->volume = 0.0f;
                    uint pos;
                    i_shows[i].show->GetCurrentPos(&pos);
                    s->unknown_11 = 0;
                    s->pos = pos;
                }
            }
            if (i_shows[i].name)
                s->name_len = strlen(i_shows[i].name);
            else
                s->name_len = 0;
            *buf = (uchar *)*buf + sizeof(S_i_show_savegame);
            if (s->name_len)
            {
                memcpy(*buf, i_shows[i].name, s->name_len);
                *buf = (uchar *)*buf + s->name_len;
            }
        }
        for (i = 0; i < i_show_fades.size(); i++)
        {
            memcpy(*buf, &i_show_fades[i], sizeof(S_i_show_fade));
            *buf = (uchar *)*buf + sizeof(S_i_show_fade);
        }
    }
    if (rec->num_characters)
    {
        memcpy(*buf, &character_stack[0], rec->num_characters * sizeof(float));
        *buf = (uchar *)*buf + character_stack.size() * sizeof(float);
    }
    if (rec->num_inventories)
        for (int i = 0; i < rec->num_inventories; i++)
            inventory_stack[i]->SaveGameSave(buf);
    if (rec->num_floats)
    {
        memcpy(*buf, &float_stack[0], rec->num_floats * sizeof(float));
        *buf = (uchar *)*buf + float_stack.size() * sizeof(float);
    }
    using_object.SaveGameSave(buf);
    return true;
}

// The game's record of a saved game from *buf, which it advances (PS2 name): SaveGameSave's, the videos opened again
// (the stacks emptied first); false for another version or if the chases' fails
// FUNCTION: GAME 0x5b4d10
bool C_game::SaveGameLoad(void **buf)
{
    S_game_savegame *rec = (S_game_savegame *)*buf;
    if (rec->version != GAME_SAVEGAME_VERSION)
        return false;
    city_music = rec->city_music;
    city_music_old = rec->city_music_old;
    game_time = rec->game_time;
    game_frames = rec->game_frames;
    if (rec->timer)
    {
        mafiaIndicators.show_flags |= INDF_TIMER;
        mafiaIndicators.TimerSetTime(rec->timer_hour, rec->timer_minute, rec->timer_second);
        mafiaIndicators.TimerSetInterval(rec->timer_interval);
    }
    else
        mafiaIndicators.show_flags &= ~INDF_TIMER;
    *buf = (uchar *)*buf + sizeof(S_game_savegame);
    if (!sledovacka.SaveGameLoad(buf))
        return false;
    FloatStackClear();
    InventoryStackClear();
    CharacterStackClear();
    score_on = rec->score_on;
    score = rec->score;
    ScoreSetOn(score_on);
    if (score_on)
        ScoreSet(score);
    int i;
    for (i = i_shows.size(); i--;)
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
    next_i_show_id = rec->next_i_show_id;
    if (rec->num_i_shows)
    {
        i_shows.resize(rec->num_i_shows);
        for (i = 0; i < rec->num_i_shows; i++)
        {
            S_i_show_savegame *s = (S_i_show_savegame *)*buf;
            *buf = (uchar *)*buf + sizeof(S_i_show_savegame);
            i_shows[i].flags = s->flags;
            i_shows[i].id = s->id;
            i_shows[i].show = igraph->CreateIShow();
            if (s->name_len)
            {
                i_shows[i].name = new char[s->name_len + 1];
                memcpy(i_shows[i].name, *buf, s->name_len);
                i_shows[i].name[s->name_len] = 0;
                *buf = (uchar *)*buf + s->name_len;
                i_shows[i].show->Open(i_shows[i].name, 0);
                if (s->playing)
                {
                    i_shows[i].show->Play();
                    i_shows[i].show->SetCurrentPos(s->pos);
                    i_shows[i].show->SetVolume(mafiaLoadSave.settings.music_volume * s->volume);
                }
            }
            else
                i_shows[i].name = NULL;
        }
    }
    if (rec->num_i_show_fades)
    {
        i_show_fades.resize(rec->num_i_show_fades);
        for (i = 0; i < rec->num_i_show_fades; i++)
        {
            i_show_fades[i] = *(S_i_show_fade *)*buf;
            *buf = (uchar *)*buf + sizeof(S_i_show_fade);
        }
    }
    if (rec->num_characters)
    {
        character_stack.resize(rec->num_characters);
        memcpy(&character_stack[0], *buf, rec->num_characters * sizeof(float));
        *buf = (uchar *)*buf + rec->num_characters * sizeof(float);
    }
    if (rec->num_inventories)
        for (i = 0; i < rec->num_inventories; i++)
        {
            G_Inventory *inv = new G_Inventory;
            inv->SaveGameLoad(buf);
            inventory_stack.push_back(inv);
        }
    if (rec->num_floats)
    {
        float_stack.resize(rec->num_floats);
        memcpy(&float_stack[0], *buf, rec->num_floats * sizeof(float));
        *buf = (uchar *)*buf + rec->num_floats * sizeof(float);
    }
    GetUsingObject().SaveGameLoad(buf);
    return true;
}

// The mission's number from a saved game's record, before the game's Init (PS2 name): false for another version
// FUNCTION: GAME 0x5b5410
bool C_game::SaveGameLoadBefore(void **buf)
{
    S_game_savegame *rec = (S_game_savegame *)*buf;
    if (rec->version != GAME_SAVEGAME_VERSION)
        return false;
    act_mission->SetMissionNumber(rec->mission_number);
    act_mission->KeepMissionNumber();
    return true;
}

// An actor's frame whose lightness LightnessTick keeps up (PS2 name; PS2's is empty), once: a visual of a type that has
// SetLightness, or with LIGHTNESSF_SUBTREE any frame
// FUNCTION: GAME 0x5b5450
void C_game::LightnessAdd(C_actor *act, I3D_frame *frm, ushort flags, S_vector &pos)
{
    if (!frm)
        return;
    for (int i = lightness.size(); i--;)
        if (lightness[i]->frame == frm)
            return;
    if (!(flags & LIGHTNESSF_SUBTREE))
    {
        if (frm->GetType() != FRAME_VISUAL)
            return;
        switch (((I3D_visual *)frm)->GetVisualType())
        {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_SINGLEMESH:
        case I3D_VISUAL_LENSF:
        case I3D_VISUAL_MORPH:
        case I3D_VISUAL_SINGLEMORPH:
        case I3D_VISUAL_PART_BASE:
        case I3D_VISUAL_PART_ELEMENT:
        case I3D_VISUAL_LANDPATCH:
            break;
        default:
            return;
        }
    }
    S_lightness *l = new S_lightness;
    l->actor = act;
    l->frame = frm;
    l->flags = flags;
    l->pos = pos;
    l->lightness = 1.0f;
    l->target = 1.0f;
    l->time = 0;
    lightness.push_back(l);
}

// The frame's lightness forgotten (PS2 name), the last entry taking its place
// FUNCTION: GAME 0x5b56f0
void C_game::LightnessDel(I3D_frame *frm)
{
    for (int i = lightness.size(); i--;)
    {
        if (lightness[i]->frame == frm)
        {
            delete lightness[i];
            lightness[i] = lightness.back();
            lightness.pop_back();
            return;
        }
    }
}

// Deletes the lightness entries (PS2 name)
// FUNCTION: GAME 0x5b5780
void C_game::LightnessClear()
{
    for (int i = lightness.size(); i--;)
        delete lightness[i];
    lightness.clear();
}

// The frames' lightness (PS2 name; PS2's is empty): one entry a frame, in turn, measures the scene's lightness at its
// actor's frame (offset by pos, the frame itself turned off meanwhile), at most every 250 ms and within 40 m of the
// camera, while the actor is rendered and active (an entry without an actor would read a NULL frame); every frame's
// lightness moves towards its target by 4 a second. One vector for the camera's distance and the point: the second is
// passed by reference, so the first is stored too.
// FUNCTION: GAME 0x5b57e0
void C_game::LightnessTick(uint time)
{
    if (lightness_next >= lightness.size())
        lightness_next = 0;
    S_vector pos = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    pos -= lightness[lightness_next]->frame->GetWorldPos();
    if (act_mission->GetGame()->GetGameTime() - lightness[lightness_next]->time > 250 && pos.Square() < 1600.0f)
    {
        lightness[lightness_next]->time = act_mission->GetGame()->GetGameTime();
        I3D_frame *frm = NULL;
        I3D_frame *sector = NULL;
        pos = lightness[lightness_next]->pos;
        bool measure = true;
        if (lightness[lightness_next]->actor)
        {
            frm = lightness[lightness_next]->actor->GetFrame();
            if (frm)
            {
                pos += frm->GetWorldPos();
                sector = lightness[lightness_next]->actor->GetFrame();
                while (sector)
                {
                    if (sector->GetType() == FRAME_SECTOR)
                        break;
                    sector = sector->GetParent();
                }
            }
            if (!lightness[lightness_next]->actor->GetRendered() ||
                lightness[lightness_next]->actor->GetActState() != ACTSTATE_ACTIVE ||
                lightness[lightness_next]->flags & LIGHTNESSF_SUBTREE)
                measure = false;
        }
        if (measure)
        {
            bool on = frm->IsOn();
            frm->SetOn(false);
            lightness[lightness_next]->target = act_mission->GetScene()->GetLightness(pos, (I3D_sector *)sector,
                I3D_LIGHTNESS_TESTCOL, lightness[lightness_next]->frame);
            frm->SetOn(on);
        }
    }
    lightness_next++;
    float step = time * .004f;
    for (int i = lightness.size(); i--;)
    {
        if ((float)fabs(lightness[i]->target - lightness[i]->lightness) > MRG_ZERO)
        {
            if ((float)fabs(lightness[i]->target - lightness[i]->lightness) < step)
                lightness[i]->lightness = lightness[i]->target;
            else if (lightness[i]->target > lightness[i]->lightness)
                lightness[i]->lightness += step;
            else
                lightness[i]->lightness -= step;
            if (lightness[i]->flags & LIGHTNESSF_SUBTREE)
                S_hlp::cbLightness(lightness[i]->frame, lightness[i]->lightness);
            else
                ((I3D_object *)lightness[i]->frame)->SetLightness(lightness[i]->lightness);
        }
    }
}

// LightnessTick's walk (the name is ours): the lightness given to the subtree's visuals of the types that have it
// FUNCTION: GAME 0x5b5ad0
void C_game::S_hlp::cbLightness(I3D_frame *frm, float lightness)
{
    if (frm->GetType() == FRAME_VISUAL)
    {
        switch (((I3D_visual *)frm)->GetVisualType())
        {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_SINGLEMESH:
        case I3D_VISUAL_MORPH:
        case I3D_VISUAL_SINGLEMORPH:
            ((I3D_object *)frm)->SetLightness(lightness);
        }
    }
    for (int i = frm->NumChildren(); i--;)
        cbLightness(frm->GetChild(i), lightness);
}

// A shot's light at pos (PS2 name; PS2's is empty): the first of the ten that is off, from the last, linked into the
// scene's sector there at range (from half of it), with a random power, for time ms
// FUNCTION: GAME 0x5b5b40
void C_game::AddShootLight(S_vector &pos, float range, int time, ushort flags)
{
    for (int i = 10; i--;)
    {
        if (shoot_lights[i].time < 1)
        {
            shoot_lights[i].light->SetRange(range * .5f, range);
            shoot_lights[i].light->SetPos(pos);
            shoot_lights[i].light->Update();
            shoot_lights[i].light->LinkTo(NULL, 0);
            act_mission->GetScene()->SetFrameSector(shoot_lights[i].light);
            I3D_frame *parent = shoot_lights[i].light->GetParent();
            if (parent && parent->GetType() == FRAME_SECTOR)
                ((I3D_sector *)parent)->AddLight(shoot_lights[i].light);
            shoot_lights[i].light->SetOn(true);
            shoot_lights[i].light->SetDiffusePower(rnd() * .4f + .8f);
            shoot_lights[i].time = time;
            shoot_lights[i].flags = flags;
            return;
        }
    }
}

// The light cache filled again (PS2 name): the point and spot lights of the scene's primary sector's subtree, which
// LightCacheTick turns on near the camera
// FUNCTION: GAME 0x5b5c80
void C_game::UseLightCacheSystem()
{
    light_cache.clear();
    light_cache_time = -1;
    S_hlp::cbLight(act_mission->GetScene()->GetPrimarySector(), light_cache);
}

// The point and spot lights of the frame's subtree added to the list, but for an actor's frames and their subtrees (PS2
// name)
// FUNCTION: GAME 0x5b5cd0
void C_game::S_hlp::cbLight(I3D_frame *frm, std::vector<I3D_light *> &lights)
{
    for (I3D_frame *child = frm->GetFirstChild(); child; child = child->m_pNext)
    {
        if (GetFrameActor(child) && *GetFrameActor(child))
            continue;
        if (child->GetType() == FRAME_LIGHT && ((I3D_light *)child)->ltype > I3DLIGHT_NULL &&
            ((I3D_light *)child)->ltype <= I3DLIGHT_SPOT)
            lights.push_back((I3D_light *)child);
        if (child->GetFirstChild())
            cbLight(child, lights);
    }
}

// The light cache's lights turned on or off at once by the camera (PS2 name), the next check as before
// FUNCTION: GAME 0x5b5d40
void C_game::RecomputeLightCache()
{
    light_cache_time = -1;
    LightCacheTick(0);
    light_cache_time = -1;
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

// The stream of the id stopped, closed and released, and taken from the list, the last one taking its place (PS2
// name), from the last one
// FUNCTION: GAME 0x5b62c0
void C_game::DestroyIShow(int show)
{
    for (int i = i_shows.size(); i--;)
    {
        if (i_shows[i].id == show)
        {
            if (i_shows[i].show)
            {
                i_shows[i].show->Stop();
                i_shows[i].show->Close();
                i_shows[i].show->Release();
            }
            delete[] i_shows[i].name;
            i_shows[i] = i_shows.back();
            i_shows.pop_back();
            return;
        }
    }
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

// Every video playing faded out over time ms (PS2 name), stopped at once while the music is all but off, from the last
// one
// FUNCTION: GAME 0x5b6660
void C_game::IShowFadeDownAll(int time)
{
    for (int i = i_shows.size(); i--;)
    {
        i_shows[i].show->GetEvent();
        if (i_shows[i].show->GetEvent() == ISHOW_EVENT_PLAYING)
        {
            if (mafiaLoadSave.settings.music_volume < .1f)
                i_shows[i].show->Stop();
            else
                IShowFade(i_shows[i].id, i_shows[i].volume, 0.0f, time);
        }
    }
}

// The stream's volume faded from from to to over time ms (PS2 name), which Tick runs: another fade of the stream is
// dropped, and the stream set to from at once
// FUNCTION: GAME 0x5b6710
void C_game::IShowFade(int id, float from, float to, int time)
{
    if (time < 1)
        return;
    for (int i = i_show_fades.size(); i--;)
    {
        if (i_show_fades[i].id == id)
        {
            i_show_fades[i] = i_show_fades.back();
            i_show_fades.pop_back();
            break;
        }
    }
    IShow *show = GetIShow(id);
    if (!show)
        return;
    show->SetVolume(mafiaLoadSave.settings.music_volume * from);
    S_i_show_fade f;
    f.id = id;
    f.from = from;
    f.to = to;
    f.time = 0;
    f.length = time;
    i_show_fades.push_back(f);
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

// The taxi's passenger is alive in a car, the player's car if the player is in one (PS2 name)
// FUNCTION: GAME 0x5b6d10
bool C_game::IsValidTaxiPassenger()
{
    if (!taxi_passenger || taxi_passenger->dead || !taxi_passenger->car || !player)
        return false;
    if (player->car && player->car != taxi_passenger->car)
        return false;
    return true;
}

// The semaphore of the model taken from the semaphores (PS2 name), which a crash object's destruction calls with its
// model. Nothing sets the semaphores' models (PS2's neither), so it finds none.
// FUNCTION: GAME 0x5b6d50
void C_game::DelSemaphore(I3D_frame *frm)
{
    if (!frm)
        return;
    for (int i = semaphores.size(); i--;)
    {
        if (frm == semaphores[i].model)
        {
            semaphores[i] = semaphores.back();
            semaphores.pop_back();
            return;
        }
    }
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

// The frame's telephone taken from the telephones, the last one taking its place (PS2 name)
// FUNCTION: GAME 0x5b7440
void C_game::DelTelephones(I3D_frame *frm)
{
    if (!frm)
        return;
    for (int i = telephones.size(); i--;)
    {
        if (frm == telephones[i].frame)
        {
            telephones[i] = telephones.back();
            telephones.pop_back();
            return;
        }
    }
}

// Whether the frame is one of the telephones (PS2 name), the scripts' question
// FUNCTION: GAME 0x5b7500
bool C_game::IsTelephoneExist(I3D_frame *frm)
{
    if (!frm)
        return false;
    for (int i = telephones.size(); i--;)
    {
        if (frm == telephones[i].frame)
            return true;
    }
    return false;
}

// A human speaking on a dubbing channel (PS2 name), the scripts' (C_program's), whose speech DeathHumanTalk stops when he
// dies
// FUNCTION: GAME 0x5b7560
void C_game::AddHumanTalk(C_human *human, int chanel)
{
    S_enemy_talk t;
    t.human = human;
    t.chanel = chanel;
    enemy_talks.push_back(t);
}

// The talks on the channel taken from the list, the last one taking each one's place (PS2 name; the human isn't
// compared). An entry that stays skips the next one, PS2's too.
// FUNCTION: GAME 0x5b7730
void C_game::DelHumanTalk(C_human *human, int chanel)
{
    for (uint i = 0; i < enemy_talks.size(); i++)
    {
        if (enemy_talks[i].chanel == chanel)
        {
            enemy_talks[i] = enemy_talks.back();
            enemy_talks.pop_back();
        }
        else
            i++;
    }
}

// The dead human's speech stopped and his talks taken from the list (PS2 name), as DelHumanTalk does it, the entry
// after each one that stays skipped too
// FUNCTION: GAME 0x5b77c0
void C_game::DeathHumanTalk(C_human *human)
{
    for (uint i = 0; i < enemy_talks.size(); i++)
    {
        if (enemy_talks[i].human == human)
        {
            DabingStop(enemy_talks[i].chanel);
            enemy_talks[i] = enemy_talks.back();
            enemy_talks.pop_back();
        }
        else
            i++;
    }
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

// A shot at a level's collision of control signals (PS2 name): the nearest visible model of the base blocks' traffic
// signs (signal_mark_idx) within 5 m of the hit in height and 10 m along the ground is broken into crash actors along
// the shot, then hidden for good, the collision disabled, and the police told of the shooter. The shot's start isn't
// used (PS2's neither).
// FUNCTION: GAME 0x5b7be0
void C_game::DestroyControlSignal(g_collision_header *coll, S_vector &pos, S_vector &from, S_vector &dir,
    C_human *human)
{
    if (!coll || !(coll->flags & COLLHF_CONTROL_SIGNAL) || (coll->flags & COLLHF_DISABLED))
        return;
    float best = FLT_MAX;
    S_cache_model_inbb *found = NULL;
    if (!base_cache.IsOpen())
        return;
    for (int i = base_cache.blocks.size(); i--;)
    {
        if (!base_cache.blocks[i].active)
            continue;
        for (int j = base_cache.blocks[i].models.size(); j--;)
        {
            if (!base_cache.blocks[i].models[j].visible)
                continue;
            for (int k = signal_mark_idx.size(); k--;)
            {
                if (base_cache.blocks[i].models[j].model == signal_mark_idx[k] &&
                    base_cache.blocks[i].models[j].visible)
                {
                    S_vector d = base_cache.blocks[i].models[j].pos;
                    d -= pos;
                    if (d.y * d.y < 25.0f)
                    {
                        d.y = 0.0f;
                        float dist = d.Square();
                        if (dist < best)
                        {
                            found = &base_cache.blocks[i].models[j];
                            best = dist;
                        }
                    }
                    break;
                }
            }
        }
    }
    if (found && best < 100.0f)
    {
        I3D_frame *frm = base_cache.FindModel(found->model, found->instance);
        if (frm)
        {
            char *name = ((I3D_model *)frm)->file_name ? ((I3D_model *)frm)->file_name : "";
            char *file = strrchr(name, '\\');
            if (file)
                name = file + 1;
            CreateCrashActors(frm, pos, dir, dir.Magnitude(), name, false, 0, 0, true, true);
        }
        coll->flags |= COLLHF_DISABLED;
        base_cache.DestroyFrame(found);
        if (act_mission->game->shvestky.active)
            act_mission->game->shvestky.DestroySignalMark(human);
    }
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

// The actor added to invisible_actors unless it is there (PS2 name), RACING_AUTOINVISIBLE's: Tick changes its state by
// whether it is seen
// FUNCTION: GAME 0x5b7f70
void C_game::AddInvisibleActor(C_actor *act)
{
    if (!act)
        return;
    for (int i = invisible_actors.size(); i--;)
    {
        if (invisible_actors[i].actor == act)
            return;
    }
    S_invisible_actor ia;
    ia.actor = act;
    ia.frame = 0;
    invisible_actors.push_back(ia);
}

// The scripts' actor registers waiting for an actor, filled through C_program::FindActorLater and forgotten (PS2 name),
// from the last
// FUNCTION: GAME 0x5b8150
void C_game::FindActorsVect()
{
    for (int i = find_actors.size(); i--;)
    {
        if (find_actors[i].program)
            find_actors[i].program->FindActorLater(find_actors[i].reg, find_actors[i].id);
    }
    find_actors.clear();
}

// A particle EMITPARTICLE created while the cutscene plays, which RECUNLOAD destroys (PC only, the name is ours)
// FUNCTION: GAME 0x5b81e0
void C_game::AddCutsceneParticle(int particle)
{
    cutscene_particles.push_back(particle);
}

// The cutscene's particles destroyed and forgotten, RECUNLOAD's (PC only, the name is ours)
// FUNCTION: GAME 0x5b8390
void C_game::DestroyCutsceneParticles()
{
    for (std::vector<int>::iterator it = cutscene_particles.begin(); it < cutscene_particles.end(); it++)
        act_mission->GetEffects()->DestroyParticle(*it);
    cutscene_particles.clear();
}

// Every actor's sounds off (PS2 name)
// FUNCTION: GAME 0x5b83f0
void C_game::StopAllSounds()
{
    for (int i = C_actor::global_actors.size(); i--;)
        C_actor::global_actors[i]->SoundsChange(0);
}

// Every sound paused, the videos' too (PS2 name): the dubbing's, then the scene's playing sounds (the primary and the
// backdrop sector's subtrees: PS2 walks a list of the sounds instead), each kept in stop_sounds with its volumes, loop
// and position and turned off; then each video that hasn't ended (no event), whose id goes to paused_i_shows. The scene
// through its accessors, whose pending calls leave the clears' copy out of line, as the original has it
// FUNCTION: GAME 0x5b8430
void C_game::PauseAllSounds()
{
    stop_sounds.clear();
    paused_i_shows.clear();
    DabingPauseSounds(true, false);
    S_hlp::cbPauseSounds(act_mission->GetScene()->GetPrimarySector(), stop_sounds);
    S_hlp::cbPauseSounds(act_mission->GetScene()->backdrop_sector, stop_sounds);
    for (int i = i_shows.size(); i--;)
    {
        if (!i_shows[i].show->GetEvent())
        {
            paused_i_shows.push_back(i_shows[i].id);
            i_shows[i].show->Pause();
        }
    }
}

// PauseAllSounds' walk of a frame's subtree, recursive (the name is ours): each playing sound kept in the list, then
// turned off
// FUNCTION: GAME 0x5b8540
void C_game::S_hlp::cbPauseSounds(I3D_frame *frm, std::vector<S_stop_sounds> &list)
{
    for (I3D_frame *child = frm->GetFirstChild(); child; child = child->m_pNext)
    {
        if (child->GetType() == FRAME_SOUND && ((I3D_sound *)child)->IsPlaying(false))
        {
            S_stop_sounds s;
            s.sound = (I3D_sound *)child;
            s.time = s.sound->GetCurrTime();
            s.volume = s.sound->volume;
            s.volume_out = s.sound->volume_out;
            s.loop = s.sound->loop;
            s.sound->SetOn(false, true);
            list.push_back(s);
        }
        if (child->GetFirstChild())
            cbPauseSounds(child, list);
    }
}

// What PauseAllSounds paused played again (PS2 name): the sounds with their volumes, loop and position, with i_shows
// the videos too (those still there), then the dubbing's (the parameter's name is ours)
// FUNCTION: GAME 0x5b85f0
void C_game::PlayAllSounds(bool i_shows_too)
{
    for (int i = stop_sounds.size(); i--;)
    {
        stop_sounds[i].sound->SetLoop(stop_sounds[i].loop);
        stop_sounds[i].sound->SetVolume(stop_sounds[i].volume);
        stop_sounds[i].sound->SetOutVol(stop_sounds[i].volume_out);
        stop_sounds[i].sound->SetOn(true);
        stop_sounds[i].sound->SetOn(true, true);
        stop_sounds[i].sound->SetCurrTime(stop_sounds[i].time);
        stop_sounds[i].sound->Update();
        stop_sounds[i].sound->ForceUpdate();
    }
    if (i_shows_too)
    {
        for (int j = paused_i_shows.size(); j--;)
        {
            IShow *show = GetIShow(paused_i_shows[j]);
            if (show)
                show->Play();
        }
    }
    stop_sounds.clear();
    paused_i_shows.clear();
    DabingPauseSounds(false, false);
}

// A scratch on the car's frame (PS2 name; C_car's Deform_Vehicle_Part, the vehicle's callback, passes the hit): on an
// object of the car but for its parts of types 2 to 5 and 7, the scratch ScratchGetFreeIdx gives is put where the hit
// was, turned toward the car's position (around its axis at random while the car stands, else up along its velocity),
// sized by the force (divided by the car's mass above 1, 1/55 of it, 0.2 to 1) and as opaque as it was hit there (0.3
// and 0.1 for each of the first five hits), and projected on the frame alone, which takes it as projector mode 1
// meanwhile. Where it lands on no face it is given back (the parameters' names are ours)
// FUNCTION: GAME 0x5b8780
void C_game::ScratchAdd(C_car *car, I3D_frame *frm, uint part_type, const S_vector &pos, const S_vector &force)
{
    if (!car || !frm)
        return;
    if (frm->GetType() != FRAME_VISUAL || ((I3D_visual *)frm)->GetVisualType() != I3D_VISUAL_OBJECT)
        return;
    switch (part_type)
    {
    case 2:
    case 3:
    case 4:
    case 5:
    case 7:
        return;
    }
    int idx = ScratchGetFreeIdx(car, frm, pos);
    if (idx < 0)
        return;
    if (!scratches[idx].frame)
        return;
    if (scratches[idx].car_frame)
        scratches[idx].frame->LinkTo(NULL, 0);
    scratches[idx].car = car;
    scratches[idx].car_frame = frm;
    SHADOW_MODE mode = ((I3D_visual *)frm)->GetProjectorMode();
    ((I3D_visual *)frm)->SetProjectorMode(SHADOW_MODE_1);
    ScratchRecompile(car, frm);
    scratches[idx].frame->SetOn(true);
    scratches[idx].frame->SetPos(pos);
    S_vector dir = car->world_pos - pos;
    S_vector vel = car->GetVelocity();
    if (vel.Square() < .1f)
        scratches[idx].frame->SetDir(dir, rnd() * (2.0f * PI));
    else
        scratches[idx].frame->SetDir1(dir, vel);
    float scale = force.Magnitude();
    if (car->mass > 1.0f)
        scale /= car->mass;
    scale /= 55.0f;
    if (scale > 1.0f)
        scale = 1.0f;
    else if (scale < .2f)
        scale = .2f;
    scratches[idx].frame->SetScale(S_vector(scale, scale, 1.0f));
    scratches[idx].frame->Update();
    scratches[idx].frame->LinkTo(frm, I3DLINK_UPDMATRIX);
    ((I3D_projector *)scratches[idx].frame)->SetMode(PROJECTOR_MODE_BLEND);
    ((I3D_projector *)scratches[idx].frame)->SetOrthogonal(true);
    ((I3D_projector *)scratches[idx].frame)->SetTexture(track_textures[TRACK_TEXTURE_SCRATCH]);
    ((I3D_projector *)scratches[idx].frame)->SetAlpha(min(5, scratches[idx].hits) * .1f + .3f);
    float light = act_mission->scene->GetLightness(pos, NULL, 0, NULL);
    S_vector color(light, light, light);
    ((I3D_projector *)scratches[idx].frame)->SetColor(color);
    ((I3D_projector *)scratches[idx].frame)->MakeProjection(PROJF_NO_CHILDREN, frm);
    if (!(((I3D_projector *)scratches[idx].frame)->num_faces + ((I3D_projector *)scratches[idx].frame)->num_faces_t))
    {
        scratches[idx].car = NULL;
        scratches[idx].car_frame = NULL;
        scratches[idx].frame->SetOn(false);
        scratches[idx].frame->LinkTo(NULL, 0);
    }
    ((I3D_visual *)frm)->SetProjectorMode(mode);
}

// The car's scratches on the frame projected again (PS2 name), the frame taken as projector mode 1 meanwhile. The
// frame the scratch has in a local, as PS2's keeps it too: compared with frm, its calls go through it
// FUNCTION: GAME 0x5b8bb0
void C_game::ScratchRecompile(C_car *car, I3D_frame *frm)
{
    for (int i = 100; i--;)
    {
        if (scratches[i].car == car)
        {
            I3D_visual *vis = (I3D_visual *)scratches[i].car_frame;
            if (vis == frm)
            {
                SHADOW_MODE mode = vis->GetProjectorMode();
                vis->SetProjectorMode(SHADOW_MODE_1);
                ((I3D_projector *)scratches[i].frame)->MakeProjection(PROJF_NO_CHILDREN, scratches[i].car_frame);
                vis->SetProjectorMode(mode);
            }
        }
    }
}

// The car's scratches unlinked and turned off (PS2 name); they stay the car's until another hit takes them
// FUNCTION: GAME 0x5b8c20
void C_game::ScratchDeleteFromCar(C_car *car)
{
    for (int i = 100; i--;)
    {
        if (scratches[i].car == car && scratches[i].frame)
        {
            scratches[i].frame->LinkTo(NULL, 0);
            scratches[i].frame->SetOn(false);
        }
    }
}

// The scratch to use for a hit at pos on the car's frame (PS2 name): once three of its scratches there are within 0.4
// m, the one of them hit least, its hits counted; else a scratch no car has, its hits cleared; else the farthest from
// the camera, if one of another car is more than 5 m from it, its hits cleared (-1 if none is)
// FUNCTION: GAME 0x5b8c60
int C_game::ScratchGetFreeIdx(C_car *car, I3D_frame *frm, const S_vector &pos)
{
    int near_idx[3] = { -1, -1, -1 };
    int num_near = 0;
    int i;
    for (i = 100; i--;)
    {
        if (scratches[i].car == car && scratches[i].car_frame == frm)
        {
            if ((pos - scratches[i].frame->GetWorldPos()).Square() < .4f * .4f)
            {
                near_idx[num_near] = i;
                num_near++;
                if (num_near > 2)
                {
                    int m = 0;
                    if (scratches[near_idx[1]].hits < scratches[near_idx[m]].hits)
                        m = 1;
                    if (scratches[near_idx[2]].hits < scratches[near_idx[m]].hits)
                        m = 2;
                    scratches[near_idx[m]].hits++;
                    return near_idx[m];
                }
            }
        }
    }
    for (i = 100; i--;)
    {
        if (!scratches[i].car)
        {
            scratches[i].hits = 0;
            return i;
        }
    }
    int best = -1;
    float best_dist = 5.0f * 5.0f;
    S_vector cam_pos = act_mission->scene->GetActiveCamera()->GetWorldPos();
    for (i = 100; i--;)
    {
        if (scratches[i].car == car)
            continue;
        float dist = (cam_pos - scratches[i].frame->GetWorldPos()).Square();
        if (dist > best_dist)
        {
            best_dist = dist;
            best = i;
        }
    }
    scratches[best].hits = 0;
    return best;
}

// A frame the scripts created (MODEL_CREATE's), kept in created_frames (PS2 name): false for none or one there already
// FUNCTION: GAME 0x5b8e70
bool C_game::AddCreatedFrameToVector(I3D_frame *frm)
{
    if (!frm)
        return false;
    for (int i = created_frames.size(); i--;)
    {
        if (created_frames[i] == frm)
            return false;
    }
    created_frames.push_back(frm);
    return true;
}

// The frame taken from created_frames (PS2 name; MODEL_DESTROY's, which releases it then), the last one in its place:
// false if it isn't there
// FUNCTION: GAME 0x5b9040
bool C_game::DelCreatedFrameToVector(I3D_frame *frm)
{
    if (!frm)
        return false;
    for (int i = created_frames.size(); i--;)
    {
        if (created_frames[i] == frm)
        {
            created_frames[i] = created_frames.back();
            created_frames.pop_back();
            return true;
        }
    }
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
            a.area = (I3D_area *)frm;
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

// Which scream areas the camera is in, their ids ORed into camera_scream_areas (1 in none), and the same for each active
// player and entity, a meter above his position, into his scream_areas (PS2 name; Tick runs it every 513 ms). One
// position variable for both, as PS2's (one stack slot) and the original's frame have it
// FUNCTION: GAME 0x5b9390
void C_game::TickScreamAreas()
{
    S_vector pos = act_mission->scene->GetActiveCamera()->GetWorldPos();
    bool none = true;
    camera_scream_areas = 0;
    int i;
    for (i = scream_areas.size(); i--;)
    {
        if (scream_areas[i].area->CheckPoint(pos))
        {
            camera_scream_areas |= scream_areas[i].id;
            none = false;
        }
    }
    if (none)
        camera_scream_areas = 1;
    for (i = active_actors.size(); i--;)
    {
        if (active_actors[i]->type == ACTOR_PLAYER || active_actors[i]->type == ACTOR_ENTITY)
        {
            bool human_none = true;
            ushort ids = 0;
            pos = active_actors[i]->frame->GetWorldPos();
            pos.y += 1.0f;
            for (int j = scream_areas.size(); j--;)
            {
                if (scream_areas[j].area->CheckPoint(pos))
                {
                    human_none = false;
                    ids |= scream_areas[j].id;
                }
            }
            if (human_none)
                ids = 1;
            ((C_human *)active_actors[i])->scream_areas = ids;
        }
    }
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

// Adds a dead human to those TickDeadHumanForTick ticks, unless he is there already (PS2 name)
// FUNCTION: GAME 0x5b9690
void C_game::AddDeadHumanForTick(C_human *human)
{
    for (int i = dead_humans.size(); i--;)
        if (dead_humans[i] == (uint)human)
            return;
    // the parameter itself pushed, no temporary: a reference cast
    dead_humans.push_back((uint &)human);
}

// The human taken from dead_humans, the last one put in his place (PS2 name)
// FUNCTION: GAME 0x5b9850
void C_game::DelDeadHumanForTick(C_human *human)
{
    for (int i = dead_humans.size(); i--;)
    {
        if (dead_humans[i] == (uint)human)
        {
            dead_humans[i] = dead_humans.back();
            dead_humans.pop_back();
            return;
        }
    }
}

// The dead humans thrown out of a car (PS2 name): each one's model ticked, and his animations while they run, the
// frame following the human; once the active animation has ended, his body gets its "Search dead body" at the frame
// back1 (the using object the player searches it through, given to the player's using objects unless the human is in a
// car), and he is taken from the list
// FUNCTION: GAME 0x5b98c0
void C_game::TickDeadHumanForTick(uint time)
{
    for (uint i = 0; i < dead_humans.size();)
    {
        I3D_model *model = (I3D_model *)((C_human *)dead_humans[i])->frame;
        model->Tick(time);
        if (((C_human *)dead_humans[i])->anim_machine.IsActive())
        {
            ((C_human *)dead_humans[i])->anim_machine.Tick(time);
            ((C_human *)dead_humans[i])->frame->SetPos(((C_human *)dead_humans[i])->GetPos());
            ((C_human *)dead_humans[i])->frame->SetDir(((C_human *)dead_humans[i])->GetDir(), 0.0f);
            ((C_human *)dead_humans[i])->frame->Update();
        }
        uint index = ((C_human *)dead_humans[i])->anim_machine.anim_index;
        if (model->AnimIsOn(index))
        {
            i++;
            continue;
        }
        I3D_frame *back = ((C_human *)dead_humans[i])->frame->FindChildFrame("back1", ENUMF_ALL);
        if (back)
        {
            ((C_human *)dead_humans[i])->using_objects[0].pos = back->GetWorldPos();
            ((C_human *)dead_humans[i])->using_objects[0].pos.y += .5f;
            ((C_human *)dead_humans[i])->using_objects[0].radius_2 = .7f;
            ((C_human *)dead_humans[i])->using_objects[0].matrix = NULL;
            ((C_human *)dead_humans[i])->using_objects[0].flags = 0;
            ((C_human *)dead_humans[i])->using_objects[0].frame = NULL;
            ((C_human *)dead_humans[i])->using_objects[0].actor = (C_human *)dead_humans[i];
            ((C_human *)dead_humans[i])->using_objects[0].item.id = GAMEITEM_SPECIAL_ACTION;
            ((C_human *)dead_humans[i])->using_objects[0].item.ammo_loaded = USING_ACTION_SEARCH_DEAD_BODY;
            ((C_human *)dead_humans[i])->using_objects[0].item.using_object =
                &((C_human *)dead_humans[i])->using_objects[0];
            if (!((C_human *)dead_humans[i])->car)
                act_mission->GetGame()->GetUsingObject().AddObject(&((C_human *)dead_humans[i])->using_objects[0]);
        }
        dead_humans[i] = dead_humans.back();
        dead_humans.pop_back();
    }
}

// A hydrant's water at the frame (PS2 name), only with the particles at High and within 150 m of the camera: the
// effects' particle at the frame's position, spraying up, and a looping sound there
// FUNCTION: GAME 0x5b9b90
void C_game::AddHydrant(I3D_frame *frm)
{
    if (act_mission->options.particles < OPTION_HIGH)
        return;
    S_vector pos = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    if ((pos - frm->GetWorldPos()).Square() > 22500.0f)
        return;
    S_hydrant h;
    h.pos = frm->GetWorldPos();
    h.blocked = false;
    h.particle = act_mission->GetEffects()->CreateParticle(PARTICLE_HYDRANT, h.pos, S_vector(0.0f, 0.0f, 1.0f), 0,
        NULL);
    h.frame = driver->CreateFrame(FRAME_SOUND);
    if (h.frame)
    {
        sound_table.ConstructSound((I3D_sound *)h.frame, SOUND_HYDRANT);
        ((I3D_sound *)h.frame)->SetLoop(true);
        h.frame->SetPos(h.pos);
        h.frame->Update();
        h.frame->SetOn(true);
    }
    hydrants.push_back(h);
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

// The active camera's rotation kept, which Tick sets again in the seventh frame since Init (PS2 name; CAMERA_LOCK's)
// FUNCTION: GAME 0x5ba010
void C_game::SetCameraRotRepair()
{
    I3D_camera *cam = act_mission->GetScene()->GetActiveCamera();
    if (cam)
    {
        camera_rot_repair = true;
        camera_rot = cam->GetRot();
    }
}

// The cartridges' frames unlinked (PS2 name)
// FUNCTION: GAME 0x5ba080
void C_game::ClearCartridges()
{
    int i;
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
}

// A car and the square of its distance from the camera (PS2 name, TickCarSoundManager's list; the members' names are
// ours)
struct S_car_snd
{
    C_car *car;
    float dist;
};

// The cars' order for qsort, the nearest first (PS2 name; not static, as PS2's, so in its place before its user)
// FUNCTION: GAME 0x5ba0e0
int __cdecl compare_car_snd(const void *a, const void *b)
{
    if (((S_car_snd *)a)->dist == ((S_car_snd *)b)->dist)
        return 0;
    if (((S_car_snd *)a)->dist < ((S_car_snd *)b)->dist)
        return -1;
    return 1;
}

// The cars' sounds (PS2 name), every 200 ms or 800 ms while car_sounds is positive: the cars in the game sorted by
// their distance from the camera, the nearest car_sounds of them get their sounds on in the short period, the others'
// go off in the long one
// FUNCTION: GAME 0x5ba110
void C_game::TickCarSoundManager()
{
    if (car_sounds < 1)
        return;
    std::vector<S_car_snd> cars;
    S_vector pos = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    for (int i = C_actor::global_actors.size(); i--;)
    {
        C_actor *act = C_actor::global_actors[i];
        if (act->type == ACTOR_CAR && act->act_state != ACTSTATE_NONE)
        {
            S_car_snd s;
            s.car = (C_car *)act;
            s.dist = (s.car->frame->GetWorldPos() - pos).Square();
            cars.push_back(s);
        }
    }
    qsort(cars.begin(), cars.size(), sizeof(S_car_snd), compare_car_snd);
    if (car_sounds_short)
    {
        int n = car_sounds;
        if ((uint)n > cars.size())
            n = cars.size();
        for (int j = 0; j < n; j++)
        {
            if (!cars[j].car->SoundsEnabled())
                cars[j].car->SoundsChange(SOUNDS_ON);
        }
    }
    else
    {
        int n = cars.size();
        for (int j = car_sounds; j < n; j++)
        {
            if (cars[j].car->SoundsEnabled())
                cars[j].car->SoundsChange(0);
        }
    }
}

// The items' blurs ended (PS2 name): each one's copies released, the list emptied
// FUNCTION: GAME 0x5ba310
void C_game::ItemBlurDone()
{
    for (int i = item_blurs.size(); i--;)
        ItemBlurDelItem(i, false);
    item_blurs.clear();
    item_blur = false;
}

// The items' blurs (PS2 name; nothing of the PC's fills the list): an item whose actor is rendered, on, linked and
// within 30 m of the camera gets copies of its frame trailing behind it, along the way its actor moved in the camera's
// space since the last tick, while that speed averages 1 or more per second: num_frames copies spread over the speed
// times its average a hundredth, at most spread. Otherwise the copies are unlinked.
// FUNCTION: GAME 0x5ba390
void C_game::ItemBlurTick(uint time)
{
    if (time < 1)
        return;
    float dt = time * .001f;
    S_vector cam_pos = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    S_matrix m = act_mission->GetScene()->GetActiveCamera()->GetMatrix();
    S_matrix inv = m;
    inv.Inverse(S_matrix(m));
    m(3).Zero();
    for (int i = item_blurs.size(); i--;)
    {
        bool ok = item_blurs[i].actor->rendered && item_blurs[i].actor->frame->IsOn() &&
            item_blurs[i].actor->frame->GetParent() &&
            (item_blurs[i].actor->frame->GetWorldPos() - cam_pos).Square() < 900.0f;
        if (!ok)
        {
            if (item_blurs[i].on)
            {
                for (int j = item_blurs[i].num_frames; j--;)
                {
                    if (item_blurs[i].frames[j])
                        item_blurs[i].frames[j]->LinkTo(NULL, 0);
                }
                item_blurs[i].on = false;
            }
            continue;
        }
        S_vector p = item_blurs[i].actor->frame->GetWorldPos();
        p *= inv;
        S_vector d = p - item_blurs[i].last_pos;
        item_blurs[i].last_pos = p;
        float v = d.Magnitude() / dt;
        item_blurs[i].speed += v;
        item_blurs[i].speed *= .5f;
        d *= m;
        if (item_blurs[i].speed < 1.0f)
        {
            if (item_blurs[i].on)
            {
                for (int j = item_blurs[i].num_frames; j--;)
                {
                    if (item_blurs[i].frames[j])
                        item_blurs[i].frames[j]->LinkTo(NULL, 0);
                }
                item_blurs[i].on = false;
            }
            continue;
        }
        int j;
        for (j = item_blurs[i].num_frames; j--;)
        {
            if (item_blurs[i].frames[j])
                item_blurs[i].frames[j]->LinkTo(NULL, 0);
        }
        item_blurs[i].on = true;
        float s = v * item_blurs[i].speed * .01f;
        if (s > item_blurs[i].spread)
            s = item_blurs[i].spread;
        d.Normalize();
        d *= s;
        d *= 1.0f / item_blurs[i].num_frames;
        S_vector pos = item_blurs[i].frame->GetWorldPos();
        S_quat q;
        q.Make(item_blurs[i].frame->GetMatrix());
        for (j = 0; j < item_blurs[i].num_frames; j++)
        {
            pos -= d;
            item_blurs[i].frames[j]->SetPos(pos);
            item_blurs[i].frames[j]->SetRot(q);
            item_blurs[i].frames[j]->Update();
            act_mission->GetScene()->SetFrameSector(item_blurs[i].frames[j]);
        }
    }
}

// An item's blur taken away (PS2 name; the parameters' names are ours): its copies released and their array deleted,
// and with remove the last item put in its place
// FUNCTION: GAME 0x5baa40
void C_game::ItemBlurDelItem(int index, bool remove)
{
    if (index < 0 || index >= item_blurs.size())
        return;
    for (int i = item_blurs[index].num_frames; i--;)
    {
        if (item_blurs[index].frames[i])
            item_blurs[index].frames[i]->Release();
    }
    delete[] item_blurs[index].frames;
    if (remove)
    {
        item_blurs[index] = item_blurs.back();
        item_blurs.pop_back();
    }
}

// Every global actor of ACTOR_PLAYER or ACTOR_ENTITY set up for a recording (PC only, the name is ours: RECLOAD's and
// RECLOADFULL's): his weapon effects reset and his weapon put back into its place in the hands
// FUNCTION: GAME 0x5bab20
void C_game::RecPrepareHumans()
{
    for (int i = C_actor::global_actors.size(); i--;)
    {
        switch (C_actor::global_actors[i]->type)
        {
        case ACTOR_PLAYER:
        case ACTOR_ENTITY:
            ((C_human *)C_actor::global_actors[i])->ResetWeaponEffects();
            ((C_human *)C_actor::global_actors[i])->weapon_displaced = true;
            ((C_human *)C_actor::global_actors[i])->RepairWeaponPosition();
            break;
        }
    }
}

// A box of collision at pos, turned by angle degrees around axis and scaled by scale, as a dummy frame of the unit box
// and a collision object of its matrix and inverse, added to the collisions as the n-th (the name is ours; static,
// defined before its caller, VC6 puts it after it)
// FUNCTION: GAME 0x5bb570
static void AddInterCityBox(tDynamicCollObject **colls, S_matrix *matrices, S_matrix *inv_matrices, int *n,
    I3D_frame **frames, S_vector pos, S_vector axis, float angle, S_vector scale)
{
    I3D_frame *frm = driver->CreateFrame(FRAME_DUMMY);
    ((I3D_dummy *)frm)->SetBBox(I3D_bbox(S_vector(-.5f, -.5f, -.5f), S_vector(.5f, .5f, .5f)));
    frm->SetPos(pos);
    S_quat q;
    q.Make(axis, angle * PI / 180.0f);
    frm->SetRot(q);
    frm->SetScale(scale);
    frm->Update();
    frames[*n] = frm;
    I3D_bbox bb = ((I3D_dummy *)frm)->bbox;
    float radius = (bb.max - bb.min).Magnitude() * .5f;
    S_vector ws = frm->GetWorldScale();
    radius = radius * (ws.Magnitude() * .5f);
    colls[*n] = new tDynamicCollObject;
    memset(colls[*n], 0, sizeof(tDynamicCollObject));
    colls[*n]->type = DYNCOLL_UNKNOWN_40;
    colls[*n]->flags = DYNCOLLF_UNKNOWN_4000;
    ((I3D_bsphere &)colls[*n]->bsphere).pos = frm->GetWorldPos();
    ((I3D_bsphere &)colls[*n]->bsphere).radius = radius;
    bb.min *= frm->GetWorldScale();
    bb.max *= frm->GetWorldScale();
    (I3D_bbox &)colls[*n]->bbox = bb;
    S_matrix m = frm->GetWorldRot().RotationMatrix();
    m(3) = frm->GetWorldPos();
    matrices[*n] = m;
    colls[*n]->matrix = &matrices[*n];
    inv_matrices[*n] = m;
    inv_matrices[*n].Inverse(S_matrix(inv_matrices[*n]));
    colls[*n]->inv_matrix = &inv_matrices[*n];
    colls[*n]->frame = NULL;
    colls[*n]->owner = NULL;
    gifCollision.DynInsert(colls[*n]);
    (*n)++;
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




