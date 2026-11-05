// The city's base blocks (PS2 m_cache.cpp, the name its helper structs carry; the PC's file is most likely the one at
// 0x401030-0x404cdc, docs/game-files.md, the first after the empty one), written whole: base_cache's initializer, then
// PS2's functions in PS2's order, the constructor (0x401070), the destructor (0x4010a0), Close (0x4011f0), PrepareScene
// (0x4012e0), cbDupl (0x401310), Open (0x4013f0, which closes first), OpenModel (0x401ca0), the Tick GameLoopSingle
// calls (0x4020b0), ActivateBlock (0x4025d0), DeactivateBlock (0x402a50, inline: VisibleAllFrames inlines it),
// TransparentModel (0x402b60), FindModel (0x402cb0), DestroyFrame (0x402cf0), VisibleAllFrames (0x402dc0) and
// IsBlockInRange (0x402f90, inline: Tick's first call inlines it). PS2's ShowModel and HideModel are inline, without
// copies; PS2's AssociateBaseBlocks, GetNumModelVisual, IsModelOnlyObject, ShowObject, HideObject and ClearUnusedModels
// have no PC counterpart. The copies of the inline functions the file calls out of line and of its templates follow
// (m_cache.h, ls3df/I3D/I3d_math.h, common/C_str.h).
// Open: Open (see its comment); OpenModel's inlined model cache Open sends I3DERR_FILECORRUPTED past the result test,
// one jump (C_I3D_cache::Open's tests as a switch fix it, and LoadParticles', but move five other files' /YX order);
// DeactivateBlock's copy adds the inlined HideModel's set and copy offsets the other way round; the precompiled
// header (below).

// The original was compiled with a precompiled header: C_str's copies follow IsBlockInRange, not OpenModel, their
// first caller (and the chunks' Descend and Read at 0x404ce0, which Open calls, are most likely this file's copies
// that never came due). Without one, as here, the copies of S_vector2's inline functions Tick calls (operator-
// 0x402530, Magnitude 0x402550, DistanceToLine 0x402570) come right after it, as the original's do, and
// IsBlockInRange's copy needs Magnitude's compiled before it; under /YX ours are deferred past it (97.66%).
#pragma hdrstop
#include "mission/m_cache.h"

#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "C_chunk.h"
#include "I3D_cache.h"
#include "mission/Mission.h"
#include "mission/game.h"

// GLOBAL: GAME 0x63d268
C_cache_base_block base_cache;

struct C_cache_base_block::S_hlp
{
    static I3DENUMRET __stdcall cbDupl(I3D_frame *frm, uint user);
};

// FUNCTION: GAME 0x401070
C_cache_base_block::C_cache_base_block()
{
    open = false;
    timer = 0;
}

// FUNCTION: GAME 0x4010a0
C_cache_base_block::~C_cache_base_block()
{
    Close();
}

// PS2's without the model sets' own model, which PS2 releases in place of their frames, and the list of frames after
// them (0x34), which the PC doesn't have
// FUNCTION: GAME 0x4011f0
void C_cache_base_block::Close()
{
    open = false;
    blocks.clear();
    for (int i = model_sets.size(); i--;)
    {
        delete[] model_sets[i].name;
        for (int j = model_sets[i].instances.size(); j--;)
        {
            if (model_sets[i].instances[j].frame)
            {
                model_sets[i].instances[j].frame->LinkTo(NULL, 0);
                model_sets[i].instances[j].frame->Release();
            }
        }
    }
    model_sets.clear();
}

// Links the scene's visuals named LLwcol... to the first of them, and those named wcol... to the first of those
// (S_hlp::cbDupl)
// FUNCTION: GAME 0x4012e0
void C_cache_base_block::PrepareScene()
{
    I3D_frame *first = NULL;
    act_mission->scene->EnumFrames(S_hlp::cbDupl, (uint)&first, ENUMF_VISUAL, NULL);
}

// The first frame of a name goes into the primary sector, switched off, and the others under it
// FUNCTION: GAME 0x401310
I3DENUMRET __stdcall C_cache_base_block::S_hlp::cbDupl(I3D_frame *frm, uint user)
{
    const char *name = frm->GetName();
    I3D_frame **first = (I3D_frame **)user;
    // STRING: GAME 0x62e428
    if (strlen(name) >= 6 && !_strnicmp(name, "LLwcol", 6))
    {
        if (*first)
            frm->LinkTo(*first, I3DLINK_UPDMATRIX);
        else
        {
            frm->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
            frm->SetOn(false);
            *first = frm;
        }
    }
    // STRING: GAME 0x62e420
    if (strlen(name) >= 4 && !_strnicmp(name, "wcol", 4))
    {
        if (*first)
            frm->LinkTo(*first, I3DLINK_UPDMATRIX);
        else
        {
            frm->LinkTo(act_mission->scene->primary_sector, I3DLINK_UPDMATRIX);
            frm->SetOn(false);
            *first = frm;
        }
    }
    return I3DENUMRET_OK;
}

// Reads cache.bin: the version chunk, then a chunk for each block, whose frame the scene has to have (switched off with
// its visual children until the block is active), then a chunk for each of the block's models, loaded through the
// model cache. The name buffer grows as longer names come. A missing model is reported in a message box (Czech
// "Chybi model", the model is missing); 0rek23.i3d is replaced by 0rek17.i3d.
// Open: the original calls blocks.size() out of line (0x4036c0) where ours inlines it: its budget is about three
// dead statements smaller (a Spend(0) of three, or three statements less anywhere, gives it; no real spelling found),
// and three reload orders (name_size, name, this) are the reverse of ours (c2's build ids: the original numbers name
// and name_size after this)
// FUNCTION: GAME 0x4013f0
int C_cache_base_block::Open(C_chunk *chunk, I3D_scene *scene)
{
    Close();
    PrepareScene();
    ushort type;
    chunk->RAscend(type);
    if (type != CHUNK_CACHE)
        return CACHE_OPEN_NOT_CACHE;
    int version;
    chunk->Read(&version, sizeof(version));
    if (version != CACHE_VERSION)
        return CACHE_OPEN_BAD_VERSION;
    char *name = NULL;
    int name_size = 0;
    while (chunk->Size())
    {
        chunk->RAscend(type);
        ShowProgress(chunk);
        if (type == CHUNK_CACHE_BLOCK)
        {
            int len;
            chunk->Read(&len, sizeof(len));
            if (!name)
            {
                name_size = len + 1;
                name = new char[name_size];
            }
            else if (name_size < len + 1)
            {
                delete[] name;
                name_size = len + 1;
                name = new char[name_size];
            }
            chunk->Read(name, len);
            name[len] = 0;
            I3D_frame *frm = scene->FindFrame(name, ENUMF_VISUAL);
            if (!frm)
            {
                delete[] name;
                return CACHE_OPEN_NO_FRAME;
            }
            frm->SetOn(false);
            S_cache_base_block block;
            block.frame = frm;
            block.active = false;
            for (int i = frm->NumChildren(); i--;)
            {
                if (frm->GetChild(i)->GetType() == FRAME_VISUAL)
                {
                    frm->GetChild(i)->SetOn(false);
                    block.visuals.push_back(frm->GetChild(i));
                }
            }
            chunk->Read(&block.pos, sizeof(block.pos));
            chunk->Read(&block.radius, sizeof(block.radius));
            for (int j = 0; j < 4; j++)
            {
                chunk->Read(&block.points[j], sizeof(block.points[j]));
                chunk->Read(&block.dirs[j], sizeof(block.dirs[j]));
            }
            while (chunk->Size())
            {
                chunk->RAscend(type);
                if (type == CHUNK_CACHE_MODEL)
                {
                    S_cache_model_inbb m;
                    m.instance = 0xffff;
                    chunk->Read(&len, sizeof(len));
                    if (!name)
                    {
                        name_size = len + 1;
                        name = new char[name_size];
                    }
                    else if (name_size < len + 1)
                    {
                        delete[] name;
                        name_size = len + 1;
                        name = new char[name_size];
                    }
                    chunk->Read(name, len);
                    name[len] = 0;
                    // STRING: GAME 0x62e460
                    if (!_strcmpi(name, "0rek23.i3d"))
                        // STRING: GAME 0x62e454
                        strcpy(name, "0rek17.i3d");
                    m.model = OpenModel(name);
                    if (m.model == 0xffff)
                    {
                        char text[128];
                        // STRING: GAME 0x62e43c
                        sprintf(text, "model \"%s\" not found.", name);
                        // STRING: GAME 0x62e430
                        MessageBox(igraph->GetMainHWND(), text, "Chybi model", MB_OK);
                        delete[] name;
                        return CACHE_OPEN_NO_MODEL;
                    }
                    chunk->Read(&m.pos, sizeof(m.pos));
                    chunk->Read(&m.rot, sizeof(m.rot));
                    chunk->Read(&m.scale, sizeof(m.scale));
                    chunk->Read(&m.unknown_30, sizeof(m.unknown_30));
                    chunk->Read(&m.visual_scale, sizeof(m.visual_scale));
                    m.visible = true;
                    m.block = blocks.size();
                    block.models.push_back(m);
                }
                chunk->Descend();
            }
            blocks.push_back(block);
        }
        chunk->Descend();
    }
    chunk->Descend();
    if (name)
        delete[] name;
    tick_set = 0;
    open = true;
    timer = 0xfffffff;
    cam_pos = S_vector(0.0f, -1000.0f, 0.0f);
    return CACHE_OPEN_OK;
}

// A new set's model is named after its index and gets the copy the cache loaded
// FUNCTION: GAME 0x401ca0
ushort C_cache_base_block::OpenModel(const char *name)
{
    ushort index = 0xffff;
    for (int i = model_sets.size(); i--;)
    {
        if (!_stricmp(name, model_sets[i].name))
        {
            index = i;
            break;
        }
    }
    if (index == 0xffff)
    {
        index = model_sets.size();
        S_cache_model_set set;
        I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        if (model_cache.Open(model, name, 0, NULL, NULL, NULL) == I3D_OK)
        {
            set.name = new char[strlen(name) + 1];
            strcpy(set.name, name);
            S_cache_model_one one;
            one.used = false;
            one.frame = model;
            char buf[128];
            // STRING: GAME 0x62e46c
            sprintf(buf, "off%iseg0", index);
            model->SetName(buf);
            set.instances.push_back(one);
            model_sets.push_back(set);
        }
        else
        {
            model->Release();
            index = 0xffff;
        }
    }
    return index;
}

// Releases an unused copy of one model set a tick, then, at most every 100 ms unless the camera has moved more than 100
// since, switches the blocks around the camera: off outside the outer radius, on inside the inner one
// FUNCTION: GAME 0x4020b0
void C_cache_base_block::Tick(uint time)
{
    if (model_sets.size())
    {
        int last = model_sets[tick_set].instances.size() - 1;
        if (last && !model_sets[tick_set].instances[last].used)
        {
            model_sets[tick_set].instances[last].frame->Release();
            model_sets[tick_set].instances.pop_back();
        }
        if (++tick_set >= model_sets.size())
            tick_set = 0;
    }
    I3D_camera *cam = act_mission->scene->GetActiveCamera();
    if (!cam)
        return;
    // Named and squared written out: the temporary's Square() squares in another order
    S_vector diff = cam->GetWorldPos() - cam_pos;
    bool moved = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z > 10000.0f;
    timer += time;
    if (timer < 100 && !moved)
        return;
    timer = 0;
    cam_pos = cam->GetWorldPos();
    float inner = 200.0f, outer = 230.0f;
    // Converted to bool, which VC6 reads with a shift (shr 9; test cl, 1)
    if ((bool)(act_mission->game->cheats & GAMEF_FAR_BLOCKS))
    {
        inner = 550.0f;
        outer = 580.0f;
    }
    S_vector cp = cam->GetWorldPos();
    S_vector2 pos(cp.x, cp.z);
    int i;
    for (i = blocks.size(); i--;)
    {
        if (!IsBlockInRange(i, pos, outer) && blocks[i].active)
            DeactivateBlock(i);
    }
    for (i = blocks.size(); i--;)
    {
        if (IsBlockInRange(i, pos, inner) && !blocks[i].active)
            ActivateBlock(i);
    }
    if (moved)
        act_mission->FinishTimeTransparentObjects();
}

// Switches the block on, with its visuals, after placing its visible models' copies (the copies of the model sets'
// models, ShowModel's) and putting them into their sectors, the last placed first
// FUNCTION: GAME 0x4025d0
void C_cache_base_block::ActivateBlock(uint index)
{
    std::vector<I3D_frame *> frames;
    for (uint j = 0; j < blocks[index].models.size(); j++)
    {
        if (blocks[index].models[j].visible)
        {
            I3D_frame *frm = ShowModel(blocks[index].models[j].model, &blocks[index].models[j].instance);
            if (frm)
            {
                frm->SetPos(blocks[index].models[j].pos);
                frm->SetRot(blocks[index].models[j].rot);
                frm->SetScale(blocks[index].models[j].scale);
                frm->Update();
                uint num = ((I3D_model *)frm)->frames.size();
                I3D_frame **frms = &((I3D_model *)frm)->frames[0];
                for (uint k = 0; k < num; k++)
                {
                    if (frms[k]->GetType() == FRAME_VISUAL)
                    {
                        frms[k]->SetScale(blocks[index].models[j].visual_scale);
                        frms[k]->Update();
                    }
                }
            }
            frames.push_back(frm);
        }
    }
    int i;
    for (i = frames.size(); i--;)
    {
        if (frames[i])
        {
            act_mission->scene->SetFrameSector(frames[i]);
            frames[i]->Update();
        }
    }
    blocks[index].frame->SetOn(true);
    blocks[index].active = true;
    for (i = blocks[index].visuals.size(); i--;)
        blocks[index].visuals[i]->SetOn(true);
}

// Switches the block off, with its visuals, and gives its models' copies back
// FUNCTION: GAME 0x402a50
inline void C_cache_base_block::DeactivateBlock(uint index)
{
    blocks[index].frame->SetOn(false);
    blocks[index].active = false;
    for (int i = blocks[index].visuals.size(); i--;)
        blocks[index].visuals[i]->SetOn(false);
    for (int j = blocks[index].models.size(); j--;)
    {
        if (blocks[index].models[j].visible)
            HideModel(blocks[index].models[j].model, blocks[index].models[j].instance);
    }
}

// A model placed by an active block fades in (PS2 name; static there too): its objects, unless the whole model is
// smaller than 2 in every direction. The scale is the world scale's y, for all three sizes.
// FUNCTION: GAME 0x402b60
static void TransparentModel(I3D_frame *frm, bool on)
{
    if (on)
    {
        I3D_bvolume bv = frm->GetHRBoundVolume();
        S_vector size = bv.bbox.max - bv.bbox.min;
        S_vector scale = frm->GetWorldScale();
        size *= scale.y;
        if (size.x < 2.0f && size.y < 2.0f && size.z < 2.0f)
            return;
    }
    for (int i = frm->NumChildren(); i--;)
    {
        TransparentModel(frm->GetChild(i), false);
        if (frm->GetChild(i)->GetType() == FRAME_VISUAL &&
            ((I3D_visual *)frm->GetChild(i))->GetVisualType() == I3D_VISUAL_OBJECT)
            act_mission->AddTimeTransparentObject((I3D_object *)frm->GetChild(i), 700);
    }
}

// The first unused copy, or a new one duplicated from the set's first and named after the set and the copy; it fades in.
// NULL for no set: the test is here, as PS2's has it (ActivateBlock's NULL comes from the inlined return).
inline I3D_frame *C_cache_base_block::ShowModel(ushort model, ushort *instance)
{
    if (model == 0xffff)
        return NULL;
    *instance = 0xffff;
    for (uint k = 0; k < model_sets[model].instances.size(); k++)
    {
        if (!model_sets[model].instances[k].used)
        {
            *instance = k;
            model_sets[model].instances[k].used = true;
            TransparentModel(model_sets[model].instances[k].frame, true);
            return model_sets[model].instances[k].frame;
        }
    }
    *instance = model_sets[model].instances.size();
    S_cache_model_one one;
    one.used = true;
    one.frame = driver->CreateFrame(FRAME_MODEL);
    one.frame->Duplicate(model_sets[model].instances[0].frame);
    char buf[128];
    // STRING: GAME 0x62e478
    sprintf(buf, "off%iseg%i", model, *instance);
    one.frame->SetName(buf);
    model_sets[model].instances.push_back(one);
    TransparentModel(one.frame, true);
    return one.frame;
}

inline void C_cache_base_block::HideModel(ushort model, ushort instance)
{
    if (model == 0xffff || instance == 0xffff)
        return;
    if (model_sets[model].instances[instance].used)
    {
        model_sets[model].instances[instance].used = false;
        model_sets[model].instances[instance].frame->LinkTo(NULL, 0);
    }
}

// The copy of a set's model, NULL for none (C_game::DestroyControlSignal's)
// FUNCTION: GAME 0x402cb0
I3D_frame *C_cache_base_block::FindModel(ushort model, ushort instance)
{
    if (model == 0xffff || instance == 0xffff)
        return NULL;
    return model_sets[model].instances[instance].frame;
}

// The block is looked up through base_cache, as PS2's (C_game::DestroyControlSignal's). The index is read first: a
// local, or an inline accessor taking it
// FUNCTION: GAME 0x402cf0
bool C_cache_base_block::DestroyFrame(S_cache_model_inbb *m)
{
    if (!m || !m->visible)
        return false;
    m->visible = false;
    uint index = m->block;
    S_cache_base_block *b = index < base_cache.blocks.size() ? &base_cache.blocks[index] : NULL;
    if (b->active)
        HideModel(m->model, m->instance);
    return true;
}

// Every model visible again, and every block inactive until the next Tick, which the timer lets run at once
// FUNCTION: GAME 0x402dc0
void C_cache_base_block::VisibleAllFrames()
{
    for (int i = blocks.size(); i--;)
    {
        for (int j = blocks[i].models.size(); j--;)
        {
            blocks[i].models[j].visible = true;
            if (blocks[i].active)
                DeactivateBlock(i);
        }
    }
    timer = 10000;
}

// Within the block's bounding circle, then between both pairs of its edges, or between one pair and within radius of
// the other, or within radius of a corner. The edges' points are the original's (points[3] twice, points[0] for none).
// The sign tests through if/else and the radius tests as else-if chains are the original's amount of source: Tick's
// first call inlines it, and the budget left decides Tick's later calls and the copy's nested ones.
// FUNCTION: GAME 0x402f90
inline bool C_cache_base_block::IsBlockInRange(uint index, S_vector2 pos, float radius)
{
    S_vector2 d = blocks[index].pos - pos;
    float r = radius + blocks[index].radius;
    if (!(d.x * d.x + d.y * d.y <= r * r))
        return false;
    bool in_range = false;
    float dist[4];
    dist[0] = pos.DistanceToLine(blocks[index].points[3], blocks[index].dirs[0]);
    dist[1] = pos.DistanceToLine(blocks[index].points[2], blocks[index].dirs[1]);
    dist[2] = pos.DistanceToLine(blocks[index].points[3], blocks[index].dirs[2]);
    dist[3] = pos.DistanceToLine(blocks[index].points[1], blocks[index].dirs[3]);
    bool between01;
    if ((dist[0] < 0.0f && dist[1] >= 0.0f) || (dist[0] >= 0.0f && dist[1] < 0.0f))
        between01 = true;
    else
        between01 = false;
    bool between23;
    if ((dist[2] < 0.0f && dist[3] >= 0.0f) || (dist[2] >= 0.0f && dist[3] < 0.0f))
        between23 = true;
    else
        between23 = false;
    if (between01)
    {
        if (between23)
            in_range = true;
        else if ((float)fabs(dist[2]) <= radius)
            in_range = true;
        else if ((float)fabs(dist[3]) <= radius)
            in_range = true;
    }
    else if (between23)
    {
        if ((float)fabs(dist[0]) <= radius)
            in_range = true;
        else if ((float)fabs(dist[1]) <= radius)
            in_range = true;
    }
    else
    {
        for (int k = 0; k < 4; k++)
        {
            S_vector2 c = pos - blocks[index].points[k];
            if (c.Magnitude() <= radius)
            {
                in_range = true;
                break;
            }
        }
    }
    return in_range;
}
