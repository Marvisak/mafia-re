// The pedestrians (PS2 C_person_traffic.cpp): the don's tail, the generators and their pedestrians (C_don_ocas,
// C_traffic_generator, C_traffic_element), the PC's file at 0x44c520-0x459e8c (docs/game-files.md), in PS2's order.
// The vectors' initializers start the file (0x44c520, 0x44c5a0), then last_panic's empty one (0x44c620). Open: the
// vectors' destructors (0x44c560, 0x44c5e0) test and set a guard byte they share (0x63d760), a bit each, as
// C_actor.cpp's do.

#include "ai/C_person_traffic.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "actors/C_animations.h"
#include "actors/C_human.h"
#include "actors/C_public_physics.h"
#include "ai/C_shvestky.h"
#include "ai/jp_ai_entity.h"
#include "ai/jp_ai_main.h"
#include "ai/pathweb.h"
#include "ai/road_web.h"
#include "C_chunk.h"
#include "collision/g_colls.h"
#include "g_material.h"
#include "I3D_cache.h"
#include "mission/C_effects.h"
#include "mission/game.h"
#include "mission/Mission.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "vehicles/C_car.h"
#include "vehicles/C_railway.h"
#include "vehicles/C_trf_c.h"
#include "stub.h"

// GLOBAL: GAME 0x63d764
std::vector<C_traffic_element *> C_traffic_generator::array;
// GLOBAL: GAME 0x63d750
std::vector<C_animations_machine_de_luxe *> C_traffic_generator::anims;
// GLOBAL: GAME 0x63d740
S_vector C_traffic_generator::last_panic;

// GLOBAL: GAME 0x632638
float C_traffic_generator::ratio_all = 1.0f;
// GLOBAL: GAME 0x63263c
float C_traffic_generator::ratio_police = 1.0f;
// GLOBAL: GAME 0x63d728
bool C_traffic_generator::global_valid_scream;
// GLOBAL: GAME 0x63d729
bool C_traffic_generator::player_in_taxi;
// GLOBAL: GAME 0x63d72a
bool C_traffic_generator::enable_taxi_driver;
// GLOBAL: GAME 0x63d730
bool C_traffic_generator::all_visible;
// GLOBAL: GAME 0x63d738
int C_traffic_generator::t_horn;
// GLOBAL: GAME 0x63d734
int C_traffic_generator::try_do_rici;
// GLOBAL: GAME 0x63d72c
int C_traffic_generator::taxi_timer;
// GLOBAL: GAME 0x63d73c
int C_traffic_generator::t_scream;
// GLOBAL: GAME 0x63d74c
bool C_traffic_generator::first_frame;

// FUNCTION: GAME 0x44c640
C_don_ocas::C_don_ocas()
{
    type = ACTOR_DON_OCAS;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x44c680
C_don_ocas::~C_don_ocas()
{
}

// FUNCTION: GAME 0x44c690
void C_don_ocas::GameInit()
{
    C_actor::GameInit();
    unknown_65 = true;
}

// Whether the actor at pos moving by dir runs into a pedestrian, a sphere of radius + 1.2 around his chest moving
// by his direction by his animation's speed (the parameters' names are ours): the part of the move where they first
// meet into dist, for the last pedestrian that does
// FUNCTION: GAME 0x44c6a0
bool C_don_ocas::IsPossibleColl(C_actor *act, S_vector pos, S_vector dir, float radius, float &dist, C_actor **coll)
{
    bool ret = false;
    for (int i = C_traffic_generator::array.size(); i--;)
    {
        C_traffic_element *e = C_traffic_generator::array[i];
        if (!e->active || !e->frame || e->dead)
            continue;
        S_vector d = e->dir;
        d.Normalize();
        switch (e->anim)
        {
        case ANIM_RUN1:
            d *= 20.0f;
            break;
        case ANIM_WALK1:
            d *= 7.0f;
            break;
        default:
            if (e->machine->track == -1)
                d *= .1f;
            else
                d = e->anim_end;
            break;
        }
        if (e->hit_wait)
            continue;
        S_vector center(e->pos.x, e->pos.y + .7f, e->pos.z);
        S_vector a = center - pos;
        S_vector b = d - dir;
        float r = radius + 1.2f;
        float c = a.Square();
        float r2 = r * r;
        float t;
        if (c <= r2)
            t = 0.0f;
        else
        {
            float aa = b.Square();
            if (aa <= 0.0f)
                t = -1.0f;
            else
            {
                float bb = b.Dot(a) * 2.0f;
                float disc = bb * bb - (c - r2) * aa * 4.0f;
                if (disc < 0.0f)
                    t = disc;
                else
                {
                    float s = (float)sqrt(disc);
                    float inv = 1.0f / (aa * 2.0f);
                    float t1 = (s - bb) * inv;
                    t = (-bb - s) * inv;
                    if (t1 < t)
                        t = t1;
                }
            }
        }
        if (t > dist && t >= 0.0f)
        {
            dist = t;
            ret = true;
        }
    }
    return ret;
}

// A pedestrian run into (by a car: a hit by its speed, the police told of its owner's crime), or taken out of the game
// beyond 20 meters
// FUNCTION: GAME 0x44ca50
bool C_don_ocas::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    C_traffic_element *e = C_traffic_generator::GetElemFromFrame(frm);
    if (e)
    {
        e->EndMavaniNaTaxi();
        if (act)
        {
            if (act->GetType() != ACTOR_CAR)
            {
                if (e->model_type[2] == 1 || e->model_type[2] == 3)
                    act_mission->GetGame()->shvestky.DalJsemPochuzkariDoDrzky((C_human *)act);
            }
            else if (e->model_type[2] == 1 || e->model_type[2] == 3)
                act_mission->GetGame()->shvestky.DalJsemPochuzkariDoDrzky(((C_car *)act)->GetOwner(0));
        }
        bool near_cam = e->visible;
        if (!near_cam)
        {
            I3D_frame *frame = e->frame;
            near_cam = (act_mission->GetScene()->GetActiveCamera()->GetWorldPos() - frame->GetWorldPos()).Square() <
                400.0f;
        }
        if (near_cam)
        {
            e->Hit(HIT_IN_CAR, v2, 5, act, v2.Magnitude() * 4.2f);
            if (v2.Square() > 25.0f)
            {
                I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
                if (sound)
                {
                    sound->SetPos(S_vector(e->pos.x, e->pos.y + 1.0f, e->pos.z));
                    sound_table.ConstructSound(sound, SOUND_RUN_OVER);
                    sound->SetOn(true);
                    act_mission->GetGame()->Play3DSound(sound);
                }
            }
            if (act && act->GetType() == ACTOR_CAR && e->energy <= 0.0f &&
                act_mission->GetGame()->shvestky.active)
                act_mission->GetGame()->shvestky.NextNaSrackySoudruh(((C_car *)act)->GetOwner(0));
        }
        else
            e->Deactivate();
        return true;
    }
    return false;
}

// The pedestrian of the frame hit: a stroke from behind with a bat or a knife kills it, a shot goes to the bone it
// meets (the parameters' names are ours)
// FUNCTION: GAME 0x44ccf0
bool C_don_ocas::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    C_traffic_element *e = C_traffic_generator::GetElemFromFrame(frm);
    if (e)
    {
        e->EndMavaniNaTaxi();
        if (type == HIT_STROKE)
        {
            if (act && (act->GetType() == ACTOR_PLAYER || act->GetType() == ACTOR_ENTITY) &&
                ((C_human *)act)->strike_index == -1 &&
                (((C_human *)act)->inventory.right_hand.id == GAMEITEM_BASEBALL_BAT ||
                    ((C_human *)act)->inventory.right_hand.id == GAMEITEM_KNIFE))
                    {
                S_vector dir = e->frame->GetWorldDir();
                float angle = dir.AngleTo(act->GetWorldDir());
                S_vector apos = act->GetWorldPos();
                S_vector diff = e->frame->GetWorldPos() - apos;
                dir = e->frame->GetWorldDir();
                float angle_to = dir.AngleTo(diff);
                if (angle < PI * .4f && angle_to < PI * .3f)
                    f = e->energy + 1.0f;
            }
            e->Hit(HIT_STROKE, v1, 5, act, f);
            if (act_mission->options.particles >= OPTION_MEDIUM)
                act_mission->effects->CreateParticle(SHOTPARTICLE_BODY, v2, v3, 0, NULL);
            if (e->model_type[2] == 1 || e->model_type[2] == 3)
                act_mission->GetGame()->shvestky.DalJsemPochuzkariDoDrzky((C_human *)act);
            return true;
        }
        C_ShotSkeleton skeleton;
        skeleton.Init(e->frame);
        uint part = skeleton.TestCollision(v2, v1, FLT_MAX);
        if (part)
        {
            e->Hit(type, v1, part, act, f);
            return true;
        }
    }
    return false;
}

// A pedestrian's bounds: the four points .7 around the frame
// FUNCTION: GAME 0x44d1c0
bool C_don_ocas::GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm)
{
    S_vector pos = frm->GetWorldPos();
    points.push_back(S_vector2(pos.x + .7f, pos.z));
    points.push_back(S_vector2(pos.x - .7f, pos.z));
    points.push_back(S_vector2(pos.x, pos.z + .7f));
    points.push_back(S_vector2(pos.x, pos.z - .7f));
    return true;
}

// C_actor's, then the type and the defaults of a generator that has no models yet
// FUNCTION: GAME 0x44d410
C_traffic_generator::C_traffic_generator()
    : elements(NULL), models(NULL), num_models(0), model_counts(NULL)
{
    type = ACTOR_TRAFFIC_GENERATOR;
    activate_max = 90.0f;
    max_dist = 100.0f;
    activate_min = 50.0f;
    unknown_94 = 50.0f;
    range = 100.0f;
    no_range = true;
    max_active = 10;
}

// The models' names from the last, the array, and the counts
// FUNCTION: GAME 0x44d4a0
C_traffic_generator::~C_traffic_generator()
{
    for (int i = num_models; i--;)
        delete[] models[i];
    delete[] models;
    models = NULL;
    delete[] model_counts;
    model_counts = NULL;
}

// The base's, its result converted again: the linker folded it with C_door's
// FUNCTION: GAME 0x4393a0 FOLDED
bool C_traffic_generator::Init(I3D_frame *frm)
{
    return C_actor::Init(frm) ? true : false;
}

// The base's, then the settings and a copy of the models' names and counts
// FUNCTION: GAME 0x44d510
void C_traffic_generator::DuplicateFrom(C_actor *act)
{
    C_actor::DuplicateFrom(act);
    C_traffic_generator *gen = (C_traffic_generator *)act;
    no_range = gen->no_range;
    range = gen->range;
    max_dist = gen->max_dist;
    activate_min = gen->activate_min;
    activate_max = gen->activate_max;
    unknown_94 = gen->unknown_94;
    for (int i = num_models; i--;)
        delete[] models[i];
    delete[] models;
    models = NULL;
    num_models = gen->num_models;
    if (num_models)
        models = new char *[num_models];
    for (int j = 0; j < num_models; j++)
    {
        models[j] = new char[0x11];
        char *s = gen->models[j];
        char *d = models[j];
        char c;
        do
        {
            c = *s++;
            *d++ = c;
        } while (c);
    }
    delete[] model_counts;
    model_counts = NULL;
    if (num_models)
        model_counts = new int[num_models];
    for (int k = 0; k < num_models; k++)
        model_counts[k] = gen->model_counts[k];
}

// The version, the settings, the models' count, each model's name (0x11 bytes) and a dword for each
// FUNCTION: GAME 0x44d670
bool C_traffic_generator::SaveData(C_chunk *chunk)
{
    int version = 1;
    chunk->Write(&version, sizeof(version));
    chunk->Write(&no_range, sizeof(no_range));
    chunk->Write(&range, sizeof(range));
    chunk->Write(&max_dist, sizeof(max_dist));
    chunk->Write(&activate_min, sizeof(activate_min));
    chunk->Write(&activate_max, sizeof(activate_max));
    chunk->Write(&unknown_94, sizeof(unknown_94));
    chunk->Write(&max_active, sizeof(max_active));
    chunk->Write(&num_models, sizeof(num_models));
    for (int i = 0; i < num_models; i++)
        chunk->Write(models[i], 0x11);
    if (num_models)
        chunk->Write(model_counts, num_models * sizeof(int));
    return true;
}

// Version 0 had no limit (the sum of the models' dwords is it), 1 has it
// FUNCTION: GAME 0x44d820
bool C_traffic_generator::LoadData(C_chunk *chunk)
{
    int version;
    chunk->Read(&version, sizeof(version));
    switch (version)
    {
    case 0:
    {
        chunk->Read(&no_range, sizeof(no_range));
        chunk->Read(&range, sizeof(range));
        chunk->Read(&max_dist, sizeof(max_dist));
        chunk->Read(&activate_min, sizeof(activate_min));
        chunk->Read(&activate_max, sizeof(activate_max));
        chunk->Read(&unknown_94, sizeof(unknown_94));
        max_active = 0;
        for (int i = num_models; i--;)
            delete[] models[i];
        delete[] models;
        models = NULL;
        chunk->Read(&num_models, sizeof(num_models));
        if (num_models)
            models = new char *[num_models];
        for (int j = 0; j < num_models; j++)
        {
            models[j] = new char[0x11];
            char *name = models[j];
            chunk->Read(name, 0x11);
        }
        delete[] model_counts;
        model_counts = NULL;
        if (num_models)
        {
            model_counts = new int[num_models];
            chunk->Read(model_counts, num_models * sizeof(int));
        }
        for (int k = 0; k < num_models; k++)
            max_active += model_counts[k];
        break;
    }
    case 1:
        chunk->Read(&no_range, sizeof(no_range));
        chunk->Read(&range, sizeof(range));
        chunk->Read(&max_dist, sizeof(max_dist));
        chunk->Read(&activate_min, sizeof(activate_min));
        chunk->Read(&activate_max, sizeof(activate_max));
        chunk->Read(&unknown_94, sizeof(unknown_94));
        chunk->Read(&max_active, sizeof(max_active));
        for (int m = num_models; m--;)
            delete[] models[m];
        delete[] models;
        models = NULL;
        chunk->Read(&num_models, sizeof(num_models));
        if (num_models)
            models = new char *[num_models];
        for (int n = 0; n < num_models; n++)
        {
            models[n] = new char[0x11];
            chunk->Read(models[n], 0x11);
        }
        delete[] model_counts;
        model_counts = NULL;
        if (num_models)
        {
            model_counts = new int[num_models];
            chunk->Read(model_counts, num_models * sizeof(int));
        }
        return true;
    }
    return true;
}

// The base's, the header and a record for each pedestrian out
// FUNCTION: GAME 0x44de00
int C_traffic_generator::SaveGameGetSize()
{
    int size = C_actor::SaveGameGetSize();
    size += sizeof(S_traffic_generator_savegame);
    return size + num_active * sizeof(S_traffic_element_savegame);
}

// The header, then a record for each pedestrian out
// FUNCTION: GAME 0x44de20
bool C_traffic_generator::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    S_traffic_generator_savegame *g = (S_traffic_generator_savegame *)*buf;
    g->version = TRAFFIC_SAVEGAME_VERSION;
    g->active = active;
    g->num_active = num_active;
    g->spawn_timer = spawn_timer;
    *(char **)buf += sizeof(*g);
    for (int i = 0; i < num_elements; i++)
    {
        if (!elements[i].active)
            continue;
        S_traffic_element_savegame *s = (S_traffic_element_savegame *)*buf;
        s->index = i;
        s->dead = elements[i].dead;
        s->pos = elements[i].pos;
        s->dir = elements[i].dir;
        s->velocity = elements[i].velocity;
        s->anim_pos = elements[i].anim_pos;
        s->visible = elements[i].visible;
        s->dist = elements[i].dist;
        s->keep_alive = elements[i].keep_alive;
        s->node = elements[i].node;
        s->target = elements[i].target;
        s->node_width = elements[i].node_width;
        s->prev_node = elements[i].prev_node;
        for (int j = 0; j < 4; j++)
            s->way[j] = elements[i].way[j];
        s->num_way = elements[i].num_way;
        s->alpha = elements[i].alpha;
        s->alpha_dir = elements[i].alpha_dir;
        s->energy = elements[i].energy;
        s->anim = elements[i].anim;
        s->anim_after = elements[i].anim_after;
        s->anim_once = elements[i].anim_once;
        s->anim_after_once = elements[i].anim_after_once;
        s->anim_time = elements[i].anim_time;
        s->ground_timer = elements[i].ground_timer;
        s->wait_time = elements[i].wait_time;
        s->line_walk = elements[i].line_walk;
        s->unknown_198 = elements[i].unknown_198;
        s->running = elements[i].running;
        s->turning = elements[i].turning;
        s->fleeing = elements[i].fleeing;
        s->panic_time = elements[i].panic_time;
        s->mega_panic = elements[i].mega_panic;
        s->made_human = elements[i].made_human;
        if (elements[i].car)
            s->car = elements[i].car->number;
        else
            s->car = -1;
        *(char **)buf += sizeof(*s);
    }
    return true;
}

// The pedestrians out are taken back, then the header, and each record puts its pedestrian back where it was: its
// walk goes on from there (anim_next starts the animation again, SetAlpha 1 makes it visible)
// FUNCTION: GAME 0x44e120
bool C_traffic_generator::SaveGameLoad(void **buf)
{
    for (int i = 0; i < num_elements; i++)
    {
        if (elements[i].active)
            elements[i].Deactivate();
    }
    if (!C_actor::SaveGameLoad(buf))
        return false;
    S_traffic_generator_savegame *g = (S_traffic_generator_savegame *)*buf;
    if (g->version != TRAFFIC_SAVEGAME_VERSION)
        return false;
    active = g->active;
    num_active = 0;
    spawn_timer = g->spawn_timer;
    *(char **)buf += sizeof(*g);
    for (int j = 0; j < g->num_active; j++)
    {
        S_traffic_element_savegame *s = (S_traffic_element_savegame *)*buf;
        int n = s->index;
        if (elements[n].frame)
        {
            elements[n].frame->SetPos(s->pos);
            elements[n].frame->SetDir(s->dir, 0.0f);
            elements[n].frame->Update();
            elements[n].web_flags = 1;
            elements[n].ActivateFree(false, false);
            elements[n].dead = s->dead;
            elements[n].pos = s->pos;
            elements[n].dir = s->dir;
            elements[n].velocity = s->velocity;
            elements[n].anim_pos = s->anim_pos;
            elements[n].visible = s->visible;
            elements[n].dist = s->dist;
            elements[n].keep_alive = s->keep_alive;
            elements[n].node = s->node;
            elements[n].target = s->target;
            elements[n].node_width = s->node_width;
            elements[n].prev_node = s->prev_node;
            S_vector *src = s->way;
            for (int k = 0; k < 4; k++, src++)
                elements[n].way[k] = *src;
            elements[n].num_way = s->num_way;
            elements[n].alpha = 1.0f;
            elements[n].alpha_dir = s->alpha_dir;
            elements[n].energy = s->energy;
            elements[n].anim = 0;
            elements[n].anim_next = s->anim;
            elements[n].anim_after = s->anim_after;
            elements[n].anim_once = s->anim_once;
            elements[n].anim_after_once = s->anim_after_once;
            elements[n].anim_time = s->anim_time;
            elements[n].ground_timer = s->ground_timer;
            elements[n].wait_time = s->wait_time;
            elements[n].line_walk = s->line_walk;
            elements[n].unknown_198 = s->unknown_198;
            elements[n].running = s->running;
            elements[n].turning = s->turning;
            elements[n].fleeing = s->fleeing;
            elements[n].panic_time = s->panic_time;
            elements[n].mega_panic = s->mega_panic;
            elements[n].made_human = s->made_human;
            C_game *game = act_mission->game;
            C_actor *car = game->GetActorFromGUID(s->car);
            if (car && car->type != ACTOR_CAR)
                car = NULL;
            elements[n].car = (C_car *)car;
            elements[n].SetAlpha(1.0f);
        }
        *(char **)buf += sizeof(*s);
    }
    return true;
}

// The pedestrians made: the models' counts as set, scaled by ratio_all, each in a random place of the array, the
// policemen's share scaled by ratio_police
// FUNCTION: GAME 0x44e500
void C_traffic_generator::GameInit()
{
    taxi_timer = 0;
    active = false;
    C_actor::GameInit();
    max_active_orig = max_active;
    max_active = (int)(max_active * ratio_all + 0.5f);
    if (max_active < 1 && ratio_all > 0.05f)
        max_active = 1;
    num_active = 0;
    num_elements = 0;
    t_horn = 0;
    int n;
    for (n = 0; n < num_models; n++)
        num_elements += model_counts[n];
    int police = 0;
    if (num_elements)
    {
        int *order = new int[num_elements];
        int i;
        for (i = 0; i < num_elements; i++)
            order[i] = i;
        for (i = num_elements * 10; i; i--)
        {
            int a = rnd(num_elements);
            int b = rnd(num_elements);
            int t = order[a];
            order[a] = order[b];
            order[b] = t;
        }
        elements = new C_traffic_element[num_elements];
        num_elements = 0;
        for (int m = 0; m < num_models; m++)
        {
            for (int k = 0; k < model_counts[m]; k++)
            {
                elements[order[num_elements]].CreateModel(models[m], this);
                elements[order[num_elements]].max_dist = max_dist;
                elements[order[num_elements]].made_human = false;
                if (elements[order[num_elements]].model_type[2] == 1 || elements[order[num_elements]].model_type[2] == 3)
                    police++;
                num_elements++;
            }
        }
        delete[] order;
    }
    police = (int)(police * ratio_police + 0.5f);
    police_last = -1;
    for (int j = 0; j < num_elements && police >= 1; j++)
    {
        if (elements[j].model_type[2] == 1 || elements[j].model_type[2] == 3)
        {
            police_last = j;
            police--;
        }
    }
    if (max_active > num_elements)
        max_active = num_elements;
    next_try = max_active;
    if (next_try >= num_elements)
        next_try = num_elements - 1;
    t_scream = 0;
    first_frame = true;
    try_do_rici = 2123;
    all_visible = true;
}

// The pedestrians' models given back and the array deleted
// FUNCTION: GAME 0x44e7f0
void C_traffic_generator::GameDone()
{
    for (int i = 0; i < num_elements; i++)
        elements[i].DestroyModel();
    delete[] elements;
    elements = NULL;
    num_elements = 0;
    C_actor::GameDone();
    max_active = max_active_orig;
}

// The pedestrian of the frame talked to
// FUNCTION: GAME 0x44e870
void C_traffic_generator::PlayerPromluvilNaTehoChuja(I3D_frame *frm)
{
    C_traffic_element *e = GetElemFromFrame(frm);
    if (e)
        e->PlayerNaMneMluvi();
}

// The player has a weapon out: the active pedestrians within 25 of him that have no neck of another's yet take his
// "neck" frame (look_at), which they look at
// FUNCTION: GAME 0x44e890
void C_traffic_generator::PlayerHoMaVytaseneho()
{
    C_human *player = act_mission->game->player;
    if (!player)
        return;
    S_vector pos = player->GetWorldPos();
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && array[i]->frame && !array[i]->look_at)
        {
            S_vector d = array[i]->pos - pos;
            if (d.Square() < 625.0f)
                array[i]->look_at = player->frame->FindChildFrame("neck", ENUMF_ALL);
        }
    }
}

// From the last one, as PS2's, which also gives the shown pedestrians their fake shadows and takes them from the hidden
// ones
// FUNCTION: GAME 0x44e960
void C_traffic_generator::SetVisible(bool visible)
{
    if (all_visible == visible)
        return;
    all_visible = visible;
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && array[i]->frame)
            array[i]->frame->SetOn(visible);
    }
}

// The part of the move from pos by dir after which a sphere of radius 0.6 around center is first met, where the
// sphere moves by move: 0 where it is met already, negative where it isn't
static inline float ElemSphere(S_vector center, const S_vector &pos, const S_vector &move, const S_vector &dir)
{
    S_vector d = center - pos;
    S_vector v = move - dir;
    float rr = 0.36f;
    float t;
    if (d.Dot(d) <= rr)
        t = 0.0f;
    else
    {
        float a = v.Dot(v);
        if (a <= 0.0f)
            t = -1.0f;
        else
        {
            float b = v.Dot(d) * 2.0f;
            float disc = b * b - (d.Dot(d) - rr) * a * 4.0f;
            if (disc < 0.0f)
                t = disc;
            else
            {
                float s = (float)sqrt(disc);
                float inv = 1.0f / (a * 2.0f);
                float t1 = (s - b) * inv;
                t = (-b - s) * inv;
                if (t1 < t)
                    t = t1;
            }
        }
    }
    return t;
}

// The pedestrian e would walk into first: its centre (a metre over the feet) against the centres of the other active
// ones within 10, a sphere of radius 0.6 met by the move of e relative to the other's (as C_human::IsPossibleColl's
// test) within the first fraction of it; NULL for none
// FUNCTION: GAME 0x44e9c0
C_traffic_element *C_traffic_generator::IsPossibleCollElem(C_traffic_element *e)
{
    S_vector pos(e->pos.x, e->pos.y + 1.0f, e->pos.z);
    float best = 1.0f;
    C_traffic_element *ret = NULL;
    for (int i = array.size(); i--;)
    {
        C_traffic_element *o = array[i];
        if (o != e && o->active && o->frame && !o->dead)
        {
            S_vector oc(o->pos.x, o->pos.y + 1.0f, o->pos.z);
            if ((pos - oc).Square() < 100.0f)
            {
                float t = ElemSphere(oc, pos, o->velocity, e->velocity);
                if (t < best && t >= 0.0f)
                {
                    best = t;
                    ret = array[i];
                }
            }
        }
    }
    return ret;
}

// The pedestrian into the array, with the animations' machine it uses (made when there is none for its place yet);
// NULL where it is in already
// FUNCTION: GAME 0x44ebe0
C_animations_machine_de_luxe *C_traffic_generator::AddElement(C_traffic_element *e)
{
    for (int i = array.size(); i--;)
    {
        if (array[i] == e)
            return NULL;
    }
    array.push_back(e);
    if (anims.size() < array.size())
    {
        anims.push_back(new C_animations_machine_de_luxe);
        return anims.back();
    }
    return anims[array.size() - 1];
}

// The pedestrian out of the array: its machine goes to the end of the animations, the last pedestrian takes its place
// FUNCTION: GAME 0x44ed00
void C_traffic_generator::DelElement(C_traffic_element *e)
{
    for (int i = array.size(); i--;)
    {
        if (array[i] == e)
        {
            C_animations_machine_de_luxe *machine = anims[i];
            anims[i] = anims[array.size() - 1];
            anims[array.size() - 1] = machine;
            array[i] = array.back();
            array.pop_back();
            return;
        }
    }
}

// Whether the pedestrian at index i may be activated: the policemen's models only up to the last one the generator
// chose (police_last) (the name is ours)
static inline bool CanActivate(C_traffic_element *e, int i, int last_police)
{
    if (e->model_type[2] == 1 || e->model_type[2] == 3)
        return i <= last_police;
    return true;
}

// While the pedestrians are shown, and the generator is the active one with some to make: all that may be made at the
// first call, then one from the one after the last made every 112 ms. The generator goes out of use (state 1) once it
// has none out
// FUNCTION: GAME 0x44edb0
void C_traffic_generator::AI(uint time)
{
    if (!all_visible)
        return;
    if (active && num_active < max_active)
    {
        if (first_frame)
        {
            for (int i = 0; i < num_elements; i++)
            {
                C_traffic_element *e = &elements[i];
                if (e->active || !e->frame || !CanActivate(e, i, police_last) || e->made_human)
                    continue;
                e->Activate(true, activate_min, activate_max);
                if (num_active >= max_active)
                    break;
            }
            first_frame = false;
        }
        else
        {
            spawn_timer += time;
            if (spawn_timer >= 0x71)
            {
                int i = next_try;
                spawn_timer = 0;
                do
                {
                    if (!elements[i].active && elements[i].frame && !elements[i].made_human &&
                        CanActivate(&elements[i], i, police_last) &&
                        elements[i].Activate(false, activate_min, activate_max))
                        break;
                    i++;
                    if (i >= num_elements)
                        i = 0;
                } while (i != next_try);
                next_try++;
                if (next_try >= num_elements)
                    next_try = 0;
            }
        }
    }
    if (num_active == 0 && !active)
        SetActState(1);
    if (t_scream > 0)
        t_scream -= time;
}

// Nothing while the pedestrians are hidden. The active ones from the last, each with the camera's distance in the
// horizontal plane first (PS2's the same, the height zeroed), then the timers.
// FUNCTION: GAME 0x44ef70
void C_traffic_generator::TickElements(uint time)
{
    if (!all_visible)
        return;
    if (t_horn > 0)
    {
        if (time > t_horn)
            t_horn = 0;
        else
            t_horn -= time;
    }
    S_vector cam = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    for (int i = array.size(); i--;)
    {
        if (array[i]->active)
        {
            S_vector d = cam - array[i]->pos;
            d.y = 0.0f;
            array[i]->dist = d.Square();
            array[i]->Tick(time);
        }
    }
    try_do_rici -= time;
    if (try_do_rici < 0)
    {
        try_do_rici = 2123;
        TryGoToAssHole();
    }
    if (enable_taxi_driver && player_in_taxi)
    {
        if (taxi_timer < 0)
            GeneratePassengerForTaxi();
        else
            taxi_timer -= time;
    }
}

// FUNCTION: GAME 0x44f090
bool C_traffic_generator::IsCarHasOcas(C_car *car)
{
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && !array[i]->dead && !array[i]->fleeing && array[i]->car == car)
            return true;
    }
    return false;
}

// The distance of point from the line through orig along dir, -1e16 for a null dir (PS2's; the name is ours). PS2 has
// it written in TryGoToAssHole, inlined and in another order (dir * t from orig - point). On the PC its vector calls
// stay out of line where those around them are inlined, so they are nested calls, and VC6 folds the null dir's
// -1e16 < 5 into the caller's jump.
static inline float DistToLine(const S_vector &orig, const S_vector &dir, const S_vector &point)
{
    float sq = dir.Dot(dir);
    if (sq < MRG_ZERO)
        return -1e16f;
    return ((orig + dir * (dir.Dot(point - orig) / sq)) - point).Magnitude();
}

// Sends a free pedestrian, from the last one, to the nearest parked car the two masks allow (a car parked longer than 15
// s, nobody in it or stealing it, no other pedestrian walking to it) within 20 m and ahead of it, if the car lies within
// 5 m of the line it walks along: the way to the car's placement and two of its navigation points, each put on the
// ground. One pedestrian at most, as PS2's.
// The car's mask is a local: VC6 calls it first and keeps it in ebx across the name's strrchr. p2 is the car's position
// first, one variable with the navigation point and the ground's hit (PS2 has the last two in one too): the original's
// stack area for the position is the point's, and a variable of its own, in the block around the points, gets one to
// itself. Open: from the placement call on, the code generator's temporaries take the register after the original's
// (ecx for eax) with the instructions the same; the first navigation point's square comes x, z, y where the original's
// comes z, y, x (a sum written out gives that, but frees inline budget the helper's first Dot then takes); and the
// position's third word is copied after the element's frame is loaded.
// FUNCTION: GAME 0x44f0e0
void C_traffic_generator::TryGoToAssHole()
{
    if (!all_visible)
        return;
    std::vector<C_traffic_car::S_ELEMENT *> &parking = C_traffic_car::GetParkingCars();
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && !array[i]->dead && !array[i]->fleeing && array[i]->generator &&
            !array[i]->car)
        {
            float best = FLT_MAX;
            int best_i = -1;
            for (int j = parking.size(); j--;)
            {
                uint mask = mafiaCarDatabase.GetCivilMaskByModel(parking[j]->car->GetModelName());
                const char *name = strrchr(array[i]->frame->GetFileName(), '\\');
                if (mask &
                        mafiaCarDatabase.GetPlayerCivilMaskByModel(name ? name + 1 : array[i]->frame->GetFileName()) &&
                    parking[j]->car->GetTimeParking() > 15000 && !C_human::IsThisCarStealing(parking[j]->car) &&
                    !IsCarHasOcas(parking[j]->car))
                {
                    S_vector d = parking[j]->frame->GetWorldPos() - array[i]->frame->GetWorldPos();
                    float dist = d.Square();
                    if (dist < best && dist < 400.0f && d.AngleTo(array[i]->dir) < PI / 2)
                    {
                        best_i = j;
                        best = dist;
                    }
                }
            }
            if (best_i > -1)
            {
                S_vector p2 = parking[best_i]->frame->GetWorldPos();
                S_vector dir = array[i]->target - array[i]->frame->GetWorldPos();
                if (DistToLine(array[i]->target, dir, p2) < 5.0f &&
                    parking[best_i]->car->GetPlacementFromTCK1(0, array[i]->way[0], array[i]->way_dir, true))
                {
                    array[i]->car = parking[best_i]->car;
                    S_vector p1, p3, p4;
                    parking[best_i]->car->GetNavigationPoints(0.5f, p1, p2, p3, p4);
                    p1 *= parking[best_i]->car->frame->GetMatrix();
                    p2 *= parking[best_i]->car->frame->GetMatrix();
                    p3 *= parking[best_i]->car->frame->GetMatrix();
                    p4 *= parking[best_i]->car->frame->GetMatrix();
                    if ((p1 - array[i]->GetPos()).Square() < (p3 - array[i]->GetPos()).Square())
                    {
                        array[i]->way[2] = p2;
                        array[i]->way[1] = p1;
                    }
                    else
                    {
                        array[i]->way[2] = p4;
                        array[i]->way[1] = p3;
                    }
                    array[i]->num_way = 3;
                    array[i]->detour = 0;
                    for (int k = 0; k < 3; k++)
                    {
                        p1 = array[i]->way[k];
                        p1.y += 1.0f;
                        if (gifCollision.TestLineV(p1, S_vector(0.0f, -3.0f, 0.0f), &p2, NULL, 0, 0))
                        {
                            array[i]->way[k] = p2;
                            array[i]->way[k].y += 0.01f;
                        }
                    }
                    array[i]->SetNewNode();
                    return;
                }
            }
        }
    }
}

// The PC's nine taxis, from the last one (PS2 has eleven, from a table of its own)
// FUNCTION: GAME 0x44f740
void C_traffic_generator::PlayerInTaxi(C_car *car)
{
    player_in_taxi = false;
    if (!car)
        return;
    const char *taxis[] = {
        // STRING: GAME 0x6326a8
        "taxi",
        // STRING: GAME 0x63269c
        "hotrodp4",
        // STRING: GAME 0x632690
        "hotrodp5",
        // STRING: GAME 0x632684
        "tbirdold",
        // STRING: GAME 0x63267c
        "fordhot",
        // STRING: GAME 0x632670
        "phantomtaxi",
        // STRING: GAME 0x632664
        "hotrodp2",
        // STRING: GAME 0x632654
        "fordapicktaxi",
        // STRING: GAME 0x632648
        "blackdragon",
    };
    int i = sizeof(taxis) / sizeof(*taxis);
    while (i--)
    {
        if (!_stricmp(car->model_name, taxis[i]))
        {
            player_in_taxi = true;
            return;
        }
    }
}

// A passenger for the player's taxi: a pedestrian of the sidewalk ahead of it waves (the timer set again, at once if
// two wave already)
// FUNCTION: GAME 0x44f7d0
void C_traffic_generator::GeneratePassengerForTaxi()
{
    taxi_timer = rnd(4) * 524 + 500;
    C_game *game = act_mission->GetGame();
    if (!game->player)
        return;
    C_car *car = game->player->car;
    if (!car || car->type != ACTOR_CAR)
        return;
    if (mafiaIndicators.IsShown(INDF_TIMER))
        return;
    if (game->player->seat)
        return;
    if (car->speed > 20.0f)
        return;
    if (game->taxi_points.size() < 1)
        return;
    if (act_mission->GetGame()->shvestky.active &&
        act_mission->GetGame()->shvestky.PocetNasranych(game->player) > 0)
        return;
    for (int s = 1; s < (int)car->seats.size(); s++)
    {
        if (car->GetOwner(s))
            return;
    }
    int waving_count = 0;
    for (int j = array.size(); j--;)
    {
        if (array[j]->waving)
            waving_count++;
    }
    if (waving_count > 1)
    {
        taxi_timer += 4000;
        return;
    }
    S_vector pos = car->frame->GetWorldPos();
    S_vector dir = car->frame->GetWorldDir();
    int found = -1;
    int i;
    for (i = array.size(); i--;)
    {
        if (array[i]->model_type[2] != 1 && array[i]->model_type[2] != 3 && array[i]->model_type[2] != 4 &&
            array[i]->active && !array[i]->dead && !array[i]->fleeing && array[i]->generator && !array[i]->car &&
            !array[i]->waving && array[i]->anim == ANIM_WALK1 && array[i]->alpha >= 1.0f && array[i]->on_sidewalk)
        {
            S_vector p = array[i]->frame->GetWorldPos();
            S_vector d = p - pos;
            float dist = d.Square();
            if (dist > 400.0f && dist < 8100.0f && dir.AngleTo(d) < PI / 5)
            {
                found = i;
                break;
            }
        }
    }
    if (found != -1)
    {
        array[found]->waving = true;
        taxi_timer += 4000;
        I3D_animation_set *set = NULL;
        const char *name;
        switch (rnd(3))
        {
        case 0:
            // STRING: GAME 0x6326b0
            name = "Mavani01 HPT.i3d";
            break;
        case 1:
            // STRING: GAME 0x6326c4
            name = "Mavani02 HPT.i3d";
            break;
        default:
            // STRING: GAME 0x6326d8
            name = "Mavani03 HPT.i3d";
            break;
        }
        anim_cache.Create(name, &set, NULL, NULL);
        if (set)
        {
            array[found]->machine->SetExtendedAnim(set, 0.9f, 0.0f, false);
            array[found]->machine->unknown_10e = 0xdca9;
            set->Release();
        }
    }
}

// FUNCTION: GAME 0x44fcf0
void C_traffic_generator::EnableTaxiDriver(bool enable)
{
    if (!enable && enable_taxi_driver)
        ZrusVsechnyMavajici();
    enable_taxi_driver = enable;
}

// Ends the waving at a taxi of every pedestrian (PS2's name, Czech: "cancel all the wavers"), from the last one
// FUNCTION: GAME 0x44fd10
void C_traffic_generator::ZrusVsechnyMavajici()
{
    for (int i = array.size(); i--;)
    {
        if (array[i]->waving)
            array[i]->EndMavaniNaTaxi();
    }
}

// PS2's order: the pedestrians forgotten, their animations freed from the last one
// FUNCTION: GAME 0x44fd50
void C_traffic_generator::ClearElements()
{
    array.clear();
    for (int i = anims.size(); i--;)
        delete anims[i];
    anims.clear();
}

// Pedestrians near the human made policemen, for the police to act on (FizlosNamakatDoAkce, Czech: "cop, get into
// action"): each one in range (range, or a cone in front of the pedestrian: in_front) and in sight is replaced by an
// entity of the police, put among actors; then the police are told
// FUNCTION: GAME 0x44fde0
void C_traffic_generator::FizlosNamakatDoAkce(C_human *human, uint u, std::vector<C_actor *> &actors, bool b1,
    float range, bool in_front)
{
    if (all_visible)
    {
        for (int i = array.size(); i--;)
        {
            if (array[i]->active && !array[i]->dead && !array[i]->made_human &&
                (array[i]->model_type[2] == 1 || array[i]->model_type[2] == 3))
            {
                S_vector d = human->frame->GetWorldPos() - array[i]->frame->GetWorldPos();
                float dist = d.Magnitude();
                float reach = range;
                if (in_front)
                {
                    float ang = d.AngleTo(array[i]->frame->GetWorldDir());
                    if (ang < PI / 2)
                        reach = cos(ang) * (range - 10.0f) + 10.0;
                    else
                        reach = 0.0f;
                }
                if (dist < reach)
                {
                    if (dist > 1.0f)
                    {
                        S_vector p = array[i]->frame->GetWorldPos();
                        p.y += 1.2f;
                        if (gifCollision.TestLineH(p, d, NULL, NULL, COLF_ALL, COLF2_UNKNOWN_100000))
                            continue;
                    }
                    I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
                    array[i]->SetAlpha(1.0f);
                    act_mission->GetGame()->DelFrameForShadow(array[i]->frame, SHADOW_TYPE_0);
                    model->Duplicate(array[i]->frame);
                    act_mission->GetGame()->AddFrameForShadow(array[i]->frame, SHADOW_TYPE_0,
                        (E_shadow_type)(array[i]->model_type[0] == 1 ? 3 : 2), NULL, NULL);
                    model->SetPos(array[i]->pos);
                    model->SetDir(array[i]->dir, 0.0f);
                    model->Update();
                    model->LinkTo(array[i]->frame->GetParent(), 0);
                    C_entity *entity = new C_entity;
                    entity->voice_type = rnd(2) + HUMAN_VOICE_POLICE;
                    entity->max_properties[HUMAN_PROPERTY_SPEED] = 1.1f;
                    if (entity->Init(model))
                    {
                        entity->human_type = HUMAN_TYPE_POLICE;
                        entity->max_properties[HUMAN_PROPERTY_15] = 1.0f;
                        entity->max_properties[HUMAN_PROPERTY_AGGRESIVITY] = 1.0f;
                        entity->max_properties[HUMAN_PROPERTY_SIGHT] = 0.6f;
                        entity->max_properties[HUMAN_PROPERTY_SPEED] = 0.9f;
                        act_mission->GetGame()->AddTemporaryActor(entity);
                        // The police's revolver: a S&W M&P with 6 rounds loaded and 30 spare
                        S_GameItem item;
                        item.id = GAMEITEM_SW_MP;
                        item.ammo_loaded = 6;
                        item.ammo_spare = 30;
                        item.using_object = NULL;
                        entity->inventory.Insert(&item, false);
                        entity->ChangeWeaponModel();
                        actors.push_back(entity);
                        if (array[i]->generator)
                        {
                            if (!entity->traffic)
                                entity->traffic = new S_human_traffic;
                            memset(entity->traffic, 0, sizeof(S_human_traffic));
                            entity->traffic->traffic_frame = true;
                            entity->traffic->element = array[i];
                        }
                        array[i]->made_human = true;
                        array[i]->Deactivate();
                        act_mission->GetGame()->shvestky.AddDoRiciActor(entity);
                    }
                    else
                    {
                        entity->Release();
                        model->Release();
                    }
                }
            }
        }
        if (actors.size())
        {
            float reach = 200.0f;
            AIManager.police.MamProVasAkciHosi(human, u, actors, b1, &reach);
        }
    }
}

// The active pedestrians within range hit, by the distance, where the explosion's line reaches the pedestrian or a
// part of its model
// FUNCTION: GAME 0x450450
void C_traffic_generator::Explosion(S_vector &pos, float range, float power)
{
    if (!all_visible)
        return;
    float range_2 = range * range;
    S_vector hit, normal;
    for (int i = array.size(); i--;)
    {
        if (array[i]->active)
        {
            S_vector dir = array[i]->pos - pos;
            if (!(dir.Square() > range_2))
            {
                if (gifCollision.TestLineHStatic(pos, dir, &hit, &normal, 0))
                {
                    int n = array[i]->frame->NumChildren();
                    for (int j = 0; j < n; j++)
                    {
                        I3D_frame *part = array[i]->frame->GetChild(j);
                        const S_vector &wp = part->GetWorldPos();
                        S_vector to_part;
                        to_part.x = wp.x - pos.x;
                        to_part.y = wp.y - pos.y;
                        to_part.z = wp.z - pos.z;
                        if (!gifCollision.TestLineHStatic(pos, to_part, &hit, &normal, 0))
                            goto visible;
                    }
                    continue;
                }
            visible:
                float k = 1.0f - dir.Magnitude() / range;
                if (k < 0.0f)
                    k = 0.0f;
                dir.Normalize();
                array[i]->Hit(HIT_EXPLOSION, dir, 5, NULL, k * power);
            }
        }
    }
}

// The active pedestrians within range of the fire hurt
// FUNCTION: GAME 0x450780
void C_traffic_generator::Fire(S_vector &pos, float range_2, float damage)
{
    if (!all_visible)
        return;
    for (int i = array.size(); i--;)
    {
        if (array[i]->active)
        {
            S_vector dir = array[i]->pos - pos;
            if (!(dir.Square() > range_2))
                array[i]->Hit(HIT_FIRE, dir, 5, NULL, damage);
        }
    }
}

// From the last one
// FUNCTION: GAME 0x450830
C_traffic_element *C_traffic_generator::GetElemFromFrame(I3D_frame *frm)
{
    for (int i = array.size(); i--;)
    {
        if (array[i]->frame == frm)
            return array[i];
    }
    return NULL;
}

// The active pedestrians taken out of the game, from the last one
// FUNCTION: GAME 0x450860
void C_traffic_generator::FreeDynCollElements()
{
    for (int i = array.size(); i--;)
    {
        if (array[i]->active)
            array[i]->Deactivate();
    }
}

// The squared distance of the nearest active, living pedestrian, 1e30 for none
// FUNCTION: GAME 0x4508a0
float C_traffic_generator::GetNearElementDist2(S_vector &pos)
{
    float best = 1e30f;
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && !array[i]->dead)
        {
            float d = (pos - array[i]->pos).Square();
            if (d < best)
                best = d;
        }
    }
    return best;
}

// The pedestrian within half a metre, in the horizontal plane, of the point half a step ahead of e, NULL for none
// FUNCTION: GAME 0x450920
C_traffic_element *C_traffic_generator::IsNearElementBefore(C_traffic_element *e)
{
    S_vector ahead = e->pos + e->dir * 0.5f;
    ahead.y = 0.0f;
    for (int i = array.size(); i--;)
    {
        if (array[i] == e || !array[i]->active || array[i]->dead)
            continue;
        S_vector p = array[i]->pos;
        p.y = 0.0f;
        if ((ahead - p).Square() < 0.25f)
            return array[i];
    }
    return NULL;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_traffic_generator::Update(uint time)
{
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_traffic_generator::NetDirect(uint time)
{
}

// The pedestrians panic at pos
// FUNCTION: GAME 0x4509d0
void C_traffic_generator::NewPanic(S_vector &pos)
{
    last_panic = pos;
    for (int i = array.size(); i--;)
    {
        array[i]->EndMavaniNaTaxi();
        if (array[i]->active && !array[i]->dead)
            array[i]->SetPanic();
    }
}

// The pedestrian of the frame panics
// FUNCTION: GAME 0x450a30
void C_traffic_generator::NewPanic(I3D_frame *frm)
{
    C_traffic_element *e = GetElemFromFrame(frm);
    if (e)
    {
        e->EndMavaniNaTaxi();
        if (e->active && !e->dead)
            e->SetPanic();
    }
}

// Each entry of the rail vehicle at a station takes the nearest pedestrian walking about, which turns into a model
// of the vehicle's: the pedestrian is out of the game once the vehicle has it
// FUNCTION: GAME 0x450a60
void C_traffic_generator::RailwayInStation(C_railway *rail, std::vector<rentry> &entries)
{
    for (int k = entries.size(); k--;)
    {
        S_vector away = -entries[k].dir;
        int best = -1;
        float best_dist2 = 225.0f;
        for (int i = array.size(); i--;)
        {
            if (array[i]->active && !array[i]->dead && !array[i]->fleeing && array[i]->generator &&
                array[i]->num_way < 0 && array[i]->model_type[2] != 1 && array[i]->model_type[2] != 3 &&
                array[i]->model_type[2] != 4 && !array[i]->dog_head)
            {
                S_vector d = array[i]->pos - entries[k].pos;
                float dist2 = d.Square();
                if (dist2 < best_dist2 && (float)fabs(array[i]->pos.y - entries[k].pos.y) < 2.0f && away.AngleTo(d) < 1.3f)
                {
                    best_dist2 = dist2;
                    best = i;
                }
            }
        }
        if (best > -1)
        {
            array[best]->SetAlpha(1.0f);
            I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            act_mission->GetGame()->DelFrameForShadow(array[best]->frame, SHADOW_TYPE_0);
            model->Duplicate(array[best]->frame);
            act_mission->GetGame()->AddFrameForShadow(array[best]->frame, SHADOW_TYPE_0,
                (E_shadow_type)(array[best]->model_type[0] == 1 ? 3 : 2), NULL, NULL);
            model->m_UserData = 0;
            model->SetPos(array[best]->frame->GetPos());
            model->SetDir(array[best]->frame->GetDir(), 0.0f);
            model->LinkTo(array[best]->frame->GetParent(), 0);
            model->Update();
            if (rail->AddPerson(model, entries[k].wagon, entries[k].entry))
                array[best]->Deactivate();
            else
            {
                model->LinkTo(NULL, 0);
                model->Release();
            }
        }
    }
}

// A pedestrian of that model into the game, walking from where the model stands
// FUNCTION: GAME 0x450f00
void C_traffic_generator::AddFreePerson(I3D_model *model, C_actor *act, bool b)
{
    C_traffic_element *e = new C_traffic_element;
    if (e->SetModel(model))
        e->ActivateFree(true, b);
}

// A pedestrian of that model getting out of a car: the model half a metre up, walking to the two points
// FUNCTION: GAME 0x450f80
void C_traffic_generator::AddFreePersonFromCar(I3D_model *model, C_actor *act, S_vector &pos, S_vector &dir)
{
    model->SetPos(S_vector(model->GetPos().x, model->GetPos().y + 0.5f, model->GetPos().z));
    model->Update();
    C_traffic_element *e = new C_traffic_element;
    if (e->SetModel(model))
    {
        e->ActivateFree(true, false);
        e->way[0] = dir;
        e->way[1] = pos;
        e->num_way = 2;
        e->detour = 0;
        e->SetNewNode();
    }
}

// A pedestrian's scream (the voice i2 - 2 of voices.def in the voice i1) heard from the camera within f2, at pos
// FUNCTION: GAME 0x4510b0
void C_traffic_generator::ElementNewScream(int i1, int i2, S_vector &pos, float f1, float f2, float f3, float f4)
{
    if (!global_valid_scream || !isound || t_scream > 100)
        return;
    S_vector cam = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    float dx = cam.x - pos.x;
    float dz = cam.z - pos.z;
    if (dx * dx + dz * dz > f2 * f2)
        return;
    t_scream += 500;
    i2 -= 2;
    if ((uint)i2 >= numOfGameVoices)
        return;
    uint sound = gameVoices[i2].sounds[i1];
    if (!sound)
        return;
    char name[16];
    sprintf(name, "%08lu.wav", sound);
    I3D_sound *snd = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    if (!snd)
        return;
    if (sound_cache.Open(snd, name, 0, NULL, NULL, NULL) == I3D_OK)
    {
        snd->SetLoop(false);
        snd->SetPos(pos);
        snd->Update();
        snd->SetSoundType(I3DSOUND_POINT);
        snd->SetRange(f1, f2, f3, f4);
        snd->SetVolume(mafiaLoadSave.settings.speech_volume / (mafiaLoadSave.settings.sound_volume + .01f) * .95f +
            .05f);
        snd->SetOutVol(0.0f);
        snd->SetOn(true, true);
        act_mission->GetGame()->Play3DSound(snd);
    }
    else
        snd->Release();
}

// The pedestrians walking to the car turn away: each gives up its car and walks on from its last point
// FUNCTION: GAME 0x4512a0
void C_traffic_generator::VyfunOdTeMasiny(C_car *car)
{
    if (!car)
        return;
    for (int i = array.size(); i--;)
    {
        if (array[i]->active && !array[i]->dead && array[i]->car == car)
        {
            switch (array[i]->num_way)
            {
            case 0:
                array[i]->way[0] = array[i]->way[2];
                array[i]->num_way = 2;
                break;
            case 1:
                array[i]->way[0] = array[i]->way[2];
                array[i]->num_way = 1;
                break;
            default:
                array[i]->num_way = 0;
                break;
            }
            array[i]->car = NULL;
            array[i]->SetNewNode();
        }
    }
}

// FUNCTION: GAME 0x4513a0
C_traffic_element::C_traffic_element()
{
    generator = NULL;
    active = false;
    frame = NULL;
    machine = NULL;
}

// Walks around the actor in the way: two points to either side of the walk, the side nearer the next node's line
// FUNCTION: GAME 0x451400
bool C_traffic_element::ObejdiTuPicovinu(C_actor *act, I3D_frame *frm)
{
    if (num_way > -1)
    {
        if (!target_human)
            return car ? false : true;
    }
    else if (act)
    {
        switch (act->type)
        {
        case ACTOR_PLAYER:
        case ACTOR_ENTITY:
        {
            S_vector fwd = target - pos;
            fwd.Normalize();
            S_vector side(fwd.z, fwd.y, -fwd.x);
            fwd *= 2.0f;
            side *= 1.5f;
            const S_vector &node_pos = act_mission->web_path->nodes[prev_node].pos;
            S_vector2 np(node_pos.x, node_pos.z);
            S_vector2 left(pos.x + side.x, pos.z + side.z);
            S_vector2 right(pos.x - side.x, pos.z - side.z);
            S_vector2 to_target(target.x - np.x, target.z - np.y);
            float dl = (float)fabs(left.DistanceToLine(np, to_target));
            float dr = (float)fabs(right.DistanceToLine(np, to_target));
            if (dr < dl)
                side *= -1.0f;
            way[1] = pos + side;
            way[0] = pos + side + fwd;
            num_way = 2;
            detour = 1;
            S_vector *p = &way[2];
            for (int i = 2; i; i--)
            {
                p--;
                S_vector down(0.0f, -3.0f, 0.0f);
                S_vector from(p->x, p->y + 1.0f, p->z);
                S_vector hit;
                if (gifCollision.TestLineV(from, down, &hit, NULL, 0, 0))
                {
                    *p = hit;
                    p->y += 0.05f;
                }
            }
            SetNewNode();
            return true;
        }
        }
    }
    return false;
}

// The pedestrian's next point: along way while num_way counts down (TryGoToAssHole's walk to a car), then what the
// walk's end asks (a human to talk to, the car to board, a fixed node), else from the node it stands at along one of the
// web's connections its flags take: the one turning most away from the last panic if it flees, or a random one but the
// way back, and the target a random way across the node's width
// FUNCTION: GAME 0x451860
bool C_traffic_element::SetNewNode()
{
    if (num_way > -1)
    {
        if (--num_way < 0)
        {
            if (target_human)
            {
                running = false;
                wait_time = 3000;
                if (act_mission->GetGame()->shvestky.DalsiChalpiDoPartie(target_human, 0, 200.0f))
                {
                    if (target_human == act_mission->game->player)
                        mafiaIndicators.ConsoleAddTextByID(0x836, 0xffff0000);
                    C_traffic_generator::ElementNewScream(voice, rnd(3) + 0xb0, pos, 1.0f, 30.0f, 0.4f, 0.5f);
                }
                target_human = NULL;
                return true;
            }
            if (car)
            {
                I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
                SetAlpha(1.0f);
                act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
                model->Duplicate(frame);
                act_mission->game->AddFrameForShadow(frame, SHADOW_TYPE_0,
                    (E_shadow_type)(model_type[0] == 1 ? 3 : 2), NULL, NULL);
                model->LinkTo(frame->GetParent(), 0);
                C_traffic_car::ChodDoRici(model, car);
                car = NULL;
                Deactivate();
                return true;
            }
            if (detour)
            {
                C_web_path *wp = act_mission->web_path;
                target = wp->nodes[node].GetPos();
                node_width = (wp->nodes[node].width & 0xffff) * 0.01f;
                return true;
            }
            C_web_path *wp = act_mission->web_path;
            int n = wp->GetNearNode(pos, -1.0f, web_flags);
            if (n != -1)
            {
                node = n;
                target = wp->nodes[n].GetPos();
                node_width = (wp->nodes[n].width & 0xffff) * 0.01f;
                return true;
            }
        }
        else
            target = way[num_way];
        return true;
    }
    C_web_path *wp = act_mission->web_path;
    if (fleeing)
    {
        S_vector away = pos - C_traffic_generator::last_panic;
        float best = 10.0f;
        C_web_node *n = &wp->nodes[node];
        for (int i = n->num_conns; i--;)
        {
            C_web_connection *c = n->GetConn(i);
            if (web_flags & c->flags)
            {
                float a = (wp->nodes[c->node].GetPos() - pos).AngleTo(away);
                if (a < best)
                {
                    best = a;
                    prev_node = node;
                    node = c->node;
                    target = wp->nodes[node].GetPos();
                }
            }
        }
        if (best > 5.0f)
        {
            int t = prev_node;
            prev_node = node;
            node = t;
            target = wp->nodes[node].GetPos();
        }
    }
    else
    {
        C_web_node *n = &wp->nodes[node];
        int cnt = 0;
        for (int i = n->num_conns; i--;)
        {
            C_web_connection *c = n->GetConn(i);
            if ((web_flags & c->flags) && c->node != prev_node)
                cnt++;
        }
        if (cnt)
        {
            int r = rnd(cnt);
            for (int i = n->num_conns; i--;)
            {
                C_web_connection *c = n->GetConn(i);
                if ((web_flags & c->flags) && c->node != prev_node)
                {
                    if (r == 0)
                    {
                        prev_node = node;
                        node = c->node;
                        target = wp->nodes[node].GetPos();
                        break;
                    }
                    r--;
                }
            }
        }
        else
        {
            int t = prev_node;
            prev_node = node;
            node = t;
            target = wp->nodes[node].GetPos();
        }
    }
    S_vector d = wp->nodes[node].GetPos() - wp->nodes[prev_node].GetPos();
    S_vector side(-d.z, 0.0f, d.x);
    side.Normalize();
    node_width = (wp->nodes[node].width & 0xffff) * 0.01f;
    side *= rnd() * node_width * 0.9f;
    target += side;
    if (wp->nodes[node].flags & WEBF_UNKNOWN_1)
        web_flags = 1;
    return true;
}

// A pedestrian waving at the player's taxi: it turns to the car, and once the car is slow and near it gets in as the
// taxi's passenger (an entity made of its model with the taxi passenger's script, which the police's board gets as its
// customer); too far from the generator it is deactivated
// FUNCTION: GAME 0x451ff0
void C_traffic_element::MavaNaTaxi(uint time)
{
    C_human *player = act_mission->GetGame()->player;
    C_car *taxi;
    if (!player || !(taxi = player->car) || taxi->GetType() != ACTOR_CAR)
    {
        EndMavaniNaTaxi();
        return;
    }
    float sec = time * 0.001f;
    float turn = sec * 3.0f;
    S_vector to_car = taxi->GetWorldPos() - frame->GetWorldPos();
    float angle = AngleTo2(S_vector2(to_car.x, to_car.z), S_vector2(dir.x, dir.z));
    float fa = fabs(angle);
    if (fa <= turn || fa < 0.01)
    {
        dir = to_car;
        dir.y = 0.0f;
        dir.Normalize();
        anim_next = ANIM_BREATH01A;
    }
    else if (angle > 0.0f)
    {
        turn *= -1.0f;
        anim_next = ANIM_RIGHT1;
        RotateDir(dir, turn);
    }
    else
    {
        anim_next = ANIM_LEFT1;
        RotateDir(dir, turn);
    }
    dir = to_car;
    frame->SetDir(dir, 0.0f);
    frame->Update();
    SetAnim();
    frame->Tick(time);
    if (machine->IsActive())
        machine->Tick(time);
    if (taxi->speed < 0.5f && to_car.x * to_car.x + to_car.y * to_car.y + to_car.z * to_car.z < 100.0f)
    {
        bool male = true;
        char name[64];
        I3D_frame *back = frame->FindChildFrame("back1", ENUMF_ALL);
        if (back)
        {
            strcpy(name, back->m_pProperty ? back->m_pProperty : "");
            if (strlen(name) >= 3 && (uchar)(name[0] - 'A') == 1)
                male = false;
        }
        I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
        model->Duplicate(frame);
        E_shadow_type shadow = (E_shadow_type)(model_type[0] == 1 ? 3 : 2);
        act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, shadow, NULL, NULL);
        model->SetPos(frame->GetPos());
        model->SetRot(frame->GetRot());
        model->LinkTo(frame->GetParent(), 0);
        model->Update();
        model->SetOn(true);
        C_entity *entity = new C_entity;
        if (entity->Init(model))
        {
            entity->voice_type = (male ? HUMAN_VOICE_TRAFFIC_MALE : HUMAN_VOICE_TRAFFIC_FEMALE) + rnd(3);
            entity->human_type = HUMAN_TYPE_UNKNOWN_80;
            C_chunk chunk;
            if (chunk.OpenFile("system\\taxi_pas.dat", false))
            {
                entity->LoadSourceCode(&chunk);
                char *source = entity->source;
                if (source && strlen(source))
                {
                    int index = 0;
                    C_game *game = act_mission->GetGame();
                    if (game->taxi_points.size())
                    {
                        int tries = 0;
                        S_vector d;
                        do
                        {
                            tries++;
                            index = rnd(game->taxi_points.size());
                            S_vector p = game->taxi_points[index]->GetWorldPos();
                            d = frame->GetWorldPos() - p;
                            if (d.Dot(d) > 250000.0f)
                                break;
                        } while (tries < 10);
                    }
                    ushort from = act_mission->GetRoads()->GetNearNode(game->taxi_points[index]->GetWorldPos()) | NEARNODE_FLAG;
                    ushort to = act_mission->GetRoads()->GetNearNode(frame->GetWorldPos()) | NEARNODE_FLAG;
                    std::vector<ushort> path;
                    float total = 1000.0f;
                    if (act_mission->GetRoads()->FindPath(from, to, path) && path.size() > 1)
                    {
                        total = 0.0f;
                        for (int i = path.size() - 1; i--;)
                        {
                            S_vector p1 = act_mission->GetRoads()->GetPos(path[i + 1]);
                            S_vector p2 = act_mission->GetRoads()->GetPos(path[i]);
                            total += (p2 - p1).Magnitude();
                        }
                    }
                    int t = 20 - (int)(total * (-1.0f / 13.8f));
                    int minutes = (t / 60 + 1) * 60;
                    int d2 = ((int)total / 1000 + 1) * 200;
                    char *buf = new char[strlen(source) + 0x80];
                    sprintf(buf, source, game->taxi_points[index]->GetName(), game->taxi_points[index]->GetName(), minutes, d2, d2, 12, 0, 0,
                        minutes);
                    entity->SetSourceCode(buf);
                    delete buf;
                }
                chunk.Close();
            }
            act_mission->GetGame()->AddTemporaryActor(entity);
            if (!entity->traffic)
                entity->traffic = new S_human_traffic;
            memset(entity->traffic, 0, sizeof(S_human_traffic));
            entity->traffic->traffic_frame = true;
            AIManager.police.TuMasZakaznika(entity);
            act_mission->GetGame()->taxi_passenger = entity;
        }
        else
        {
            entity->Release();
            model->Release();
        }
        C_traffic_generator::ZrusVsechnyMavajici();
        Deactivate();
        C_traffic_generator::taxi_timer = 20000;
    }
    else if (dist > max_dist * max_dist)
        Deactivate();
}

// Ends the waving at the taxi: the waving animation (0xdca9, the machine's extended one) taken off
// FUNCTION: GAME 0x452ca0
void C_traffic_element::EndMavaniNaTaxi()
{
    if (machine->unknown_10e == 0xdca9)
        machine->DelExtendedAnim(4.0f);
    waving = false;
}

// The end of an animation with nothing queued, or of the timed state (Tick, twice): a pedestrian without health is
// taken out of the collision and the shadows and stays lying; the others walk on or go on with the panic
inline void C_traffic_element::EndOfAnim()
{
    if (energy > 0.0f)
    {
        if (!fleeing || !mega_panic)
        {
            anim_once = false;
            anim_next = running ? ANIM_RUN1 : ANIM_WALK1;
            return;
        }
        switch (mega_panic)
        {
        case 1:
            anim_next = ANIM_TRAFFIC_PANIKA_DREP_DOLU;
            anim_after = ANIM_TRAFFIC_PANIKA_DREP_STATIV;
            break;
        case 2:
            anim_next = ANIM_TRAFFIC_PANIKA_NA4_DOLU;
            anim_after = ANIM_TRAFFIC_PANIKA_NA4_STATIV;
            break;
        case 3:
            anim_next = ANIM_TRAFFIC_PANIKA_LEH_DOLU;
            anim_after = ANIM_TRAFFIC_PANIKA_LEH_STATIV;
            break;
        }
        anim_after_once = false;
        anim_once = false;
    }
    else
    {
        gifCollision.DynErase(&coll);
        in_coll = false;
        dead = true;
        act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
        anim_once = false;
    }
}

// The pedestrian by the time passed: falling, the model's animations and the track's collisions, the panic, the cars
// that come near (a car is driven while its driver's seat has a mass), the walk along the web's nodes with its detours
// around the player, the cars and the other pedestrians
// FUNCTION: GAME 0x452cd0
void C_traffic_element::Tick(uint time)
{
    // The shared locals: time in ms and in seconds (set where the pedestrian is alive and not falling), the step the
    // walk takes (a speed of the walk, the scale and the time), and the vector to the point it walks to
    float fms;
    float t;
    float spd;
    S_vector d;
    bool blocked;
    S_matrix m1;

    // The alpha of the pedestrian near the camera is made full
    if (dist < 400.0f && alpha < 1.0f)
    {
        alpha = 1.0f;
        SetAlpha(1.0f);
    }
    if (active && frame)
    {
        // A dead one is taken out once it is out of its range
        if (dead)
        {
            if (dist > max_dist * max_dist)
            {
                Deactivate();
                return;
            }
        }
        else if (fall_speed > 0.0f)
        {
            // Falling (fall_speed, the speed): down onto the ground, a splash and a sound at the water, gone at
            // 50
            float tt = (float)time * 0.001f;
            fall_speed += 8.9f * tt;
            float dy = fall_speed * tt;
            gifCollision.DynSetNextCollOwner(&coll, 0);
            S_vector hit_pos;
            S_vector hit_normal;
            S_vector line;
            line.x = 0.0f;
            line.y = -dy;
            line.z = 0.0f;
            g_collision_header *hit = (g_collision_header *)gifCollision.TestLineV(pos, line, &hit_pos, &hit_normal,
                -1, 0);
            gifCollision.DynSetNextCollOwner(NULL, 0);
            if (hit)
            {
                if (hit->material == MAT_WATER_DEATH)
                {
                    if (act_mission->options.particles >= 1)
                    {
                        S_vector splash_dir;
                        splash_dir.x = 0.0f;
                        splash_dir.y = 0.0f;
                        splash_dir.z = 1.0f;
                        act_mission->effects->CreateParticle(0x25, hit_pos, splash_dir, 0, NULL);
                    }
                    if (act_mission->options.game_sounds >= 1)
                    {
                        const S_vector &fp = frame->GetWorldPos();
                        const S_vector &cp = act_mission->scene->GetActiveCamera()->GetWorldPos();
                        float dx = cp.x - fp.x;
                        float dy2 = cp.y - fp.y;
                        float dz = cp.z - fp.z;
                        if (dy2 * dy2 + dz * dz + dx * dx < 900.0f)
                        {
                            I3D_sound *sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
                            if (sound)
                            {
                                sound->SetPos(hit_pos);
                                sound_table.ConstructSound(sound, 0xec);
                                sound->SetVolume(0.6f);
                                sound->SetOn(true);
                                act_mission->game->Play3DSound(sound);
                            }
                        }
                    }
                    pos.y -= dy;
                }
                else
                {
                    fall_speed = -1.0f;
                    pos.y = hit_pos.y;
                    no_ground_count = 0;
                }
            }
            else
                pos.y -= dy;
            frame->SetPos(pos);
            frame->SetDir(dir, 0.0f);
            frame->Update();
            act_mission->scene->SetFrameSectorPos(frame, frame->GetWorldPos());
            ((I3D_bsphere &)coll.bsphere).pos = frame->GetWorldPos() + S_vector(0.0f, 0.9f, 0.0f);
            inv_matrix = frame->GetMatrix();
            m1 = inv_matrix;
            inv_matrix.Inverse(m1);
            if (!coll_wait && in_coll)
                gifCollision.DynUpdate(&coll);
            frame->Tick(time);
            if (machine->IsActive())
                machine->Tick(time);
            if (fall_speed > 50.0f)
            {
                Deactivate();
                return;
            }
        }
        else if (hit_wait)
        {
            // Waiting after a car hit it (wait_time, the time left): once it is over and no car is near any
            // more, it gets up
            if (machine->IsActive())
                machine->Tick(time);
            frame->Tick(time);
            if (wait_time > 0)
            {
                wait_time -= time;
                if (wait_time < 0)
                    wait_time = 0;
                return;
            }
            hit_wait = false;
            for (int i = C_actor::global_actors.size(); i--;)
            {
                C_actor *a = C_actor::global_actors[i];
                if (a->type == ACTOR_CAR && a->act_state != ACTSTATE_NONE)
                {
                    const S_vector &cp = a->frame->GetWorldPos();
                    float dx = cp.x - pos.x;
                    float dy2 = cp.y - pos.y;
                    float dz = cp.z - pos.z;
                    if (dx * dx + dy2 * dy2 + dz * dz < 16.0f)
                    {
                        hit_wait = true;
                        break;
                    }
                }
            }
            if (hit_wait)
                wait_time = rnd(200) + 500;
            else
            {
                anim_after = 0;
                anim_next = ANIM_TRAFFIC_PANIKA_LEH_NAHORU;
                SetAnim();
            }
            float r2 = max_dist * max_dist;
            if (dist > r2)
            {
                Deactivate();
                return;
            }
        }
        else
        {
            // Run over by a car (hit_car): the fall, and the police are told. A wave at the taxi goes on.
            if (!in_coll && energy > 0.0f && hit_car && hit_car->speed < 10.0f)
            {
                const S_vector &cp = hit_car->frame->GetWorldPos();
                float dx = pos.x - cp.x;
                float dy2 = pos.y - cp.y;
                float dz = pos.z - cp.z;
                if (dx * dx + dy2 * dy2 + dz * dz < 4.0f)
                {
                    anim_next = ANIM_TRAFFIC_PANIKA_LEH_STATIV;
                    machine->prev_track = -1;
                    machine->track = -1;
                    coll_wait = 2;
                    SetAnim();
                    hit_wait = true;
                    wait_time = 1000;
                    C_game *game = act_mission->game;
                    if (!game->shvestky.active)
                        return;
                    game->shvestky.NextNaSrackySoudruh(hit_car->GetOwner(0));
                    return;
                }
            }
            if (waving)
            {
                C_game *game = act_mission->game;
                if (!game->shvestky.active)
                    return;
                if (game->shvestky.PocetNasranych(game->player) > 0)
                {
                    C_traffic_generator::ZrusVsechnyMavajici();
                    return;
                }
                MavaNaTaxi(time);
                return;
            }

            // The car it walks to drives off: a new node. Its alpha fades; out of sight too long or out of its
            // range it is taken out.
            if (car && car->seats[0].mass > 0.01f)
            {
                car = NULL;
                num_way = 0;
                SetNewNode();
            }
            velocity.x = 0.0f;
            velocity.y = 0.0f;
            velocity.z = 0.0f;
            if (keep_alive)
                keep_alive--;
            if (alpha_dir)
            {
                alpha += (float)alpha_dir * time * 0.001f;
                if (alpha > 1.0f)
                {
                    alpha = 1.0f;
                    alpha_dir = 0;
                }
                else if (alpha < 0.0f)
                {
                    alpha = 0.0f;
                    alpha_dir = 0;
                    Deactivate();
                    return;
                }
                SetAlpha(alpha);
            }
            visible = act_mission->render_number == frame->m_RenderNumber;
            if (visible)
                invisible_time = 0;
            else
                invisible_time += time;
            if (!visible && !generator && !keep_alive && invisible_time > 10000)
            {
                Deactivate();
                return;
            }
            if (alpha_dir >= 0 && !keep_alive && dist > max_dist * max_dist)
            {
                if (visible)
                    alpha_dir = -1;
                else
                {
                    Deactivate();
                    return;
                }
            }
            if (anim_next != anim)
                SetAnim();

            // The animations' tick: a track moves it, as far as the collisions let it (its move is added to
            // machine->track_offset); the end of an animation gives the next, or EndOfAnim
            if (machine)
            {
                if (machine->IsActive())
                {
                    uint r = machine->Tick(time);
                    if (machine && (machine->track != -1 || machine->prev_track != -1))
                    {
                        S_vector mv(pos.x - anim_pos.x, pos.y - anim_pos.y, pos.z - anim_pos.z);
                        if (mv.Square() > 0.0049f)
                        {
                            gifCollision.DynSetNextCollOwner(&coll, 0);
                            S_vector hp;
                            S_vector hn;
                            int h = gifCollision.TestSphereB(anim_pos + S_vector(0.0f, 0.6f, 0.0f), mv, 0.5f, &hp,
                                &hn, 2, 0);
                            gifCollision.DynSetNextCollOwner(NULL, 0);
                            if (h)
                            {
                                hp -= S_vector(0.0f, 0.6f, 0.0f);
                                machine->track_offset += hp - pos;
                                pos = hp;
                            }
                        }
                        UpdateModel();
                    }
                    if (r & 1)
                    {
                        if (anim_after == 0)
                        {
                            EndOfAnim();
                            return;
                        }
                        anim_next = anim_after;
                        anim_after = 0;
                        anim_once = anim_after_once;
                    }
                }
            }

            // The model's tick, while it is seen, moved by a track or in a timed state (anim_once): the neck
            // (dog_head is set) made small; a track again moves it (collisions) and that is all
            if (visible || (machine && (machine->track != -1 || machine->prev_track != -1)) || anim_once)
            {
                frame->Tick(time);
                if (dog_head)
                {
                    I3D_frame *nk = frame->FindChildFrame("neck", ENUMF_ALL);
                    if (nk)
                    {
                        S_vector sc = nk->GetScale();
                        if (sc.x > 0.02f)
                        {
                            nk->SetScale(S_vector(0.01f, 0.01f, 0.01f));
                            nk->Update();
                        }
                    }
                }
            }
            if (machine && (machine->track != -1 || machine->prev_track != -1))
            {
                S_vector mv(pos.x - anim_pos.x, pos.y - anim_pos.y, pos.z - anim_pos.z);
                if (mv.Square() > 0.0049f)
                {
                    gifCollision.DynSetNextCollOwner(&coll, 0);
                    S_vector hp;
                    S_vector hn;
                    int h = gifCollision.TestSphereB(anim_pos + S_vector(0.0f, 0.6f, 0.0f), mv, 0.5f, &hp, &hn, 2,
                        0);
                    gifCollision.DynSetNextCollOwner(NULL, 0);
                    if (h)
                    {
                        hp -= S_vector(0.0f, 0.6f, 0.0f);
                        machine->track_offset += hp - pos;
                        pos = hp;
                    }
                }
                UpdateModel();
                return;
            }

            // The timed state (anim_once, anim_time the time left) ends: dead where the health is gone, else
            // the animation of the panic (EndOfAnim, as at the end of an animation)
            if (anim_once)
            {
                anim_time -= time;
                if (anim_time > 0)
                    return;
                EndOfAnim();
                return;
            }

            // Fleeing (the panic of last_panic's place): screams and a stronger panic by the distance; it ends
            // with the time (panic_time) or far away; the animation of a state of the panic (mega_panic)
            if (fleeing)
            {
                float dx = C_traffic_generator::last_panic.x - pos.x;
                float dy2 = C_traffic_generator::last_panic.y - pos.y;
                float dz = C_traffic_generator::last_panic.z - pos.z;
                float d2 = dz * dz + dy2 * dy2 + dx * dx;
                if (!mega_panic)
                {
                    if (d2 < 625.0f && rnd(100) == 0)
                        SetMegaPanic(rnd(3) + 1);
                    if (!mega_panic && rnd(100) == 0)
                    {
                        if (rnd(10) == 0)
                            C_traffic_generator::ElementNewScream(voice, rnd(4) + 0xb3, pos, 1.0f, 45.0f,
                                0.3f, 0.3f);
                        else
                            C_traffic_generator::ElementNewScream(voice, rnd(4) + 0x42, pos, 1.0f, 55.0f,
                                0.3f, 0.3f);
                    }
                }
                panic_time -= time;
                if (panic_time <= 0 || d2 > 2500.0f)
                {
                    fleeing = false;
                    running = false;
                }
                if (mega_panic)
                {
                    if (!fleeing)
                    {
                        switch (mega_panic)
                        {
                        case 1:
                            anim_next = ANIM_TRAFFIC_PANIKA_DREP_NAHORU;
                            break;
                        case 2:
                            anim_next = ANIM_TRAFFIC_PANIKA_NA4_NAHORU;
                            break;
                        case 3:
                            anim_next = ANIM_TRAFFIC_PANIKA_LEH_NAHORU;
                            break;
                        }
                    }
                    return;
                }
            }

            // The neck turns towards what it looks at (look_at, a frame; neck_reset, it turns back) by the
            // time passed
            if (energy < 0.0f)
                return;
            fms = (float)time;
            t = fms * 0.001f;
            if (neck_reset || look_at)
            {
                I3D_frame *nk = NULL;
                S_vector look(0.0f, 0.0f, 1.0f);
                if (look_at)
                {
                    nk = frame->FindChildFrame("neck", ENUMF_ALL);
                    if (nk)
                    {
                        look = look_at->GetWorldPos() - nk->GetWorldPos();
                        m1 = frame->GetMatrix();
                        m1(3).x = m1(3).y = m1(3).z = 0.0f;
                        S_matrix wm_copy = m1;
                        m1.Inverse(wm_copy);
                        look *= m1;
                    }
                }
                S_quat neck_rot = neck->GetRot1();
                m1 = neck_rot.RotationMatrix();
                S_vector cur = m1(2);
                bool turn = true;
                float ang = cur.AngleTo(look);
                if (ang < 0.1f)
                {
                    neck->SetDir(look, 0.0f);
                    neck_reset = false;
                    turn = false;
                }
                if (look.AngleTo(S_vector(0.0f, 0.0f, 1.0f)) > 1.3f)
                {
                    look = S_vector(0.0f, 0.0f, 1.0f);
                    look_at = NULL;
                    neck_reset = true;
                    ang = cur.AngleTo(look);
                    if (ang < 0.1f)
                        goto neck_done;
                }
                if (turn)
                {
                    float f = fms * 0.0031415927f / ang;
                    look.Normalize();
                    cur.Normalize();
                    float g = 1.0f - f;
                    neck->SetDir(cur * g + look * f, 0.0f);
                }
            }
neck_done:

            // A car that would run it over (honking or coming near) makes it step aside: an animation by the
            // angle to the car's way and a scream
            {
                C_car *near_car = NULL;
                for (int i = act_mission->game->active_actors.size(); i--;)
                {
                    C_actor *a = act_mission->game->active_actors[i];
                    if (a->type == ACTOR_CAR)
                    {
                        float range = C_traffic_generator::t_horn > 0 ? (((C_car *)a)->speed > 1.0f ? 50.0f : 10.0f) : -1.0f;
                        near_car = ((C_car *)a)->ChcesMeSejmout(pos, range);
                        if (near_car)
                        {
                            if (C_traffic_generator::t_horn)
                                break;
                            float r = 900.0f;
                            float ca = dir.AngleTo(near_car->frame->GetWorldDir());
                            if (ca < 0.7853981852531433)
                                r = 540.0f;
                            if (ca < 2.35619455575943)
                                r *= 0.8f;
                            if (!((near_car->frame->GetWorldPos() - pos).Square() > r))
                                break;
                            near_car = NULL;
                        }
                    }
                    else if (a->type == ACTOR_TRAFFIC_CAR)
                    {
                        near_car = ((C_traffic_car *)a)->ChcesMeSejmout(pos);
                        if (near_car)
                        {
                            if (C_traffic_generator::t_horn)
                                break;
                            float r = 400.0f;
                            float ca = dir.AngleTo(near_car->frame->GetWorldDir());
                            if (ca < 0.7853981852531433)
                                r = 200.0f;
                            if (ca < 2.35619455575943)
                                r *= 0.75f;
                            if (!((near_car->frame->GetWorldPos() - pos).Square() > r))
                                break;
                            near_car = NULL;
                        }
                    }
                }
                if (near_car && machine->track == -1 && machine->prev_track == -1 && !anim_once)
                {
                    S_vector cd = near_car->velocity;
                    float ca = dir.AngleTo(cd);
                    if (ca < 0.7853981852531433f)
                        anim_next = ANIM_USKOK_LEFT_BACK;
                    else if (ca > 2.3561945f)
                        anim_next = ANIM_USKOK_RIGHT_FRONT;
                    else
                    {
                        if (AngleTo2(S_vector2(dir.x, dir.z), S_vector2(cd.x, cd.z)) > 0.0f)
                            anim_next = ANIM_USKOK_BACK_RIGHT;
                        else
                            anim_next = ANIM_USKOK_FRONT_LEFT;
                    }
                    S_vector2 pp(pos.x, pos.z);
                    S_vector2 cp2(near_car->GetPos().x, near_car->GetPos().z);
                    S_vector2 side(-cd.z, cd.x);
                    side.Normalize();
                    if (side.Dot(pp) - cp2.Dot(side) < 0.0f)
                        anim_next++;
                    if (anim != anim_next)
                        SetAnim();
                    C_traffic_generator::ElementNewScream(voice, rnd(6) + 200, pos, 5.0f, 40.0f, 0.5f, 0.4f);
                    return;
                }
            }

            // Waiting (wait_time); then the walk along the node's line (line_walk), until it has gone too far
            // from the line (node_width)
            if (wait_time > 0)
            {
                anim_next = ANIM_BREATH01A;
                wait_time -= time;
                if (wait_time < 0)
                    wait_time = 0;
                return;
            }
            if (line_walk)
            {
                anim_next = ANIM_STRAFL1;
                C_web_node *na = &act_mission->web_path->nodes[prev_node];
                C_web_node *nb = &act_mission->web_path->nodes[node];
                S_vector seg = nb->pos - na->pos;
                S_vector mv = S_vector(-dir.z, 0.0f, dir.x) * t;
                seg.Normalize();
                S_vector np = mv + pos;
                float dl = np.DistanceToLine(na->GetPos(), seg);
                pos = np;
                if (dl > node_width)
                {
                    line_walk = false;
                    wait_time = rnd(500) + 500;
                    return;
                }
            }

            // Turning in place towards its point (turning), until it faces it: then it waits 10 s
            if (turning)
            {
                float a = AngleTo2(S_vector2(-(target.z - pos.z), target.x - pos.x), S_vector2(dir.x, dir.z));
                float lim = t * 3.0f;
                if (fabs(a) <= lim || fabs(a) < 0.01)
                {
                    wait_time = 10000;
                    return;
                }
                if (a > 0.0f)
                {
                    anim_next = ANIM_RIGHT1;
                    lim = lim * -1.0f;
                }
                else
                    anim_next = ANIM_LEFT1;
                RotateDir(dir, lim);
            }

            // Now and then (ground_timer) the ground is looked for: the place of the line, a splash into water
            // (falls on), or a fast car coming along makes it wait; no ground for 4 times makes it fall
            ground_timer -= time;
            if (ground_timer < 0)
            {
                ground_timer = rnd(100) + 100;
                gifCollision.DynSetNextCollOwner(&coll, 0);
                S_vector hp;
                S_vector hn;
                S_vector from;
                from.x = pos.x;
                from.y = pos.y + 1.0f;
                from.z = pos.z;
                S_vector line;
                line.x = 0.0f;
                line.y = -2.5f;
                line.z = 0.0f;
                g_collision_header *hit = (g_collision_header *)gifCollision.TestLineV(from, line, &hp, &hn, -1, 0);
                gifCollision.DynSetNextCollOwner(NULL, 0);
                if (hit)
                {
                    pos = hp;
                    on_sidewalk = hit->material == MAT_SIDEWALK;
                    if (hit->material == MAT_WATER_DEATH)
                    {
                        fall_speed = 0.5f;
                        no_ground_count = 0;
                    }
                    else if (!on_sidewalk)
                    {
                        for (int i = act_mission->game->active_actors.size(); i--;)
                        {
                            C_actor *a = act_mission->game->active_actors[i];
                            if (a->type != ACTOR_TRAFFIC_CAR)
                                continue;
                            S_vector nd = dir;
                            nd.Normalize();
                            S_vector v2(nd.x * 5.0f, nd.y * 5.0f, nd.z * 5.0f);
                            S_vector v1(pos.x, pos.y + 1.0f, pos.z);
                            float cdist = -1.0f;
                            C_actor *cact = NULL;
                            if (a->IsPossibleColl(NULL, v1, v2, 0.7f, cdist, &cact) && cact &&
                                cact->type == ACTOR_CAR && ((C_car *)cact)->speed > 5.0f)
                            {
                                wait_time = 3000;
                                return;
                            }
                        }
                        no_ground_count = 0;
                    }
                    else
                        no_ground_count = 0;
                }
                else if (++no_ground_count > 3)
                {
                    fall_speed = 0.1f;
                    anim_next = ANIM_PLACHTENI;
                    SetAnim();
                }
            }
            if (!line_walk && !turning)
            {
                // The step of the walk to its point; arrived it turns to a crossing (turning), takes the next
                // node or is taken out
                spd = (running ? 4.0f : 1.4f) * t * scale;
                d.x = target.x - pos.x;
                d.y = target.y - pos.y;
                d.z = target.z - pos.z;
                float tol = num_way > -1 ? 0.3f : node_width;
                if (tol * tol > d.Square())
                {
                    if (generator && (act_mission->web_path->nodes[node].flags & 0x200) && model_type[2] != 1 &&
                        model_type[2] != 3 && model_type[2] != 4)
                    {
                        turning = true;
                        return;
                    }
                    if (!SetNewNode())
                        Deactivate();
                    return;
                }

                // It turns towards the point (a step of 3 per second): to it, if it is near
                d.Normalize();
                float lim = t * 3.0f;
                float ang = AngleTo2(S_vector2(d.x, d.z), S_vector2(dir.x, dir.z));
                float aang = (float)fabs(ang);
                if (aang <= lim || aang < 0.01)
                {
                    dir = d;
                    dir.y = 0.0f;
                    dir.Normalize();
                }
                else if (ang > 0.0f)
                {
                    lim = lim * -1.0f;
                    anim_next = ANIM_RIGHT1;
                    RotateDir(dir, lim);
                }
                else
                {
                    anim_next = ANIM_LEFT1;
                    RotateDir(dir, lim);
                }
                if (aang < 1.0f)
                {
                    // The velocity, and the place a bit in front where the player, the railway and the cars
                    // are looked for
                    velocity = d * spd;
                    velocity.Normalize();
                    velocity *= scale * 5.0f;
                    blocked = false;
                    S_vector probe;
                    probe.x = dir.x * 0.5f + pos.x;
                    probe.z = dir.z * 0.5f + pos.z;
                    {
                        // The player and the entities (a detour round them, the neck turned to them) and the
                        // railways in the way (back to the last node), the actors from the last; then the cars
                        for (int i = C_actor::global_actors.size(); i--;)
                        {
                            C_actor *a = C_actor::global_actors[i];
                            if (a->act_state != ACTSTATE_ACTIVE)
                                continue;
                            if (a->type == ACTOR_PLAYER || a->type == ACTOR_ENTITY)
                            {
                                const S_vector &ap = a->frame->GetWorldPos();
                                float dx = probe.x - ap.x;
                                float dz = probe.z - ap.z;
                                float pd2 = dz * dz + dx * dx;
                                if (!(pd2 < 0.25f))
                                    continue;
                                if (fleeing)
                                {
                                    if (mega_panic)
                                        break;
                                    SetMegaPanic(rnd(3) + 1);
                                    return;
                                }
                                if (!look_at)
                                    look_at = a->frame->FindChildFrame("neck", ENUMF_ALL);
                                if (pd2 < 0.09f && detour)
                                {
                                    num_way = 0;
                                    detour = 0;
                                    SetNewNode();
                                }
                                if (pd2 < 0.04f)
                                {
                                    if (ObejdiTuPicovinu(a, a->frame))
                                        break;
                                }
                                if (detour)
                                {
                                    num_way = 0;
                                    detour = 0;
                                    SetNewNode();
                                }
                                wait_time = 1000;
                                spd = 0.0f;
                                if (!greeted)
                                {
                                    may_greet = false;
                                    if (rnd(10) == 0)
                                    {
                                        if (rnd(3) == 0)
                                            C_traffic_generator::ElementNewScream(voice, rnd(7) + 0xc1, pos,
                                                1.0f, 25.0f, 0.4f, 0.3f);
                                        else
                                            C_traffic_generator::ElementNewScream(voice, rnd(2) + 0xa8, pos,
                                                1.0f, 25.0f, 0.4f, 0.3f);
                                    }
                                }
                                goto walk;
                            }
                            else if (a->type == ACTOR_RAILWAY && !((uchar *)a)[0x1d8])
                            {
                                const S_vector &rp = a->frame->GetWorldPos();
                                S_vector rv;
                                rv.x = probe.x - rp.x;
                                rv.y = 0.0f;
                                rv.z = probe.z - rp.z;
                                if (!(rv.z * rv.z + rv.x * rv.x < 25.0f))
                                    continue;
                                S_vector to_target;
                                to_target.x = target.x - pos.x;
                                to_target.y = target.y - pos.y;
                                to_target.z = target.z - pos.z;
                                if (!(rv.AngleTo(to_target) > 1.5707964f))
                                    continue;
                                if (fleeing)
                                {
                                    if (!mega_panic)
                                    {
                                        SetMegaPanic(rnd(3) + 1);
                                        return;
                                    }
                                    continue;
                                }
                                wait_time = 0;
                                target = act_mission->web_path->nodes[prev_node].pos;
                                int old_node = node;
                                node = prev_node;
                                prev_node = old_node;
                                node_width = (act_mission->web_path->nodes[node].width & 0xffff) * 0.01f;
                            }
                        }
                        for (int j = C_actor::global_actors.size(); j--;)
                        {
                            C_actor *a = C_actor::global_actors[j];
                            if (a->act_state == ACTSTATE_NONE || a->type != ACTOR_CAR)
                                continue;
                            const S_vector &fp = frame->GetWorldPos();
                            S_vector my_pos = fp;
                            const S_vector &cp = a->frame->GetWorldPos();
                            S_vector d3;
                            d3.x = cp.x - my_pos.x;
                            d3.y = cp.y - my_pos.y;
                            d3.z = cp.z - my_pos.z;
                            if (car == (C_car *)a)
                                continue;
                            if (!(d3.z * d3.z + d3.y * d3.y + d3.x * d3.x < 9.0f))
                                continue;
                            S_vector cdir = a->frame->GetMatrix()(2);
                            cdir.Normalize();
                            if (!(d3.AngleTo(cdir) < 1.5f && aang < 0.5f))
                                continue;
                            my_pos.y += 0.8f;
                            d3.y = 0.0f;
                            d3.Normalize();
                            d3.x *= 0.5f;
                            d3.y *= 0.5f;
                            d3.z *= 0.5f;
                            I3D_COLLISION col;
                            if (!act_mission->scene->TestColHierarchy(my_pos, d3, a->frame, col, 0, NULL, 0, NULL))
                                continue;
                            hit_car = NULL;
                            if (fleeing && !mega_panic)
                            {
                                SetMegaPanic(rnd(3) + 1);
                                return;
                            }
                            if (model_type[2] != 1 && model_type[2] != 3)
                            {
                                wait_time = 1000;
                                spd = 0.0f;
                                blocked = true;
                                if (!greeted)
                                {
                                    may_greet = false;
                                    if ((((C_car *)a)->GetOwner(0) || ((C_car *)a)->seats[0].mass != 0.0f) && rnd(10) == 0)
                                        C_traffic_generator::ElementNewScream(voice, rnd(7) + 0xc1, pos, 1.0f,
                                            25.0f, 0.4f, 0.3f);
                                }
                            }
                            if (rnd(3) == 0 || model_type[2] == 1 || model_type[2] != 3)
                            {
                                wait_time = 0;
                                target = act_mission->web_path->nodes[prev_node].pos;
                                int old_node = node;
                                node = prev_node;
                                prev_node = old_node;
                                node_width = (act_mission->web_path->nodes[node].width & 0xffff) * 0.01f;
                            }
                            if (blocked)
                                goto walk;
                            break;
                        }
                    }

                    // Another pedestrian just ahead: back to the last node at a crossing, else the walk along the
                    // line; or a pedestrian in the way to step round, the neck turned to it
                    if (C_traffic_generator::IsNearElementBefore(this))
                    {
                        if (act_mission->web_path->nodes[node].flags & 0x200)
                        {
                            target = act_mission->web_path->nodes[prev_node].pos;
                            int old_node = node;
                            node = prev_node;
                            prev_node = old_node;
                            node_width = (act_mission->web_path->nodes[node].width & 0xffff) * 0.01f;
                        }
                        else
                        {
                            line_walk = true;
                            spd = 0.0f;
                        }
                    }
                    else
                    {
                        C_traffic_element *e = C_traffic_generator::IsPossibleCollElem(this);
                        if (e)
                        {
                            if (!look_at)
                                look_at = e->frame->FindChildFrame("neck", ENUMF_ALL);
                            S_vector side;
                            side.x = -d.z * (t * 0.5f);
                            side.y = 0.0f * (t * 0.5f);
                            side.z = d.x * (t * 0.5f);
                            const S_vector &la = act_mission->web_path->nodes[prev_node].pos;
                            const S_vector &lb = act_mission->web_path->nodes[node].pos;
                            S_vector seg;
                            seg.x = lb.x - la.x;
                            seg.y = lb.y - la.y;
                            seg.z = lb.z - la.z;
                            seg.Normalize();
                            S_vector np;
                            np.x = side.x + pos.x;
                            np.y = side.y + pos.y;
                            np.z = side.z + pos.z;
                            if (np.DistanceToLine(la, seg) <= node_width)
                                pos = np;
                        }
                    }
walk:
                    // The step along its way, the walk's animation, now and then a call ("Karle_hoj")
                    pos.x = d.x * spd + pos.x;
                    pos.y = d.y * spd + pos.y;
                    pos.z = d.z * spd + pos.z;
                    anim_next = running ? ANIM_RUN1 : ANIM_WALK1;
                    if (greet_timer > 0)
                        greet_timer -= time;
                    else
                    {
                        greet_timer = rnd(100) + 1500;
                        if (rnd(10) == 0)
                            Karle_hoj();
                    }
                }

                // The model put at the place, its direction, the sector, the collision's place and matrix
                frame->SetPos(pos);
                frame->SetDir(dir, 0.0f);
                frame->Update();
                act_mission->scene->SetFrameSector(frame);
                {
                    const S_vector &wp = frame->GetWorldPos();
                    ((I3D_bsphere &)coll.bsphere).pos = S_vector(wp.x, wp.y + 0.9f, wp.z);
                }
                inv_matrix = frame->GetMatrix();
                m1 = inv_matrix;
                inv_matrix.Inverse(m1);
                if (!coll_wait && in_coll)
                    gifCollision.DynUpdate(&coll);
            }
            else
            {
                // The same where it stands still or goes along the line
                frame->SetPos(pos);
                frame->SetDir(dir, 0.0f);
                frame->Update();
                act_mission->scene->SetFrameSector(frame);
                ((I3D_bsphere &)coll.bsphere).pos = frame->GetWorldPos() + S_vector(0.0f, 0.9f, 0.0f);
                inv_matrix = frame->GetMatrix();
                m1 = inv_matrix;
                inv_matrix.Inverse(m1);
                if (!coll_wait && in_coll)
                    gifCollision.DynUpdate(&coll);
            }
        }
    }
    else
    {
        // Not active or without a model: out of the collisions
        if (in_coll)
        {
            gifCollision.DynErase(&coll);
            in_coll = false;
        }
    }
}

// The model placed at pos along dir, its collision moved along
// FUNCTION: GAME 0x4564f0
void C_traffic_element::UpdateModel()
{
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    act_mission->scene->SetFrameSector(frame);
    const S_vector &wpos = frame->GetWorldPos();
    ((I3D_bsphere &)coll.bsphere).pos = S_vector(wpos.x, wpos.y + .9f, wpos.z);
    inv_matrix = frame->GetMatrix();
    inv_matrix.Inverse(S_matrix(inv_matrix));
    if (!coll_wait && in_coll)
        gifCollision.DynUpdate(&coll);
}

// A model given: it takes it (false if it has one), makes the frame the don_ocas's, finds the "base" visual and
// the scale in the "back1" frame's name, and makes the collision and the usable object
// FUNCTION: GAME 0x456630
bool C_traffic_element::SetModel(I3D_model *model)
{
    if (frame)
        return false;
    max_dist = 90.0f;
    active = false;
    generator = NULL;
    frame = model;
    C_actor **p = new C_actor *;
    *p = act_mission->GetGame()->don_ocas;
    SetFrameActor(frame, p);
    base = NULL;
    I3D_frame *b = frame->FindChildFrame("base", ENUMF_ALL);
    if (b && b->GetType() == FRAME_VISUAL &&
        (((I3D_visual *)b)->GetVisualType() == I3D_VISUAL_SINGLEMESH ||
         ((I3D_visual *)b)->GetVisualType() == I3D_VISUAL_SINGLEMORPH))
        base = (I3D_object *)b;
    neck = driver->CreatePose();
    frame->SetPose("neck", neck);
    scale = 1.0f;
    I3D_frame *back = frame->FindChildFrame("back1", ENUMF_ALL);
    if (back)
    {
        char prop[32];
        strcpy(prop, back->GetProperty());
        model_type[0] = model_type[1] = model_type[2] = 0;
        if (strlen(prop) == 6)
        {
            strupr(prop);
            scale = atoi(&prop[3]) * .01f;
            frame->SetScale(S_vector(scale, scale, scale));
            frame->Update();
            model_type[0] = prop[0] - 'A';
            model_type[1] = prop[1] - 'A';
            model_type[2] = prop[2] - 'A';
        }
    }
    memset(&coll, 0, sizeof(coll));
    coll.type = DYNCOLL_UNKNOWN_40;
    const S_vector &wpos = frame->GetWorldPos();
    ((I3D_bsphere &)coll.bsphere).pos = S_vector(wpos.x, wpos.y + .9f, wpos.z);
    ((I3D_bbox &)coll.bbox).min = S_vector(-.5f, 0.0f, -.5f);
    ((I3D_bsphere &)coll.bsphere).radius = scale * 1.15f;
    ((I3D_bbox &)coll.bbox).max = S_vector(.5f, 1.8f, .5f);
    coll.matrix = (S_matrix *)&frame->GetMatrix();
    inv_matrix = frame->GetMatrix();
    inv_matrix.Inverse(S_matrix(inv_matrix));
    coll.inv_matrix = &inv_matrix;
    coll.owner = act_mission->GetGame()->don_ocas;
    coll.flags = DYNCOLLF_UNKNOWN_100;
    coll.frame = frame;
    using_object.pos = S_vector(0.0f, 1.0f, 0.0f);
    using_object.radius_2 = .7f;
    using_object.matrix = &frame->GetMatrix();
    using_object.flags = 0;
    using_object.frame = frame;
    using_object.actor = act_mission->GetGame()->don_ocas;
    using_object.item.id = GAMEITEM_SPECIAL_ACTION;
    using_object.item.ammo_loaded = USING_ACTION_TALK;
    using_object.item.using_object = &using_object;
    return true;
}

// A model from the mission's buffer or made from the file: its frame is the generator's, the rest as SetModel's
// FUNCTION: GAME 0x456a40
bool C_traffic_element::CreateModel(char *name, C_traffic_generator *gen)
{
    active = false;
    generator = gen;
    frame = act_mission->FrmActBuff_GetModel(name);
    if (!frame)
    {
        frame = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        if (!frame)
            return false;
        if (model_cache.Open(frame, name, 0, NULL, NULL, NULL) != I3D_OK)
        {
            frame->Release();
            frame = NULL;
            return false;
        }
    }
    // STRING: GAME 0x632714
    frame->SetName("p_traff_elem");
    frame->SetOn(false);
    C_actor::DebustizeFrame(frame);
    C_actor::NoShadowFrame(frame);
    C_actor **p = new C_actor *;
    *p = (C_actor *)gen;
    SetFrameActor(frame, p);
    base = NULL;
    I3D_frame *b = frame->FindChildFrame("base", ENUMF_ALL);
    if (b && b->GetType() == FRAME_VISUAL &&
        (((I3D_visual *)b)->GetVisualType() == I3D_VISUAL_SINGLEMESH ||
         ((I3D_visual *)b)->GetVisualType() == I3D_VISUAL_SINGLEMORPH))
        base = (I3D_object *)b;
    neck = driver->CreatePose();
    frame->SetPose("neck", neck);
    scale = 1.0f;
    I3D_frame *back = frame->FindChildFrame("back1", ENUMF_ALL);
    if (back)
    {
        char prop[32];
        strcpy(prop, back->GetProperty());
        if (strlen(prop) == 6)
        {
            strupr(prop);
            scale = atoi(&prop[3]) * .01f;
            frame->SetScale(S_vector(scale, scale, scale));
            frame->Update();
            model_type[0] = prop[0] - 'A';
            model_type[1] = prop[1] - 'A';
            model_type[2] = prop[2] - 'A';
        }
    }
    memset(&coll, 0, sizeof(coll));
    coll.type = DYNCOLL_UNKNOWN_40;
    const S_vector &wpos = frame->GetWorldPos();
    ((I3D_bsphere &)coll.bsphere).pos = S_vector(wpos.x, wpos.y + .9f, wpos.z);
    ((I3D_bbox &)coll.bbox).min = S_vector(-.5f, 0.0f, -.5f);
    ((I3D_bsphere &)coll.bsphere).radius = scale * 1.15f;
    ((I3D_bbox &)coll.bbox).max = S_vector(.5f, 1.8f, .5f);
    coll.matrix = (S_matrix *)&frame->GetMatrix();
    inv_matrix = frame->GetMatrix();
    inv_matrix.Inverse(S_matrix(inv_matrix));
    coll.inv_matrix = &inv_matrix;
    coll.owner = act_mission->GetGame()->don_ocas;
    coll.flags = DYNCOLLF_UNKNOWN_100;
    coll.frame = frame;
    using_object.pos = S_vector(0.0f, 1.0f, 0.0f);
    using_object.radius_2 = .7f;
    using_object.matrix = &frame->GetMatrix();
    using_object.flags = 0;
    using_object.frame = frame;
    using_object.actor = act_mission->GetGame()->don_ocas;
    using_object.item.id = GAMEITEM_SPECIAL_ACTION;
    using_object.item.ammo_loaded = USING_ACTION_TALK;
    using_object.item.using_object = &using_object;
    return true;
}

// The "base" visual's alpha
// FUNCTION: GAME 0x456ea0
void C_traffic_element::SetAlpha(float alpha)
{
    if (base)
        base->SetTransparency(alpha);
}

// The model given back to the mission's buffer (a generator's) or released, the usable object taken out
// FUNCTION: GAME 0x456ec0
void C_traffic_element::DestroyModel()
{
    if (frame)
    {
        C_actor **p = GetFrameActor(frame);
        if (p)
            *p = NULL;
        SetFrameActor(frame, p);
        if (neck)
        {
            frame->SetPose("neck", NULL);
            neck->Release();
            neck = NULL;
        }
        frame->StopAnimation(0);
        frame->LinkTo(NULL, 0);
        if (generator)
            act_mission->FrmActBuff_StoreModel(frame);
        else
        {
            frame->Release();
            frame = NULL;
        }
    }
    act_mission->GetGame()->GetUsingObject().DelObject(&using_object);
}

// A free pedestrian (AddFreePerson's, C_human::GameDone's) sets out from where its model stands: b1 gives it the
// short range of GetNearNode's flags, b2 looks for the node ahead of it. False where it has no model or no node.
// FUNCTION: GAME 0x456fa0
bool C_traffic_element::ActivateFree(bool b1, bool b2)
{
    if (!frame)
        return false;
    if (b1)
        web_flags = 3;
    pos = frame->GetPos();
    dir = frame->GetDir();
    dir.y = 0.0f;
    dir.Normalize();
    C_web_path *web = act_mission->web_path;
    if (b2)
    {
        S_vector p = pos + dir * 5.0f;
        node = web->GetNearNode(p, -1.0f, web_flags);
    }
    else
        node = web->GetNearNode(pos, -1.0f, web_flags);
    if (node == -1)
        return false;
    if (web->nodes[node].num_conns < 1)
        return false;
    dog_head = NULL;
    umbrella = NULL;
    waving = false;
    prev_node = -1;
    C_web_node *n = &web->nodes[node];
    float best = 10.0f;
    int i = n->num_conns;
    while (i)
    {
        i--;
        S_vector from = n->GetPos();
        S_vector to = web->nodes[n->GetConn(i)->node].GetPos() - from;
        S_vector me = pos - n->GetPos();
        float angle = to.AngleTo(me);
        if (angle < best)
        {
            best = angle;
            prev_node = n->GetConn(i)->node;
        }
    }
    if (prev_node == -1)
        prev_node = web->nodes[node].GetConn(0)->node;
    node_width = (float)(uint)(web->nodes[node].width & 0xffff) * 0.01f;
    target = web->nodes[node].GetPos();
    active = true;
    keep_alive = 2;
    frame->SetOn(true);
    frame->SetPos(pos);
    frame->SetDir(dir, 0.0f);
    frame->Update();
    I3D_frame *f = frame;
    I3D_scene *scene = act_mission->scene;
    scene->SetFrameSectorPos(f, f->GetWorldPos());
    frame->Tick(1);
    machine = C_traffic_generator::AddElement(this);
    if (machine)
        machine->Init(this);
    ground_timer = 0;
    running = false;
    anim_next = ANIM_WALK1;
    anim = 0;
    anim_after = 0;
    anim_once = false;
    anim_after_once = true;
    num_way = -1;
    detour = 0;
    fall_speed = -1.0f;
    no_ground_count = 0;
    alpha = 1.0f;
    SetAlpha(1.0f);
    alpha_dir = 0;
    energy = (float)(rnd(20) + 60);
    dead = false;
    look_at = NULL;
    neck_reset = false;
    if (neck)
        neck->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
    wait_time = 0;
    line_walk = false;
    turning = false;
    fleeing = false;
    car = NULL;
    target_human = 0;
    invisible_time = 0;
    made_human = false;
    walk = rnd(model_type[2] == 1 || model_type[2] == 3 ? 2 : 5);
    gifCollision.DynInsert(&coll);
    in_coll = true;
    coll_wait = 0;
    hit_car = 0;
    hit_wait = false;
    if (model_type[2] != 1 && model_type[2] != 3)
    {
        voice = rnd(3);
        if (model_type[0] == 1)
            voice += 0x1d;
        else
            voice += 0x1a;
        may_greet = true;
    }
    else
    {
        voice = rnd(2) + 0xa;
        may_greet = false;
    }
    greet_timer = 0;
    greeted = false;
    act_mission->game->AddFrameForShadow(frame, SHADOW_TYPE_0, (E_shadow_type)(model_type[0] == 1 ? 3 : 2), NULL,
        NULL);
    act_mission->game->using_object.AddObject(&using_object);
    if (generator)
        generator->num_active++;
    I3D_frame *neck_frame = frame->FindChildFrame("neck", 0xffff);
    if (neck_frame)
    {
        neck_frame->SetScale(S_vector(1.0f, 1.0f, 1.0f));
        neck_frame->Update();
    }
    return true;
}

// The place of Activate's pedestrian (the name is ours): the node near its position, the way from it that its direction
// follows, a point on that way (the pedestrian's pos), and whether no pedestrian, no actor and, unless b, not the camera
// is near it. One inline function: written in Activate, VC6 inlines the vector operations in it, which the original
// leaves out of line.
static inline bool PlaceOk(C_traffic_element *e, bool b)
{
    C_web_path *web = act_mission->web_path;
    int start = web->GetNearNode(e->pos, -1.0f, e->web_flags);
    if (start == -1)
        return false;
    C_web_node *from = &web->nodes[start];
    float best = 10.0f;
    C_web_node *to = NULL;
    int i = from->num_conns;
    while (i)
    {
        i--;
        C_web_connection *conn = from->GetConn(i);
        if (e->web_flags & conn->flags)
        {
            C_web_node *other = &web->nodes[conn->node];
            float ang = e->dir.AngleTo(other->GetPos() - from->GetPos());
            if (ang < best)
            {
                to = other;
                e->node = conn->node;
                best = ang;
                e->node_width = (web->nodes[conn->node].width & 0xffff) * 0.01f;
            }
        }
    }
    if (!to)
        return false;
    e->prev_node = start;
    e->target = to->GetPos();
    float d1 = (e->pos - from->GetPos()).Magnitude();
    S_vector way_vec;
    way_vec = to->GetPos() - from->GetPos();
    float t = way_vec.Magnitude();
    if (t > d1)
        t = d1;
    t = rnd() * t;
    way_vec.Normalize();
    e->pos = from->GetPos() + way_vec * t;
    if (C_traffic_generator::GetNearElementDist2(e->pos) < 4.0f)
        return false;
    std::vector<C_actor *> &actors = act_mission->GetGame()->active_actors;
    for (int j = actors.size(); j--;)
    {
        S_vector p = actors[j]->pos;
        if ((p - e->pos).Square() < 25.0f)
            return false;
    }
    if (!b && e->generator)
    {
        float sp = act_mission->GetGame()->player_speed;
        if (sp > 30.0f)
            sp = 30.0f;
        else if (sp < 5.0f)
            sp = 0.0f;
        float r = sp * (1.0f / 30.0f);
        float kk = r * 0.3f + 1.0f;
        I3D_frame *cam = act_mission->GetScene()->GetActiveCamera();
        S_vector d = cam->GetWorldPos() - e->pos;
        d.y = 0.0f;
        if (d.Magnitude() < kk * e->generator->activate_min)
            return false;
    }
    return true;
}

// Sets a pedestrian out of the generator's AI, from a node of the web near the camera: the place along the way the
// camera looks, turned aside, its distance between the two limits (the player's speed stretches them and narrows the
// turn), the node ahead of it and the place on the way there, which no other pedestrian and no actor is near, and, if
// it is not forced, which is not in the generator's near range. The model is put there, the walk set up, its
// collision, its usable object and its shadow added; the head of a dog and an umbrella, the latter in the rain, are
// models of its own.
// FUNCTION: GAME 0x4577f0
bool C_traffic_element::Activate(bool b, float f1, float f2)
{
    web_flags = 1;
    float angle = 1.0 - cos(rnd() * 1.5707964f);
    float speed = act_mission->GetGame()->player_speed;
    if (speed > 30.0f)
        speed = 30.0f;
    else if (speed < 5.0f)
        speed = 0.0f;
    float r = speed * (1.0f / 30.0f);
    float k = r * 0.3f + 1.0f;
    f1 *= k;
    f2 *= k;
    float tt = (30.0f - speed) * (1.0f / 30.0f);
    angle = tt * angle * PI;
    angle = (rnd(2) == 0 ? 1.0f : -1.0f) * angle;
    dir = -act_mission->GetScene()->GetActiveCamera()->GetWorldDir();
    dir.y = 0.0f;
    dir.Normalize();
    S_quat q;
    q.Make(S_vector(0.0f, 1.0f, 0.0f), angle);
    dir *= q.RotationMatrix();
    walk = rnd(model_type[2] == 1 || model_type[2] == 3 ? 2 : 5);
    pos = act_mission->GetScene()->GetActiveCamera()->GetWorldPos();
    float dist_cam;
    if (b)
        dist_cam = rnd() * (f2 - 5.0f) + 5.0f;
    else
        dist_cam = rnd() * (f2 - f1) + f1;
    pos -= dir * dist_cam;
    float a = rnd() * 6.2831855f;
    dog_head = NULL;
    umbrella = NULL;
    waving = false;
    dir = S_vector((float)cos(a), 0.0f, (float)sin(a));
    if (PlaceOk(this, b))
    {
        active = true;
        keep_alive = 2;
        if (generator)
            generator->num_active++;
        frame->SetOn(true);
        frame->SetPos(pos);
        frame->SetDir(dir, 0.0f);
        frame->Update();
        I3D_scene *scene = act_mission->GetScene();
        scene->SetFrameSectorPos(frame, frame->GetWorldPos());
        frame->Tick(1);
        machine = C_traffic_generator::AddElement(this);
        if (machine)
            machine->Init(this);
        ground_timer = 0;
        running = false;
        anim_next = ANIM_WALK1;
        anim = 0;
        anim_after = 0;
        anim_once = false;
        anim_after_once = true;
        alpha = 0.0f;
        SetAlpha(0.0f);
        alpha_dir = 1;
        energy = (float)(rnd(20) + 60);
        dead = false;
        num_way = -1;
        detour = 0;
        invisible_time = 0;
        fall_speed = -1.0f;
        no_ground_count = 0;
        look_at = NULL;
        neck_reset = false;
        neck->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
        wait_time = 0;
        line_walk = false;
        turning = false;
        fleeing = false;
        car = NULL;
        target_human = 0;
        made_human = false;
        gifCollision.DynInsert(&coll);
        in_coll = true;
        coll_wait = 0;
        hit_car = 0;
        hit_wait = false;
        if (model_type[2] != 1 && model_type[2] != 3)
        {
            voice = rnd(3);
            if (model_type[0] == 1)
                voice += 0x1d;
            else
                voice += 0x1a;
            may_greet = true;
        }
        else
        {
            voice = rnd(2) + 0xa;
            may_greet = false;
        }
        greet_timer = 0;
        greeted = false;
        act_mission->GetGame()->using_object.AddObject(&using_object);
        I3D_frame *neck_frame = frame->FindChildFrame("neck", ENUMF_ALL);
        if (neck_frame)
        {
            neck_frame->SetScale(S_vector(1.0f, 1.0f, 1.0f));
            if (model_type[2] != 1 && model_type[2] != 3 && model_type[2] != 4 && rnd(5000) == 0)
            {
                dog_head = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
                if (dog_head)
                {
                    // STRING: GAME 0x632748
                    if (!model_cache.Open(dog_head, "hlavapsa.i3d", 0, NULL, NULL, NULL))
                    {
                        neck_frame->SetScale(S_vector(0.01f, 0.01f, 0.01f));
                        dog_head->LinkTo(neck_frame, 0);
                        dog_head->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
                        dog_head->SetPos(0.0f, 10.0f, 10.0f);
                        dog_head->SetScale(S_vector(100.0f, 100.0f, 100.0f));
                        dog_head->Update();
                    }
                    else
                    {
                        dog_head->Release();
                        dog_head = NULL;
                    }
                }
            }
            neck_frame->Update();
        }
        if (act_mission->GetScene()->GetWeatherSystemParam(WS_PARAM_ON) && rnd(5))
        {
            // STRING: GAME 0x632740
            I3D_frame *hand = frame->FindChildFrame("r_hand", ENUMF_ALL);
            if (hand && model_type[2] != 1 && model_type[2] != 3 && model_type[2] != 4)
            {
                umbrella = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
                if (umbrella)
                {
                    // STRING: GAME 0x632734
                    if (!model_cache.Open(umbrella, "9dest.i3d", 0, NULL, NULL, NULL))
                    {
                        umbrella->LinkTo(hand, 0);
                        S_quat rot;
                        rot.Make(S_vector(1.0f, 0.0f, 0.0f), -1.5707964f);
                        umbrella->SetRot(rot);
                        umbrella->SetPos(0.093f, -0.013f, 0.346f);
                        umbrella->Update();
                    }
                    else
                    {
                        umbrella->Release();
                        umbrella = NULL;
                    }
                    I3D_animation_set *set = NULL;
                    // STRING: GAME 0x632724
                    anim_cache.Create("destnik01.i3d", &set, NULL, NULL);
                    if (set)
                    {
                        machine->SetExtendedAnim(set, 0.9f, 0.0f, false);
                        set->Release();
                    }
                }
            }
        }
        act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0,
            (E_shadow_type)(model_type[0] == 1 ? 3 : 2), NULL, NULL);
        return true;
    }
    return false;
}

// Takes the pedestrian out of the game: the model off and its shadow, out of the active ones and the collision grid,
// the usable object and the others' look at its neck gone; a free pedestrian (no generator) deletes itself
// FUNCTION: GAME 0x4587c0
void C_traffic_element::Deactivate()
{
    active = false;
    frame->StopAnimation(0);
    frame->SetOn(false);
    if (machine)
    {
        machine->StopAnims();
        machine = NULL;
    }
    act_mission->game->DelFrameForShadow(frame, SHADOW_TYPE_0);
    C_traffic_generator::DelElement(this);
    gifCollision.DynErase(&coll);
    in_coll = false;
    act_mission->game->using_object.DelObject(&using_object);
    I3D_frame *neck_frame = frame->FindChildFrame("neck", 0xffff);
    for (int i = C_traffic_generator::array.size(); i--;)
    {
        if (C_traffic_generator::array[i]->look_at == neck_frame)
            C_traffic_generator::array[i]->look_at = NULL;
    }
    if (dog_head)
    {
        dog_head->Release();
        dog_head = NULL;
    }
    if (umbrella)
    {
        umbrella->Release();
        umbrella = NULL;
    }
    if (generator)
        generator->num_active--;
    else
    {
        DestroyModel();
        delete this;
    }
}

// A pedestrian hit by a shot, a stroke, an explosion, a fire or a car (dir the hit's direction): its health goes, a
// fall, a stroke's or a shot's reaction starts, and it screams
// FUNCTION: GAME 0x4588d0
void C_traffic_element::Hit(int type, const S_vector &dir, uint u, C_actor *act, float f)
{
    if (energy < 0.0f)
        return;
    if (model_type[2] == 4)
    {
        CreateMafianos(type, dir, u, act, f);
        return;
    }
    may_greet = false;
    bool damage = true;
    if (type == HIT_IN_CAR)
    {
        damage = false;
        if (act && act->GetType() == ACTOR_CAR && ((C_car *)act)->GetOwner(0) == act_mission->GetGame()->GetPlayer())
            damage = true;
    }
    if (damage)
        energy -= f;
    if (energy < .5f)
    {
        energy = -1.0f;
        if (in_coll)
        {
            gifCollision.DynErase(&coll);
            in_coll = false;
        }
        if (act_mission->GetGame()->score_on && act)
        {
            bool by_player = false;
            if (act->GetType() == ACTOR_CAR)
            {
                if (((C_car *)act)->GetOwner(0) == act_mission->GetGame()->GetPlayer())
                    by_player = true;
            }
            else if (act->GetType() == ACTOR_PLAYER && act == act_mission->GetGame()->GetPlayer())
                by_player = true;
            if (by_player)
            {
                if (model_type[2] == 1 || model_type[2] == 3)
                    act_mission->GetGame()->ScoreSet(act_mission->GetGame()->score);
                else if (model_type[2] == 4)
                    act_mission->GetGame()->ScoreSet(act_mission->GetGame()->score + 500);
                else
                    act_mission->GetGame()->ScoreSet(act_mission->GetGame()->score);
            }
        }
    }
    bool fall = false;
    bool stroke = false;
    switch (type)
    {
    case HIT_IN_CAR:
        if (f > 16.0f)
        {
            fall = true;
            if (act && act->GetType() == ACTOR_CAR)
                hit_car = (C_car *)act;
        }
        else if (energy > .5f)
        {
            if (machine->track != -1)
                return;
            S_vector dir2 = dir;
            float angle = this->dir.AngleTo(dir2);
            if (angle < PI / 4)
                anim_next = ANIM_USKOK_LEFT_BACK;
            else if (angle > PI * 3 / 4)
                anim_next = ANIM_USKOK_RIGHT_FRONT;
            else
            {
                S_vector2 look(this->dir.x, this->dir.z);
                S_vector2 hit(dir2.x, dir2.z);
                if (AngleTo2(look, hit) > 0.0f)
                    anim_next = ANIM_USKOK_BACK_RIGHT;
                else
                    anim_next = ANIM_USKOK_FRONT_LEFT;
            }
            S_vector2 d2(dir2.x, dir2.z);
            S_vector2 p(act->GetPos().x, act->GetPos().z);
            S_vector2 hp(pos.x, pos.z);
            if (hp.DistanceToLine(p, d2) < 0.0f)
                anim_next++;
            if (anim != anim_next)
                SetAnim();
            return;
        }
        break;
    case HIT_STROKE:
        stroke = true;
        break;
    case HIT_EXPLOSION:
        fall = true;
        break;
    case HIT_SHOT:
    {
        S_vector blood_dir = dir + S_vector(rnd(), rnd(), rnd()) * .3f;
        blood_dir.Normalize();
        S_vector blood_pos = pos;
        blood_pos.y += 1.0f;
        blood_dir *= 3.0f;
        if (act_mission->options.game_effects >= OPTION_MEDIUM)
            act_mission->GetGame()->blood_manager.AddBlood(blood_pos, blood_dir, 1.0f, 0, false);
        break;
    }
    }
    if (energy < .05f)
    {
        energy = 0.0f;
        if (act)
        {
            if (act == act_mission->GetGame()->GetPlayer())
                act_mission->GetGame()->Do_NPCKillEvent((char *)frame->GetFileName());
            else if (act->GetType() == ACTOR_CAR &&
                ((C_car *)act)->GetOwner(0) == act_mission->GetGame()->GetPlayer())
                act_mission->GetGame()->Do_NPCKillEvent((char *)frame->GetFileName());
        }
        act_mission->GetGame()->using_object.DelObject(&using_object);
    }
    anim = 0;
    anim_after = 0;
    if (fall)
        HitFall(dir, u);
    else if (stroke)
        HitStroke(dir, u);
    else
        HitGun(dir, u, false);
}

// The fall the hit throws the pedestrian into, turned to the hit (PS2 name)
inline void C_traffic_element::HitFall(const S_vector &dir, uint u)
{
    DropUmbrella();
    gifCollision.DynErase(&coll);
    in_coll = false;
    float angle = dir.AngleTo(this->dir);
    if (angle < PI / 4)
    {
        coll_wait = 2;
        this->dir = dir;
        anim_next = energy > 0.0f ? ANIM_SMRT_PAD_DOPREDU_C :
            rnd(2) ? ANIM_SMRT_PAD_DOPREDU_A : ANIM_SMRT_PAD_DOPREDU_B;
    }
    else
    {
        coll_wait = 3;
        if (angle > PI * 3 / 4)
        {
            this->dir = -dir;
            anim_next = ANIM_SMRT_PAD_DOZADU;
        }
        else
        {
            S_vector2 look(this->dir.x, this->dir.z);
            S_vector2 hit(dir.x, dir.z);
            if (AngleTo2(look, hit) > 0.0f)
                anim_next = ANIM_SMRT_PAD_VLEVO;
            else
                anim_next = ANIM_SMRT_PAD_VPRAVO;
        }
        anim_after = energy > 0.0f ? ANIM_SMRT_PAD_END_VSTAVANI :
            rnd(2) ? ANIM_SMRT_PAD_END_A : ANIM_SMRT_PAD_END_B;
        anim_after_once = true;
    }
    this->dir.y = 0.0f;
    this->dir.Normalize();
    frame->SetDir(this->dir, 0.0f);
    frame->Update();
}

// A stroke's hit (PS2 name)
inline void C_traffic_element::HitStroke(const S_vector &dir, uint u)
{
    if (energy > 0.0f)
    {
        if (u == HIT_PART_HEAD)
        {
            float angle = dir.AngleTo(this->dir);
            if (angle < PI / 4)
                anim_next = ANIM_ZASAH_SM_HLAVA_B;
            else if (angle > PI * 3 / 4)
                anim_next = ANIM_BOJ_ZASAH_HL_D;
            else
            {
                S_vector2 look(this->dir.x, this->dir.z);
                S_vector2 hit(dir.x, dir.z);
                if (AngleTo2(look, hit) > 0.0f)
                    anim_next = ANIM_BOJ_ZASAH_HL_L;
                else
                    anim_next = ANIM_BOJ_ZASAH_HL_R;
            }
        }
        else
        {
            S_vector2 look(this->dir.x, this->dir.z);
            S_vector2 hit(dir.x, dir.z);
            if (AngleTo2(look, hit) > 0.0f)
                anim_next = ANIM_BOJ_ZASAH_BR_R;
            else
                anim_next = ANIM_BOJ_ZASAH_BR_L;
        }
    }
    else
    {
        DropUmbrella();
        anim_next = C_human::GetAnimIDForDeath(u, !mega_panic, this->dir, dir);
    }
    anim_once = true;
    C_traffic_generator::NewPanic(pos);
    C_traffic_generator::ElementNewScream(voice, rnd(7) + 0x55, pos, 1.0f, 25.0f, .4f, .3f);
}

// A shot's hit (PS2 name), the hit part u (HIT_PART_*)
inline void C_traffic_element::HitGun(const S_vector &dir, uint u, bool b)
{
    if (energy > 0.0f)
    {
        float angle = dir.AngleTo(this->dir);
        switch (u)
        {
        case HIT_PART_HAND_R:
            anim_next = angle < PI / 2 ? ANIM_ZASAH_SM_RAMENO_BR : ANIM_ZASAH_SM_RAMENO_FR;
            break;
        case HIT_PART_HAND_L:
            anim_next = angle < PI / 2 ? ANIM_ZASAH_SM_RAMENO_BL : ANIM_ZASAH_SM_RAMENO_FL;
            break;
        case HIT_PART_LEG_R:
            anim_next = ANIM_ZASAH_SM_NOHA_R;
            break;
        case HIT_PART_LEG_L:
            anim_next = ANIM_ZASAH_SM_NOHA_L;
            break;
        case HIT_PART_BODY:
            anim_next = angle < PI / 2 ? ANIM_ZASAH_SM_ZADA : ANIM_ZASAH_SM_BRICHO;
            break;
        case HIT_PART_HEAD:
            anim_next = angle < PI / 2 ? ANIM_ZASAH_SM_HLAVA_B : ANIM_ZASAH_SM_HLAVA_F;
            break;
        }
    }
    else
    {
        DropUmbrella();
        anim_next = C_human::GetAnimIDForDeath(u, !mega_panic, this->dir, dir);
    }
    anim_once = true;
    C_traffic_generator::ElementNewScream(voice, rnd(7) + 0x55, pos, 1.0f, 25.0f, .4f, .3f);
}

// The animation for the state started (anim_next): the walk's file by the model and walk, or the state's own from
// _anim_info_, then the track's end and the animation's time
// FUNCTION: GAME 0x459450
void C_traffic_element::SetAnim()
{
    if (!machine || anim_next == anim)
        return;
    if (coll_wait == 1 && !dead && energy > .1f)
    {
        gifCollision.DynInsert(&coll);
        in_coll = true;
        hit_car = 0;
    }
    if (coll_wait)
        coll_wait--;
    anim_pos = pos;
    anim = anim_next;
    const char *name = NULL;
    if (anim_next == ANIM_WALK1)
    {
        if (model_type[0] == 1)
        {
            // STRING: GAME 0x6327c0
            name = "walk woman1.i3d";
        }
        else
        {
            switch (walk)
            {
            case 0:
                // STRING: GAME 0x6327a8
                name = "ChuzeRuceZaZady1.i3d";
                break;
            case 1:
                // STRING: GAME 0x632794
                name = "ChuzeVychazkova.i3d";
                break;
            case 2:
                // STRING: GAME 0x63277c
                name = "ChuzeDelnasTaska.i3d";
                break;
            case 3:
                // STRING: GAME 0x632768
                name = "ChuzeRukaVKapse.i3d";
                break;
            case 4:
                // STRING: GAME 0x632758
                name = "ChuzeSpech.i3d";
                break;
            }
        }
    }
    else
    {
        int index = GetAnimIndex(anim_next);
        if (index <= -1)
            return;
        name = _anim_info_[index].name;
    }
    machine->SetAnim(name, false, false, anim_once, 4.0f, 0.0f, true, false, NULL, true, -1);
    if (machine->track != -1)
    {
        S_vector end;
        C_animations_machine_de_luxe::GetEndPointOfTCK(name, end);
        anim_end = end;
        anim_end *= frame->GetWorldRot().RotationMatrix();
    }
    if (anim_once)
    {
        uint time, length;
        machine->GetActiveAnimTime(time, length);
        anim_time = length > 250 ? length - 250 : length;
    }
}

// The panic takes it away from the last panic's place: it flees the way the panic isn't, turning back to the node it
// came from where the one it walks to lies towards it
// FUNCTION: GAME 0x459650
void C_traffic_element::SetPanic()
{
    if (target_human == 0 && model_type[2] != 1 && model_type[2] != 3 && model_type[2] != 4)
    {
        running = true;
        panic_time = rnd(5000) + 20000;
        if (!fleeing)
        {
            fleeing = true;
            mega_panic = 0;
            C_web_path *web = act_mission->web_path;
            S_vector next = web->nodes[prev_node].GetPos();
            if ((pos - C_traffic_generator::last_panic).AngleTo(target - next) > PI / 2)
            {
                target = next;
                int node_old = node;
                node = prev_node;
                prev_node = node_old;
                node_width = (web->nodes[node].width & 0xffff) * 0.01f;
            }
        }
    }
}

// A panic at a shot or an explosion near: the pedestrian drops its umbrella and throws itself down (1: crouching, 2: on
// all fours, 3: lying), staying there in the "stativ" animation
// FUNCTION: GAME 0x4597b0
void C_traffic_element::SetMegaPanic(int i)
{
    if (dead || energy < .1f)
        return;
    DropUmbrella();
    mega_panic = i;
    switch (mega_panic)
    {
    case 1:
        anim_next = ANIM_TRAFFIC_PANIKA_DREP_DOLU;
        anim_after = ANIM_TRAFFIC_PANIKA_DREP_STATIV;
        break;
    case 2:
        anim_next = ANIM_TRAFFIC_PANIKA_NA4_DOLU;
        anim_after = ANIM_TRAFFIC_PANIKA_NA4_STATIV;
        break;
    case 3:
        anim_next = ANIM_TRAFFIC_PANIKA_LEH_DOLU;
        anim_after = ANIM_TRAFFIC_PANIKA_LEH_STATIV;
        break;
    }
    anim_after_once = false;
}

// The player talks to the pedestrian: it answers with one of the voices of a greeted one, or the first time with a
// greeting of its own
// FUNCTION: GAME 0x459850
void C_traffic_element::PlayerNaMneMluvi()
{
    if (greeted)
        C_traffic_generator::ElementNewScream(voice, rnd(6) + 0xaa, pos, 1.0f, 15.0f, .4f, .3f);
    else
    {
        C_traffic_generator::ElementNewScream(voice, rnd(7) + 0xba, pos, 1.0f, 15.0f, .4f, .3f);
        may_greet = false;
    }
}

// A pedestrian that waits to greet the player (a Karel) does it once the player, on foot in its sector and not wanted,
// comes within 5 m ahead of it: it says hello and looks at the player's neck
// FUNCTION: GAME 0x4598d0
void C_traffic_element::Karle_hoj()
{
    if (!may_greet || fleeing)
        return;
    C_human *player = act_mission->GetGame()->player;
    if (!player)
        return;
    if (player->frame->GetParent() != frame->GetParent())
        return;
    if (act_mission->GetGame()->shvestky.active && act_mission->GetGame()->shvestky.PocetNasranych(player))
        return;
    if (!AIManager.police.SemTrefenaLocklaKurva(player))
        return;
    if (player->car)
        return;
    greeted = true;
    S_vector player_pos = player->frame->GetWorldPos();
    S_vector d = player_pos - pos;
    if (d.Square() < 25.0f && d.AngleTo(dir) < 2.0f)
    {
        C_traffic_generator::ElementNewScream(voice, rnd(6) + 0xaa, pos, 1.0f, 15.0f, .4f, .3f);
        look_at = player->frame->FindChildFrame("neck", ENUMF_ALL);
    }
}

// The umbrella dropped: it goes into the scene as a physical thing and the animation of holding it ends
// FUNCTION: GAME 0x459a50
void C_traffic_element::DropUmbrella()
{
    if (umbrella)
    {
        umbrella->LinkTo(act_mission->GetScene()->GetPrimarySector(), 1);
        act_mission->GetScene()->SetFrameSector(umbrella);
        C_public_physics *phys = new C_public_physics;
        if (phys->Init(umbrella))
        {
            S_THING_BOX box;
            box.weight = 5.0f;
            box.friction = .45f;
            box.velocity = S_vector(0.0f, -.1f, 0.0f);
            phys->InitBox(&box);
            act_mission->GetGame()->AddTemporaryActor(phys);
        }
        else
            phys->Release();
        machine->DelExtendedAnim(0.0f);
        umbrella->Release();
        umbrella = NULL;
    }
}

// The pedestrian hit becomes a mafioso: an entity with a Thompson takes the model's place and the hit goes on to it
// FUNCTION: GAME 0x459b90
void C_traffic_element::CreateMafianos(int type, const S_vector &hit_pos, uint u, C_actor *act, float f)
{
    I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
    SetAlpha(1.0f);
    act_mission->GetGame()->DelFrameForShadow(frame, SHADOW_TYPE_0);
    model->Duplicate(frame);
    act_mission->GetGame()->AddFrameForShadow(frame, SHADOW_TYPE_0, (E_shadow_type)(model_type[0] != 1 ? 2 : 3), NULL,
        NULL);
    model->SetPos(pos);
    model->SetDir(dir, 0.0f);
    model->Update();
    model->LinkTo(frame->GetParent(), 0);
    C_entity *entity = new C_entity;
    entity->voice_type = rnd(5) + 12;
    entity->max_properties[HUMAN_PROPERTY_SPEED] = 1.1f;
    entity->max_properties[HUMAN_PROPERTY_ENERGY] = 200.0f;
    if (entity->Init(model))
    {
        entity->human_type = HUMAN_TYPE_UNKNOWN_10;
        entity->max_properties[HUMAN_PROPERTY_15] = 1.0f;
        entity->max_properties[HUMAN_PROPERTY_AGGRESIVITY] = 1.0f;
        act_mission->GetGame()->AddTemporaryActor(entity);
        S_GameItem item;
        item.id = GAMEITEM_THOMPSON;
        item.ammo_loaded = 50;
        item.ammo_spare = 300;
        item.using_object = NULL;
        entity->inventory.Insert(&item, false);
        entity->ChangeWeaponModel();
        entity->score_bonus = true;
        if (generator)
        {
            if (!entity->traffic)
                entity->traffic = new S_human_traffic;
            memset(entity->traffic, 0, sizeof(S_human_traffic));
            entity->traffic->traffic_frame = true;
            entity->traffic->element = this;
        }
        made_human = true;
        Deactivate();
        act_mission->GetGame()->shvestky.AddDoRiciActor(entity);
        S_vector p = model->GetWorldPos();
        p.y += 1.0f;
        if (type != HIT_IN_CAR)
            entity->Hit(type, hit_pos, p, -hit_pos, f, act, HIT_PART_BODY, NULL);
    }
    else
    {
        entity->Release();
        model->Release();
    }
}
