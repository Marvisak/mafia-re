// The game's tools (PS2 tools.cpp; the PC's file is the one at 0x55cfa0-0x56040c, docs/game-files.md): the logo videos
// (PC only), the compass, the cinema's bars, the fires, the objects the player can use, the sound table and the sounds
// the humans hear, after PS2's direction tables.

#include "tools.h"
#include "Winstuff.h"
#include "I3D_cache.h"
#include "g_material.h"
#include "actors/C_actor.h"
#include "actors/C_drop_in_weapon.h"
#include "ai/C_person_traffic.h"
#include "ai/jp_ai_entity.h"
#include "collision/g_colls.h"
#include "mission/C_effects.h"
#include "mission/Mission.h"
#include "mission/game.h"

#include <string.h>

#include "IGraph/IGraph.h"
#include "rw_data.h"

// The horizontal plane's eight directions, unit and not, and sixteen (PS2 names). PS2's every file has them, a
// header's statics; the PC has this file's only, and nothing reads them. Their dynamic initializers come first.
// Open: the original's .bss has Dir8Table, Dir16Table, a vector that file 71's initializer clears (0x646ee0),
// Dir8TableN, sound_table and sound_group. The first four are in their buckets' order (tools/bsshash.py: 72, 258, the
// vector where this file's ZeroSVector of I3D/I3d_math.h goes in ours, 275, and 873), but sound_table (788) and
// sound_group (169) follow them; ours has all six in bucket order.
// GLOBAL: GAME 0x646ef0
static S_vector Dir8TableN[8] = {
    S_vector(0.0f, 0.0f, 1.0f),
    S_vector(.70710678f, 0.0f, .70710678f),
    S_vector(1.0f, 0.0f, 0.0f),
    S_vector(.70710678f, 0.0f, -.70710678f),
    S_vector(0.0f, 0.0f, -1.0f),
    S_vector(-.70710678f, 0.0f, -.70710678f),
    S_vector(-1.0f, 0.0f, 0.0f),
    S_vector(-.70710678f, 0.0f, .70710678f),
};

// GLOBAL: GAME 0x646dc0
static S_vector Dir8Table[8] = {
    S_vector(0.0f, 0.0f, 1.0f),
    S_vector(1.0f, 0.0f, 1.0f),
    S_vector(1.0f, 0.0f, 0.0f),
    S_vector(1.0f, 0.0f, -1.0f),
    S_vector(0.0f, 0.0f, -1.0f),
    S_vector(-1.0f, 0.0f, -1.0f),
    S_vector(-1.0f, 0.0f, 0.0f),
    S_vector(-1.0f, 0.0f, 1.0f),
};

// GLOBAL: GAME 0x646e20
static S_vector Dir16Table[16] = {
    S_vector(0.0f, 0.0f, 1.0f),
    S_vector(.5f, 0.0f, 1.0f),
    S_vector(1.0f, 0.0f, 1.0f),
    S_vector(1.0f, 0.0f, .5f),
    S_vector(1.0f, 0.0f, 0.0f),
    S_vector(1.0f, 0.0f, -.5f),
    S_vector(1.0f, 0.0f, -1.0f),
    S_vector(.5f, 0.0f, -1.0f),
    S_vector(0.0f, 0.0f, -1.0f),
    S_vector(-.5f, 0.0f, -1.0f),
    S_vector(-1.0f, 0.0f, -1.0f),
    S_vector(-1.0f, 0.0f, -.5f),
    S_vector(-1.0f, 0.0f, 0.0f),
    S_vector(-1.0f, 0.0f, .5f),
    S_vector(-1.0f, 0.0f, 1.0f),
    S_vector(-.5f, 0.0f, 1.0f),
};

// GLOBAL: GAME 0x646f6c
std::vector<S_sound_group> sound_group;

// GLOBAL: GAME 0x646f50
C_sound_table sound_table;

// The AVI file first, the Bink file if there is no AVI file or it doesn't open. The video keeps its aspect ratio at the
// screen's width (a band above and below), and on a screen more than twice or three times as wide as high it takes the
// left half or the middle third: two or three monitors side by side. The original multiplies by 6 and 9 and divides
// by 3 (lea, then the reciprocal multiply), which VC6 doesn't fold, so the source has * 6 / 3 and * 9 / 3.
// FUNCTION: GAME 0x55d460
bool PlayLogo(const char *name, float sx, float sy)
{
    char file_name[256];
    char path[256];
    strcpy(file_name, name);
    if (file_name[strlen(file_name) - 1] != '.')
        // STRING: GAME 0x636628
        strcat(file_name, ".");
    // STRING: GAME 0x636624
    strcat(file_name, "avi");
    bool avi = true;
    if (!FindGameFile(file_name, path))
    {
        avi = false;
        strcpy(file_name, name);
        if (file_name[strlen(file_name) - 1] != '.')
            strcat(file_name, ".");
        // STRING: GAME 0x636620
        strcat(file_name, "bik");
        if (!FindGameFile(file_name, path))
            return false;
    }
    IShow *show = igraph->CreateIShow();
    if (show->Open(path, 0) != I3D_OK)
    {
        if (!avi)
        {
            show->Release();
            return false;
        }
        strcpy(file_name, name);
        if (file_name[strlen(file_name) - 1] != '.')
            strcat(file_name, ".");
        strcat(file_name, "bik");
        if (!FindGameFile(file_name, path))
        {
            show->Release();
            return false;
        }
        if (show->Open(path, 0) != I3D_OK)
        {
            show->Release();
            return false;
        }
    }
    int band = (int)(igraph->Scrn_sy() * (1.0f - sy / sx * (4.0f / 3.0f)));
    RECT rc;
    rc.left = 0;
    rc.right = igraph->Scrn_sx();
    rc.top = band / 2;
    rc.bottom = igraph->Scrn_sy() - band;
    if (igraph->Scrn_sx() > igraph->Scrn_sy() * 6 / 3)
    {
        rc.left = 0;
        rc.right = igraph->Scrn_sx() / 2;
    }
    if (igraph->Scrn_sx() > igraph->Scrn_sy() * 9 / 3)
    {
        rc.left = igraph->Scrn_sx() / 3;
        rc.right = igraph->Scrn_sx() / 3;
    }
    show->SetRect(&rc);
    show->Play();
    show->RenderVideoUntilKeyPressOrEnd();
    show->Release();
    return true;
}

// FUNCTION: GAME 0x55d770
void C_compass::Activate(I3D_frame *frm)
{
    unknown_c = igraph->Scrn_sx() / 25;
    unknown_10 = igraph->Scrn_sy() / 40;
    unknown_4 = igraph->Scrn_sx() / 15;
    unknown_8 = igraph->Scrn_sy() / 20;
    frame = frm;
}

// PS2's (which divides by 1000): the parentheses keep the time first, where VC6 would start with the direction
// FUNCTION: GAME 0x55d810
void C_silver_screen::Tick(uint time)
{
    if (!moving)
        return;
    amount += ((time * .001f) * speed) * direction;
    if (amount > 1.0f)
    {
        amount = 1.0f;
        moving = false;
    }
    else if (amount < 0.0f)
    {
        amount = 0.0f;
        active = false;
        moving = false;
    }
}

// FUNCTION: GAME 0x55d880
C_fire::C_fire()
{
}

// The sound turned off and taken out of the scene; the particle is Tick's
// FUNCTION: GAME 0x55d8a0
C_fire::~C_fire()
{
    if (sound)
    {
        sound->SetOn(false, true);
        sound->LinkTo(NULL, 0);
        sound->Release();
    }
}

// The particle along z; the sound at the fire's world position, looped
// FUNCTION: GAME 0x55d8d0
void C_fire::Init(S_vector &at, I3D_frame *frm, int time, float range, float dmg, bool hurt)
{
    pos = at;
    frame = frm;
    life = time;
    range_2 = range * range;
    damage = dmg;
    timer.Reset(rnd(100) + 450);
    hurts = hurt;
    particle = act_mission->GetEffects()->CreateParticle(PARTICLE_FIRE, pos, S_vector(0.0f, 0.0f, 1.0f), 0, frame);
    if (time >= 2000)
    {
        sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
        if (sound)
        {
            if (sound_table.ConstructSound(sound, SOUND_FIRE_BURNING))
            {
                S_vector p = pos;
                if (frame)
                    p *= frame->GetMatrix();
                sound->SetPos(p);
                act_mission->scene->SetFrameSector(sound);
                sound->SetLoop(true);
                sound->Update();
                sound->SetOn(true);
            }
            else
            {
                sound->Release();
                sound = NULL;
            }
        }
    }
    else
        sound = NULL;
}

// The sound fades out over the last 2 s. With hurts, every period of the timer, the active and the inactive actors
// within range (but the frame's) and the frame's actor are hit by HIT_FIRE, as are the pedestrians, by damage times the
// seconds left. On a frame the fire's own position is transformed by its matrix each time (PS2's the same), but the
// hits take the position from before. The two returns that end it early push all the registers at the start, as the
// original does (one if around the rest pushes ebx after the sound's part).
// FUNCTION: GAME 0x55da80
bool C_fire::Tick(uint time)
{
    life -= time;
    if (life < 0)
    {
        if (sound)
        {
            sound->SetOn(false);
            sound->LinkTo(NULL, 0);
            sound->Release();
            sound = NULL;
        }
        if (particle > -1)
            act_mission->effects->DestroyParticle(particle);
        return false;
    }
    if (sound && life < 2000)
    {
        float vol = life * .0005f;
        if (vol < 0.0f)
            vol = 0.0f;
        sound->SetVolume(vol);
    }
    if (!hurts)
        return true;
    if (!timer.Tick(time))
        return true;
    S_vector p = pos;
    C_actor *act = NULL;
    if (frame)
    {
        pos *= frame->GetMatrix();
        if (GetFrameActor(frame))
            act = *GetFrameActor(frame);
    }
    float dmg = (life * .001f) * damage;
    std::vector<C_actor *> &active = act_mission->game->active_actors;
    for (int i = active.size(); i--;)
    {
        if (act == active[i])
            continue;
        if ((active[i]->frame->GetWorldPos() - p).Square() < range_2)
            active[i]->Hit(HIT_FIRE, p, p, p, dmg, NULL, 0, NULL);
    }
    std::vector<C_actor *> &inactive = act_mission->game->inactive_actors;
    for (int j = inactive.size(); j--;)
    {
        if (act == inactive[j])
            continue;
        if ((inactive[j]->frame->GetWorldPos() - p).Square() < range_2)
            inactive[j]->Hit(HIT_FIRE, p, p, p, dmg, NULL, 0, NULL);
    }
    if (act)
    {
        S_vector act_pos = act->GetWorldPos();
        act->Hit(HIT_FIRE, act_pos, act_pos, act_pos, dmg, NULL, 0, NULL);
    }
    C_traffic_generator::Fire(p, range_2, dmg);
    return true;
}

// FUNCTION: GAME 0x55dd60
void C_using_object::AddObject(S_using_object *obj)
{
    for (int i = objects.size(); i--;)
    {
        if (objects[i] == obj)
            return;
    }
    objects.push_back(obj);
}

// Looked for from the last object, its place taken by the last one. PS2's also releases a member at its 0x2c.
// FUNCTION: GAME 0x55df10
void C_using_object::DelObject(S_using_object *obj)
{
    if (!obj)
        return;
    for (int i = objects.size(); i--;)
    {
        if (objects[i] == obj)
        {
            if (objects[i]->frame && objects[i]->flags & USINGF_FRAME)
            {
                act_mission->game->InvalidateFrame(objects[i]->frame);
                objects[i]->frame->LinkTo(NULL, 0);
                objects[i]->frame->Release();
            }
            if (objects[i]->flags & USINGF_DELETE)
                delete objects[i];
            objects[i] = objects.back();
            objects.pop_back();
            return;
        }
    }
}

// The objects, from the last: each frame the game forgets, unlinks and releases, each object allocated here deleted
// (PS2 name)
// FUNCTION: GAME 0x55dfe0
void C_using_object::ClearObjects()
{
    for (int i = objects.size(); i--;)
    {
        if (objects[i]->flags & USINGF_FRAME && objects[i]->frame)
        {
            act_mission->game->InvalidateFrame(objects[i]->frame);
            objects[i]->frame->LinkTo(NULL, 0);
            objects[i]->frame->Release();
        }
        if (objects[i]->flags & USINGF_DELETE)
            delete objects[i];
    }
    objects.clear();
}

// FUNCTION: GAME 0x55e080
bool C_using_object::IsAnyObjectNear(const S_vector &pos, const S_vector &dir, I3D_frame *sector)
{
    for (int i = objects.size(); i--;)
    {
        if (IsInvalidItem(objects[i]->item.ammo_loaded))
            continue;
        if (objects[i]->sector && objects[i]->sector != sector)
            continue;
        S_vector d = objects[i]->pos;
        if (objects[i]->matrix)
            d *= *objects[i]->matrix;
        d -= pos;
        if (d.y * d.y < 2.25f && d.x * d.x + d.z * d.z <= objects[i]->radius_2)
            return true;
    }
    return false;
}

// Found as IsAnyObjectNear's, then sorted by their distances' squares, the nearest first
// FUNCTION: GAME 0x55e180
void C_using_object::FindNearObjects(const S_vector &pos, const S_vector &dir, std::vector<S_GameItem> &items,
    I3D_frame *sector)
{
    std::vector<float> dists;
    items.clear();
    for (int i = objects.size(); i--;)
    {
        if (IsInvalidItem(objects[i]->item.ammo_loaded))
            continue;
        if (objects[i]->sector && objects[i]->sector != sector)
            continue;
        S_vector d = objects[i]->pos;
        if (objects[i]->matrix)
            d *= *objects[i]->matrix;
        d -= pos;
        if (d.y * d.y < 2.25f)
        {
            float dist = d.x * d.x + d.z * d.z;
            if (dist <= objects[i]->radius_2)
            {
                items.push_back(objects[i]->item);
                dists.push_back(dist);
            }
        }
    }
    int num = items.size();
    if (num > 0)
    {
        for (int j = 0; j < num - 1; j++)
        {
            int best = j;
            for (int k = j + 1; k < num; k++)
            {
                if (dists[k] < dists[best])
                    best = k;
            }
            float f = dists[j];
            dists[j] = dists[best];
            dists[best] = f;
            S_GameItem item = items[j];
            items[j] = items[best];
            items[best] = item;
        }
    }
}

// FUNCTION: GAME 0x55e3e0
void C_using_object::ClearInvalidItems()
{
    invalid_items.clear();
}

// FUNCTION: GAME 0x55e410
void C_using_object::AddInvalidItem(uint id)
{
    for (int i = invalid_items.size(); i--;)
    {
        if (invalid_items[i] == id)
            return;
    }
    invalid_items.push_back(id);
}

// Every one of the id, from the last, its place taken by the last one
// FUNCTION: GAME 0x55e5c0
void C_using_object::DelInvalidItem(uint id)
{
    for (int i = invalid_items.size(); i--;)
    {
        if (invalid_items[i] == id)
        {
            invalid_items[i] = invalid_items.back();
            invalid_items.pop_back();
        }
    }
}

// FUNCTION: GAME 0x55e620
bool C_using_object::IsInvalidItem(uint id)
{
    for (int i = invalid_items.size(); i--;)
    {
        if (invalid_items[i] == id)
            return true;
    }
    return false;
}

// A count and a record for each put down weapon: the objects that free their frame and themselves, but the script's,
// and the dropped weapons (C_drop_in_weapon) with a frame
// FUNCTION: GAME 0x55e660
int C_using_object::SaveGameGetSize()
{
    int size = sizeof(int);
    for (int i = objects.size(); i--;)
    {
        if (!objects[i]->scripted && objects[i]->item.id > GAMEITEM_SPECIAL_ACTION &&
            objects[i]->flags == (USINGF_DELETE | USINGF_FRAME))
            size += sizeof(S_using_object_savegame);
    }
    for (int j = C_actor::global_actors.size(); j--;)
    {
        C_drop_in_weapon *weapon = (C_drop_in_weapon *)C_actor::global_actors[j];
        if (weapon->type == ACTOR_DROP_IN_WEAPON && weapon->frame && weapon->item.id > GAMEITEM_SPECIAL_ACTION)
            size += sizeof(S_using_object_savegame);
    }
    return size;
}

// A dropped weapon's record lies on the ground: its position dropped 0.2 above what is up to 49 m below a point 1 m
// above it
// FUNCTION: GAME 0x55e6e0
bool C_using_object::SaveGameSave(void **buf)
{
    int *num = (int *)*buf;
    *buf = (uchar *)*buf + sizeof(int);
    *num = 0;
    for (int i = objects.size(); i--;)
    {
        if (!objects[i]->scripted && objects[i]->item.id > GAMEITEM_SPECIAL_ACTION &&
            objects[i]->flags == (USINGF_DELETE | USINGF_FRAME))
        {
            S_using_object_savegame *s = (S_using_object_savegame *)*buf;
            *buf = (uchar *)*buf + sizeof(S_using_object_savegame);
            (*num)++;
            s->item = objects[i]->item;
            s->pos = objects[i]->frame->GetWorldPos();
            s->rot = objects[i]->frame->GetWorldRot();
            s->radius_2 = objects[i]->radius_2;
        }
    }
    for (int j = C_actor::global_actors.size(); j--;)
    {
        if (C_actor::global_actors[j]->type == ACTOR_DROP_IN_WEAPON)
        {
            C_drop_in_weapon *weapon = (C_drop_in_weapon *)C_actor::global_actors[j];
            if (!weapon->frame || weapon->item.id <= GAMEITEM_SPECIAL_ACTION)
                continue;
            S_using_object_savegame *s = (S_using_object_savegame *)*buf;
            *buf = (uchar *)*buf + sizeof(S_using_object_savegame);
            (*num)++;
            s->item = weapon->item;
            s->pos = weapon->frame->GetWorldPos();
            s->rot = weapon->frame->GetWorldRot();
            s->radius_2 = .5f;
            S_vector hit;
            if (gifCollision.TestLineV(s->pos + S_vector(0.0f, 1.0f, 0.0f), S_vector(0.0f, -50.0f, 0.0f),
                &hit, NULL, COLF_ALL, 0))
                s->pos.y = hit.y + .2f;
        }
    }
    return true;
}

// Each record's weapon model opened through the model cache into the primary sector, "load_wpn", and given an object
// that frees it; a record of an item gameItemTable doesn't have, or whose model doesn't open, is skipped
// FUNCTION: GAME 0x55e980
bool C_using_object::SaveGameLoad(void **buf)
{
    int *num = (int *)*buf;
    *buf = (uchar *)*buf + sizeof(int);
    for (int i = *num; i; i--)
    {
        S_using_object_savegame *s = (S_using_object_savegame *)*buf;
        *buf = (uchar *)*buf + sizeof(S_using_object_savegame);
        if (numOfGameItems <= s->item.id)
            continue;
        I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        if (!model)
            continue;
        S_item_def &def = gameItemTable[s->item.id];
        if (model_cache.Open(model, def.model, 0, NULL, NULL, NULL) == I3D_OK)
        {
            // STRING: GAME 0x63662c
            model->SetName("load_wpn");
            model->SetPos(s->pos);
            model->SetRot(s->rot);
            model->LinkTo(act_mission->scene->GetPrimarySector(), 0);
            act_mission->scene->SetFrameSector(model);
            S_using_object *obj = new S_using_object;
            obj->pos = s->pos;
            obj->radius_2 = s->radius_2;
            obj->matrix = NULL;
            obj->flags = USINGF_DELETE | USINGF_FRAME;
            obj->actor = NULL;
            obj->frame = model;
            obj->item = s->item;
            obj->item.using_object = obj;
            act_mission->GetGame()->GetUsingObject().AddObject(obj);
        }
        else
            model->Release();
    }
    return true;
}

// FUNCTION: GAME 0x55edd0
C_sound_table::C_sound_table()
{
    next_group_id = 0;
    next_sound_id = 0;
}

// FUNCTION: GAME 0x55ee00
C_sound_table::~C_sound_table()
{
    FreeTables();
}

// The sounds from the last one, as PS2's
// FUNCTION: GAME 0x55ee60
void C_sound_table::FreeTables()
{
    for (int i = tables.size(); i--;)
    {
        delete[] tables[i].name;
        delete[] tables[i].file_names[0];
        delete[] tables[i].file_names[1];
        delete[] tables[i].file_names[2];
        delete[] tables[i].file_names[3];
        delete[] tables[i].file_names[4];
        delete[] tables[i].file_names[5];
    }
    tables.clear();
}

// FUNCTION: GAME 0x55ef30
void C_sound_table::AddGroup(const char *name)
{
    S_sound_group group;
    sound_group.push_back(group);
    strcpy(sound_group.back().name, name);
    sound_group.back().id = next_group_id;
    next_group_id++;
}

// The search starts again after each sound deleted (a found flag: a goto back gives other code)
// FUNCTION: GAME 0x55ef80
void C_sound_table::DelGroup(int index)
{
    if (index >= sound_group.size() || index < 0)
        return;
    bool found;
    do
    {
        found = false;
        for (int i = tables.size(); i--;)
        {
            if (tables[i].group == sound_group[index].id)
            {
                DelSound(tables[i].id);
                found = true;
                break;
            }
        }
    } while (found);
    sound_group[index] = sound_group.back();
    sound_group.pop_back();
}

// FUNCTION: GAME 0x55f080
void C_sound_table::AddSound(const char *name, uint group)
{
    S_sound_table t;
    tables.push_back(t);
    memset(&tables.back(), 0, sizeof(S_sound_table));
    tables.back().name = new char[strlen(name) + 1];
    strcpy(tables.back().name, name);
    tables.back().id = next_sound_id;
    next_sound_id++;
    tables.back().group = group;
}

// FUNCTION: GAME 0x55f120
void C_sound_table::DelSound(uint id)
{
    for (int i = tables.size(); i--;)
    {
        if (tables[i].id == id)
        {
            delete[] tables[i].name;
            tables[i].name = NULL;
            for (int j = 0; j < 6; j++)
            {
                delete[] tables[i].file_names[j];
                tables[i].file_names[j] = NULL;
            }
            tables.erase(tables.begin() + i);
            return;
        }
    }
}

// FUNCTION: GAME 0x55f210
C_sound_table::S_sound_table *C_sound_table::GetTableFromIDX(uint id)
{
    for (int i = tables.size(); i--;)
    {
        if (tables[i].id == id)
            return &tables[i];
    }
    return NULL;
}

// FUNCTION: GAME 0x55f270
void C_sound_table::SetFileName(S_sound_table *table, int index, const char *file_name)
{
    if (!table)
        return;
    delete[] table->file_names[index];
    table->file_names[index] = NULL;
    if (file_name)
    {
        table->file_names[index] = new char[strlen(file_name) + 1];
        strcpy(table->file_names[index], file_name);
    }
}

// A selection sort
// FUNCTION: GAME 0x55f2d0
void C_sound_table::OptimalizeTable(S_sound_table *table)
{
    for (int i = 0; i < table->num_files - 1; i++)
    {
        int best = i;
        for (int j = i + 1; j < table->num_files; j++)
        {
            if (table->file_probability[j] < table->file_probability[best])
                best = j;
        }
        if (i != best)
        {
            int prob = table->file_probability[i];
            table->file_probability[i] = table->file_probability[best];
            table->file_probability[best] = prob;
            char *file_name = table->file_names[i];
            table->file_names[i] = table->file_names[best];
            table->file_names[best] = file_name;
        }
    }
}

// Version 2 only. The file has the two counters, the groups and the sounds, each with its name, which the game skips
// (only the editor needs them), its files and its parameters.
// FUNCTION: GAME 0x55f390
void C_sound_table::LoadTables(const char *name)
{
    int h = dtaOpen(name, false);
    if (h == -1)
        return;
    int version;
    dtaRead(h, &version, 4);
    if (version != 2)
    {
        dtaClose(h);
        return;
    }
    dtaRead(h, &next_group_id, 4);
    dtaRead(h, &next_sound_id, 4);
    int num;
    dtaRead(h, &num, 4);
    sound_group.clear();
    if (num)
        dtaSeek(h, num * sizeof(S_sound_group), 1);
    tables.clear();
    dtaRead(h, &num, 4);
    for (int i = 0; i < num; i++)
    {
        S_sound_table t;
        memset(&t, 0, sizeof(t));
        tables.push_back(t);
        memset(&tables.back(), 0, sizeof(S_sound_table));
        tables.back().visible = true;
        int len;
        dtaRead(h, &len, 4);
        if (len)
            dtaSeek(h, len, 1);
        dtaRead(h, &tables.back().id, 4);
        dtaRead(h, &tables.back().group, 4);
        dtaRead(h, &tables.back().num_files, 1);
        dtaRead(h, &tables.back().num_params, 1);
        for (int j = 0; j < tables.back().num_files; j++)
        {
            dtaRead(h, &len, 4);
            if (len)
            {
                tables.back().file_names[j] = new char[len + 1];
                dtaRead(h, tables.back().file_names[j], len);
                tables.back().file_names[j][len] = 0;
            }
            dtaRead(h, &tables.back().file_probability[j], 4);
        }
        if (tables.back().num_params > 0)
            dtaRead(h, tables.back().params, tables.back().num_params * sizeof(S_sound_param));
    }
    dtaClose(h);
}

// LoadTables' file, version 2, the names written too
// FUNCTION: GAME 0x55f5e0
void C_sound_table::SaveTables(const char *name)
{
    int h = dtaOpenWrite(name, DTA_CREATE_FILE);
    if (h == DTA_INVALID_HANDLE)
        return;
    int version = 2;
    dtaWrite(h, &version, 4);
    dtaWrite(h, &next_group_id, 4);
    dtaWrite(h, &next_sound_id, 4);
    int num = sound_group.size();
    dtaWrite(h, &num, 4);
    if (num)
        dtaWrite(h, sound_group.begin(), num * sizeof(S_sound_group));
    num = tables.size();
    dtaWrite(h, &num, 4);
    for (int i = 0; i < num; i++)
    {
        int len = strlen(tables[i].name);
        dtaWrite(h, &len, 4);
        if (len)
            dtaWrite(h, tables[i].name, len);
        dtaWrite(h, &tables[i].id, 4);
        dtaWrite(h, &tables[i].group, 4);
        dtaWrite(h, &tables[i].num_files, 1);
        dtaWrite(h, &tables[i].num_params, 1);
        for (int j = 0; j < tables[i].num_files; j++)
        {
            len = strlen(tables[i].file_names[j]);
            dtaWrite(h, &len, 4);
            if (len)
                dtaWrite(h, tables[i].file_names[j], len);
            dtaWrite(h, &tables[i].file_probability[j], 4);
        }
        if (tables[i].num_params > 0)
            dtaWrite(h, tables[i].params, tables[i].num_params * sizeof(S_sound_param));
    }
    dtaClose(h);
}

// Picks one of the sound's files by their chances and one of its parameter sets at random. A file that doesn't open
// hides the sound until ClearInvisibleFlag.
// FUNCTION: GAME 0x55f820
bool C_sound_table::ConstructSound(I3D_sound *sound, uint id)
{
    for (int i = tables.size(); i--;)
    {
        if (tables[i].id == id)
        {
            if (!tables[i].visible || !tables[i].num_files || !tables[i].num_params)
                return false;
            int r = rnd(100);
            int f;
            for (f = 0; f < tables[i].num_files && r > tables[i].file_probability[f]; f++)
                ;
            if (f == tables[i].num_files)
                f--;
            int p = rnd(tables[i].num_params);
            if (sound_cache.Open(sound, tables[i].file_names[f], 0, NULL, NULL, NULL) != I3D_OK)
            {
                tables[i].visible = false;
                return false;
            }
            switch (tables[i].params[p].type)
            {
            case SOUNDPARAM_AMBIENT:
                sound->SetSoundType(I3DSOUND_AMBIENT);
                break;
            case SOUNDPARAM_POINT:
                sound->SetSoundType(I3DSOUND_POINT);
                break;
            case SOUNDPARAM_SPOT:
                sound->SetSoundType(I3DSOUND_SPOT);
                break;
            case SOUNDPARAM_VOLUME:
                igraph->Present();
                MsgBox(
                    // STRING: GAME 0x636638
                    "Sound type I3DSOUND_VOLUME not exist.",
                    // STRING: GAME 0x636660
                    "ConstructSound",
                    MSGBOX_OK);
                break;
            }
            sound->SetRange(tables[i].params[p].near_range, tables[i].params[p].far_range, tables[i].params[p].mid,
                tables[i].params[p].mid_vol);
            sound->SetCone(tables[i].params[p].in_angle, tables[i].params[p].out_angle);
            sound->SetVolume(tables[i].params[p].volume);
            sound->SetOutVol(tables[i].params[p].out_vol);
            return true;
        }
    }
    return false;
}

// FUNCTION: GAME 0x55fa00
bool C_sound_table::TestConstructSound(I3D_sound *sound, uint id)
{
    return ConstructSound(sound, id);
}

// FUNCTION: GAME 0x55fa10
void C_sound_table::ClearInvisibleFlag()
{
    for (int i = tables.size(); i--;)
        tables[i].visible = true;
}

// FUNCTION: GAME 0x55fa50
void soundArena::SoundEmit(S_vector &pos, float range, float loudness, uint id, C_actor *act)
{
    uint i;
    if (act_mission->game->game_time > next_clear)
    {
        next_clear = act_mission->game->game_time + 2000;
        for (i = 0; i < NUM_ARENA_SOUNDS; i++)
        {
            if (act_mission->game->game_time - sounds[i].time > 2000)
            {
                sounds[i].id = 0;
                sounds[i].act = NULL;
            }
        }
    }
    for (i = 0; i < NUM_ARENA_SOUNDS; i++)
    {
        if (sounds[i].act == act && sounds[i].id == id)
        {
            sounds[i].pos = pos;
            sounds[i].time = act_mission->game->game_time;
            sounds[i].loudness = loudness;
            sounds[i].range = range;
            return;
        }
    }
    last = (last + 1) % NUM_ARENA_SOUNDS;
    sounds[last].pos = pos;
    sounds[last].id = id;
    sounds[last].time = act_mission->game->game_time;
    sounds[last].loudness = loudness;
    sounds[last].range = range;
    sounds[last].act = act;
}

// A sound's loudness falls linearly to 0 at its range; the entity hears it above 1.05 less its hearing, at most at 1
// FUNCTION: GAME 0x55fba0
void soundArena::GetSounds(C_entity *ent)
{
    for (int i = 0; i < NUM_ARENA_SOUNDS; i++)
    {
        if (!sounds[i].id || sounds[i].act == ent)
            continue;
        float dist = (ent->frame->GetWorldPos() - sounds[i].pos).Magnitude();
        float level = sounds[i].loudness - (dist * sounds[i].loudness) / sounds[i].range;
        if (level > 1.05f - ent->GetHearQuality())
        {
            if (level > 1.0f)
                level = 1.0f;
            ai_sound_contact con;
            con.time = sounds[i].time;
            con.id = sounds[i].id;
            con.pos = sounds[i].pos;
            con.act = sounds[i].act;
            con.level = level;
            ent->sensors.NewSoundCon(&con);
        }
        else
            ent->sensors.CancelSoundCon(sounds[i].act, sounds[i].id);
    }
}

// FUNCTION: GAME 0x55fcf0
void soundArena::Init()
{
    memset(sounds, 0, sizeof(sounds));
    last = 0;
    next_clear = 0;
}

// FUNCTION: GAME 0x55fd10
void soundArena::InvalidateActor(C_actor *act)
{
    for (int i = 0; i < NUM_ARENA_SOUNDS; i++)
    {
        if (sounds[i].act == act)
        {
            sounds[i].id = 0;
            sounds[i].act = NULL;
        }
    }
}
