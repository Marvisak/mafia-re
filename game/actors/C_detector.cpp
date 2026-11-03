// The script objects (PS2 C_detector.cpp; the PC's file is the one at 0x432870-0x433fdc, docs/game-files.md), whose
// constructor (0x432880) comes first. Written so far: the constructor and Init, which C_mission::CreateActor and
// OpenBin call, the destructor, LoadData, which OpenBin calls next, and GameInit and GameDone, which C_game::Init and
// Done call, with EraseColls (0x433ae0), AI and Update, which C_actor::Tick calls, and Process, the detector's
// script commands. Not written: the rest; the vtable's DuplicateFrom (0x4329c0) and its save functions, and
// CreateColls and CreateUO, which Process calls, are stubs.

#include "actors/C_detector.h"

#include "C_chunk.h"
#include "actors/C_human.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "collision/g_colls.h"
#include "stub.h"

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

#pragma auto_inline(off)

// STUB: GAME 0x4329c0
void C_detector::DuplicateFrom(C_actor *act)
{
    StubLog("C_detector::DuplicateFrom");
}

#pragma auto_inline(on)

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
    unknown_dd = false;
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

// A byte nothing reads, the dword at 0xd8, then the script, as PS2's
// FUNCTION: GAME 0x432b20
bool C_detector::LoadData(C_chunk *chunk)
{
    uchar unknown;
    chunk->Read(&unknown, sizeof(unknown));
    chunk->Read(&human_types, sizeof(human_types));
    LoadSourceCode(chunk);
    return true;
}

#pragma auto_inline(off)

// STUB: GAME 0x432c10
int C_detector::SaveGameGetSize()
{
    StubLog("C_detector::SaveGameGetSize");
    return 0;
}

// STUB: GAME 0x432c30
bool C_detector::SaveGameSave(void **buf)
{
    StubLog("C_detector::SaveGameSave");
    return false;
}

// STUB: GAME 0x432ca0
bool C_detector::SaveGameLoad(void **buf)
{
    StubLog("C_detector::SaveGameLoad");
    return false;
}

#pragma auto_inline(on)

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
            SetActState(ACTSTATE_2);
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
            SetActState(ACTSTATE_1);
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

#pragma auto_inline(off)

// STUB: GAME 0x4330b0
void C_detector::CreateColls(bool dynamic)
{
    StubLog("C_detector::CreateColls");
}

// STUB: GAME 0x433b80
void C_detector::CreateUO(int unknown, bool unknown2)
{
    StubLog("C_detector::CreateUO");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x432ac0
bool C_detector::SaveData(C_chunk *chunk)
{
    StubLog("C_detector::SaveData");
    C_actor::SaveData(chunk);
    return true;
}

// STUB: GAME 0x433020
bool C_detector::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_detector::Hit");
    return C_actor::Hit(type, v1, v2, v3, f, act, u, frm);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_detector::NetDirect(uint time)
{
}

#pragma auto_inline(on)
