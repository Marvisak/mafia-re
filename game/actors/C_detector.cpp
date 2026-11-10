// The script objects (PS2 C_detector.cpp; the PC's file is the one at 0x432870-0x433fdc, docs/game-files.md), whose
// constructor (0x432880) comes first, in PS2's order, written whole: the constructor and Init, which
// C_mission::CreateActor and OpenBin call, the destructor, DuplicateFrom, LoadData and SaveData, GameInit and GameDone,
// which C_game::Init and Done call, the saved game's functions, AI and Update, which C_actor::Tick calls, Hit, the
// collisions (CreateColls with its callbacks, EraseColls), the using object (CreateUO, UseDetector, which the player
// calls) and Process, the detector's script commands.

#include "actors/C_detector.h"

#include "C_chunk.h"
#include "actors/C_human.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "collision/g_colls.h"
#include "g_material.h"

#include <string.h>

struct C_detector::S_hlp
{
    static I3DENUMRET __stdcall cbDummyCount(I3D_frame *frm, uint user);
    static I3DENUMRET __stdcall cbDummyCreate(I3D_frame *frm, uint user);
};

// FUNCTION: GAME 0x432880
C_detector::C_detector()
{
    type = ACTOR_DETECTOR;
}

// FUNCTION: GAME 0x432930
C_detector::~C_detector()
{
}

// FUNCTION: GAME 0x432990
bool C_detector::Init(I3D_frame *frm)
{
    if (!C_actor::Init(frm))
        return false;
    human_types = 0;
    actor = this;
    return true;
}

// The other detector's script source and the humans it counts (PS2's the same)
// FUNCTION: GAME 0x4329c0
void C_detector::DuplicateFrom(C_actor *act)
{
    C_actor::DuplicateFrom(act);
    DuplicateProgramFrom((C_detector *)act);
    human_types = ((C_detector *)act)->human_types;
}

// The script compiled for the actor; one without instructions sets the actor's unknown_65, as a plain actor's
// FUNCTION: GAME 0x432a00
void C_detector::GameInit()
{
    C_actor::GameInit();
    actor = this;
    C_program::Init();
    if (instructions.size() < 1)
        C_actor::unknown_65 = true;
    DebustizeFrame(frame);
    signal = false;
    wait_for_use = false;
    wait_for_hit = false;
    dyn_colls = false;
    colls = NULL;
    coll_matrices = NULL;
    coll_inv_matrices = NULL;
    num_colls = 0;
}

// FUNCTION: GAME 0x432a80
void C_detector::GameDone()
{
    act_mission->game->using_object.DelObject(&using_object);
    C_actor::GameDone();
    C_program::Done();
    EraseColls();
}

// A byte of 1, which LoadData skips, the dword at 0xd8, then the script (PS2's the same)
// FUNCTION: GAME 0x432ac0
bool C_detector::SaveData(C_chunk *chunk)
{
    uchar unknown = 1;
    chunk->Write(&unknown, sizeof(unknown));
    chunk->Write(&human_types, sizeof(human_types));
    return C_program::SaveSourceCode(chunk);
}

// A byte nothing reads, the dword at 0xd8, then the script, as PS2's
// FUNCTION: GAME 0x432b20
bool C_detector::LoadData(C_chunk *chunk)
{
    uchar unknown;
    chunk->Read(&unknown, sizeof(unknown));
    chunk->Read(&human_types, sizeof(human_types));
    return LoadSourceCode(chunk);
}

// The actor's, the script's, then the detector's record
// FUNCTION: GAME 0x432c10
int C_detector::SaveGameGetSize()
{
    int size = C_actor::SaveGameGetSize();
    return size + C_program::SaveGameGetSize() + sizeof(S_detector_savegame);
}

// FUNCTION: GAME 0x432c30
bool C_detector::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    if (!C_program::SaveGameSave(buf))
        return false;
    S_detector_savegame *s = (S_detector_savegame *)*buf;
    s->version = DETECTOR_SAVEGAME_VERSION;
    s->human_types = human_types;
    s->signal = signal;
    s->wait_for_use = wait_for_use;
    s->wait_for_hit = wait_for_hit;
    s->action = using_object.item.ammo_loaded;
    *buf = (uchar *)*buf + sizeof(S_detector_savegame);
    return true;
}

// The using object and the collisions taken away first; a signal on clears the actor's unknown_65, as SetSignal,
// and the waits make their collisions and their using object again
// FUNCTION: GAME 0x432ca0
bool C_detector::SaveGameLoad(void **buf)
{
    act_mission->game->using_object.DelObject(&using_object);
    EraseColls();
    if (!C_actor::SaveGameLoad(buf))
        return false;
    if (!C_program::SaveGameLoad(buf))
        return false;
    S_detector_savegame *s = (S_detector_savegame *)*buf;
    if (s->version != DETECTOR_SAVEGAME_VERSION)
        return false;
    human_types = s->human_types;
    signal = s->signal;
    if (signal)
        C_actor::unknown_65 = false;
    wait_for_use = s->wait_for_use;
    wait_for_hit = s->wait_for_hit;
    *buf = (uchar *)*buf + sizeof(S_detector_savegame);
    if (wait_for_hit)
    {
        CreateColls(false);
        SetActState(ACTSTATE_INACTIVE);
    }
    if (wait_for_use)
        CreateUO(s->action, true);
    return true;
}

// The script, while it has a command to run; past its end, END's return (PS2's the same)
// FUNCTION: GAME 0x432d70
void C_detector::AI(uint time)
{
    if (cur_command < instructions.size())
        CallProcess(time);
    else
        ClearAfterEnd();
}

// DETECTOR_CREATEDYNCOLL's collisions follow the frame, from the last one: each a sphere around the hierarchy's box,
// its radius scaled by the frame's world scale, with the frame's matrix and its inverse. PS2 also returns first where
// the frame moved by less than a distance; the PC tests nothing. The half size is halved in place and the radius
// scaled in place (each one stack slot, as the original's), and the inverse is Invert's (inlined, which pushes the
// arguments before the copy).
// FUNCTION: GAME 0x432db0
void C_detector::Update(uint time)
{
    if (!dyn_colls)
        return;
    for (int i = num_colls; i--;)
    {
        if (!colls[i])
            continue;
        I3D_bvolume bvol = frame->GetHRBoundVolume();
        S_vector half = bvol.bbox.max - bvol.bbox.min;
        half *= 0.5f;
        float radius = half.Magnitude();
        radius *= frame->GetWorldScale().y;
        ((I3D_bsphere &)colls[i]->bsphere).pos = half + bvol.bbox.min;
        ((I3D_bsphere &)colls[i]->bsphere).pos *= frame->GetMatrix();
        ((I3D_bsphere &)colls[i]->bsphere).radius = radius;
        coll_matrices[i] = frame->GetMatrix();
        coll_inv_matrices[i] = coll_matrices[i];
        coll_inv_matrices[i].Invert();
        gifCollision.DynUpdate(colls[i]);
    }
}

// A hit ends DETECTOR_WAITFORHIT's wait (its collisions taken away), or sends the script EVENT_DETECTOR_HIT while it
// has DETECTOR_CREATEDYNCOLL's collisions
// FUNCTION: GAME 0x433020
bool C_detector::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    if (wait_for_hit)
    {
        if (cur_command < instructions.size() && instructions[cur_command].command == CMD_DETECTOR_WAITFORHIT)
            NextCommand(NULL);
        wait_for_hit = false;
        SetActState(ACTSTATE_ACTIVE);
        EraseColls();
        return true;
    }
    if (dyn_colls)
    {
        SetEvent(EVENT_DETECTOR_HIT);
        return true;
    }
    return false;
}

// A collision for each of the frame's dummies whose property starts with DYNB, counted first, unless dynamic; with
// none, one around the frame's hierarchy (a dummy's box in place of a dummy's bound volume), its sphere and its box
// scaled by the frame's world scale, its matrices the frame's rotation at its world position
// FUNCTION: GAME 0x4330b0
void C_detector::CreateColls(bool dynamic)
{
    num_colls = 0;
    if (!dynamic)
        frame->EnumFrames(S_hlp::cbDummyCount, (uint)this, ENUMF_DUMMY, NULL);
    if (num_colls)
    {
        colls = new tDynamicCollObject *[num_colls];
        coll_matrices = new S_matrix[num_colls];
        coll_inv_matrices = new S_matrix[num_colls];
        num_colls = 0;
        frame->EnumFrames(S_hlp::cbDummyCreate, (uint)this, ENUMF_DUMMY, NULL);
        return;
    }
    colls = new tDynamicCollObject *[1];
    coll_matrices = new S_matrix[1];
    coll_inv_matrices = new S_matrix[1];
    I3D_bvolume bvol;
    if (frame->GetType() == FRAME_DUMMY)
        bvol.bbox = ((I3D_dummy *)frame)->bbox;
    else
        bvol = frame->GetHRBoundVolume();
    S_vector half = bvol.bbox.max - bvol.bbox.min;
    half *= 0.5f;
    float radius = half.Magnitude();
    radius *= frame->GetWorldScale().y;
    colls[num_colls] = new tDynamicCollObject;
    memset(colls[num_colls], 0, sizeof(tDynamicCollObject));
    colls[num_colls]->type = DYNCOLL_UNKNOWN_40;
    colls[num_colls]->flags = DYNCOLLF_UNKNOWN_4000 | DYNCOLLF_UPPER;
    ((I3D_bsphere &)colls[num_colls]->bsphere).pos = half + bvol.bbox.min;
    ((I3D_bsphere &)colls[num_colls]->bsphere).pos *= frame->GetMatrix();
    ((I3D_bsphere &)colls[num_colls]->bsphere).radius = radius;
    bvol.bbox.min *= frame->GetWorldScale();
    bvol.bbox.max *= frame->GetWorldScale();
    (I3D_bbox &)colls[num_colls]->bbox = bvol.bbox;
    S_matrix m = frame->GetWorldRot().RotationMatrix();
    m(3) = frame->GetWorldPos();
    coll_matrices[num_colls] = m;
    colls[num_colls]->matrix = &coll_matrices[num_colls];
    coll_inv_matrices[num_colls] = m;
    coll_inv_matrices[num_colls].Invert();
    colls[num_colls]->inv_matrix = &coll_inv_matrices[num_colls];
    colls[num_colls]->frame = frame;
    colls[num_colls]->owner = this;
    gifCollision.DynInsert(colls[num_colls]);
    num_colls++;
}

// FUNCTION: GAME 0x433620
I3DENUMRET __stdcall C_detector::S_hlp::cbDummyCount(I3D_frame *frm, uint user)
{
    C_detector *det = (C_detector *)user;
    if (!det)
        return I3DENUMRET_OK;
    const char *prop = frm->GetProperty();
    // STRING: GAME 0x63243c
    if (strlen(prop) >= 4 && !memcmp(prop, "DYNB", 4))
        det->num_colls++;
    return I3DENUMRET_OK;
}

// The dummy's collision, as CreateColls' around the frame but at the dummy: its box, its world scale, rotation and
// position; the collision's frame is the detector's
// FUNCTION: GAME 0x433670
I3DENUMRET __stdcall C_detector::S_hlp::cbDummyCreate(I3D_frame *frm, uint user)
{
    C_detector *det = (C_detector *)user;
    if (!det)
        return I3DENUMRET_OK;
    const char *prop = frm->GetProperty();
    if (strlen(prop) >= 4 && !memcmp(prop, "DYNB", 4))
    {
        I3D_bbox bbox = ((I3D_dummy *)frm)->bbox;
        float radius = (bbox.max - bbox.min).Magnitude() * .5f;
        radius *= frm->GetWorldScale().y;
        det->colls[det->num_colls] = new tDynamicCollObject;
        memset(det->colls[det->num_colls], 0, sizeof(tDynamicCollObject));
        det->colls[det->num_colls]->type = DYNCOLL_UNKNOWN_40;
        det->colls[det->num_colls]->flags = DYNCOLLF_UNKNOWN_4000;
        ((I3D_bsphere &)det->colls[det->num_colls]->bsphere).pos = frm->GetWorldPos();
        ((I3D_bsphere &)det->colls[det->num_colls]->bsphere).radius = radius;
        bbox.min *= frm->GetWorldScale();
        bbox.max *= frm->GetWorldScale();
        (I3D_bbox &)det->colls[det->num_colls]->bbox = bbox;
        S_matrix m = frm->GetWorldRot().RotationMatrix();
        m(3) = frm->GetWorldPos();
        det->coll_matrices[det->num_colls] = m;
        det->colls[det->num_colls]->matrix = &det->coll_matrices[det->num_colls];
        det->coll_inv_matrices[det->num_colls] = m;
        det->coll_inv_matrices[det->num_colls].Invert();
        det->colls[det->num_colls]->inv_matrix = &det->coll_inv_matrices[det->num_colls];
        det->colls[det->num_colls]->frame = det->frame;
        det->colls[det->num_colls]->owner = det;
        gifCollision.DynInsert(det->colls[det->num_colls]);
        det->num_colls++;
    }
    return I3DENUMRET_OK;
}

// From the last one, each taken from the game's collisions first. Each array is tested before its delete, which VC6
// doesn't do itself (PS2 tests them too).
// FUNCTION: GAME 0x433ae0
void C_detector::EraseColls()
{
    for (int i = num_colls; i--;)
    {
        if (colls[i])
        {
            gifCollision.DynErase(colls[i]);
            delete colls[i];
        }
    }
    num_colls = 0;
    if (colls)
        delete[] colls;
    colls = NULL;
    if (coll_matrices)
        delete[] coll_matrices;
    coll_matrices = NULL;
    if (coll_inv_matrices)
        delete[] coll_inv_matrices;
    coll_inv_matrices = NULL;
}

// The using object at the frame, 1.5 around it (the square of the distance), the special action its item
// FUNCTION: GAME 0x433b80
void C_detector::CreateUO(int action, bool in_sector)
{
    using_object.pos = frame->GetWorldPos();
    using_object.radius_2 = 1.5f;
    using_object.matrix = NULL;
    using_object.flags = 0;
    using_object.frame = NULL;
    using_object.actor = this;
    using_object.item.id = GAMEITEM_SPECIAL_ACTION;
    using_object.item.ammo_loaded = action;
    using_object.item.using_object = &using_object;
    if (in_sector)
    {
        using_object.sector = frame;
        while (using_object.sector && using_object.sector->GetType() != FRAME_SECTOR)
            using_object.sector = using_object.sector->GetParent();
    }
    else
        using_object.sector = NULL;
    act_mission->game->using_object.AddObject(&using_object);
    wait_for_use = true;
    SetActState(ACTSTATE_INACTIVE);
}

// FUNCTION: GAME 0x433c60
void C_detector::UseDetector()
{
    if (!wait_for_use)
        return;
    wait_for_use = false;
    SetActState(ACTSTATE_ACTIVE);
    act_mission->game->using_object.DelObject(&using_object);
    if (cur_command < instructions.size() && instructions[cur_command].command == CMD_DETECTOR_WAITFORUSE)
        NextCommand(NULL);
}

// FUNCTION: GAME 0x433ce0
bool C_detector::DecodeParams(char *params, S_instruction &ins)
{
    return C_program::DecodeParams(params, ins);
}

// FUNCTION: GAME 0x433cf0
int C_detector::DecodeInstruction(char *line, int &len)
{
    return C_program::DecodeInstruction(line, len);
}

// FUNCTION: GAME 0x433d00
void C_detector::DecompileCommand(int i, char *s)
{
    C_program::DecompileCommand(i, s);
}

// The detector's commands, PS2's in PS2's order; the others are C_program's, which adds their time itself. The rest
// add the time the command has run, through a flag as PS2's, which VC6 threads into each case.
// FUNCTION: GAME 0x433d10
void C_detector::Process(uint time)
{
    if (cur_command >= instructions.size())
        return;
    S_instruction &ins = instructions[cur_command];
    bool add_time = true;
    switch (ins.command)
    {
    // Inactive where the script runs no other's code
    case CMD_PROGRAM_STORAGE:
        if (!saved_code)
            SetActState(ACTSTATE_NONE);
        else
            NextCommand(NULL);
        break;
    // Counts into the register the entities and the player near enough to the detector, of the groups it watches
    case CMD_DETECTOR_INRANGE:
    {
        S_int_float_params *p = (S_int_float_params *)ins.params;
        float range = p->value2 * p->value2;
        if (num_float_regs > p->value1 && p->value1 >= 0)
        {
            float_regs[p->value1] = 0.0f;
            std::vector<C_actor *> &acts = act_mission->game->active_actors;
            for (int i = acts.size(); i--;)
            {
                C_actor *act = acts[i];
                if ((act->type == ACTOR_ENTITY || act->type == ACTOR_PLAYER)
                    && ((C_human *)act)->human_type & human_types)
                {
                    S_vector d = frame->GetWorldPos() - act->frame->GetWorldPos();
                    if (d.x * d.x + d.y * d.y + d.z * d.z <= range)
                        float_regs[p->value1] += 1.0f;
                }
            }
        }
        NextCommand(NULL);
        break;
    }
    // Waits until the player uses it
    case CMD_DETECTOR_WAITFORUSE:
    {
        int *p = (int *)ins.params;
        act_mission->game->using_object.DelObject(&using_object);
        CreateUO(p[0], !p[1]);
        break;
    }
    // Waits for a hit, with collisions that don't move
    case CMD_DETECTOR_WAITFORHIT:
        if (!dyn_colls)
        {
            wait_for_hit = true;
            SetActState(ACTSTATE_INACTIVE);
            EraseColls();
            CreateColls(false);
        }
        else
            NextCommand(NULL);
        break;
    case CMD_DETECTOR_CREATEDYNCOLL:
        if (!wait_for_hit && !dyn_colls)
        {
            dyn_colls = true;
            EraseColls();
            CreateColls(true);
        }
        NextCommand(NULL);
        break;
    case CMD_DETECTOR_ERASEDYNCOLL:
        if (!wait_for_hit && dyn_colls)
        {
            dyn_colls = false;
            EraseColls();
        }
        NextCommand(NULL);
        break;
    default:
        C_program::Process(time);
        add_time = false;
    }
    if (add_time)
        command_time += time;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_detector::NetDirect(uint time)
{
}
