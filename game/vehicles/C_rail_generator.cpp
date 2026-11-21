// The rail vehicles' generator (PS2 C_rail_generator.cpp), file 87 of docs/game-files.md: 0x596420-0x5983dc, in PS2's
// order: the statics' initializers, then the generator. GameInit has PS2's InitTracks and CreateTracks inlined, GameDone
// DoneTracks; PS2's GetFreeMetro, GetFreeRailHuman, ISFree, ChangeSceneInit/Done and Get*TracksCount have no PC copy.
// Open: GameInit's registers and frame slots and its first push_back's budget, AI's S_vector constructors (the budget).

#include "vehicles/C_rail_generator.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "actors/C_animations.h"
#include "ai/C_person_traffic.h"
#include "ai/pathweb.h"
#include "ai/road_web.h"
#include "C_chunk.h"
#include "I3D_cache.h"
#include "mission/game.h"
#include "mission/Mission.h"
#include "vehicles/C_railway.h"

// GLOBAL: GAME 0x6478ec
std::vector<S_rail_human *> C_rail_generator::actor_persons;

// GLOBAL: GAME 0x6478d8
std::vector<S_rail_track *> C_rail_generator::rail_tracks;

// GLOBAL: GAME 0x6478c8
std::vector<C_railway *> C_rail_generator::actor_tram;

// GLOBAL: GAME 0x6478b8
std::vector<C_railway *> C_rail_generator::actor_metro;

// GLOBAL: GAME 0x636ed4
int C_rail_generator::max_person_count = 10;

// FUNCTION: GAME 0x596620
C_rail_generator::C_rail_generator()
{
    unknown_7c = 1;
    num_vehicles = 1;
    ai_period = 5000;
    type = ACTOR_RAIL_GENERATOR;
    range = 130.0f;
    max_person_count = 10;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x596680
C_rail_generator::~C_rail_generator()
{
}

// FUNCTION: GAME 0x596690
bool C_rail_generator::Init(I3D_frame *frm)
{
    frame = frm;
    return true;
}

// A version byte, then num_vehicles and ai_period
// FUNCTION: GAME 0x5966a0
bool C_rail_generator::SaveData(C_chunk *chunk)
{
    uchar version = 0;
    chunk->Write(&version, sizeof(version));
    chunk->Write(&num_vehicles, sizeof(num_vehicles));
    chunk->Write(&ai_period, sizeof(ai_period));
    return true;
}

// FUNCTION: GAME 0x596710
bool C_rail_generator::LoadData(C_chunk *chunk)
{
    uchar version;
    chunk->Read(&version, sizeof(version));
    chunk->Read(&num_vehicles, sizeof(num_vehicles));
    chunk->Read(&ai_period, sizeof(ai_period));
    return true;
}

// The web's nodes (PS2's GetCheckpoints takes a bool more, for the second web)
inline C_web_path *C_rail_generator::GetCheckpoints()
{
    return act_mission->web_path;
}

// Creates the trams and the elevated railway's trains, the tracks they run on (the closed ways of the web's rail
// nodes), the semaphores by the road crossings and the passengers
// FUNCTION: GAME 0x596860
void C_rail_generator::GameInit()
{
    uint i, m;
    for (i = 0; i < num_vehicles; i++)
    {
        // STRING: GAME 0x636eec
        I3D_model *model = act_mission->FrmActBuff_GetModel("salina.i3d");
        if (!model)
        {
            model = (I3D_model *)::driver->CreateFrame(FRAME_MODEL);
            if (model_cache.Open(model, "salina.i3d", 0, NULL, NULL, NULL) != I3D_OK)
            {
                model->Release();
                model = NULL;
            }
        }
        if (model)
        {
            char name[24];
            // STRING: GAME 0x636ee0
            sprintf(name, "Salina%u", i);
            model->SetName(name);
            model->SetOn(false);
            C_railway *tram = new C_railway;
            if (tram->Init(model))
            {
                tram->num_wagons = 0;
                tram->GameInit();
                actor_tram.push_back(tram);
            }
            else
            {
                model->Release();
                delete tram;
            }
        }
    }
    for (m = 0; m < num_vehicles; m++)
    {
        I3D_model *model = act_mission->FrmActBuff_GetModel("metro.i3d");
        if (!model)
        {
            model = (I3D_model *)::driver->CreateFrame(FRAME_MODEL);
            if (model_cache.Open(model, "metro.i3d", 0, NULL, NULL, NULL) != I3D_OK)
            {
                model->Release();
                model = NULL;
            }
        }
        if (model)
        {
            char name[24];
            // STRING: GAME 0x636ed8
            sprintf(name, "metro%u", m);
            model->SetName(name);
            model->SetOn(false);
            C_railway *train = new C_railway;
            if (train->Init(model))
            {
                train->num_wagons = 1;
                train->GameInit();
                actor_tram.push_back(train);
            }
            else
            {
                model->Release();
                delete train;
            }
        }
    }
    // The tracks: the rail's nodes (WEBF_UNKNOWN_8) joined into closed ways
    std::vector<C_web_node *> nodes;
    for (i = 0; i < act_mission->web_path->num_nodes; i++)
    {
        C_web_node *node = &GetCheckpoints()->nodes[i];
        if (node->flags & WEBF_UNKNOWN_8)
            nodes.push_back(node);
    }
    while (nodes.size())
    {
        S_rail_track *track = new S_rail_track;
        C_web_node *start = nodes[0];
        if (start->id == 999)
            track->metro = true;
        track->nodes.push_back(start);
        nodes.erase(nodes.begin());
        C_web_node *cur = NULL;
        int j;
        for (j = 0; j < start->num_conns; j++)
        {
            C_web_connection *conn = start->GetConn(j);
            if (!(conn->flags & WEBF_ONEWAY))
                cur = &GetCheckpoints()->nodes[conn->node];
        }
        while (cur != start && cur)
        {
            if (cur->id == 999)
                track->metro = true;
            track->nodes.push_back(cur);
            if (cur->flags == WEBF_UNKNOWN_8)
            {
                for (uint k = 0; k < nodes.size(); k++)
                {
                    if (nodes[k] == cur)
                    {
                        nodes.erase(nodes.begin() + k);
                        break;
                    }
                }
            }
            for (j = 0; j < cur->num_conns; j++)
            {
                C_web_connection *conn = cur->GetConn(j);
                if (!(conn->flags & WEBF_ONEWAY))
                    cur = &GetCheckpoints()->nodes[conn->node];
            }
        }
        if (cur)
            rail_tracks.push_back(track);
        else
            delete track;
    }
    // The semaphores: the nodes of a tram's track nearest the road crossings with a light
    for (uint t = 0; t < rail_tracks.size(); t++)
    {
        if (rail_tracks[t]->metro)
            continue;
        for (int c = 0; c < (int)act_mission->GetRoads()->crossings.size(); c++)
        {
            S_road_crossing *crossing = &act_mission->GetRoads()->crossings[(ushort)c];
            if (!crossing->semaphore)
                continue;
            for (int k = 0; k < 4; k++)
            {
                ushort cn = crossing->conns[k];
                if (cn == 0xffff)
                    continue;
                if (!rail_tracks[t]->nodes.size())
                    continue;
                S_road_node *rn = &act_mission->GetRoads()->nodes[cn & NEARNODE_INDEX];
                C_web_node *best = rail_tracks[t]->nodes[0];
                float best_len = (rn->pos - best->GetPos()).Square();
                for (uint n = 0; n < rail_tracks[t]->nodes.size(); n++)
                {
                    C_web_node *cur = rail_tracks[t]->nodes[n];
                    float len = (rn->pos - cur->GetPos()).Square();
                    if (best_len > len)
                    {
                        best_len = len;
                        best = cur;
                    }
                }
                float angle = (crossing->pos - rn->pos).AngleTo(best->GetPos() - GetPrevTrackNode(best)->GetPos());
                if (best_len < 6.0f && angle > -.5f && angle < .5f)
                {
                    best->flags |= WEBF_UNKNOWN_800;
                    S_rail_semaphore *sema = new S_rail_semaphore;
                    sema->crossing = crossing;
                    sema->node = best;
                    C_railway::semaphores.push_back(sema);
                }
            }
        }
    }
    // The passengers, from the pedestrians' generator's models
    for (i = 0; i < act_mission->GetActors().size(); i++)
    {
        C_traffic_generator *gen = (C_traffic_generator *)act_mission->GetActors()[i];
        if (gen->type != ACTOR_TRAFFIC_GENERATOR)
            continue;
        gen->no_range = true;
        for (uint p = 0; p < max_person_count; p++)
        {
            const char *model_name = gen->models[rnd(gen->num_models)];
            I3D_model *mdl = act_mission->FrmActBuff_GetModel(model_name);
            if (!mdl)
            {
                mdl = (I3D_model *)::driver->CreateFrame(FRAME_MODEL);
                if (model_cache.Open(mdl, model_name, 0, NULL, NULL, NULL) != I3D_OK)
                {
                    mdl->Release();
                    mdl = NULL;
                }
            }
            if (!mdl)
                continue;
            I3D_frame *back = mdl->FindChildFrame("back1", ENUMF_ALL);
            if (back)
            {
                char prop[64];
                strcpy(prop, back->GetProperty());
                if (strlen(prop) > 3)
                {
                    strupr(prop);
                    if (prop[6] == 'B' || prop[6] == 'D' || prop[6] == 'E')
                    {
                        mdl->Release();
                        continue;
                    }
                }
            }
            S_rail_human *person = new S_rail_human;
            person->unknown_8c |= 2;
            person->unknown_8c |= 1;
            person->frame_model = mdl;
            person->should_keep_sitting = rnd(3);
            I3D_frame *target_e = person->frame_model->FindChildFrame("targetE", ENUMF_TARGET);
            I3D_frame *target_n = person->frame_model->FindChildFrame("targetN", ENUMF_TARGET);
            if (target_e)
                target_e->SetOn(false);
            if (target_n)
                target_n->SetOn(false);
            person->anim_container = new C_animations_machine_de_luxe;
            person->animation_number = -1;
            if (person->IsWoman())
                person->new_animation_number = RANIM_ZEN_SALSEZ + rnd(2);
            else
                person->new_animation_number = RANIM_SED;
            person->anim_container->Init((I3D_model *)person->frame_model);
            person->SetAnim(500);
            actor_persons.push_back(person);
        }
        break;
    }
}

// Deletes the tracks, the trams, the trains and the passengers
// FUNCTION: GAME 0x597a20
void C_rail_generator::GameDone()
{
    uint i;
    for (i = 0; i < rail_tracks.size(); i++)
        delete rail_tracks[i];
    rail_tracks.clear();
    for (i = 0; i < actor_tram.size(); i++)
    {
        actor_tram[i]->GameDone();
        actor_tram[i]->frame->LinkTo(NULL, 0);
        act_mission->FrmActBuff_StoreModel((I3D_model *)actor_tram[i]->frame);
        delete actor_tram[i];
    }
    actor_tram.clear();
    for (i = 0; i < actor_metro.size(); i++)
    {
        actor_metro[i]->GameDone();
        act_mission->FrmActBuff_StoreModel((I3D_model *)actor_metro[i]->frame);
        delete actor_metro[i];
    }
    actor_metro.clear();
    for (i = 0; i < actor_persons.size(); i++)
    {
        if (actor_persons[i]->frame_model)
        {
            actor_persons[i]->frame_model->LinkTo(NULL, 0);
            act_mission->FrmActBuff_StoreModel((I3D_model *)actor_persons[i]->frame_model);
        }
        actor_persons[i]->Done();
        delete actor_persons[i];
    }
    actor_persons.clear();
}

// Every ai_period ms gives a track without a vehicle that is near the camera a free tram, set where the camera does
// not see it
// FUNCTION: GAME 0x597bf0
void C_rail_generator::AI(uint time)
{
    ai_timer += time;
    if (ai_timer > ai_period && act_mission->scene->GetActiveCamera())
    {
        ai_timer -= ai_period;
        C_web_node *node;
        int track = GetNearFreeTrackIDandNodeInRange(&node);
        if (track != -1)
        {
            C_web_node *next = GetNextTrackNode(node);
            S_vector from = node->GetPos();
            S_vector to = next->GetPos();
            S_vector dir = to - from;
            S_vector cam_dir = act_mission->scene->GetActiveCamera()->GetDir();
            bool forward = rnd() < 0.5f ? (cam_dir.Dot(dir) > 0.0f ? false : true) : (cam_dir.Dot(dir) > 0.0f ? true : false);
            if (GetNodeOutOfRange(forward, node))
            {
                C_railway *tram = GetFreeTram();
                if (tram)
                {
                    rail_tracks[track]->time = 0;
                    rail_tracks[track]->vehicle = tram;
                }
            }
        }
    }
    for (uint i = 0; i < rail_tracks.size(); i++)
    {
        S_rail_track *track = rail_tracks[i];
        if (track->vehicle)
        {
            if (track->vehicle->visibility == 3 && track->time > ai_period)
            {
                std::vector<C_actor *> &actors = act_mission->game->active_actors;
                for (uint j = 0; j < actors.size(); j++)
                {
                    if (actors[j] == track->vehicle)
                    {
                        act_mission->game->InvalidateActor(actors[j], false);
                        actors.erase(actors.begin() + j);
                        rail_tracks[i]->vehicle = NULL;
                        rail_tracks[i]->time = 0;
                        break;
                    }
                }
            }
            rail_tracks[i]->time += time;
        }
    }
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_rail_generator::Update(uint time)
{
}

// The track without a vehicle that has a node nearest the camera but within the range, and the node
// FUNCTION: GAME 0x597ff0
int C_rail_generator::GetNearFreeTrackIDandNodeInRange(C_web_node **node)
{
    float best = -1.0f;
    int best_track = -1;
    *node = NULL;
    if (act_mission->scene->GetActiveCamera())
    {
        for (uint i = 0; i < rail_tracks.size(); i++)
        {
            S_rail_track *track = rail_tracks[i];
            if (track->vehicle)
                continue;
            for (uint j = 0; j < track->nodes.size(); j++)
            {
                C_web_node *n = track->nodes[j];
                S_vector pos = n->GetPos();
                float dist = (act_mission->scene->GetActiveCamera()->GetWorldPos() - pos).Square();
                if (dist < range * range && (best == -1.0f || dist < best))
                {
                    best = dist;
                    best_track = i;
                    *node = track->nodes[j];
                }
            }
        }
    }
    return best_track;
}

// The last node the track leaves for through a connection that goes that way (a connection of WEBF_ONEWAY goes the
// other), NULL for none
// FUNCTION: GAME 0x598150
C_web_node *C_rail_generator::GetNextTrackNode(C_web_node *node)
{
    C_web_node *next = NULL;
    for (int i = 0; i < node->num_conns; i++)
    {
        C_web_connection *conn = node->GetConn(i);
        if (!(conn->flags & WEBF_ONEWAY))
            next = &act_mission->web_path->nodes[conn->node];
    }
    return next;
}

// The last node a connection of WEBF_ONEWAY leads to, NULL for none
// FUNCTION: GAME 0x5981c0
C_web_node *C_rail_generator::GetPrevTrackNode(C_web_node *node)
{
    C_web_node *prev = NULL;
    for (int i = 0; i < node->num_conns; i++)
    {
        C_web_connection *conn = node->GetConn(i);
        if (conn->flags & WEBF_ONEWAY)
            prev = &act_mission->web_path->nodes[conn->node];
    }
    return prev;
}

// The first node along the track from the node that is farther from the camera than the range, NULL for none
// FUNCTION: GAME 0x598230
C_web_node *C_rail_generator::GetNodeOutOfRange(bool forward, C_web_node *node)
{
    if (act_mission->scene->GetActiveCamera())
    {
        S_vector cam = act_mission->scene->GetActiveCamera()->GetPos();
        C_web_node *cur = forward ? GetNextTrackNode(node) : GetPrevTrackNode(node);
        while (cur != node)
        {
            float range_2 = range * range;
            S_vector d = cam - cur->GetPos();
            if (d.Square() > range_2)
                return cur;
            cur = forward ? GetNextTrackNode(cur) : GetPrevTrackNode(cur);
        }
    }
    return NULL;
}

// A tram no track has
// FUNCTION: GAME 0x598320
C_railway *C_rail_generator::GetFreeTram()
{
    for (uint i = 0; i < actor_tram.size(); i++)
    {
        bool free = true;
        for (uint j = 0; j < rail_tracks.size(); j++)
        {
            if (rail_tracks[j]->vehicle == actor_tram[i])
                free = false;
        }
        if (free)
            return actor_tram[i];
    }
    return NULL;
}
