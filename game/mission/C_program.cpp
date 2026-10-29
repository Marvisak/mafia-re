// The scripts (PS2 C_program.cpp; the PC's file is most likely the one at 0x460a50-0x47c31c, whose strings are the
// script language's, docs/game-files.md, and which starts with the changes' initializer). Written so far: the
// constructor, the destructor and LoadSourceCode, which C_mission::OpenBin and Close call, the patch's changes,
// LoadPatchFile, the helper it reads their strings through, and UnloadPatchFiles, TryChangeProgramData with
// EqualFinalPath, and SetSourceCode, which LoadSourceCode calls; Init and Done, which the mission's and the game's Init
// and Done call, the compiler Init calls (Compile, DecodeInstruction with its table of the commands' names, and
// DecodeParams with its helpers, ExtrudeParams to DecodeIf, and its names' tables), DecompileCommand, PS2's helpers in
// PS2's order (the registers' getters and setters, NextCommand, LeaveCompactBlock, ClearAfterEnd, SetEvent, IsEventID,
// InvalidateActorReg, FindActorLater), CallSubroutine and CallProcess, and Process, the interpreter (0x46d3b0-0x4799b9,
// a switch of 396 commands, about 50 KB of code). Not written: DuplicateProgramFrom (0x4617d0, between Done and
// ExtrudeParams), which the detector's and the AI humans' Duplicate call. Stubs: SaveSourceCode and the save functions.
// Process is too large for VC6 to inline anything in it: every inline function it uses is called out of line, and
// their copies are the file's end (0x47a070-0x47c31c), the precompiled headers' deferred items in header order.
// Its cases are in the original's order (the bodies' order, one jump table). Open: the other commands' functions are
// mostly stubs of their files, and Process matches only in part (registers, frame, the copies' order).
// Open: the changes' destructor (0x460a90, registered by 0x460a80) tests and sets a guard byte (0x63d774) of its own,
// as C_animations_machine_de_luxe::tracks' does. Compile: the original stores the line's end pointer to its stack home
// at the top of the scan loop (ours after it), and its slot and the error count's are swapped. DecodeInstruction: VC6
// schedules the table's stores differently (the stores themselves are the original's).

#include "mission/C_program.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "C_chunk.h"
#include "I3D_cache.h"
#include "IGraph/IGraph.h"
#include "actors/C_PhThings.h"
#include "actors/C_actor.h"
#include "actors/C_bridge.h"
#include "actors/C_detector.h"
#include "actors/C_door.h"
#include "actors/C_drop_in_weapon.h"
#include "actors/C_human.h"
#include "actors/C_player.h"
#include "actors/C_public_physics.h"
#include "actors/C_pumper.h"
#include "actors/g_inventory.h"
#include "ai/C_person_traffic.h"
#include "ai/C_shvestky.h"
#include "ai/C_sledovacka.h"
#include "ai/jp_ai_entity.h"
#include "ai/jp_ai_groups.h"
#include "ai/jp_ai_main.h"
#include "ai/jp_ai_prislusnici.h"
#include "camera/g_camera.h"
#include "collision/g_colls.h"
#include "g_material.h"
#include "input/controls.h"
#include "mission/C_cutscene.h"
#include "mission/C_effects.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "mission/m_cache.h"
#include "stub.h"
#include "tools.h"
#include "ui/g_dabing.h"
#include "ui/g_indicators.h"
#include "ui/g_loadsave.h"
#include "ui/g_m_func.h"
#include "ui/g_menu.h"
#include "ui/racing.h"
#include "vehicles/C_Vehicle.h"
#include "vehicles/C_airplane.h"
#include "vehicles/C_car.h"
#include "vehicles/C_car_ex.h"
#include "vehicles/C_wagon.h"

// CITYCACHING_TICK's time for C_cache_base_block::Tick: all of it at once (the name is ours)
#define CITYCACHING_TICK_TIME 0xfffffff

// GLOBAL: GAME 0x63d778
std::vector<C_program::S_prog_change> C_program::change_progs;

// The vectors' allocators are copies of their default arguments' temporaries, an uninitialized byte each
// FUNCTION: GAME 0x460ad0
C_program::C_program() :
    name(NULL),
    source(NULL),
    actor(NULL),
    float_regs(NULL),
    num_float_regs(0),
    frame_regs(NULL),
    num_frame_regs(0),
    actor_regs(NULL),
    num_actor_regs(0),
    ai_priority(.5f)
{
}

// FUNCTION: GAME 0x460b40
C_program::~C_program()
{
    if (name)
        delete[] name;
    if (source)
        delete[] source;
}

#pragma auto_inline(off)

// STUB: GAME 0x460ba0
bool C_program::SaveSourceCode(C_chunk *chunk)
{
    StubLog("C_program::SaveSourceCode");
    return true;
}

#pragma auto_inline(on)

// A byte nothing reads, then the name and the source, each as its length and its bytes, as PS2's
// FUNCTION: GAME 0x460c90
bool C_program::LoadSourceCode(C_chunk *chunk)
{
    if (name)
    {
        delete[] name;
        name = NULL;
    }
    if (source)
    {
        delete[] source;
        source = NULL;
    }
    uchar unknown;
    chunk->Read(&unknown, sizeof(unknown));
    int len;
    chunk->Read(&len, sizeof(len));
    if (len)
    {
        name = new char[len + 1];
        chunk->Read(name, len);
        name[len] = 0;
    }
    chunk->Read(&len, sizeof(len));
    if (len)
    {
        source = new char[len + 1];
        chunk->Read(source, len);
        source[len] = 0;
    }
    TryChangeProgramData();
    return true;
}

// The patch files' chunk of changes (the name is ours)
#define CT_PROG_CHANGES 0x64

static void ReadPatchString(C_chunk &chunk, char *s, int len, int index);

// The changes' chunk holds the four strings of each change, each as its length and its bytes, or a zero length (1.2's
// four files hold 23 changes, each with the mission, a frame's name and the script's source, no script name)
// FUNCTION: GAME 0x460f20
void LoadPatchFile(const char *name)
{
    C_chunk chunk;
    chunk.OpenFile(name, false);
    ushort type;
    if (chunk.RAscend(type) && type == CT_PROG_CHANGES)
    {
        for (int i = 0; chunk.Size(); i++)
        {
            C_program::S_prog_change change;
            int len;
            chunk.Read(&len, sizeof(len));
            if (len)
            {
                change.mission = new char[len];
                ReadPatchString(chunk, change.mission, len, i);
            }
            else
                change.mission = NULL;
            chunk.Read(&len, sizeof(len));
            if (len)
            {
                change.program = new char[len];
                ReadPatchString(chunk, change.program, len, i);
            }
            else
                change.program = NULL;
            chunk.Read(&len, sizeof(len));
            if (len)
            {
                change.frame = new char[len];
                ReadPatchString(chunk, change.frame, len, i);
            }
            else
                change.frame = NULL;
            chunk.Read(&len, sizeof(len));
            if (len)
            {
                change.source = new char[len];
                ReadPatchString(chunk, change.source, len, i);
            }
            else
                change.source = NULL;
            C_program::change_progs.push_back(change);
        }
    }
    chunk.Descend();
    chunk.Close();
}

// Reads a string of len bytes, which the patch files store with each byte xored with a key that depends on the
// change's index (PC only, the name is ours)
// FUNCTION: GAME 0x461270
static void ReadPatchString(C_chunk &chunk, char *s, int len, int index)
{
    int key = index % 13 + 0x85;
    chunk.Read(s, len);
    for (int i = 0; i < len; i++)
        s[i] ^= key;
}

// The last change first
// FUNCTION: GAME 0x461310
void UnloadPatchFiles()
{
    for (int i = C_program::change_progs.size(); i--;)
    {
        delete[] C_program::change_progs[i].mission;
        delete[] C_program::change_progs[i].program;
        delete[] C_program::change_progs[i].frame;
        delete[] C_program::change_progs[i].source;
    }
    C_program::change_progs.clear();
}

// Whether the two paths end in the same name, case aside (PS2 name)
// FUNCTION: GAME 0x4613c0
bool EqualFinalPath(const char *path1, const char *path2)
{
    const char *name1 = strrchr(path1, '\\');
    name1 = name1 ? name1 + 1 : path1;
    const char *name2 = strrchr(path2, '\\');
    name2 = name2 ? name2 + 1 : path2;
    return !_stricmp(name1, name2);
}

// The frame's name read twice, its test (GetName never returns NULL) compiled away
// FUNCTION: GAME 0x461420
void C_program::TryChangeProgramData()
{
    int found = -1;
    for (int i = change_progs.size(); i--;)
    {
        if (!EqualFinalPath(change_progs[i].mission, act_mission->mission_name))
            continue;
        bool frame_fits = false;
        bool program_fits = false;
        if (name && change_progs[i].program)
        {
            if (!_stricmp(name, change_progs[i].program))
                program_fits = true;
        }
        else if (!name && !change_progs[i].program)
            program_fits = true;
        if (actor && actor->frame)
        {
            if (actor->frame->GetName() && change_progs[i].frame)
            {
                if (!_stricmp(actor->frame->GetName(), change_progs[i].frame))
                    frame_fits = true;
            }
        }
        else if (!change_progs[i].frame)
            frame_fits = true;
        if (program_fits && frame_fits)
        {
            found = i;
            break;
        }
    }
    if (found > -1)
        SetSourceCode(change_progs[found].source);
}

// FUNCTION: GAME 0x461530
void C_program::SetSourceCode(char *src)
{
    if (source)
    {
        delete[] source;
        source = NULL;
    }
    if (src)
    {
        source = new char[strlen(src) + 1];
        strcpy(source, src);
    }
}

// Both clear the registers the same way (PS2's too)
// FUNCTION: GAME 0x461580
void C_program::Init()
{
    cur_command = 0;
    prev_command = -1;
    command_time = 0;
    gosub_stack.clear();
    saved_code = NULL;
    run_through = false;
    in_block = false;
    block_mode = 0;
    ai_priority = .5f;
    stop_at_return = false;
    if (float_regs)
        delete[] float_regs;
    float_regs = NULL;
    num_float_regs = 0;
    if (frame_regs)
        delete[] frame_regs;
    frame_regs = NULL;
    num_frame_regs = 0;
    if (actor_regs)
        delete[] actor_regs;
    actor_regs = NULL;
    num_actor_regs = 0;
    instructions.clear();
    if (source)
    {
        // Compile writes into the lines
        int err_len, err_pos;
        char *src = new char[strlen(source) + 1];
        strcpy(src, source);
        Compile(src, false, &err_pos, &err_len);
        delete[] src;
    }
    if (!instructions.size())
        ai_priority = 0;
}

// The parameters are the script's own commands', the oldest saved code's while it runs another script's
// FUNCTION: GAME 0x4616a0
void C_program::Done()
{
    gosub_stack.clear();
    if (saved_code)
    {
        S_saved_code *sc = saved_code;
        while (sc->next)
            sc = sc->next;
        for (int i = sc->instructions.size(); i--;)
        {
            if (sc->instructions[i].params)
                delete sc->instructions[i].params;
        }
    }
    else
    {
        for (int i = instructions.size(); i--;)
        {
            if (instructions[i].params)
                delete instructions[i].params;
        }
    }
    instructions.clear();
    while (saved_code)
    {
        S_saved_code *sc = saved_code;
        saved_code = saved_code->next;
        delete sc;
    }
    if (float_regs)
        delete[] float_regs;
    float_regs = NULL;
    num_float_regs = 0;
    if (frame_regs)
        delete[] frame_regs;
    frame_regs = NULL;
    num_frame_regs = 0;
    if (actor_regs)
        delete[] actor_regs;
    actor_regs = NULL;
    num_actor_regs = 0;
}

// The commas become the parameters' ends, and so do the spaces after them; a last parameter only if it isn't empty
// FUNCTION: GAME 0x461830
void C_program::ExtrudeParams(char *s, int &num, char **params)
{
    num = 0;
    params[0] = s;
    int len = strlen(s);
    int i;
    for (i = 0; i < len; i++)
    {
        if (s[i] == ',')
        {
            s[i] = 0;
            num++;
            if (num > MAX_PARAMS - 1)
                break;
            params[num] = &s[i + 1];
        }
    }
    if (num < MAX_PARAMS && strlen(params[num]))
        num++;
    for (i = 0; i < num; i++)
    {
        while (*params[i] == ' ')
            params[i]++;
        if (strlen(params[i]))
        {
            char *e = params[i] + strlen(params[i]) - 1;
            while (*e == ' ' && e > params[i])
                *e-- = 0;
        }
    }
}

// FUNCTION: GAME 0x461900
bool C_program::DecodeOperand(char *s, int &reg, float &value)
{
    while (*s && *s == ' ')
        s++;
    while (s[strlen(s) - 1] == ' ')
        s[strlen(s) - 1] = 0;
    // STRING: GAME 0x6327d0
    if (_strnicmp(s, "flt[", 4))
    {
        reg = -1;
        value = (float)atof(s);
        return true;
    }
    char *e = strchr(s + 4, ']');
    if (!e)
        return false;
    *e = 0;
    reg = atoi(s + 4);
    return true;
}

// FUNCTION: GAME 0x4619b0
bool C_program::DecodeOperandInt(char *s, int &reg, int &value)
{
    while (*s && *s == ' ')
        s++;
    while (s[strlen(s) - 1] == ' ')
        s[strlen(s) - 1] = 0;
    if (_strnicmp(s, "flt[", 4))
    {
        reg = -1;
        value = atoi(s);
        return true;
    }
    char *e = strchr(s + 4, ']');
    if (!e)
        return false;
    *e = 0;
    reg = atoi(s + 4);
    return true;
}

// FUNCTION: GAME 0x461a60
char *C_program::ExtrudeString(char *s)
{
    while (*s && *s == ' ')
        s++;
    if (*s != '"')
        return NULL;
    s++;
    char *e = strchr(s, '"');
    if (!e)
        return NULL;
    *e = 0;
    return s;
}

// flt[dest] = a, then the first of the operators found in that order, + / * -, splits a from b. The operands' results
// aren't tested.
// FUNCTION: GAME 0x461aa0
bool C_program::DecodeLet(S_instruction &ins, char *s)
{
    while (*s && *s == ' ')
        s++;
    if (_strnicmp(s, "flt[", 4))
        return false;
    s += 4;
    char *e = strchr(s, ']');
    if (!e)
        return false;
    S_let_params *let = (S_let_params *)ins.params;
    *e = 0;
    let->dest = atoi(s);
    char *eq = strchr(e + 1, '=');
    if (!eq)
        return false;
    eq++;
    char *op;
    if ((op = strchr(eq, '+')) != NULL)
        let->op = LET_OP_ADD;
    else if ((op = strchr(eq, '/')) != NULL)
        let->op = LET_OP_DIV;
    else if ((op = strchr(eq, '*')) != NULL)
        let->op = LET_OP_MUL;
    else if ((op = strchr(eq, '-')) != NULL)
        let->op = LET_OP_SUB;
    else
        let->op = LET_OP_NONE;
    if (op)
    {
        *op = 0;
        DecodeOperand(eq, let->a.reg, let->a.value);
        DecodeOperand(op + 1, let->b.reg, let->b.value);
    }
    else
    {
        DecodeOperand(eq, let->a.reg, let->a.value);
        let->b.reg = -1;
    }
    return true;
}

// The first of the comparisons found in that order, < > = !, splits a from b
// FUNCTION: GAME 0x461bd0
bool C_program::DecodeIf(S_instruction &ins, char *s)
{
    S_if_params *cond = (S_if_params *)ins.params;
    char *op;
    if ((op = strchr(s, '<')) != NULL)
        cond->op = IF_OP_LESS;
    else if ((op = strchr(s, '>')) != NULL)
        cond->op = IF_OP_GREATER;
    else if ((op = strchr(s, '=')) != NULL)
        cond->op = IF_OP_EQUAL;
    else if ((op = strchr(s, '!')) != NULL)
        cond->op = IF_OP_NOT_EQUAL;
    if (!op)
        return false;
    *op = 0;
    DecodeOperand(s, cond->a.reg, cond->a.value);
    DecodeOperand(op + 1, cond->b.reg, cond->b.value);
    return true;
}

// The humans' properties, HUMAN_SETPROPERTY's and HUMAN_GETPROPERTY's, by their index (PS2 name)
#define NUM_HUMAN_PROPERTIES 15
// GLOBAL: GAME 0x624150
static const char _human_property_[NUM_HUMAN_PROPERTIES][16] = {
    "Strenght",
    "Energy",
    "EnergyHandL",
    "EnergyHandR",
    "EnergyLegL",
    "EnergyLegR",
    "Reactions",
    "Speed",
    "Aggresivity",
    "Intelligence",
    "Shooting",
    "Sight",
    "Hearing",
    "Driving",
    "Mass"
};

// WEATHER_SETPARAM's parameters, WEATHER_* (PS2 name)
// GLOBAL: GAME 0x624240
static const char _weather_property_[NUM_WEATHER_PROPERTIES][16] = {
    "ON",
    "SECTORS",
    "DUMMIES",
    "COLORH",
    "COLORL",
    "SPEED",
    "LEN",
    "WIDTH",
    "MAX_DIST",
    "MAX_HEIGHT",
    "MAX_CNT",
    "DIR_X",
    "DIR_Y",
    "DIR_Z",
    "MODE"
};

// A value the scripts give a name (the names are ours)
struct S_named_value
{
    char name[16];
    int value;
};

// FINDNEARACTOR's actor types, ACTOR_* (PS2 name)
#define NUM_ACTOR_TYPES 16
// GLOBAL: GAME 0x624330
static const S_named_value _actor_type_[NUM_ACTOR_TYPES] = {
    {"Player", ACTOR_PLAYER},
    {"Enemy", ACTOR_ENTITY},
    {"Car", ACTOR_CAR},
    {"Detector", ACTOR_DETECTOR},
    {"Door", ACTOR_DOOR},
    {"Railway", ACTOR_RAILWAY},
    {"Box", ACTOR_DYN_OBJECT},
    {"Bottle", ACTOR_BOTTLE},
    {"Traffic_person", ACTOR_TRAFFIC_GENERATOR},
    {"Traffic_car", ACTOR_TRAFFIC_CAR},
    {"Bridge", ACTOR_BRIDGE},
    {"Dog", ACTOR_DOG},
    {"Airplane", ACTOR_AIRPLANE},
    {"Turnout", ACTOR_TURNOUT},
    {"Pumper", ACTOR_PUMPER},
    {"Race camera", ACTOR_RACECAM}
};

// CTRL_READ's controls, CTRL_* (PS2 name): the walking ones, the driving ones at the walking ones' indices, the rest;
// the last three entries are empty
#define NUM_CTRLS_TYPES 50
// GLOBAL: GAME 0x624470
static const S_named_value _ctrls_type_[NUM_CTRLS_TYPES] = {
    {"UP", CTRL_FORWARD},
    {"UP1", CTRL_FORWARD + CTRL_SECONDARY},
    {"DOWN", CTRL_BACKWARD},
    {"DOWN1", CTRL_BACKWARD + CTRL_SECONDARY},
    {"LEFT", CTRL_LEFT},
    {"LEFT1", CTRL_LEFT + CTRL_SECONDARY},
    {"RIGHT", CTRL_RIGHT},
    {"RIGHT1", CTRL_RIGHT + CTRL_SECONDARY},
    {"RUN", CTRL_WALK},
    {"RUN1", CTRL_WALK + CTRL_SECONDARY},
    {"ACTION", CTRL_ACTION},
    {"ACTION1", CTRL_ACTION + CTRL_SECONDARY},
    {"FIRE", CTRL_FIRE},
    {"FIRE1", CTRL_FIRE + CTRL_SECONDARY},
    {"CROUCH", CTRL_CROUCH},
    {"CROUCH1", CTRL_CROUCH + CTRL_SECONDARY},
    {"JUMP", CTRL_JUMP},
    {"JUMP1", CTRL_JUMP + CTRL_SECONDARY},
    {"DRVUP", CTRL_FORWARD},
    {"DRVUP1", CTRL_FORWARD + CTRL_SECONDARY},
    {"DRVDOWN", CTRL_BACKWARD},
    {"DRVDOWN1", CTRL_BACKWARD + CTRL_SECONDARY},
    {"DRVLEFT", CTRL_LEFT},
    {"DRVLEFT1", CTRL_LEFT + CTRL_SECONDARY},
    {"DRVRIGHT", CTRL_RIGHT},
    {"DRVRIGHT1", CTRL_RIGHT + CTRL_SECONDARY},
    {"HANDBRAKE", CTRL_HANDBRAKE},
    {"HANDBRAKE1", CTRL_HANDBRAKE + CTRL_SECONDARY},
    {"MOTORSWITCH", CTRL_GEARING},
    {"GEARUP", CTRL_GEAR_UP},
    {"GEARUP1", CTRL_GEAR_UP + CTRL_SECONDARY},
    {"GEARDOWN", CTRL_GEAR_DOWN},
    {"GEARDOWN1", CTRL_GEAR_DOWN + CTRL_SECONDARY},
    {"HORN", CTRL_HORN},
    {"HORN1", CTRL_HORN + CTRL_SECONDARY},
    {"CLUTCH", CTRL_CLUTCH},
    {"CLUTCH1", CTRL_CLUTCH + CTRL_SECONDARY},
    {"INVENTORY", CTRL_INVENTORY},
    {"HOLSTER", CTRL_HIDE_WEAPON},
    {"WEAPONDROP", CTRL_DROP_WEAPON},
    {"RUNSWITCH", CTRL_TOGGLE_RUN},
    {"RELOAD", CTRL_RELOAD},
    {"SNIPERMODE", CTRL_SNIPER},
    {"OBJECTIVES", CTRL_OBJECTIVES},
    {"MAP", CTRL_MAP},
    {"SPEEDLIMIT", CTRL_SPEED_LIMITER},
    {"SPEEDLIMIT1", CTRL_SPEED_LIMITER + CTRL_SECONDARY}
};

// The parameters are the text after the command's name, split at its commas. The cases in PS2's order, a body for each
// group of commands the original's switch shares one for; a command without one has none. A number of parameters
// other than the command's fails, as does a label too long for its array, and those failures leave ins.params as it
// was. BEEP, DEBUG_TEXT and the bracketed debugging commands always fail.
// FUNCTION: GAME 0x461c70
bool C_program::DecodeParams(char *params, S_instruction &ins)
{
    int num;
    char *p[MAX_PARAMS];
    ExtrudeParams(params, num, p);
    switch (ins.command)
    {
    case CMD_EVENT:
        if (num == 1 || num == 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            // STRING: GAME 0x632860
            if (!_stricmp(p[0], "use_AB"))
                v[0] = EVENT_USE_AB;
            // STRING: GAME 0x632858
            else if (!_stricmp(p[0], "death"))
                v[0] = EVENT_DEATH;
            // STRING: GAME 0x63284c
            else if (!_stricmp(p[0], "enemy_to_AI"))
                v[0] = EVENT_ENEMY_TO_AI;
            // STRING: GAME 0x63283c
            else if (!_stricmp(p[0], "enemy_to_Script"))
                v[0] = EVENT_ENEMY_TO_SCRIPT;
            // STRING: GAME 0x63282c
            else if (!_stricmp(p[0], "hit_from_player"))
                v[0] = EVENT_HIT_FROM_PLAYER;
            // STRING: GAME 0x63281c
            else if (!_stricmp(p[0], "detector_hit"))
                v[0] = EVENT_DETECTOR_HIT;
            else
                v[0] = atoi(p[0]);
            if (num == 2)
                v[1] = atoi(p[1]);
            else
                v[1] = 0;
            return true;
        }
        break;
    case CMD_BLOCK_BEGIN:
        if (num < 2)
        {
            ins.params = new int;
            *(int *)ins.params = BLOCK_MODE_NONE;
            if (num == 1)
            {
                // STRING: GAME 0x632818
                if (!_strnicmp(p[0], "UP", 2))
                    *(int *)ins.params = BLOCK_MODE_UP;
                // STRING: GAME 0x632810
                else if (!_strnicmp(p[0], "DOWN", 4))
                    *(int *)ins.params = BLOCK_MODE_DOWN;
            }
            return true;
        }
        break;
    case CMD_DIM_FLT: case CMD_DIM_FRM: case CMD_DIM_ACT: case CMD_CALL: case CMD_STOPSOUND: case CMD_SETCOMPASS:
    case CMD_COMMANDBLOCK: case CMD_AIRPLANE_START2: case CMD_EVENT_USE_CB: case CMD_GAME_NIGHTMISSION:
    case CMD_GETMISSIONNUMBER: case CMD_GARAGE_ADDCAR: case CMD_GARAGE_DELCAR: case CMD_PLAYER_LOCKCONTROLS:
    case CMD_PERSON_STOPANIM: case CMD_ENEMY_FORCESCRIPT: case CMD_DISABLECOLLS: case CMD_STOPPARTICLE:
    case CMD_HUMAN_HOLSTER: case CMD_CAMERA_LOCK: case CMD_SETMISSIONNAMEID: case CMD_MODEL_STOPANIM:
    case CMD_MODEL_DESTROY: case CMD_HUMAN_DEATH: case CMD_HUMAN_ERASEAB: case CMD_HUMAN_CREATEAB:
    case CMD_CAR_FORCESTOP: case CMD_INVENTORY_PUSH: case CMD_INVENTORY_POP: case CMD_FUCKINGBOX_MOVE:
    case CMD_MISSION_OBJECTIVES: case CMD_MISSION_OBJECTIVESREMOVE: case CMD_RACING_MISSION6_START:
    case CMD_CAR_EXPLOSION: case CMD_CAR_REMOVE_DRIVER: case CMD_SETMODELTOCAR: case CMD_STREAM_PLAY:
    case CMD_STREAM_PAUSE: case CMD_STREAM_STOP: case CMD_STREAM_DESTROY: case CMD_HUMAN_FORCEFALL:
    case CMD_DESTROY_PHYSICALOBJECT: case CMD_GETTICKTIME: case CMD_GETGAMETIME: case CMD_GETACTIVEPLAYER:
    case CMD_SETCITYTRAFFICVISIBLE: case CMD_CHARACTER_PUSH: case CMD_CHARACTER_POP: case CMD_CONSOLE_ADDTEXT:
    case CMD_ACTOR_DELETE: case CMD_HUMAN_RESET: case CMD_HUMAN_STOPTALK: case CMD_CARDAMAGEVISIBLE:
    case CMD_SUBTITLE_ADD: case CMD_MATH_COS: case CMD_MATH_SIN: case CMD_MATH_ACOS: case CMD_MATH_ASIN:
    case CMD_MATH_ATAN: case CMD_MATH_ABS: case CMD_VECT_NORMALIZE: case CMD_VECT_INVERSE: case CMD_MATRIX_ZERO:
    case CMD_MATRIX_IDENTITY: case CMD_MATRIX_INVERSE: case CMD_QUAT_INVERSE: case CMD_QUAT_NORMALIZE: case CMD_VLVP:
    case CMD_ACTORUPDATEPLACEMENT: case CMD_POLICEITCHFORPLAYER: case CMD_SETNULLACTOR: case CMD_SETNULLFRAME:
    case CMD_SETFREERIDE: case CMD_ENABLEMAP: case CMD_CAR_CALM: case CMD_GETACTIVECAMERA: case CMD_PM_SHOWSYMBOL:
    case CMD_PM_SHOWPROGRESS: case CMD_PM_SETPROGRESS: case CMD_SETFILMMUSIC: case CMD_FLOATREG_PUSH:
    case CMD_FLOATREG_POP: case CMD_WEATHER_PREPAREBUFFER: case CMD_HUMAN_CANDIE: case CMD_VERSION_IS_GERMANY:
    case CMD_VERSION_IS_EDITOR: case CMD_GARAGE_NEZAHAZUJ: case CMD_CAMERA_GETFOV: case CMD_FREERIDE_SCOREON:
    case CMD_FREERIDE_SCOREGET: case CMD_TAXIDRIVER_ENABLE: case CMD_HUMAN_RETURNTOTRAFF: case CMD_TIMER_GETINTERVAL:
    case CMD_TIMER_SETINTERVAL: case CMD_HUMAN_WAITTOREADY: case CMD_ACTOR_ADDDORICI: case CMD_CAR_REPAIR:
    case CMD_NPC_SHUTUP: case CMD_INVENTORY_CLEAR: case CMD_GETLASTSAVENUM: case CMD_CAR_SETAXIS:
    case CMD_ISVALIDTAXIPASSENGER:
        if (num == 1)
        {
            ins.params = new int;
            *(int *)ins.params = atoi(p[0]);
            return true;
        }
        break;
    case CMD_POLICEMANAGER_ON:
        if (num >= 0 && num <= 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            if (num > 0)
                v[0] = atoi(p[0]);
            else
                v[0] = 980000;
            if (num > 1)
                v[1] = atoi(p[1]);
            else
                v[1] = 0;
            return true;
        }
        break;
    case CMD_RETURN: case CMD_END: case CMD_DETECTOR_WAITFORHIT: case CMD_PROGRAM_STORAGE: case CMD_RECCLEAR:
    case CMD_RECWAITFOREND: case CMD_RECUNLOAD: case CMD_TIMEROFF: case CMD_CLEARDIFFERENCES: case CMD_BLOCK_END:
    case CMD_DIALOG_END: case CMD_GARAGE_GENERATECARS: case CMD_GARAGE_RELEASECARS: case CMD_GARAGE_CARMANAGER:
    case CMD_GUNSHOP_MENU: case CMD_CAMERA_UNLOCK: case CMD_FUCKINGBOX_RECOMPILE: case CMD_MISSION_OBJECTIVESCLEAR:
    case CMD_USE_LIGHTCACHE: case CMD_GARAGE_ADDLASTSTOLEN: case CMD_GARAGE_SHOW: case CMD_WEATHER_RESET:
    case CMD_ENEMY_BLOCK: case CMD_ENEMY_UNBLOCK: case CMD_RACING_AUTOINVISIBLE: case CMD_NOANIMPRELOAD:
    case CMD_CITYMUSIC_OFF: case CMD_CITYMUSIC_ON: case CMD_CITYCACHING_TICK: case CMD_ACTORLIGHTNESSON:
    case CMD_DETECTOR_CREATEDYNCOLL: case CMD_DETECTOR_ERASEDYNCOLL: case CMD_SUBTITLE: case CMD_PLAY_AVI_INTRO:
    case CMD_CARTRIDGE_INVALIDATE: case CMD_FREERIDE_TRAFFSETUP: case CMD_M7_ZASTAVZKURVENEPOLISE:
    case CMD_DAN_INTERNAL_1: case CMD_DAN_INTERNAL_2:
        return true;
    case CMD_DETECTOR_WAITFORUSE:
        if (num >= 0 && num <= 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            if (num > 0)
            {
                v[0] = atoi(p[0]);
                if (num > 1)
                    v[1] = atoi(p[1]);
                else
                    v[1] = 0;
            }
            else
            {
                v[0] = 0;
                v[1] = 0;
            }
            return true;
        }
        break;
    case CMD_CAR_SETPROJECTOR: case CMD_VECT_SET:
        if (num == 4)
        {
            ins.params = new S_vector_params;
            S_vector_params *v = (S_vector_params *)ins.params;
            v->reg = atoi(p[0]);
            v->x = (float)atof(p[1]);
            v->y = (float)atof(p[2]);
            v->z = (float)atof(p[3]);
            return true;
        }
        break;
    case CMD_LET:
        if (num == 1)
        {
            ins.params = new S_let_params;
            if (DecodeLet(ins, p[0]))
                return true;
            delete ins.params;
            ins.params = NULL;
            break;
        }
        break;
    case CMD_GOTO: case CMD_GOSUB:
        if (num == 1 && strlen(p[0]) < 16)
        {
            ins.params = new S_goto_params;
            S_goto_params *g = (S_goto_params *)ins.params;
            if (_strnicmp(p[0], "flt[", 4))
                strcpy(g->label, p[0]);
            else
            {
                strcpy(g->label, "");
                if (!DecodeOperand(p[0], g->target.reg, g->target.value))
                {
                    delete ins.params;
                    break;
                }
            }
            return true;
        }
        break;
    case CMD_WAIT: case CMD_SETMISSIONNUMBER: case CMD_CAMERA_SETFOV: case CMD_FREERIDE_SCORESET:
    case CMD_FREERIDE_SCOREADD: case CMD_POLICE_SPEED_FACTOR:
        if (num == 1)
        {
            ins.params = new S_operand;
            if (DecodeOperand(p[0], ((S_operand *)ins.params)->reg, ((S_operand *)ins.params)->value))
                return true;
            delete ins.params;
            ins.params = NULL;
            break;
        }
        break;
    case CMD_IF:
        if (num == 3 && strlen(p[1]) <= 15 && strlen(p[2]) <= 15)
        {
            ins.params = new S_if_params;
            if (DecodeIf(ins, p[0]))
            {
                S_if_params *c = (S_if_params *)ins.params;
                strcpy(c->label1, p[1]);
                strcpy(c->label2, p[2]);
                return true;
            }
            delete ins.params;
            ins.params = NULL;
            break;
        }
        break;
    case CMD_HUMAN_LOOKTOACTOR: case CMD_HUMAN_LOOKTOFRAME:
        if (num >= 1 && num <= 3)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = -1;
            v[2] = 0;
            if (num > 1)
                v[1] = atoi(p[1]);
            if (num > 2)
                v[2] = atoi(p[2]);
            return true;
        }
        break;
    case CMD_EMITPARTICLE:
        if (num == 2 || num == 3)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = -1;
            if (num > 2)
                v[2] = atoi(p[2]);
            return true;
        }
        break;
    case CMD_GET_PM_STATE:
        // The last one a bit, 0 to 12
        if (num == 3)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            int bit = atoi(p[2]);
            if (bit < 0)
                bit = 0;
            if (bit > 12)
                bit = 12;
            v[2] = 1 << bit;
            return true;
        }
        break;
    case CMD_HUMAN_SETFIRETARGET:
        if (num == 1 || num == 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            if (num == 2)
                v[1] = atoi(p[1]);
            else
                v[1] = -1;
            return true;
        }
        break;
    case CMD_GROUP_DISBAND:
        if (num == 1)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = 0;
            v[2] = 0;
            return true;
        }
        break;
    case CMD_AUTOSAVEGAME: case CMD_AUTOSAVEGAMEFULL:
        if (num == 1 || num == 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            if (num == 2)
                v[1] = atoi(p[1]);
            else
                v[1] = 500;
            return true;
        }
        break;
    case CMD_RND: case CMD_FRM_SETON: case CMD_FRM_SETON1: case CMD_FRM_ISON: case CMD_FRM_LINKTO:
    case CMD_PLAYSOUNDEX: case CMD_GETSOUNDTIME: case CMD_DETECTOR_SETSIGNAL: case CMD_SETLMLEVEL: case CMD_DOOR_LOCK:
    case CMD_DOOR_OPEN: case CMD_DOOR_ENABLEUS: case CMD_GETCARDAMAGE: case CMD_HUMAN_DELWEAPON:
    case CMD_HUMAN_ACTIVATEWEAPON: case CMD_HUMAN_ANYWEAPONINHAND: case CMD_HUMAN_ANYWEAPONININVENTORY:
    case CMD_HUMAN_GETITEMINRHAND: case CMD_DIALOG_BEGIN: case CMD_DIALOG_CAMSWITCH: case CMD_HUMAN_SET8SLOT:
    case CMD_HUMAN_GETOWNER: case CMD_CAR_SETDESTROYMOTOR: case CMD_ACTOR_SETDIR: case CMD_ACTOR_SETPOS:
    case CMD_ACTOR_SETPLACEMENT: case CMD_SETTIMEOUTEVENT: case CMD_SETPLAYERFIREEVENT: case CMD_SETPLAYERHORNEVENT:
    case CMD_WINGMAN_DELINDICATOR: case CMD_CAR_ENABLEUS: case CMD_ISCARUSABLE: case CMD_AIRPLANE_ISDESTROYED:
    case CMD_HUMAN_RETURNFROMPANIC: case CMD_CAR_GETSPEED: case CMD_CAR_BREAKMOTOR: case CMD_HUMAN_THROWITEM:
    case CMD_FUCKINGBOX_GETNUMBOX: case CMD_FUCKINGBOX_GETNUMDEST: case CMD_WAGON_SETEVENT: case CMD_WAGON_GETLASTNODE:
    case CMD_CAR_SETSPECCOLL: case CMD_CAR_GETSPECCOLL: case CMD_HUMAN_FROMCAR: case CMD_GETENEMYAISTATE:
    case CMD_GET_PM_CRASHTIME: case CMD_GET_PM_FIRETIME: case CMD_GET_PM_NUMPREDATORS: case CMD_GET_PM_HUMANSTATE:
    case CMD_STREAM_CONNECT: case CMD_STREAM_SETLOOP: case CMD_STREAM_SETPOS: case CMD_STREAM_GETPOS:
    case CMD_SETTANKHITCOUNT: case CMD_SETNOANIMHIT: case CMD_CARLIGHT_MAIN: case CMD_CARLIGHT_LIGHT:
    case CMD_CARLIGHT_INDIC_L: case CMD_CARLIGHT_INDIC_R: case CMD_CARLIGHT_INDIC_OFF: case CMD_CAR_MUSTSTEAL:
    case CMD_GARAGE_CANSTEAL: case CMD_CAR_INWATER: case CMD_SHOWCARDAMAGE: case CMD_HUMAN_HAVEBODY:
    case CMD_GETACTORFRAME: case CMD_IFPLAYERSTEALCAR: case CMD_INTRO_SUBTITLE_ADD: case CMD_FRM_GETDIR:
    case CMD_FRM_SETDIR: case CMD_FRM_GETPOS: case CMD_FRM_SETPOS: case CMD_FRM_GETSCALE: case CMD_FRM_SETSCALE:
    case CMD_FRM_GETROT: case CMD_FRM_SETROT: case CMD_FRM_GETWORLDDIR: case CMD_FRM_GETWORLDPOS:
    case CMD_FRM_GETWORLDSCALE: case CMD_FRM_GETWORLDROT: case CMD_FRM_GETLOCALMATRIX: case CMD_FRM_GETWORLDMATRIX:
    case CMD_VECT_COPY: case CMD_VECT_MUL_VECT: case CMD_VECT_MUL_QUAT: case CMD_VECT_MUL_MATRIX:
    case CMD_VECT_ADD_VECT: case CMD_VECT_SUB_VECT: case CMD_VECT_CROSS: case CMD_VECT_MAGNITUDE: case CMD_MATRIX_COPY:
    case CMD_MATRIX_MUL: case CMD_QUAT_COPY: case CMD_QUAT_GETROTMATRIX: case CMD_QUAT_ADD: case CMD_QUAT_SUB:
    case CMD_QUAT_MUL_QUAT: case CMD_QUAT_ROTBYMATRIX: case CMD_GETFRAMEFROMACTOR: case CMD_SETPLAYERWFALLEVENT:
    case CMD_CAR_INVISIBLE: case CMD_CAR_SWITCHSHOWENERGY: case CMD_FRAMEISTELEPHONE: case CMD_BRIDGE_SHUTDOWN:
    case CMD_FRM_GETPARENT: case CMD_SOUND_GETVOLUME: case CMD_RACING_CHANGE_MODEL: case CMD_CAR_LOCK:
    case CMD_CAR_LOCK_ALL: case CMD_GETFILMMUSIC: case CMD_HUMAN_GETSEATIDX: case CMD_AIRPLANESHOWDAMAGE:
    case CMD_CAR_GETSEATCOUNT: case CMD_FRM_SETALPHA: case CMD_CAR_UNBREAKABLE: case CMD_DOOR_GETSTATE:
    case CMD_CAR_GETMAXLEVELS: case CMD_CAR_GETACTLEVEL: case CMD_FRM_GETNUMCHILDREN: case CMD_GETCARLINENUMFROMTABLE:
    case CMD_ENEMY_GETHOSTILESTATE: case CMD_HUMAN_SHUTUP: case CMD_PUMPER_CANWORK: case CMD_HUMAN_GETACTANIMID:
        if (num == 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            return true;
        }
        break;
    case CMD_ZATMYSE:
        if (num == 1 || num == 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            if (num == 2)
                v[1] = atoi(p[1]);
            else
                v[1] = 500;
            return true;
        }
        break;
    case CMD_HUMAN_HAVEBOX:
        // The third from the second parameter again, as PS2's
        if (num == 2 || num == 3)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            if (num == 2)
                v[2] = -1;
            else
                v[2] = atoi(p[1]);
            return true;
        }
        break;
    case CMD_PERSON_PLAYANIM:
        if (num >= 2 && num < 6)
        {
            ins.params = new S_playanim_params;
            S_playanim_params *a = (S_playanim_params *)ins.params;
            a->reg = atoi(p[0]);
            char *anim = ExtrudeString(p[1]);
            if (!anim)
            {
                delete ins.params;
                break;
            }
            a->result_reg = -1;
            if (num > 2)
                a->result_reg = atoi(p[2]);
            a->simple = false;
            // STRING: GAME 0x632808
            if (num > 3 && !_stricmp(p[3], "SIMPLE"))
                a->simple = true;
            a->unknown_4c = true;
            if (num > 4)
                a->unknown_4c = atoi(p[4]) != 0;
            strcpy(a->anim, anim);
            return true;
        }
        break;
    case CMD_MODEL_PLAYANIM: case CMD_MODEL_CREATE: case CMD_HUMAN_CHANGEMODEL: case CMD_FUCKINGBOX_ADD:
    case CMD_FUCKINGBOX_ADD_DEST: case CMD_STREAM_CREATE:
        if (num == 2)
        {
            ins.params = new S_name_params;
            S_name_params *n = (S_name_params *)ins.params;
            n->reg = atoi(p[0]);
            char *name = ExtrudeString(p[1]);
            if (!name)
            {
                delete ins.params;
                break;
            }
            strcpy(n->name, name);
            return true;
        }
        break;
    case CMD_FINDFRAME: case CMD_FINDACTOR: case CMD_RECADDACTOR:
        if (num > 0 && num < 3)
        {
            ins.params = new S_name_params;
            S_name_params *n = (S_name_params *)ins.params;
            n->reg = atoi(p[0]);
            if (num == 2)
            {
                char *name = ExtrudeString(p[1]);
                if (!name)
                {
                    delete ins.params;
                    break;
                }
                strcpy(n->name, name);
            }
            else
                strcpy(n->name, "");
            return true;
        }
        break;
    case CMD_IFFLTINRANGE:
        if (num == 4 && strlen(p[3]) <= 15)
        {
            ins.params = new S_ifflt_params;
            S_ifflt_params *r = (S_ifflt_params *)ins.params;
            r->reg = atoi(p[0]);
            r->min = (float)atof(p[1]);
            r->max = (float)atof(p[2]);
            strcpy(r->label, p[3]);
            return true;
        }
        break;
    case CMD_PLAYSOUND:
        if (num >= 4 && num <= 6)
        {
            ins.params = new S_playsound_params;
            S_playsound_params *s = (S_playsound_params *)ins.params;
            char *name = ExtrudeString(p[0]);
            if (name)
                strcpy(s->name, name);
            else
            {
                delete ins.params;
                break;
            }
            s->unknown_40 = atoi(p[1]);
            s->unknown_44 = (float)atof(p[2]);
            s->unknown_48 = (float)atof(p[3]);
            s->unknown_4c = 0;
            s->unknown_50 = 0;
            if (num > 4)
            {
                s->unknown_4c = atoi(p[4]);
                if (num > 5)
                {
                    s->unknown_50 = (float)atof(p[5]);
                    if (s->unknown_50 > 1.0f)
                        s->unknown_50 = 1.0f / s->unknown_50;
                }
            }
            return true;
        }
        break;
    case CMD_ACT_SETSTATE:
        if (num == 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = ACT_STATE_NONE;
            // STRING: GAME 0x632800
            if (!_stricmp(p[1], "ACTIVE"))
                v[1] = ACT_STATE_ACTIVE;
            // STRING: GAME 0x6327f4
            else if (!_stricmp(p[1], "INACTIVE"))
                v[1] = ACT_STATE_INACTIVE;
            return true;
        }
        break;
    case CMD_GARAGE_ADDCARIDX:
        if (num > 0 && num < 4)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = v[2] = 0;
            if (num > 1)
                v[1] = atoi(p[1]);
            if (num > 2)
                v[2] = atoi(p[2]);
            return true;
        }
        break;
    case CMD_DETECTOR_ISSIGNAL:
        if (num == 3 && strlen(p[1]) <= 15 && strlen(p[2]) <= 15)
        {
            ins.params = new S_signal_params;
            S_signal_params *s = (S_signal_params *)ins.params;
            s->reg = atoi(p[0]);
            strcpy(s->label1, p[1]);
            strcpy(s->label2, p[2]);
            return true;
        }
        break;
    case CMD_SETEVENT:
        if (num == 3 && strlen(p[2]) <= 15)
        {
            ins.params = new S_event_params;
            S_event_params *e = (S_event_params *)ins.params;
            e->value1 = atoi(p[0]);
            e->value2 = atoi(p[1]);
            strcpy(e->name, p[2]);
            return true;
        }
        break;
    case CMD_SETNPCKILLEVENT:
        if (num == 3 && strlen(p[2]) <= 31)
        {
            ins.params = new S_event_params;
            S_event_params *e = (S_event_params *)ins.params;
            e->value1 = atoi(p[0]);
            e->value2 = atoi(p[1]);
            char *name = ExtrudeString(p[2]);
            if (name)
                strcpy(e->name, name);
            else
            {
                delete ins.params;
                break;
            }
            return true;
        }
        break;
    case CMD_CTRL_READ: case CMD_CTRL_READEX:
        // -1 for an unknown control
        if (num == 2)
        {
            ins.params = new int[2];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = -1;
            for (int i = 0; i < NUM_CTRLS_TYPES; i++)
            {
                if (!_stricmp(_ctrls_type_[i].name, p[1]))
                {
                    v[1] = _ctrls_type_[i].value;
                    break;
                }
            }
            return true;
        }
        break;
    case CMD_FINDNEARACTOR:
        // -1 for an unknown type
        if (num == 3)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = -1;
            for (int i = 0; i < NUM_ACTOR_TYPES; i++)
            {
                if (!_stricmp(_actor_type_[i].name, p[2]))
                {
                    v[2] = _actor_type_[i].value;
                    break;
                }
            }
            return true;
        }
        break;
    case CMD_ENDOFMISSION:
        if (num == 2 || num == 3)
        {
            ins.params = new S_event_params;
            S_event_params *e = (S_event_params *)ins.params;
            e->value1 = atoi(p[0]);
            e->value2 = atoi(p[1]);
            if (num == 3)
            {
                char *name = ExtrudeString(p[2]);
                if (!name)
                {
                    delete ins.params;
                    break;
                }
                strcpy(e->name, name);
            }
            else
                strcpy(e->name, "");
            return true;
        }
        break;
    case CMD_POLICEMANAGER_FORCEARREST:
        if (num == 2 || num == 3)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            if (num == 3)
                v[2] = atoi(p[2]);
            else
                v[2] = 0;
            return true;
        }
        break;
    case CMD_HUMAN_CANADDWEAPON: case CMD_HUMAN_ISWEAPON: case CMD_HUMAN_FORCE_SETTOCAR: case CMD_WINGMAN_SETINDICATOR:
    case CMD_GETANGLEACTORTOACTOR: case CMD_ACTOR_DUPLICATE: case CMD_GET_REMOTE_FLOAT: case CMD_GET_REMOTE_FRAME:
    case CMD_GET_REMOTE_ACTOR: case CMD_SET_REMOTE_FLOAT: case CMD_SET_REMOTE_FRAME: case CMD_SET_REMOTE_ACTOR:
    case CMD_VECT_ANGLETO: case CMD_VECT_ANGLETO_2D: case CMD_QUAT_MAKE: case CMD_QUAT_SETDIR: case CMD_QUAT_EXTRACT:
    case CMD_QUAT_DOT: case CMD_COMPAREACTORS: case CMD_COMPAREFRAMES: case CMD_ISPOINTINSECTOR: case CMD_ISPOINTINAREA:
    case CMD_HUMAN_ENTERTOTRUCK: case CMD_PHOBJ_IMPULS: case CMD_CAR_DISABLE_UO: case CMD_POCZKURVENYCHBEDENCAR:
        if (num == 3)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = atoi(p[2]);
            return true;
        }
        break;
    case CMD_PLAYSOUNDSTOP:
        if (num == 1 || num == 2)
        {
            ins.params = new S_int_float_params;
            S_int_float_params *s = (S_int_float_params *)ins.params;
            s->value1 = atoi(p[0]);
            s->value2 = 0;
            if (num == 2)
            {
                s->value2 = (float)atoi(p[1]);
                if (s->value2 > 1.0f)
                    s->value2 = 1.0f / s->value2;
            }
            return true;
        }
        break;
    case CMD_DETECTOR_INRANGE: case CMD_DOOR_SETOPENPOSE: case CMD_CAR_SETSPEED:
        if (num == 2)
        {
            ins.params = new S_int_float_params;
            S_int_float_params *s = (S_int_float_params *)ins.params;
            s->value1 = atoi(p[0]);
            s->value2 = (float)atoi(p[1]);
            return true;
        }
        break;
    case CMD_LABEL:
        if (num == 1)
        {
            ins.params = new char[32];
            strcpy((char *)ins.params, p[0]);
            return true;
        }
        break;
    case CMD_CALLSUBROUTINE: case CMD_RECLOAD: case CMD_RECLOADFULL: case CMD_LOADCOLLTREE: case CMD_PRELOADMODEL:
    case CMD_LOADDIFFERENCES: case CMD_POLICEMANAGER_ADD: case CMD_POLICEMANAGER_DEL: case CMD_SETTRAFFSECTORSND:
        // A quoted name, "unknown" without the quotes
        if (num == 1)
        {
            ins.params = new char[48];
            char *name = ExtrudeString(p[0]);
            if (name)
                strcpy((char *)ins.params, name);
            else
                // STRING: GAME 0x6327ec
                strcpy((char *)ins.params, "unknown");
            return true;
        }
        break;
    case CMD_SOUNDFADE: case CMD_STREAM_FADEVOL: case CMD_CREATE_PHYSICALOBJECT: case CMD_GRABER_SETFOCUS:
        if (num == 4)
        {
            ins.params = new S_fade_params;
            S_fade_params *f = (S_fade_params *)ins.params;
            f->value1 = atoi(p[0]);
            f->value2 = atoi(p[1]);
            f->value3 = (float)atof(p[2]);
            f->value4 = (float)atof(p[3]);
            return true;
        }
        break;
    case CMD_EXPLOSION: case CMD_POLICEMANAGER_SETSPEED: case CMD_CAMERA_SETRANGE:
        if (num == 2)
        {
            ins.params = new float[2];
            float *f = (float *)ins.params;
            f[0] = (float)atof(p[0]);
            f[1] = (float)atof(p[1]);
            return true;
        }
        break;
    case CMD_HUMAN_TALK:
        if (num == 2 || num == 3)
        {
            ins.params = new int[4];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            if (num == 3)
                v[2] = atoi(p[2]);
            else
                v[2] = 0;
            v[3] = -1;
            return true;
        }
        break;
    case CMD_GETACTORSDIST:
        if (num == 3 || num == 4)
        {
            ins.params = new int[4];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = atoi(p[2]);
            if (num == 4)
                v[3] = atoi(p[3]);
            else
                v[3] = 0;
            return true;
        }
        break;
    case CMD_TIMERON: case CMD_HUMAN_ADDWEAPON: case CMD_RACING_MISSION6_INIT: case CMD_QUAT_SLERP:
        if (num == 4)
        {
            ins.params = new int[4];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = atoi(p[2]);
            v[3] = atoi(p[3]);
            return true;
        }
        break;
    case CMD_CHANGE_MISSION:
        if (num == 3)
        {
            char *name1 = ExtrudeString(p[0]);
            char *name2 = ExtrudeString(p[1]);
            if (name1 && name2)
            {
                ins.params = new S_change_mission_params;
                S_change_mission_params *m = (S_change_mission_params *)ins.params;
                strcpy(m->name1, name1);
                strcpy(m->name2, name2);
                m->value = (float)atof(p[2]);
                return true;
            }
        }
        break;
    case CMD_CAMERA_SETSWING: case CMD_POLICE_SUPPORT:
        if (num == 3)
        {
            ins.params = new S_float_int_params;
            S_float_int_params *s = (S_float_int_params *)ins.params;
            s->value1 = atoi(p[0]);
            s->value = (float)atof(p[1]);
            s->value2 = atoi(p[2]);
            return true;
        }
        break;
    case CMD_AIRPLANE_START: case CMD_CAR_SETDOOROPEN:
        if (num == 3)
        {
            ins.params = new S_float_int_params;
            S_float_int_params *s = (S_float_int_params *)ins.params;
            s->value1 = atoi(p[0]);
            s->value2 = atoi(p[1]);
            s->value = (float)atof(p[2]);
            return true;
        }
        break;
    case CMD_SETAIPRIORITY:
        if (num == 1)
        {
            ins.params = new float;
            float *f = (float *)ins.params;
            *f = (float)atof(p[0]);
            return true;
        }
        break;
    case CMD_HUMAN_CHANGEANIM:
        if (num == 2 || num == 3)
        {
            ins.params = new S_changeanim_params;
            S_changeanim_params *a = (S_changeanim_params *)ins.params;
            a->reg = atoi(p[0]);
            a->anim2[0] = 0;
            char *anim = ExtrudeString(p[1]);
            if (!anim)
            {
                delete ins.params;
                break;
            }
            strcpy(a->anim1, anim);
            if (num == 3)
            {
                anim = ExtrudeString(p[2]);
                if (anim)
                    strcpy(a->anim2, anim);
            }
            return true;
        }
        break;
    case CMD_ENEMY_CHANGEANIM:
        if (num == 1 || num == 2)
        {
            ins.params = new S_enemy_changeanim_params;
            S_enemy_changeanim_params *a = (S_enemy_changeanim_params *)ins.params;
            a->anim2[0] = 0;
            char *anim = ExtrudeString(p[0]);
            if (!anim)
            {
                delete ins.params;
                break;
            }
            strcpy(a->anim1, anim);
            if (num == 2)
            {
                anim = ExtrudeString(p[1]);
                if (anim)
                    strcpy(a->anim2, anim);
            }
            return true;
        }
        break;
    case CMD_HUMAN_LINKTOHAND:
        if (num > 1 && num < 5)
        {
            ins.params = new int[4];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = 0;
            v[3] = 0;
            // STRING: GAME 0x6327e4
            if (num > 2 && !_stricmp(p[2], "LEFT"))
                v[2] = 1;
            // STRING: GAME 0x6327d8
            if (num > 3 && !_stricmp(p[3], "DONT_MOVE"))
                v[3] = 1;
            return true;
        }
        break;
    case CMD_HUMAN_UNLINKFROMHAND:
        if (num > 1 && num < 6)
        {
            ins.params = new int[5];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = 0;
            v[3] = 0;
            v[4] = 0;
            if (num > 2)
                v[2] = atoi(p[2]);
            if (num > 3)
                v[3] = atoi(p[3]);
            if (num > 4)
                v[4] = atoi(p[4]);
            return true;
        }
        break;
    case CMD_CREATEWEAPONFROMFRAME:
        if (num > 1 && num < 5)
        {
            ins.params = new int[4];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = 0;
            v[3] = 0;
            if (num > 2)
                v[2] = atoi(p[2]);
            if (num > 3)
                v[3] = atoi(p[3]);
            return true;
        }
        break;
    case CMD_COMPAREOWNERWITHEX:
        if (num == 4 && strlen(p[2]) <= 15 && strlen(p[3]) <= 15)
        {
            ins.params = new S_compareowner_params;
            S_compareowner_params *c = (S_compareowner_params *)ins.params;
            c->value1 = atoi(p[0]);
            c->value2 = atoi(p[1]);
            strcpy(c->name1, p[2]);
            strcpy(c->name2, p[3]);
            return true;
        }
        break;
    case CMD_HUMAN_SETPROPERTY:
        if (num == 3)
        {
            ins.params = new S_setproperty_params;
            S_setproperty_params *s = (S_setproperty_params *)ins.params;
            s->reg = atoi(p[0]);
            if (!DecodeOperand(p[1], s->value_reg, s->value))
            {
                delete s;
                break;
            }
            s->property = -1;
            for (int i = NUM_HUMAN_PROPERTIES; i--;)
            {
                if (!_stricmp(p[2], _human_property_[i]))
                {
                    s->property = i;
                    break;
                }
            }
            if (s->property < 0)
                break;
            return true;
        }
        break;
    case CMD_HUMAN_GETPROPERTY:
        if (num == 3)
        {
            ins.params = new int[3];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = -1;
            for (int i = NUM_HUMAN_PROPERTIES; i--;)
            {
                if (!_stricmp(p[2], _human_property_[i]))
                {
                    v[2] = i;
                    break;
                }
            }
            if (v[2] < 0)
                break;
            return true;
        }
        break;
    case CMD_WEATHER_SETPARAM:
        // An integer or a float, after the parameter
        if (num == 2)
        {
            ins.params = new S_weather_params;
            S_weather_params *w = (S_weather_params *)ins.params;
            w->param = -1;
            for (int i = NUM_WEATHER_PROPERTIES; i--;)
            {
                if (!_stricmp(p[0], _weather_property_[i]))
                {
                    w->param = i;
                    break;
                }
            }
            switch (w->param)
            {
            case WEATHER_ON: case WEATHER_SECTORS: case WEATHER_DUMMIES: case WEATHER_COLORH: case WEATHER_COLORL:
            case WEATHER_MAX_CNT: case WEATHER_MODE:
                {
                    int value;
                    if (!DecodeOperandInt(p[1], w->value_reg, value))
                    {
                        delete w;
                        return false;
                    }
                    w->value.i = value;
                }
                break;
            case WEATHER_SPEED: case WEATHER_LEN: case WEATHER_WIDTH: case WEATHER_MAX_DIST: case WEATHER_MAX_HEIGHT:
            case WEATHER_DIR_X: case WEATHER_DIR_Y: case WEATHER_DIR_Z:
                if (!DecodeOperand(p[1], w->value_reg, w->value.f))
                {
                    delete w;
                    return false;
                }
                break;
            default:
                delete w;
                return false;
            }
            return true;
        }
        break;
    case CMD_GARAGE_ENABLESTEAL: case CMD_FREERIDE_ENABLECAR:
        // The parameter's first character, in upper case
        if (num == 1)
        {
            ins.params = new char;
            char *c = (char *)ins.params;
            _strupr(p[0]);
            *c = p[0][0];
            return true;
        }
        break;
    case CMD_VECT_MUL_SCL: case CMD_VECT_ROTATE_2D: case CMD_QUAT_MUL_SCL: case CMD_SOUND_SETVOLUME:
    case CMD_CAR_SETACTLEVEL: case CMD_FRM_GETCHILD:
        if (num == 2)
        {
            ins.params = new S_reg_operand_params;
            S_reg_operand_params *s = (S_reg_operand_params *)ins.params;
            s->reg = atoi(p[0]);
            if (!DecodeOperand(p[1], s->value_reg, s->value))
            {
                delete ins.params;
                break;
            }
            return true;
        }
        break;
    case CMD_COLL_TESTLINE:
        if (num == 5)
        {
            ins.params = new int[5];
            int *v = (int *)ins.params;
            v[0] = atoi(p[0]);
            v[1] = atoi(p[1]);
            v[2] = atoi(p[2]);
            v[3] = atoi(p[3]);
            v[4] = atoi(p[4]);
            return true;
        }
        break;
    case CMD_HUMAN_THROWGRENADE:
        if (num == 3)
        {
            ins.params = new S_grenade_params;
            S_grenade_params *g = (S_grenade_params *)ins.params;
            g->reg = atoi(p[0]);
            g->value1 = (float)atof(p[1]);
            g->value2 = (float)atof(p[2]);
            return true;
        }
        break;
    }
    return false;
}

// A command's name as the scripts spell it, a space after it, and its number (the names are ours)
struct S_command_name
{
    char name[30];
    int command;
};

// The table is a local, filled at each call. A line matches a name that it starts with, or that it is without the
// space; the case doesn't matter
// FUNCTION: GAME 0x463960
int C_program::DecodeInstruction(char *line, int &len)
{
    S_command_name commands[NUM_COMMANDS] = {
        // STRING: GAME 0x634370
        {"LABEL ", CMD_LABEL},
        // STRING: GAME 0x634368
        {"GOTO ", CMD_GOTO},
        // STRING: GAME 0x634360
        {"GOSUB ", CMD_GOSUB},
        // STRING: GAME 0x634358
        {"RETURN ", CMD_RETURN},
        // STRING: GAME 0x634350
        {"WAIT ", CMD_WAIT},
        // STRING: GAME 0x634340
        {"BEEP ", CMD_BEEP},
        // STRING: GAME 0x634348
        {"END ", CMD_END},
        // STRING: GAME 0x634334
        {"DIM_FLT ", CMD_DIM_FLT},
        // STRING: GAME 0x63432c
        {"LET ", CMD_LET},
        // STRING: GAME 0x634328
        {"IF ", CMD_IF},
        // STRING: GAME 0x634320
        {"RND ", CMD_RND},
        // STRING: GAME 0x634314
        {"DIM_FRM ", CMD_DIM_FRM},
        // STRING: GAME 0x634308
        {"DIM_ACT ", CMD_DIM_ACT},
        // STRING: GAME 0x6342fc
        {"FINDFRAME ", CMD_FINDFRAME},
        // STRING: GAME 0x6342f0
        {"FINDACTOR ", CMD_FINDACTOR},
        // STRING: GAME 0x6342e4
        {"FRM_SETON ", CMD_FRM_SETON},
        // STRING: GAME 0x6342d8
        {"FRM_SETON1 ", CMD_FRM_SETON1},
        // STRING: GAME 0x6342cc
        {"FRM_LINKTO ", CMD_FRM_LINKTO},
        // STRING: GAME 0x6342c4
        {"CALL ", CMD_CALL},
        // STRING: GAME 0x6342b4
        {"IFFLTINRANGE ", CMD_IFFLTINRANGE},
        // STRING: GAME 0x6342a8
        {"PLAYSOUND ", CMD_PLAYSOUND},
        // STRING: GAME 0x634298
        {"PLAYSOUNDSTOP ", CMD_PLAYSOUNDSTOP},
        // STRING: GAME 0x634288
        {"PLAYSOUNDEX ", CMD_PLAYSOUNDEX},
        // STRING: GAME 0x63427c
        {"STOPSOUND ", CMD_STOPSOUND},
        // STRING: GAME 0x63426c
        {"GETSOUNDTIME ", CMD_GETSOUNDTIME},
        // STRING: GAME 0x63425c
        {"ACT_SETSTATE ", CMD_ACT_SETSTATE},
        // STRING: GAME 0x634248
        {"DETECTOR_ISSIGNAL ", CMD_DETECTOR_ISSIGNAL},
        // STRING: GAME 0x634220
        {"DETECTOR_SETSIGNAL ", CMD_DETECTOR_SETSIGNAL},
        // STRING: GAME 0x634234
        {"DETECTOR_INRANGE ", CMD_DETECTOR_INRANGE},
        // STRING: GAME 0x634214
        {"SETCOMPASS ", CMD_SETCOMPASS},
        // STRING: GAME 0x634208
        {"SETLMLEVEL ", CMD_SETLMLEVEL},
        // STRING: GAME 0x6341f0
        {"DETECTOR_WAITFORUSE ", CMD_DETECTOR_WAITFORUSE},
        // STRING: GAME 0x6341d8
        {"DETECTOR_WAITFORHIT ", CMD_DETECTOR_WAITFORHIT},
        // STRING: GAME 0x6341c8
        {"COMMANDBLOCK ", CMD_COMMANDBLOCK},
        // STRING: GAME 0x6341b8
        {"CALLSUBROUTINE ", CMD_CALLSUBROUTINE},
        // STRING: GAME 0x6341ac
        {"SOUNDFADE ", CMD_SOUNDFADE},
        // STRING: GAME 0x63419c
        {"ENDOFMISSION ", CMD_ENDOFMISSION},
        // STRING: GAME 0x634188
        {"PROGRAM_STORAGE ", CMD_PROGRAM_STORAGE},
        // STRING: GAME 0x63417c
        {"DOOR_LOCK ", CMD_DOOR_LOCK},
        // STRING: GAME 0x634170
        {"DOOR_OPEN ", CMD_DOOR_OPEN},
        // STRING: GAME 0x634160
        {"DOOR_ENABLEUS ", CMD_DOOR_ENABLEUS},
        // STRING: GAME 0x634154
        {"RECCLEAR ", CMD_RECCLEAR},
        // STRING: GAME 0x634144
        {"RECADDACTOR ", CMD_RECADDACTOR},
        // STRING: GAME 0x634138
        {"RECLOAD ", CMD_RECLOAD},
        // STRING: GAME 0x634128
        {"RECLOADFULL ", CMD_RECLOADFULL},
        // STRING: GAME 0x634118
        {"RECWAITFOREND ", CMD_RECWAITFOREND},
        // STRING: GAME 0x63410c
        {"RECUNLOAD ", CMD_RECUNLOAD},
        // STRING: GAME 0x6340fc
        {"LOADCOLLTREE ", CMD_LOADCOLLTREE},
        // STRING: GAME 0x6340f0
        {"EXPLOSION ", CMD_EXPLOSION},
        // STRING: GAME 0x6340e4
        {"TIMERON ", CMD_TIMERON},
        // STRING: GAME 0x6340d8
        {"TIMEROFF ", CMD_TIMEROFF},
        // STRING: GAME 0x6340c8
        {"CHANGE_MISSION ", CMD_CHANGE_MISSION},
        // STRING: GAME 0x6340b4
        {"CAMERA_SETSWING ", CMD_CAMERA_SETSWING},
        // STRING: GAME 0x6340a4
        {"AIRPLANE_START ", CMD_AIRPLANE_START},
        // STRING: GAME 0x634090
        {"AIRPLANE_START2 ", CMD_AIRPLANE_START2},
        // STRING: GAME 0x634084
        {"DEBUG_TEXT ", CMD_DEBUG_TEXT},
        // STRING: GAME 0x634074
        {"GETACTORSDIST ", CMD_GETACTORSDIST},
        // STRING: GAME 0x634064
        {"GETCARDAMAGE ", CMD_GETCARDAMAGE},
        // STRING: GAME 0x634050
        {"HUMAN_ADDWEAPON ", CMD_HUMAN_ADDWEAPON},
        // STRING: GAME 0x63403c
        {"HUMAN_CANADDWEAPON ", CMD_HUMAN_CANADDWEAPON},
        // STRING: GAME 0x634028
        {"HUMAN_DELWEAPON ", CMD_HUMAN_DELWEAPON},
        // STRING: GAME 0x634018
        {"HUMAN_ISWEAPON ", CMD_HUMAN_ISWEAPON},
        // STRING: GAME 0x634008
        {"PRELOADMODEL ", CMD_PRELOADMODEL},
        // STRING: GAME 0x633ff4
        {"LOADDIFFERENCES ", CMD_LOADDIFFERENCES},
        // STRING: GAME 0x633fe0
        {"CLEARDIFFERENCES ", CMD_CLEARDIFFERENCES},
        // STRING: GAME 0x633fdc
        {"{ ", CMD_BLOCK_BEGIN},
        // STRING: GAME 0x633fd8
        {"} ", CMD_BLOCK_END},
        // STRING: GAME 0x633fc8
        {"SETAIPRIORITY ", CMD_SETAIPRIORITY},
        // STRING: GAME 0x633fc0
        {"EVENT ", CMD_EVENT},
        // STRING: GAME 0x633fb4
        {"SETEVENT ", CMD_SETEVENT},
        // STRING: GAME 0x633fa4
        {"EVENT_USE_CB ", CMD_EVENT_USE_CB},
        // STRING: GAME 0x633f90
        {"ENEMY_CHANGEANIM ", CMD_ENEMY_CHANGEANIM},
        // STRING: GAME 0x633f7c
        {"HUMAN_CHANGEANIM ", CMD_HUMAN_CHANGEANIM},
        // STRING: GAME 0x633f6c
        {"DIALOG_BEGIN ", CMD_DIALOG_BEGIN},
        // STRING: GAME 0x633f60
        {"DIALOG_END ", CMD_DIALOG_END},
        // STRING: GAME 0x633f4c
        {"DIALOG_CAMSWITCH ", CMD_DIALOG_CAMSWITCH},
        // STRING: GAME 0x633f38
        {"GAME_NIGHTMISSION ", CMD_GAME_NIGHTMISSION},
        // STRING: GAME 0x633f24
        {"HUMAN_LOOKTOACTOR ", CMD_HUMAN_LOOKTOACTOR},
        // STRING: GAME 0x633f10
        {"HUMAN_LOOKTOFRAME ", CMD_HUMAN_LOOKTOFRAME},
        // STRING: GAME 0x633efc
        {"GETMISSIONNUMBER ", CMD_GETMISSIONNUMBER},
        // STRING: GAME 0x633ee8
        {"SETMISSIONNUMBER ", CMD_SETMISSIONNUMBER},
        // STRING: GAME 0x633ed4
        {"HUMAN_LINKTOHAND ", CMD_HUMAN_LINKTOHAND},
        // STRING: GAME 0x633ebc
        {"HUMAN_UNLINKFROMHAND ", CMD_HUMAN_UNLINKFROMHAND},
        // STRING: GAME 0x633ea4
        {"CREATEWEAPONFROMFRAME ", CMD_CREATEWEAPONFROMFRAME},
        // STRING: GAME 0x633e94
        {"GARAGE_ADDCAR ", CMD_GARAGE_ADDCAR},
        // STRING: GAME 0x633e80
        {"GARAGE_ADDCARIDX ", CMD_GARAGE_ADDCARIDX},
        // STRING: GAME 0x633e70
        {"GARAGE_DELCAR ", CMD_GARAGE_DELCAR},
        // STRING: GAME 0x633e58
        {"GARAGE_GENERATECARS ", CMD_GARAGE_GENERATECARS},
        // STRING: GAME 0x633e30
        {"GARAGE_RELEASECARS ", CMD_GARAGE_RELEASECARS},
        // STRING: GAME 0x633e44
        {"GARAGE_CARMANAGER ", CMD_GARAGE_CARMANAGER},
        // STRING: GAME 0x633e18
        {"PLAYER_LOCKCONTROLS ", CMD_PLAYER_LOCKCONTROLS},
        // STRING: GAME 0x633e04
        {"PERSON_PLAYANIM ", CMD_PERSON_PLAYANIM},
        // STRING: GAME 0x633df0
        {"PERSON_STOPANIM ", CMD_PERSON_STOPANIM},
        // STRING: GAME 0x633de0
        {"GUNSHOP_MENU ", CMD_GUNSHOP_MENU},
        // STRING: GAME 0x633dd0
        {"HUMAN_SET8SLOT ", CMD_HUMAN_SET8SLOT},
        // STRING: GAME 0x633dbc
        {"ENEMY_FORCESCRIPT ", CMD_ENEMY_FORCESCRIPT},
        // STRING: GAME 0x633da8
        {"COMPAREOWNERWITHEX ", CMD_COMPAREOWNERWITHEX},
        // STRING: GAME 0x633d94
        {"HUMAN_SETPROPERTY ", CMD_HUMAN_SETPROPERTY},
        // STRING: GAME 0x633d80
        {"HUMAN_GETPROPERTY ", CMD_HUMAN_GETPROPERTY},
        // STRING: GAME 0x633d70
        {"DISABLECOLLS ", CMD_DISABLECOLLS},
        // STRING: GAME 0x633d60
        {"HUMAN_GETOWNER ", CMD_HUMAN_GETOWNER},
        // STRING: GAME 0x633d48
        {"HUMAN_FORCE_SETTOCAR ", CMD_HUMAN_FORCE_SETTOCAR},
        // STRING: GAME 0x633d30
        {"CAR_SETDESTROYMOTOR ", CMD_CAR_SETDESTROYMOTOR},
        // STRING: GAME 0x633d20
        {"ACTOR_SETDIR ", CMD_ACTOR_SETDIR},
        // STRING: GAME 0x633d0c
        {"ACTOR_SETPLACEMENT ", CMD_ACTOR_SETPLACEMENT},
        // STRING: GAME 0x633cfc
        {"ACTOR_SETPOS ", CMD_ACTOR_SETPOS},
        // STRING: GAME 0x633ce8
        {"GARAGE_ENABLESTEAL ", CMD_GARAGE_ENABLESTEAL},
        // STRING: GAME 0x633cd4
        {"SETTIMEOUTEVENT ", CMD_SETTIMEOUTEVENT},
        // STRING: GAME 0x633cc4
        {"EMITPARTICLE ", CMD_EMITPARTICLE},
        // STRING: GAME 0x633cb4
        {"STOPPARTICLE ", CMD_STOPPARTICLE},
        // STRING: GAME 0x633ca0
        {"SETPLAYERFIREEVENT ", CMD_SETPLAYERFIREEVENT},
        // STRING: GAME 0x633c90
        {"FINDNEARACTOR ", CMD_FINDNEARACTOR},
        // STRING: GAME 0x633c7c
        {"SETPLAYERHORNEVENT ", CMD_SETPLAYERHORNEVENT},
        // STRING: GAME 0x633c54
        {"SETPLAYERWFALLEVENT ", CMD_SETPLAYERWFALLEVENT},
        // STRING: GAME 0x633c6c
        {"HUMAN_HOLSTER ", CMD_HUMAN_HOLSTER},
        // STRING: GAME 0x633c44
        {"CAMERA_LOCK ", CMD_CAMERA_LOCK},
        // STRING: GAME 0x633c34
        {"CAMERA_UNLOCK ", CMD_CAMERA_UNLOCK},
        // STRING: GAME 0x633c20
        {"SETMISSIONNAMEID ", CMD_SETMISSIONNAMEID},
        // STRING: GAME 0x633c08
        {"HUMAN_ACTIVATEWEAPON ", CMD_HUMAN_ACTIVATEWEAPON},
        // STRING: GAME 0x633bf0
        {"HUMAN_ANYWEAPONINHAND ", CMD_HUMAN_ANYWEAPONINHAND},
        // STRING: GAME 0x633bd4
        {"HUMAN_ANYWEAPONININVENTORY ", CMD_HUMAN_ANYWEAPONININVENTORY},
        // STRING: GAME 0x633bac
        {"WINGMAN_SETINDICATOR ", CMD_WINGMAN_SETINDICATOR},
        // STRING: GAME 0x633bc4
        {"MODEL_CREATE ", CMD_MODEL_CREATE},
        // STRING: GAME 0x633b9c
        {"MODEL_DESTROY ", CMD_MODEL_DESTROY},
        // STRING: GAME 0x633b8c
        {"MODEL_PLAYANIM ", CMD_MODEL_PLAYANIM},
        // STRING: GAME 0x633b7c
        {"MODEL_STOPANIM ", CMD_MODEL_STOPANIM},
        // STRING: GAME 0x633b68
        {"HUMAN_CHANGEMODEL ", CMD_HUMAN_CHANGEMODEL},
        // STRING: GAME 0x633b58
        {"AUTOSAVEGAME ", CMD_AUTOSAVEGAME},
        // STRING: GAME 0x633b44
        {"AUTOSAVEGAMEFULL ", CMD_AUTOSAVEGAMEFULL},
        // STRING: GAME 0x633b34
        {"HUMAN_DEATH ", CMD_HUMAN_DEATH},
        // STRING: GAME 0x633b28
        {"HUMAN_TALK ", CMD_HUMAN_TALK},
        // STRING: GAME 0x633b18
        {"HUMAN_ERASEAB ", CMD_HUMAN_ERASEAB},
        // STRING: GAME 0x633b08
        {"HUMAN_CREATEAB ", CMD_HUMAN_CREATEAB},
        // STRING: GAME 0x633af4
        {"POLICEMANAGER_ON ", CMD_POLICEMANAGER_ON},
        // STRING: GAME 0x633ae0
        {"POLICEMANAGER_ADD ", CMD_POLICEMANAGER_ADD},
        // STRING: GAME 0x633acc
        {"POLICEMANAGER_DEL ", CMD_POLICEMANAGER_DEL},
        // STRING: GAME 0x633abc
        {"CAR_ENABLEUS ", CMD_CAR_ENABLEUS},
        // STRING: GAME 0x633aa8
        {"CAR_SETDOOROPEN ", CMD_CAR_SETDOOROPEN},
        // STRING: GAME 0x633a90
        {"HUMAN_SETFIRETARGET ", CMD_HUMAN_SETFIRETARGET},
        // STRING: GAME 0x633a7c
        {"DOOR_SETOPENPOSE ", CMD_DOOR_SETOPENPOSE},
        // STRING: GAME 0x633a64
        {"AIRPLANE_ISDESTROYED ", CMD_AIRPLANE_ISDESTROYED},
        // STRING: GAME 0x633a4c
        {"HUMAN_RETURNFROMPANIC ", CMD_HUMAN_RETURNFROMPANIC},
        // STRING: GAME 0x633a3c
        {"CAR_GETSPEED ", CMD_CAR_GETSPEED},
        // STRING: GAME 0x633a2c
        {"ISCARUSABLE ", CMD_ISCARUSABLE},
        // STRING: GAME 0x633a1c
        {"CAR_BREAKMOTOR ", CMD_CAR_BREAKMOTOR},
        // STRING: GAME 0x633a0c
        {"INVENTORY_PUSH ", CMD_INVENTORY_PUSH},
        // STRING: GAME 0x6339fc
        {"INVENTORY_POP ", CMD_INVENTORY_POP},
        // STRING: GAME 0x6339ec
        {"CHARACTER_PUSH ", CMD_CHARACTER_PUSH},
        // STRING: GAME 0x6339dc
        {"CHARACTER_POP ", CMD_CHARACTER_POP},
        // STRING: GAME 0x6339c8
        {"HUMAN_THROWITEM ", CMD_HUMAN_THROWITEM},
        // STRING: GAME 0x6339b8
        {"FUCKINGBOX_ADD ", CMD_FUCKINGBOX_ADD},
        // STRING: GAME 0x6339a0
        {"FUCKINGBOX_RECOMPILE ", CMD_FUCKINGBOX_RECOMPILE},
        // STRING: GAME 0x633988
        {"FUCKINGBOX_ADD_DEST ", CMD_FUCKINGBOX_ADD_DEST},
        // STRING: GAME 0x633970
        {"FUCKINGBOX_GETNUMBOX ", CMD_FUCKINGBOX_GETNUMBOX},
        // STRING: GAME 0x633944
        {"FUCKINGBOX_GETNUMDEST ", CMD_FUCKINGBOX_GETNUMDEST},
        // STRING: GAME 0x63395c
        {"FUCKINGBOX_MOVE ", CMD_FUCKINGBOX_MOVE},
        // STRING: GAME 0x633934
        {"WAGON_SETEVENT ", CMD_WAGON_SETEVENT},
        // STRING: GAME 0x633920
        {"WAGON_GETLASTNODE ", CMD_WAGON_GETLASTNODE},
        // STRING: GAME 0x63390c
        {"CAR_SETSPECCOLL ", CMD_CAR_SETSPECCOLL},
        // STRING: GAME 0x6338f8
        {"CAR_GETSPECCOLL ", CMD_CAR_GETSPECCOLL},
        // STRING: GAME 0x6338e4
        {"MISSION_OBJECTIVES ", CMD_MISSION_OBJECTIVES},
        // STRING: GAME 0x6338c8
        {"MISSION_OBJECTIVESREMOVE ", CMD_MISSION_OBJECTIVESREMOVE},
        // STRING: GAME 0x6338ac
        {"MISSION_OBJECTIVESCLEAR ", CMD_MISSION_OBJECTIVESCLEAR},
        // STRING: GAME 0x633894
        {"RACING_MISSION6_INIT ", CMD_RACING_MISSION6_INIT},
        // STRING: GAME 0x63386c
        {"RACING_MISSION6_START ", CMD_RACING_MISSION6_START},
        // STRING: GAME 0x633884
        {"CAR_EXPLOSION ", CMD_CAR_EXPLOSION},
        // STRING: GAME 0x63385c
        {"HUMAN_FROMCAR ", CMD_HUMAN_FROMCAR},
        // STRING: GAME 0x633850
        {"[BREAK] ", CMD_BREAK},
        // STRING: GAME 0x63383c
        {"CAR_REMOVE_DRIVER ", CMD_CAR_REMOVE_DRIVER},
        // STRING: GAME 0x63382c
        {"SETMODELTOCAR ", CMD_SETMODELTOCAR},
        // STRING: GAME 0x633818
        {"GETENEMYAISTATE ", CMD_GETENEMYAISTATE},
        // STRING: GAME 0x633808
        {"GET_PM_STATE ", CMD_GET_PM_STATE},
        // STRING: GAME 0x6337f4
        {"GET_PM_CRASHTIME ", CMD_GET_PM_CRASHTIME},
        // STRING: GAME 0x6337e0
        {"GET_PM_FIRETIME ", CMD_GET_PM_FIRETIME},
        // STRING: GAME 0x6337c8
        {"GET_PM_NUMPREDATORS ", CMD_GET_PM_NUMPREDATORS},
        // STRING: GAME 0x6337b8
        {"USE_LIGHTCACHE ", CMD_USE_LIGHTCACHE},
        // STRING: GAME 0x6337a8
        {"STREAM_CREATE ", CMD_STREAM_CREATE},
        // STRING: GAME 0x633798
        {"STREAM_CONNECT ", CMD_STREAM_CONNECT},
        // STRING: GAME 0x633788
        {"STREAM_PLAY ", CMD_STREAM_PLAY},
        // STRING: GAME 0x633778
        {"STREAM_PAUSE ", CMD_STREAM_PAUSE},
        // STRING: GAME 0x633768
        {"STREAM_STOP ", CMD_STREAM_STOP},
        // STRING: GAME 0x633758
        {"STREAM_DESTROY ", CMD_STREAM_DESTROY},
        // STRING: GAME 0x633748
        {"STREAM_FADEVOL ", CMD_STREAM_FADEVOL},
        // STRING: GAME 0x633738
        {"STREAM_SETLOOP ", CMD_STREAM_SETLOOP},
        // STRING: GAME 0x633724
        {"SETTANKHITCOUNT ", CMD_SETTANKHITCOUNT},
        // STRING: GAME 0x633714
        {"SETNOANIMHIT ", CMD_SETNOANIMHIT},
        // STRING: GAME 0x633704
        {"CARLIGHT_MAIN ", CMD_CARLIGHT_MAIN},
        // STRING: GAME 0x6336f4
        {"CARLIGHT_LIGHT ", CMD_CARLIGHT_LIGHT},
        // STRING: GAME 0x6336e0
        {"CARLIGHT_INDIC_L ", CMD_CARLIGHT_INDIC_L},
        // STRING: GAME 0x6336cc
        {"CARLIGHT_INDIC_R ", CMD_CARLIGHT_INDIC_R},
        // STRING: GAME 0x6336bc
        {"CAR_MUSTSTEAL ", CMD_CAR_MUSTSTEAL},
        // STRING: GAME 0x6336a8
        {"CAR_SETPROJECTOR ", CMD_CAR_SETPROJECTOR},
        // STRING: GAME 0x633694
        {"GARAGE_CANSTEAL ", CMD_GARAGE_CANSTEAL},
        // STRING: GAME 0x633684
        {"CAR_INWATER ", CMD_CAR_INWATER},
        // STRING: GAME 0x633670
        {"HUMAN_FORCEFALL ", CMD_HUMAN_FORCEFALL},
        // STRING: GAME 0x633658
        {"CREATE_PHYSICALOBJECT ", CMD_CREATE_PHYSICALOBJECT},
        // STRING: GAME 0x633648
        {"WEATHER_RESET ", CMD_WEATHER_RESET},
        // STRING: GAME 0x633634
        {"WEATHER_SETPARAM ", CMD_WEATHER_SETPARAM},
        // STRING: GAME 0x633624
        {"GETTICKTIME ", CMD_GETTICKTIME},
        // STRING: GAME 0x633614
        {"GETGAMETIME ", CMD_GETGAMETIME},
        // STRING: GAME 0x6335fc
        {"POLICEMANAGER_SETSPEED ", CMD_POLICEMANAGER_SETSPEED},
        // STRING: GAME 0x6335e4
        {"GARAGE_ADDLASTSTOLEN ", CMD_GARAGE_ADDLASTSTOLEN},
        // STRING: GAME 0x6335d4
        {"GARAGE_SHOW ", CMD_GARAGE_SHOW},
        // STRING: GAME 0x6335c0
        {"GETACTIVEPLAYER ", CMD_GETACTIVEPLAYER},
        // STRING: GAME 0x6335a8
        {"GETANGLEACTORTOACTOR ", CMD_GETANGLEACTORTOACTOR},
        // STRING: GAME 0x63358c
        {"POLICEMANAGER_FORCEARREST ", CMD_POLICEMANAGER_FORCEARREST},
        // STRING: GAME 0x633564
        {"SETCITYTRAFFICVISIBLE ", CMD_SETCITYTRAFFICVISIBLE},
        // STRING: GAME 0x63357c
        {"CAR_SETSPEED ", CMD_CAR_SETSPEED},
        // STRING: GAME 0x633554
        {"SHOWCARDAMAGE ", CMD_SHOWCARDAMAGE},
        // STRING: GAME 0x633540
        {"CONSOLE_ADDTEXT ", CMD_CONSOLE_ADDTEXT},
        // STRING: GAME 0x63352c
        {"CAMERA_SETRANGE ", CMD_CAMERA_SETRANGE},
        // STRING: GAME 0x633518
        {"ACTOR_DUPLICATE ", CMD_ACTOR_DUPLICATE},
        // STRING: GAME 0x633508
        {"ACTOR_DELETE ", CMD_ACTOR_DELETE},
        // STRING: GAME 0x6334f8
        {"CAR_FORCESTOP ", CMD_CAR_FORCESTOP},
        // STRING: GAME 0x6334e0
        {"WINGMAN_DELINDICATOR ", CMD_WINGMAN_DELINDICATOR},
        // STRING: GAME 0x6334d0
        {"HUMAN_RESET ", CMD_HUMAN_RESET},
        // STRING: GAME 0x6334c0
        {"HUMAN_HAVEBOX ", CMD_HUMAN_HAVEBOX},
        // STRING: GAME 0x6334b0
        {"HUMAN_HAVEBODY ", CMD_HUMAN_HAVEBODY},
        // STRING: GAME 0x6334a0
        {"HUMAN_STOPTALK ", CMD_HUMAN_STOPTALK},
        // STRING: GAME 0x633490
        {"ENEMY_BLOCK ", CMD_ENEMY_BLOCK},
        // STRING: GAME 0x633480
        {"ENEMY_UNBLOCK ", CMD_ENEMY_UNBLOCK},
        // STRING: GAME 0x63346c
        {"GET_REMOTE_FLOAT ", CMD_GET_REMOTE_FLOAT},
        // STRING: GAME 0x633458
        {"GET_REMOTE_FRAME ", CMD_GET_REMOTE_FRAME},
        // STRING: GAME 0x633444
        {"GET_REMOTE_ACTOR ", CMD_GET_REMOTE_ACTOR},
        // STRING: GAME 0x633430
        {"SET_REMOTE_FLOAT ", CMD_SET_REMOTE_FLOAT},
        // STRING: GAME 0x63341c
        {"SET_REMOTE_FRAME ", CMD_SET_REMOTE_FRAME},
        // STRING: GAME 0x633408
        {"SET_REMOTE_ACTOR ", CMD_SET_REMOTE_ACTOR},
        // STRING: GAME 0x6333f8
        {"GETACTORFRAME ", CMD_GETACTORFRAME},
        // STRING: GAME 0x6333ec
        {"ZATMYSE ", CMD_ZATMYSE},
        // STRING: GAME 0x6333d8
        {"CARDAMAGEVISIBLE ", CMD_CARDAMAGEVISIBLE},
        // STRING: GAME 0x6333cc
        {"[DEBUG_ME] ", CMD_DEBUG_ME},
        // STRING: GAME 0x6333bc
        {"[DEBUG_STOP] ", CMD_DEBUG_STOP},
        // STRING: GAME 0x6333a8
        {"IFPLAYERSTEALCAR ", CMD_IFPLAYERSTEALCAR},
        // STRING: GAME 0x633388
        {"INTRO_SUBTITLE_ADD ", CMD_INTRO_SUBTITLE_ADD},
        // STRING: GAME 0x63339c
        {"MATH_COS ", CMD_MATH_COS},
        // STRING: GAME 0x63337c
        {"MATH_SIN ", CMD_MATH_SIN},
        // STRING: GAME 0x633370
        {"MATH_ACOS ", CMD_MATH_ACOS},
        // STRING: GAME 0x633364
        {"MATH_ASIN ", CMD_MATH_ASIN},
        // STRING: GAME 0x633358
        {"MATH_ATAN ", CMD_MATH_ATAN},
        // STRING: GAME 0x63334c
        {"MATH_ABS ", CMD_MATH_ABS},
        // STRING: GAME 0x633340
        {"FRM_GETDIR ", CMD_FRM_GETDIR},
        // STRING: GAME 0x633334
        {"FRM_SETDIR ", CMD_FRM_SETDIR},
        // STRING: GAME 0x633328
        {"FRM_GETPOS ", CMD_FRM_GETPOS},
        // STRING: GAME 0x63331c
        {"FRM_SETPOS ", CMD_FRM_SETPOS},
        // STRING: GAME 0x63330c
        {"FRM_GETSCALE ", CMD_FRM_GETSCALE},
        // STRING: GAME 0x6332fc
        {"FRM_SETSCALE ", CMD_FRM_SETSCALE},
        // STRING: GAME 0x6332f0
        {"FRM_GETROT ", CMD_FRM_GETROT},
        // STRING: GAME 0x6332e4
        {"FRM_SETROT ", CMD_FRM_SETROT},
        // STRING: GAME 0x6332d0
        {"FRM_GETWORLDDIR ", CMD_FRM_GETWORLDDIR},
        // STRING: GAME 0x6332bc
        {"FRM_GETWORLDPOS ", CMD_FRM_GETWORLDPOS},
        // STRING: GAME 0x6332a8
        {"FRM_GETWORLDSCALE ", CMD_FRM_GETWORLDSCALE},
        // STRING: GAME 0x633294
        {"FRM_GETWORLDROT ", CMD_FRM_GETWORLDROT},
        // STRING: GAME 0x633280
        {"FRM_GETLOCALMATRIX ", CMD_FRM_GETLOCALMATRIX},
        // STRING: GAME 0x63326c
        {"FRM_GETWORLDMATRIX ", CMD_FRM_GETWORLDMATRIX},
        // STRING: GAME 0x633260
        {"VECT_SET ", CMD_VECT_SET},
        // STRING: GAME 0x633254
        {"VECT_COPY ", CMD_VECT_COPY},
        // STRING: GAME 0x633244
        {"VECT_MUL_SCL ", CMD_VECT_MUL_SCL},
        // STRING: GAME 0x633234
        {"VECT_MUL_VECT ", CMD_VECT_MUL_VECT},
        // STRING: GAME 0x633224
        {"VECT_MUL_QUAT ", CMD_VECT_MUL_QUAT},
        // STRING: GAME 0x633210
        {"VECT_MUL_MATRIX ", CMD_VECT_MUL_MATRIX},
        // STRING: GAME 0x633200
        {"VECT_ADD_VECT ", CMD_VECT_ADD_VECT},
        // STRING: GAME 0x6331f0
        {"VECT_SUB_VECT ", CMD_VECT_SUB_VECT},
        // STRING: GAME 0x6331e4
        {"VECT_CROSS ", CMD_VECT_CROSS},
        // STRING: GAME 0x6331d4
        {"VECT_NORMALIZE ", CMD_VECT_NORMALIZE},
        // STRING: GAME 0x6331c4
        {"VECT_MAGNITUDE ", CMD_VECT_MAGNITUDE},
        // STRING: GAME 0x6331b4
        {"VECT_INVERSE ", CMD_VECT_INVERSE},
        // STRING: GAME 0x6331a4
        {"VECT_ANGLETO ", CMD_VECT_ANGLETO},
        // STRING: GAME 0x633194
        {"MATRIX_ZERO ", CMD_MATRIX_ZERO},
        // STRING: GAME 0x633180
        {"MATRIX_IDENTITY ", CMD_MATRIX_IDENTITY},
        // STRING: GAME 0x633170
        {"MATRIX_COPY ", CMD_MATRIX_COPY},
        // STRING: GAME 0x633160
        {"MATRIX_INVERSE ", CMD_MATRIX_INVERSE},
        // STRING: GAME 0x633154
        {"MATRIX_MUL ", CMD_MATRIX_MUL},
        // STRING: GAME 0x633148
        {"QUAT_COPY ", CMD_QUAT_COPY},
        // STRING: GAME 0x63313c
        {"QUAT_MAKE ", CMD_QUAT_MAKE},
        // STRING: GAME 0x63312c
        {"QUAT_SETDIR ", CMD_QUAT_SETDIR},
        // STRING: GAME 0x63311c
        {"QUAT_EXTRACT ", CMD_QUAT_EXTRACT},
        // STRING: GAME 0x63310c
        {"QUAT_INVERSE ", CMD_QUAT_INVERSE},
        // STRING: GAME 0x6330f8
        {"QUAT_GETROTMATRIX ", CMD_QUAT_GETROTMATRIX},
        // STRING: GAME 0x6330ec
        {"QUAT_ADD ", CMD_QUAT_ADD},
        // STRING: GAME 0x6330e0
        {"QUAT_SUB ", CMD_QUAT_SUB},
        // STRING: GAME 0x6330d0
        {"QUAT_MUL_SCL ", CMD_QUAT_MUL_SCL},
        // STRING: GAME 0x6330c0
        {"QUAT_MUL_QUAT ", CMD_QUAT_MUL_QUAT},
        // STRING: GAME 0x6330b4
        {"QUAT_DOT ", CMD_QUAT_DOT},
        // STRING: GAME 0x6330a4
        {"QUAT_NORMALIZE ", CMD_QUAT_NORMALIZE},
        // STRING: GAME 0x633090
        {"QUAT_ROTBYMATRIX ", CMD_QUAT_ROTBYMATRIX},
        // STRING: GAME 0x633084
        {"QUAT_SLERP ", CMD_QUAT_SLERP},
        // STRING: GAME 0x633074
        {"COLL_TESTLINE ", CMD_COLL_TESTLINE},
        // STRING: GAME 0x63306c
        {"VLVP ", CMD_VLVP},
        // STRING: GAME 0x633054
        {"POLICEITCHFORPLAYER ", CMD_POLICEITCHFORPLAYER},
        // STRING: GAME 0x633048
        {"CTRL_READ ", CMD_CTRL_READ},
        // STRING: GAME 0x633038
        {"CTRL_READEX ", CMD_CTRL_READEX},
        // STRING: GAME 0x633024
        {"GETFRAMEFROMACTOR ", CMD_GETFRAMEFROMACTOR},
        // STRING: GAME 0x63300c
        {"ACTORUPDATEPLACEMENT ", CMD_ACTORUPDATEPLACEMENT},
        // STRING: GAME 0x632fe4
        {"RACING_AUTOINVISIBLE ", CMD_RACING_AUTOINVISIBLE},
        // STRING: GAME 0x632ffc
        {"CAR_INVISIBLE ", CMD_CAR_INVISIBLE},
        // STRING: GAME 0x632fcc
        {"CAR_SWITCHSHOWENERGY ", CMD_CAR_SWITCHSHOWENERGY},
        // STRING: GAME 0x632fb8
        {"DEBUG_GETFRAMEINFO ", CMD_DEBUG_GETFRAMEINFO},
        // STRING: GAME 0x632fa8
        {"NOANIMPRELOAD ", CMD_NOANIMPRELOAD},
        // STRING: GAME 0x632f90
        {"DESTROY_PHYSICALOBJECT ", CMD_DESTROY_PHYSICALOBJECT},
        // STRING: GAME 0x632f80
        {"COMPAREACTORS ", CMD_COMPAREACTORS},
        // STRING: GAME 0x632f70
        {"SETNULLACTOR ", CMD_SETNULLACTOR},
        // STRING: GAME 0x632f60
        {"COMPAREFRAMES ", CMD_COMPAREFRAMES},
        // STRING: GAME 0x632f50
        {"SETNULLFRAME ", CMD_SETNULLFRAME},
        // STRING: GAME 0x632f40
        {"SETFREERIDE ", CMD_SETFREERIDE},
        // STRING: GAME 0x632f34
        {"ENABLEMAP ", CMD_ENABLEMAP},
        // STRING: GAME 0x632f20
        {"FRAMEISTELEPHONE ", CMD_FRAMEISTELEPHONE},
        // STRING: GAME 0x632f14
        {"CAR_CALM ", CMD_CAR_CALM},
        // STRING: GAME 0x632f00
        {"BRIDGE_SHUTDOWN ", CMD_BRIDGE_SHUTDOWN},
        // STRING: GAME 0x632eec
        {"CARLIGHT_INDIC_OFF ", CMD_CARLIGHT_INDIC_OFF},
        // STRING: GAME 0x632ed8
        {"SETTRAFFSECTORSND ", CMD_SETTRAFFSECTORSND},
        // STRING: GAME 0x632ec8
        {"CITYMUSIC_ON ", CMD_CITYMUSIC_ON},
        // STRING: GAME 0x632eb8
        {"CITYMUSIC_OFF ", CMD_CITYMUSIC_OFF},
        // STRING: GAME 0x632ea8
        {"[DAN_DEBUG] ", CMD_DAN_DEBUG},
        // STRING: GAME 0x632e98
        {"FRM_GETPARENT ", CMD_FRM_GETPARENT},
        // STRING: GAME 0x632e84
        {"ISPOINTINSECTOR ", CMD_ISPOINTINSECTOR},
        // STRING: GAME 0x632e74
        {"ISPOINTINAREA ", CMD_ISPOINTINAREA},
        // STRING: GAME 0x632e60
        {"GETACTIVECAMERA ", CMD_GETACTIVECAMERA},
        // STRING: GAME 0x632e4c
        {"SOUND_GETVOLUME ", CMD_SOUND_GETVOLUME},
        // STRING: GAME 0x632e38
        {"SOUND_SETVOLUME ", CMD_SOUND_SETVOLUME},
        // STRING: GAME 0x632e20
        {"RACING_CHANGE_MODEL ", CMD_RACING_CHANGE_MODEL},
        // STRING: GAME 0x632e0c
        {"CITYCACHING_TICK ", CMD_CITYCACHING_TICK},
        // STRING: GAME 0x632df8
        {"ACTORLIGHTNESSON ", CMD_ACTORLIGHTNESSON},
        // STRING: GAME 0x632dec
        {"CAR_LOCK ", CMD_CAR_LOCK},
        // STRING: GAME 0x632ddc
        {"CAR_LOCK_ALL ", CMD_CAR_LOCK_ALL},
        // STRING: GAME 0x632dcc
        {"PM_SHOWSYMBOL ", CMD_PM_SHOWSYMBOL},
        // STRING: GAME 0x632db8
        {"PM_SHOWPROGRESS ", CMD_PM_SHOWPROGRESS},
        // STRING: GAME 0x632da8
        {"PM_SETPROGRESS ", CMD_PM_SETPROGRESS},
        // STRING: GAME 0x632d98
        {"SETFILMMUSIC ", CMD_SETFILMMUSIC},
        // STRING: GAME 0x632d88
        {"GETFILMMUSIC ", CMD_GETFILMMUSIC},
        // STRING: GAME 0x632d74
        {"HUMAN_GETSEATIDX ", CMD_HUMAN_GETSEATIDX},
        // STRING: GAME 0x632d60
        {"HUMAN_ENTERTOTRUCK ", CMD_HUMAN_ENTERTOTRUCK},
        // STRING: GAME 0x632d50
        {"FLOATREG_PUSH ", CMD_FLOATREG_PUSH},
        // STRING: GAME 0x632d40
        {"FLOATREG_POP ", CMD_FLOATREG_POP},
        // STRING: GAME 0x632d2c
        {"AIRPLANESHOWDAMAGE ", CMD_AIRPLANESHOWDAMAGE},
        // STRING: GAME 0x632d18
        {"SETNPCKILLEVENT ", CMD_SETNPCKILLEVENT},
        // STRING: GAME 0x632d08
        {"STREAM_SETPOS ", CMD_STREAM_SETPOS},
        // STRING: GAME 0x632cf8
        {"STREAM_GETPOS ", CMD_STREAM_GETPOS},
        // STRING: GAME 0x632ce4
        {"CAR_GETSEATCOUNT ", CMD_CAR_GETSEATCOUNT},
        // STRING: GAME 0x632cd4
        {"FRM_SETALPHA ", CMD_FRM_SETALPHA},
        // STRING: GAME 0x632cc0
        {"HUMAN_THROWGRENADE ", CMD_HUMAN_THROWGRENADE},
        // STRING: GAME 0x632ca8
        {"DETECTOR_CREATEDYNCOLL ", CMD_DETECTOR_CREATEDYNCOLL},
        // STRING: GAME 0x632c90
        {"DETECTOR_ERASEDYNCOLL ", CMD_DETECTOR_ERASEDYNCOLL},
        // STRING: GAME 0x632c7c
        {"CAR_UNBREAKABLE ", CMD_CAR_UNBREAKABLE},
        // STRING: GAME 0x632c6c
        {"PHOBJ_IMPULS ", CMD_PHOBJ_IMPULS},
        // STRING: GAME 0x632c54
        {"WEATHER_PREPAREBUFFER ", CMD_WEATHER_PREPAREBUFFER},
        // STRING: GAME 0x632c48
        {"SUBTITLE ", CMD_SUBTITLE},
        // STRING: GAME 0x632c38
        {"HUMAN_CANDIE ", CMD_HUMAN_CANDIE},
        // STRING: GAME 0x632c24
        {"VERSION_IS_GERMANY ", CMD_VERSION_IS_GERMANY},
        // STRING: GAME 0x632c10
        {"VERSION_IS_EDITOR ", CMD_VERSION_IS_EDITOR},
        // STRING: GAME 0x632bfc
        {"GARAGE_NEZAHAZUJ ", CMD_GARAGE_NEZAHAZUJ},
        // STRING: GAME 0x632bec
        {"DAN_INTERNAL_1 ", CMD_DAN_INTERNAL_1},
        // STRING: GAME 0x632bdc
        {"DAN_INTERNAL_2 ", CMD_DAN_INTERNAL_2},
        // STRING: GAME 0x632bcc
        {"POLICE_SUPPORT ", CMD_POLICE_SUPPORT},
        // STRING: GAME 0x632bbc
        {"PLAY_AVI_INTRO ", CMD_PLAY_AVI_INTRO},
        // STRING: GAME 0x632bac
        {"CAMERA_SETFOV ", CMD_CAMERA_SETFOV},
        // STRING: GAME 0x632b9c
        {"CAMERA_GETFOV ", CMD_CAMERA_GETFOV},
        // STRING: GAME 0x632b8c
        {"DOOR_GETSTATE ", CMD_DOOR_GETSTATE},
        // STRING: GAME 0x632b78
        {"CAR_GETMAXLEVELS ", CMD_CAR_GETMAXLEVELS},
        // STRING: GAME 0x632b64
        {"CAR_GETACTLEVEL ", CMD_CAR_GETACTLEVEL},
        // STRING: GAME 0x632b50
        {"CAR_SETACTLEVEL ", CMD_CAR_SETACTLEVEL},
        // STRING: GAME 0x632b3c
        {"FREERIDE_SCOREON ", CMD_FREERIDE_SCOREON},
        // STRING: GAME 0x632b28
        {"FREERIDE_SCORESET ", CMD_FREERIDE_SCORESET},
        // STRING: GAME 0x632b14
        {"FREERIDE_SCOREGET ", CMD_FREERIDE_SCOREGET},
        // STRING: GAME 0x632b00
        {"FREERIDE_SCOREADD ", CMD_FREERIDE_SCOREADD},
        // STRING: GAME 0x632aec
        {"TAXIDRIVER_ENABLE ", CMD_TAXIDRIVER_ENABLE},
        // STRING: GAME 0x632ad4
        {"HUMAN_RETURNTOTRAFF ", CMD_HUMAN_RETURNTOTRAFF},
        // STRING: GAME 0x632ac0
        {"TIMER_GETINTERVAL ", CMD_TIMER_GETINTERVAL},
        // STRING: GAME 0x632aac
        {"TIMER_SETINTERVAL ", CMD_TIMER_SETINTERVAL},
        // STRING: GAME 0x632a98
        {"HUMAN_WAITTOREADY ", CMD_HUMAN_WAITTOREADY},
        // STRING: GAME 0x632a84
        {"FRM_GETNUMCHILDREN ", CMD_FRM_GETNUMCHILDREN},
        // STRING: GAME 0x632a74
        {"FRM_GETCHILD ", CMD_FRM_GETCHILD},
        // STRING: GAME 0x632a60
        {"ACTOR_ADDDORICI ", CMD_ACTOR_ADDDORICI},
        // STRING: GAME 0x632a54
        {"CAR_REPAIR ", CMD_CAR_REPAIR},
        // STRING: GAME 0x632a3c
        {"POLICE_SPEED_FACTOR ", CMD_POLICE_SPEED_FACTOR},
        // STRING: GAME 0x632a2c
        {"SUBTITLE_ADD ", CMD_SUBTITLE_ADD},
        // STRING: GAME 0x632a14
        {"CARTRIDGE_INVALIDATE ", CMD_CARTRIDGE_INVALIDATE},
        // STRING: GAME 0x632a08
        {"NPC_SHUTUP ", CMD_NPC_SHUTUP},
        // STRING: GAME 0x6329f4
        {"FREERIDE_ENABLECAR ", CMD_FREERIDE_ENABLECAR},
        // STRING: GAME 0x6329dc
        {"GETCARLINENUMFROMTABLE ", CMD_GETCARLINENUMFROMTABLE},
        // STRING: GAME 0x6329c4
        {"ENEMY_GETHOSTILESTATE ", CMD_ENEMY_GETHOSTILESTATE},
        // STRING: GAME 0x6329b4
        {"HUMAN_SHUTUP ", CMD_HUMAN_SHUTUP},
        // STRING: GAME 0x6329a4
        {"PUMPER_CANWORK ", CMD_PUMPER_CANWORK},
        // STRING: GAME 0x632994
        {"GROUP_DISBAND ", CMD_GROUP_DISBAND},
        // STRING: GAME 0x632984
        {"CAR_DISABLE_UO ", CMD_CAR_DISABLE_UO},
        // STRING: GAME 0x632970
        {"INVENTORY_CLEAR ", CMD_INVENTORY_CLEAR},
        // STRING: GAME 0x632960
        {"GETLASTSAVENUM ", CMD_GETLASTSAVENUM},
        // STRING: GAME 0x632948
        {"FREERIDE_TRAFFSETUP ", CMD_FREERIDE_TRAFFSETUP},
        // STRING: GAME 0x632930
        {"POCZKURVENYCHBEDENCAR ", CMD_POCZKURVENYCHBEDENCAR},
        // STRING: GAME 0x63290c
        {"HUMAN_GETACTANIMID ", CMD_HUMAN_GETACTANIMID},
        // STRING: GAME 0x632920
        {"CAR_SETAXIS ", CMD_CAR_SETAXIS},
        // STRING: GAME 0x632900
        {"FRM_ISON ", CMD_FRM_ISON},
        // STRING: GAME 0x6328e8
        {"HUMAN_GETITEMINRHAND ", CMD_HUMAN_GETITEMINRHAND},
        // STRING: GAME 0x6328d4
        {"GET_PM_HUMANSTATE ", CMD_GET_PM_HUMANSTATE},
        // STRING: GAME 0x6328b8
        {"M7_ZASTAVZKURVENEPOLISE ", CMD_M7_ZASTAVZKURVENEPOLISE},
        // STRING: GAME 0x6328a0
        {"ISVALIDTAXIPASSENGER ", CMD_ISVALIDTAXIPASSENGER},
        // STRING: GAME 0x63288c
        {"GRABER_SETFOCUS ", CMD_GRABER_SETFOCUS},
        // STRING: GAME 0x632878
        {"VECT_ANGLETO_2D ", CMD_VECT_ANGLETO_2D},
        // STRING: GAME 0x632868
        {"VECT_ROTATE_2D ", CMD_VECT_ROTATE_2D}
    };
    for (int i = 0; i < NUM_COMMANDS; i++)
    {
        int name_len = strlen(commands[i].name);
        int line_len = strlen(line);
        if (line_len < name_len - (line[line_len - 1] == ' ' ? 0 : 1))
            continue;
        len = Min(name_len, line_len);
        if (!_strnicmp(line, commands[i].name, len))
            return commands[i].command;
    }
    return -1;
}

// A line is a command, its name and its parameters; leading spaces, empty lines and comments are skipped
// FUNCTION: GAME 0x46c770
int C_program::Compile(char *src, bool one_line, int *err_pos, int *err_len)
{
    instructions.clear();
    if (!src || strlen(src) < 1)
        return 0;
    int errors = 0;
    char *line = src;
    char *p = src;
    for (;;)
    {
        char c;
        while (true)
        {
            c = *p;
            if (c == '\r' || c == '\n')
                break;
            if (!c)
                break;
            p++;
        }
        bool last = !c;
        *p = 0;
        while (*line == ' ')
            line++;
        // STRING: GAME 0x634378
        if (strncmp(line, "//", 2))
        {
            int len;
            int command = DecodeInstruction(line, len);
            if (command == -1)
            {
                errors++;
                if (err_pos && err_len)
                {
                    *err_pos = line - src;
                    *err_len = p - line;
                }
                if (one_line)
                    break;
            }
            else
            {
                S_instruction ins;
                memset(&ins, 0, sizeof(ins));
                ins.command = command;
                char *params = line + len;
                while (*params == ' ')
                    params++;
                if (DecodeParams(params, ins))
                    instructions.push_back(ins);
                else
                {
                    errors++;
                    if (err_pos && err_len)
                    {
                        *err_pos = line - src;
                        *err_len = p - line;
                    }
                    if (one_line)
                        break;
                }
            }
        }
        if (last)
            break;
        while (*p < ' ')
        {
            p++;
            if (!*p)
                break;
        }
        if (!*p)
            break;
        line = p;
    }
    return errors;
}

// FUNCTION: GAME 0x46ca70 FOLDED
void C_program::DecompileCommand(int, char *)
{
}

// FUNCTION: GAME 0x46ca80
float C_program::GetFloatReg(int i)
{
    if (i >= 0 && i < num_float_regs)
        return float_regs[i];
    return 0.0f;
}

// FUNCTION: GAME 0x46caa0
I3D_frame *C_program::GetFrameReg(int i)
{
    if (i >= 0 && i < num_frame_regs)
        return frame_regs[i];
    return NULL;
}

// FUNCTION: GAME 0x46cac0
C_actor *C_program::GetActorReg(int i)
{
    if (i >= 0 && i < num_actor_regs)
        return actor_regs[i];
    return NULL;
}

// FUNCTION: GAME 0x46cae0
void C_program::SetFloatReg(int i, float f)
{
    if (i >= 0 && i < num_float_regs)
        float_regs[i] = f;
}

// FUNCTION: GAME 0x46cb00
void C_program::SetFrameReg(int i, I3D_frame *frm)
{
    if (i >= 0 && i < num_frame_regs)
        frame_regs[i] = frm;
}

// FUNCTION: GAME 0x46cb20
void C_program::SetActorReg(int i, C_actor *act)
{
    if (i >= 0 && i < num_actor_regs)
        actor_regs[i] = act;
}

// The labels are searched from the last command, as PS2's
// FUNCTION: GAME 0x46cb40
void C_program::NextCommand(char *label)
{
    command_time = 0;
    prev_command = cur_command;
    if (!label)
    {
        cur_command++;
        return;
    }
    int found = -1;
    int i = instructions.size();
    while (i--)
    {
        if (instructions[i].command == CMD_LABEL && !_stricmp(label, (char *)instructions[i].params))
        {
            found = i;
            break;
        }
    }
    if (found == -1)
        cur_command++;
    else
        cur_command = found;
}

// FUNCTION: GAME 0x46cbc0
void C_program::CallSubroutine()
{
    cur_command = 0;
    gosub_stack.clear();
    actor = NULL;
    run_through = true;
    CallProcess(1);
}

// The script's ends stop the search: its first command going up, its last going down
// FUNCTION: GAME 0x46cc00
bool C_program::LeaveCompactBlock(bool up)
{
    if (!in_block)
        return false;
    int dir;
    if (block_mode == BLOCK_MODE_UP)
        dir = -1;
    else if (block_mode == BLOCK_MODE_DOWN)
        dir = 1;
    else
        dir = up ? 1 : -1;
    while (true)
    {
        if (instructions[cur_command].command == CMD_BLOCK_BEGIN && dir == -1)
            break;
        if (instructions[cur_command].command == CMD_BLOCK_END && dir == 1)
            break;
        cur_command += dir;
        if (cur_command < 0)
        {
            cur_command = 0;
            break;
        }
        if (cur_command >= instructions.size())
        {
            cur_command = instructions.size() - 1;
            break;
        }
    }
    return true;
}

// The command before CMD_END runs is the last
// FUNCTION: GAME 0x46ccb0
void C_program::CallProcess(uint time)
{
    do
    {
        if (cur_command >= instructions.size())
            return;
        if (instructions[cur_command].command == CMD_END)
            run_through = false;
        Process(time);
    } while (run_through);
}

// FUNCTION: GAME 0x46ccf0
void C_program::ClearAfterEnd()
{
    if (saved_code)
    {
        cur_command = saved_code->return_command;
        instructions.clear();
        for (int i = 0; i < saved_code->instructions.size(); i++)
            instructions.push_back(saved_code->instructions[i]);
        S_saved_code *code = saved_code;
        saved_code = saved_code->next;
        delete code;
    }
    if (actor)
        actor->SetActState(ACTSTATE_2);
}

// FUNCTION: GAME 0x46cd90
bool C_program::SetEvent(int id)
{
    int found = -1;
    int i = instructions.size();
    while (i--)
    {
        if (instructions[i].command == CMD_EVENT && ((int *)instructions[i].params)[0] == id)
        {
            found = i;
            break;
        }
    }
    if (found == -1)
        return true;
    i = gosub_stack.size();
    while (i--)
    {
        if (gosub_stack[i] & GOSUB_EVENT)
            return false;
    }
    CallBack(CB_EVENT_BEGIN);
    gosub_stack.push_back(cur_command | GOSUB_EVENT | GOSUB_EVENT_TIME);
    cur_command = found;
    event_command_time = command_time;
    event_time = act_mission->GetGame()->GetGameTime();
    command_time = 0;
    if (actor)
        actor->SetActState(ACTSTATE_0);
    return true;
}

// FUNCTION: GAME 0x46ce70
bool C_program::IsEventID(int id, int *param)
{
    int i = instructions.size();
    while (i--)
    {
        if (instructions[i].command == CMD_EVENT)
        {
            int *p = (int *)instructions[i].params;
            if (p[0] == id)
            {
                if (param)
                    *param = p[1];
                return true;
            }
        }
    }
    return false;
}

// FUNCTION: GAME 0x46cec0
void C_program::InvalidateActorReg(C_actor *act)
{
    int i = num_actor_regs;
    while (i--)
    {
        if (actor_regs[i] == act)
            actor_regs[i] = NULL;
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x46cef0
int C_program::SaveGameGetSize()
{
    StubLog("C_program::SaveGameGetSize");
    return 0;
}

// STUB: GAME 0x46cf70
bool C_program::SaveGameSave(void **buf)
{
    StubLog("C_program::SaveGameSave");
    return true;
}

// STUB: GAME 0x46d100
bool C_program::SaveGameLoad(void **buf)
{
    StubLog("C_program::SaveGameLoad");
    return true;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x46d380
void C_program::FindActorLater(int reg, uint id)
{
    if (reg >= 0 && reg < num_actor_regs)
        actor_regs[reg] = act_mission->GetGame()->GetActorFromGUID(id);
}

// SETLMLEVEL's helper (PS2's S_help, a local struct of the file's; here at file scope so that Process's lines hold
// one function for reccmp; its function is defined after Process)
struct S_help
{
    // A lightmapped object's level, the frame's and its children's but those in other sectors (PS2 name)
    static void SwitchLightMap(I3D_frame *frm, int level);
};

// The interpreter (PS2 name): runs the current command for the time passed. A command that is done goes on to the
// next one after the switch; one that waits returns, and the jumps set the next command themselves.
// FUNCTION: GAME 0x46d3b0
void C_program::Process(uint time)
{
    if (cur_command >= instructions.size())
        return;
    command_time += time;
    switch (instructions[cur_command].command)
    {
    case CMD_END:
        ClearAfterEnd();
        return;
    case CMD_GOSUB:
        gosub_stack.push_back(cur_command + 1);
    case CMD_GOTO:
    {
        // A register's command number as a label's name
        S_goto_params *g = (S_goto_params *)instructions[cur_command].params;
        if (strlen(g->label))
        {
            NextCommand(g->label);
            return;
        }
        if (num_float_regs > g->target.reg)
        {
            char label[16];
            // STRING: GAME 0x634410
            sprintf(label, "%i", (int)(float_regs[g->target.reg] + .5f));
            NextCommand(label);
            return;
        }
    }
        break;
    case CMD_WAIT:
    {
        uint wait = 0;
        S_operand *op = (S_operand *)instructions[cur_command].params;
        if (op->reg == -1)
            wait = (int)(op->value + .5f);
        else if (num_float_regs > op->reg)
            wait = (int)(float_regs[op->reg] + .5f);
        if (command_time < wait)
            return;
    }
        break;
    case CMD_RETURN:
        if (gosub_stack.size())
        {
            ushort ret = gosub_stack.back();
            if (ret & GOSUB_EVENT)
                CallBack(CB_EVENT_END);
            bool back_in_time = (ret & GOSUB_EVENT_TIME) != 0;
            ret &= GOSUB_COMMAND;
            gosub_stack.pop_back();
            NextCommand(NULL);
            cur_command = ret;
            if (back_in_time)
            {
                if (!LeaveCompactBlock(false))
                    command_time = event_command_time + act_mission->GetGame()->GetGameTime() - event_time;
            }
            if (stop_at_return)
            {
                stop_at_return = false;
                run_through = false;
            }
            return;
        }
        break;
    case CMD_DIM_FLT:
        if (num_float_regs < *(int *)instructions[cur_command].params || !*(int *)instructions[cur_command].params)
        {
            if (float_regs)
            {
                delete[] float_regs;
                float_regs = NULL;
            }
            num_float_regs = *(int *)instructions[cur_command].params;
            if (*(int *)instructions[cur_command].params)
            {
                float_regs = new float[*(int *)instructions[cur_command].params];
                memset(float_regs, 0, num_float_regs * sizeof(float));
            }
        }
        break;
    case CMD_LET:
    {
        S_let_params *let = (S_let_params *)instructions[cur_command].params;
        if (num_float_regs > let->dest && let->dest > -1 && num_float_regs > let->a.reg && num_float_regs > let->b.reg)
        {
            float a, b;
            if (let->a.reg == -1)
                a = let->a.value;
            else
                a = float_regs[let->a.reg];
            if (let->b.reg == -1)
                b = let->b.value;
            else
                b = float_regs[let->b.reg];
            switch (let->op)
            {
            case LET_OP_NONE:
                float_regs[let->dest] = a;
                break;
            case LET_OP_ADD:
                float_regs[let->dest] = a + b;
                break;
            case LET_OP_SUB:
                float_regs[let->dest] = a - b;
                break;
            case LET_OP_MUL:
                float_regs[let->dest] = a * b;
                break;
            case LET_OP_DIV:
                if (fabs(b) < MRG_ZERO)
                    float_regs[let->dest] = 0;
                else
                    float_regs[let->dest] = a / b;
                break;
            }
        }
    }
        break;
    case CMD_IF:
    {
        S_if_params *cond = (S_if_params *)instructions[cur_command].params;
        if (num_float_regs > cond->a.reg && num_float_regs > cond->b.reg)
        {
            float a, b;
            if (cond->a.reg < 0)
                a = cond->a.value;
            else
                a = float_regs[cond->a.reg];
            if (cond->b.reg < 0)
                b = cond->b.value;
            else
                b = float_regs[cond->b.reg];
            bool result = false;
            switch (cond->op)
            {
            case IF_OP_LESS:
                result = a < b;
                break;
            case IF_OP_GREATER:
                result = a > b;
                break;
            case IF_OP_EQUAL:
                result = a == b;
                break;
            case IF_OP_NOT_EQUAL:
                result = a != b;
                break;
            }
            if (result)
                NextCommand(cond->label1);
            else
                NextCommand(cond->label2);
            return;
        }
    }
        break;
    case CMD_RND:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_float_regs > p[0])
        {
            if (p[1] > 1)
                float_regs[p[0]] = (float)rnd(p[1]);
            else
                float_regs[p[0]] = rnd();
        }
    }
        break;
    case CMD_DIM_FRM:
        if (num_frame_regs < *(int *)instructions[cur_command].params || !*(int *)instructions[cur_command].params)
        {
            if (frame_regs)
            {
                delete[] frame_regs;
                frame_regs = NULL;
            }
            num_frame_regs = *(int *)instructions[cur_command].params;
            if (num_frame_regs)
            {
                frame_regs = new I3D_frame *[num_frame_regs];
                memset(frame_regs, 0, num_frame_regs * sizeof(I3D_frame *));
            }
        }
        break;
    case CMD_DIM_ACT:
        if (num_actor_regs < *(int *)instructions[cur_command].params || !*(int *)instructions[cur_command].params)
        {
            if (actor_regs)
            {
                delete[] actor_regs;
                actor_regs = NULL;
            }
            num_actor_regs = *(int *)instructions[cur_command].params;
            if (num_actor_regs)
            {
                actor_regs = new C_actor *[num_actor_regs];
                memset(actor_regs, 0, num_actor_regs * sizeof(C_actor *));
            }
        }
        break;
    case CMD_FINDFRAME:
    {
        // Without a name the script's actor's frame
        S_name_params *n = (S_name_params *)instructions[cur_command].params;
        if (num_frame_regs > n->reg)
        {
            if (strlen(n->name))
                frame_regs[n->reg] = act_mission->GetScene()->FindFrame(n->name, ENUMF_ALL | ENUMF_BACKDROP);
            else if (actor)
                frame_regs[n->reg] = actor->GetFrame();
            else
                frame_regs[n->reg] = NULL;
        }
    }
        break;
    case CMD_FINDACTOR:
    {
        // A debugging beep: "chcipnuti chlapka" is Czech for "the guy croaking"
        // STRING: GAME 0x6343fc
        if (actor && actor->GetFrame() && !_strcmpi(actor->GetFrame()->GetName(), "chcipnuti chlapka"))
            MessageBeep((UINT)-1);
        // The mission's actors from the last, the actors kept for the next mission, then every actor from the last;
        // without a name the script's actor. An actor kept for the next mission doesn't end the search (PS2's the
        // same).
        S_name_params *n = (S_name_params *)instructions[cur_command].params;
        if (num_actor_regs > n->reg)
        {
            if (strlen(n->name))
            {
                bool found = false;
                int i;
                std::vector<C_actor *> &actors = act_mission->GetActors();
                for (i = actors.size(); i--;)
                {
                    if (actors[i]->GetFrame() && !strcmp(actors[i]->GetFrame()->GetName(), n->name))
                    {
                        actor_regs[n->reg] = actors[i];
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    for (i = 0; i < 2; i++)
                    {
                        if (NewMissionActors[i] && !strcmp(NewMissionActors[i]->GetFrame()->GetName(), n->name))
                        {
                            actor_regs[n->reg] = NewMissionActors[i];
                            break;
                        }
                    }
                }
                if (!found)
                {
                    std::vector<C_actor *> &global_actors = C_actor::GetGlobalActors();
                    for (i = global_actors.size(); i--;)
                    {
                        if (global_actors[i] && global_actors[i]->GetFrame() &&
                            !strcmp(global_actors[i]->GetFrame()->GetName(), n->name))
                        {
                            actor_regs[n->reg] = global_actors[i];
                            break;
                        }
                    }
                }
            }
            else
                actor_regs[n->reg] = actor;
        }
    }
        break;
    case CMD_FRM_SETON:
    {
        // A car's frame stays as it is while the player drives it with the chase camera
        int *p = (int *)instructions[cur_command].params;
        I3D_frame *frm = actor ? actor->GetFrame() : NULL;
        if (num_frame_regs > p[0] && p[0] > -1)
            frm = frame_regs[p[0]];
        if (frm)
        {
            bool on = p[1] != 0;
            if (frm->GetType() == FRAME_SOUND)
            {
                ((I3D_sound *)frm)->SetOn(on, true);
                break;
            }
            C_actor **frm_actor = GetFrameActor(frm);
            bool set = true;
            if (on && frm_actor && *frm_actor && (*frm_actor)->GetType() == ACTOR_CAR &&
                *frm_actor == act_mission->GetGame()->GetPlayer()->GetCar() &&
                act_mission->GetGame()->GetCamera().GetMode() == CAMERA_C_CHASE)
                set = false;
            if (set)
            {
                act_mission->GetGame()->AddFramePlacement(frm);
                frm->SetOn(on);
            }
            if (!on)
                act_mission->GetGame()->ClearProjectorsForFrame(frm);
            if (frm_actor && *frm_actor)
            {
                switch ((*frm_actor)->GetType())
                {
                case ACTOR_PLAYER:
                case ACTOR_ENTITY:
                    if (on)
                    {
                        if (!((C_human *)*frm_actor)->car)
                            ((C_human *)*frm_actor)->shadow = act_mission->GetGame()->AddFrameForShadow(
                                (*frm_actor)->GetFrame(), SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
                    }
                    else
                        act_mission->GetGame()->DelFrameForShadow((*frm_actor)->GetFrame(), SHADOW_TYPE_0);
                    break;
                case ACTOR_CAR:
                    (*frm_actor)->Tick(1);
                    break;
                }
            }
        }
    }
        break;
    case CMD_FRM_SETON1:
    {
        // The frame's register in a float register
        int *p = (int *)instructions[cur_command].params;
        I3D_frame *frm = actor ? actor->GetFrame() : NULL;
        if (num_float_regs > p[0] && p[0] > -1)
        {
            int reg = (int)float_regs[p[0]];
            if (num_frame_regs > reg && reg > -1)
                frm = frame_regs[reg];
        }
        if (frm)
        {
            bool on = p[1] != 0;
            if (frm->GetType() == FRAME_SOUND)
            {
                ((I3D_sound *)frm)->SetOn(on, true);
                break;
            }
            act_mission->GetGame()->AddFramePlacement(frm);
            frm->SetOn(on);
            C_actor **frm_actor = GetFrameActor(frm);
            if (!on)
                act_mission->GetGame()->ClearProjectorsForFrame(frm);
            if (frm_actor && *frm_actor)
            {
                switch ((*frm_actor)->GetType())
                {
                case ACTOR_PLAYER:
                case ACTOR_ENTITY:
                    if (on)
                    {
                        if (!((C_human *)*frm_actor)->car)
                            ((C_human *)*frm_actor)->shadow = act_mission->GetGame()->AddFrameForShadow(
                                (*frm_actor)->GetFrame(), SHADOW_TYPE_0, SHADOW_TYPE_0, NULL, NULL);
                    }
                    else
                        act_mission->GetGame()->DelFrameForShadow((*frm_actor)->GetFrame(), SHADOW_TYPE_0);
                    break;
                case ACTOR_CAR:
                    (*frm_actor)->Tick(1);
                    break;
                }
            }
        }
    }
        break;
    case CMD_FRM_LINKTO:
    {
        // Without a parent the scene's primary sector
        int *p = (int *)instructions[cur_command].params;
        if (num_frame_regs > p[0] && p[0] >= 0 && frame_regs[p[0]])
        {
            if (num_frame_regs > p[1] && p[1] >= 0 && frame_regs[p[1]])
                frame_regs[p[0]]->LinkTo(frame_regs[p[1]], I3DLINK_UPDMATRIX);
            else
            {
                frame_regs[p[0]]->LinkTo(act_mission->GetScene()->GetPrimarySector(), I3DLINK_UPDMATRIX);
                act_mission->GetScene()->SetFrameSector(frame_regs[p[0]]);
            }
        }
    }
        break;
    case CMD_CALL:
    {
        // Runs the actor's script's commands in place of the script's own, which saved_code keeps with the command to
        // go on at
        bool next = true;
        int reg = *(int *)instructions[cur_command].params;
        if (reg >= 0 && num_actor_regs > reg && actor_regs[reg] && actor_regs[reg]->GetProgram())
        {
            next = false;
            S_saved_code *sc = new S_saved_code;
            sc->return_command = cur_command + 1;
            uint i;
            for (i = 0; i < instructions.size(); i++)
                sc->instructions.push_back(instructions[i]);
            sc->next = saved_code;
            saved_code = sc;
            cur_command = 0;
            std::vector<S_instruction> *program = actor_regs[reg]->GetProgram();
            instructions.clear();
            for (i = 0; i < program->size(); i++)
                instructions.push_back((*program)[i]);
        }
        if (next)
            NextCommand(NULL);
    }
        return;
    case CMD_IFFLTINRANGE:
    {
        char *label = NULL;
        S_ifflt_params *r = (S_ifflt_params *)instructions[cur_command].params;
        if (r->reg >= 0 && num_float_regs > r->reg && float_regs[r->reg] >= r->min && float_regs[r->reg] <= r->max &&
            strlen(r->label))
            label = r->label;
        NextCommand(label);
    }
        return;
    case CMD_PLAYSOUND:
    {
        // On the frame of a register, else on the script's actor's; the sound's number into a float register
        S_playsound_params *s = (S_playsound_params *)instructions[cur_command].params;
        I3D_frame *frm = NULL;
        if (s->unknown_40 >= 0 && num_frame_regs > s->unknown_40)
            frm = frame_regs[s->unknown_40];
        if (!frm)
            frm = actor->GetFrame();
        int sound;
        if (s->unknown_50 == 0.0f)
            sound = act_mission->GetGame()->Play3DSound(frm, s->name, I3DSOUND_POINT, S_vector(0.0f, 0.0f, 0.0f),
                s->unknown_44 / 2, s->unknown_44, s->unknown_48, false, .001f, false);
        else
            sound = act_mission->GetGame()->Play3DSound(frm, s->name, I3DSOUND_POINT, S_vector(0.0f, 0.0f, 0.0f),
                s->unknown_44 / 2, s->unknown_44, s->unknown_48, true, s->unknown_50, false);
        if (num_float_regs > s->unknown_4c && s->unknown_4c >= 0)
            float_regs[s->unknown_4c] = (float)sound;
    }
        break;
    case CMD_PLAYSOUNDSTOP:
    {
        // The sound's number in a float register
        S_int_float_params *s = (S_int_float_params *)instructions[cur_command].params;
        if (s->value1 < num_float_regs && s->value1 >= 0)
        {
            if (s->value2 == 0.0f)
                act_mission->GetGame()->Stop3DSound((int)float_regs[s->value1], false, .001f);
            else
                act_mission->GetGame()->Stop3DSound((int)float_regs[s->value1], true, s->value2);
        }
    }
        break;
    case CMD_ACT_SETSTATE:
    {
        // The script's actor without a register
        int *p = (int *)instructions[cur_command].params;
        C_actor *act = actor;
        if (num_actor_regs > p[0] && p[0] >= 0)
            act = actor_regs[p[0]];
        if (act)
        {
            switch (p[1])
            {
            case ACT_STATE_NONE:
                act->SetActState(ACTSTATE_2);
                break;
            case ACT_STATE_ACTIVE:
                act->SetActState(ACTSTATE_0);
                break;
            case ACT_STATE_INACTIVE:
                act->SetActState(ACTSTATE_1);
                break;
            }
        }
    }
        break;
    case CMD_DETECTOR_SETSIGNAL:
    {
        int *p = (int *)instructions[cur_command].params;
        C_actor *act = actor;
        if (num_actor_regs > p[0] && p[0] >= 0)
            act = actor_regs[p[0]];
        if (act && act->GetType() == ACTOR_DETECTOR)
            ((C_detector *)act)->SetSignal(p[1] != 0);
    }
        break;
    case CMD_DETECTOR_ISSIGNAL:
    {
        // An empty label goes on with the next command
        S_signal_params *s = (S_signal_params *)instructions[cur_command].params;
        C_actor *act = actor;
        if (num_actor_regs > s->reg && s->reg >= 0)
            act = actor_regs[s->reg];
        if (act && act->GetType() == ACTOR_DETECTOR)
        {
            char *label = ((C_detector *)act)->GetSignal() ? s->label1 : s->label2;
            NextCommand(strlen(label) ? label : NULL);
            return;
        }
    }
        break;
    case CMD_SETCOMPASS:
    {
        // Without a frame the compass is off
        int reg = *(int *)instructions[cur_command].params;
        if (reg >= 0 && num_frame_regs > reg)
        {
            mafiaIndicators.CompassSetDestination(frame_regs[reg]);
            mafiaIndicators.AddFlags(INDF_COMPASS);
            SetPlayerNaviPoint(frame_regs[reg]);
        }
        else
        {
            mafiaIndicators.RemoveFlags(INDF_COMPASS);
            SetPlayerNaviPoint(NULL);
        }
    }
        break;
    case CMD_SETLMLEVEL:
    {
        // A lightmapped object's level, or a sector's objects'
        int *p = (int *)instructions[cur_command].params;
        if (num_frame_regs > p[0] && p[0] >= 0 && frame_regs[p[0]])
        {
            if (frame_regs[p[0]]->GetType() == FRAME_VISUAL)
            {
                I3D_lit_object *lit = (I3D_lit_object *)frame_regs[p[0]];
                if (lit->GetVisualType() == I3D_VISUAL_LIT_OBJECT && lit->GetLevel(p[1]).constructed)
                    lit->SetActiveLevel(p[1]);
            }
            else if (frame_regs[p[0]]->GetType() == FRAME_SECTOR)
                S_help::SwitchLightMap(frame_regs[p[0]], p[1]);
        }
    }
        break;
    case CMD_DOOR_LOCK:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_DOOR)
            ((C_door *)actor_regs[p[0]])->locked = p[1] != 0;
    }
        break;
    case CMD_DOOR_OPEN:
    {
        // Opens a closed door or closes an open one, as the script's actor
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_DOOR)
        {
            C_door *door = (C_door *)actor_regs[p[0]];
            switch (door->GetState())
            {
            case C_door::STATE_CLOSED:
                if (p[1])
                    door->SetState(C_door::STATE_OPENING, actor, true, true);
                break;
            case C_door::STATE_OPEN:
                if (!p[1])
                    door->SetState(C_door::STATE_CLOSING, actor, true, true);
                break;
            }
        }
    }
        break;
    case CMD_DOOR_ENABLEUS:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_DOOR)
            ((C_door *)actor_regs[p[0]])->EnableUsingObject(p[1] != 0);
    }
        break;
    case CMD_COMMANDBLOCK:
        run_through = *(int *)instructions[cur_command].params != 0;
        break;
    case CMD_RECCLEAR:
        cutscene.RecClear();
        break;
    case CMD_RECADDACTOR:
    {
        S_name_params *n = (S_name_params *)instructions[cur_command].params;
        if (num_actor_regs > n->reg && n->reg >= 0)
            cutscene.RecAddActor(n->name, actor_regs[n->reg]);
    }
        break;
    case CMD_RECLOAD:
    {
        // records\<name>, .rep without an extension
        act_mission->GetGame()->RecPrepareHumans();
        char name[128];
        // STRING: GAME 0x6343f0
        sprintf(name, "records\\%s", (char *)instructions[cur_command].params);
        if (!strchr(name, '.'))
            // STRING: GAME 0x6343e8
            strcat(name, ".rep");
        cutscene.RecLoad(name);
    }
        break;
    case CMD_RECLOADFULL:
    {
        // The same as a cutscene: the camera locked, the HUD's flags kept and the bars on, the player out of play
        act_mission->GetGame()->RecPrepareHumans();
        S_vector x_axis(1.0f, 0.0f, 0.0f);
        S_vector y_axis(0.0f, 1.0f, 0.0f);
        act_mission->GetGame()->GetCamera().LockAt(x_axis, y_axis);
        char name[128];
        sprintf(name, "records\\%s", (char *)instructions[cur_command].params);
        if (!strchr(name, '.'))
            strcat(name, ".rep");
        mafiaIndicators.PushFlags();
        mafiaIndicators.SetFlags(0);
        mafiaIndicators.AddFlags(INDF_WIDESCREEN);
        cutscene.RecLoadFull(name);
        mafiaIndicators.WideScreenSetRatio(1.0f / 1.85f);
        mafiaIndicators.WideScreenSetOn(true, 250);
        if (act_mission->GetGame()->GetPlayer())
            act_mission->GetGame()->GetPlayer()->SetUnknown598(false);
        if (act_mission->GetUnknown78() < 1)
            act_mission->SetUnknown78(1);
        SetRecordPlaying(true);
    }
        break;
    case CMD_RECWAITFOREND:
        if (cutscene.IsPlaying())
            return;
        break;
    case CMD_RECUNLOAD:
        act_mission->GetGame()->DestroyUnknown2f8cParticles();
        cutscene.Close();
        act_mission->GetGame()->GetCamera().SetCutscene(false);
        mafiaIndicators.PopFlags();
        mafiaIndicators.WideScreenSetOn(false, 250);
        if (act_mission->GetGame()->GetPlayer())
            act_mission->GetGame()->GetPlayer()->SetUnknown598(true);
        if (act_mission->GetUnknown78() < 2)
            act_mission->SetUnknown78(2);
        act_mission->GetGame()->GetSoundArena().Init();
        SetRecordPlaying(false);
        act_mission->GetGame()->StreamDestroyAll();
        break;
    case CMD_CALLSUBROUTINE:
        act_mission->GlobalProgramRun((char *)instructions[cur_command].params);
        break;
    case CMD_SOUNDFADE:
    {
        // A sound's volume from value3 to value4 over value2 milliseconds, the command waiting till then
        S_fade_params *f = (S_fade_params *)instructions[cur_command].params;
        if (f->value1 >= 0 && num_frame_regs > f->value1 && frame_regs[f->value1] &&
            frame_regs[f->value1]->GetType() == FRAME_SOUND)
        {
            float t = (float)command_time / f->value2;
            I3D_sound *snd = (I3D_sound *)frame_regs[f->value1];
            if (t > 1.0f)
                t = 1.0f;
            float volume = (f->value4 - f->value3) * t + f->value3;
            if (!snd->IsOn())
                snd->SetOn(true);
            snd->SetVolume(volume);
            snd->Update();
        }
        if (command_time < (uint)f->value2)
            return;
    }
        break;
    case CMD_LOADCOLLTREE:
    {
        // A collision file of the mission's folder
        char name[100];
        // STRING: GAME 0x6343d8
        sprintf(name, "missions\\%s\\%s", act_mission->GetMissionName(), (char *)instructions[cur_command].params);
        gifCollision.LoadCollision(name, false);
    }
        break;
    case CMD_EXPLOSION:
    {
        // At the script's actor
        float *f = (float *)instructions[cur_command].params;
        S_vector pos = actor->GetFrame()->GetWorldPos();
        act_mission->GetGame()->NewExplosion(actor, pos, f[0], f[1], true, false, true, 0x12);
    }
        break;
    case CMD_TIMEROFF:
        mafiaIndicators.RemoveFlags(INDF_TIMER);
        break;
    case CMD_TIMERON:
    {
        // The time and its interval
        int *p = (int *)instructions[cur_command].params;
        mafiaIndicators.TimerSetTime(p[0], p[1], p[2]);
        mafiaIndicators.TimerSetInterval(p[3]);
        mafiaIndicators.AddFlags(INDF_TIMER);
        C_traffic_generator::ZrusVsechnyMavajici();
    }
        break;
    case CMD_CHANGE_MISSION:
    {
        // Not while the police chase the player. Main's loop opens the mission; the actors kept for it are the player
        // and his car, their directions and the player's position relative to the script's actor's frame.
        if (act_mission->GetGame()->GetPlayer() && AIManager.police.PlukovnikuMasVChladkuPlajeraFrafera())
            return;
        act_mission->GetGame()->SetUnknown3665(true);
        S_change_mission_params *m = (S_change_mission_params *)instructions[cur_command].params;
        strcpy(NewMission, m->name1);
        ActiveNewMission = true;
        strcpy(NewMission_ENTRY, m->name2);
        act_mission->KeepUnknown70();
        NewMissionSpeed = m->value;
        if (!strlen(NewMission_ENTRY))
            break;
        C_human *player = act_mission->GetGame()->GetPlayer();
        if (player)
        {
            if (player->GetCar())
            {
                NewMissionActors[0] = player->GetCar();
                NewMissionActors[1] = player;
            }
            else
            {
                NewMissionActors[0] = player;
                NewMissionActors[1] = NULL;
            }
        }
        for (int i = 0; i < 2; i++)
        {
            if (NewMissionActors[i])
                act_mission->RemoveActor(NewMissionActors[i]);
        }
        if (!NewMissionActors[0])
            break;
        NewMission_ROT[0] = S_vector(0.0f, 0.0f, 1.0f);
        NewMission_ROT[1] = S_vector(0.0f, 1.0f, 0.0f);
        S_matrix mat;
        mat = NewMissionActors[0]->GetFrame()->GetMatrix();
        mat(3, 0) = mat(3, 1) = mat(3, 2) = 0.0f;
        NewMission_ROT[0] *= mat;
        NewMission_ROT[1] *= mat;
        mat = actor->GetFrame()->GetMatrix();
        mat(3, 0) = mat(3, 1) = mat(3, 2) = 0.0f;
        mat.Invert();
        NewMission_ROT[0] *= mat;
        NewMission_ROT[1] *= mat;
        mat = actor->GetFrame()->GetMatrix();
        mat.Invert();
        NewMission_POS = NewMissionActors[0]->GetFrame()->GetWorldPos();
        NewMission_POS *= mat;
    }
        break;
    case CMD_CAMERA_SETSWING:
    {
        // Turning the swing off needs no frame
        S_float_int_params *p = (S_float_int_params *)instructions[cur_command].params;
        if ((p->value2 < 0 || num_frame_regs <= p->value2) && p->value1)
            break;
        act_mission->GetGame()->GetCamera().SetSwing(p->value1 != 0, p->value * .01f, frame_regs[p->value2]);
        break;
    }
    case CMD_AIRPLANE_START:
    {
        S_float_int_params *p = (S_float_int_params *)instructions[cur_command].params;
        if (p->value1 < 0 || num_actor_regs <= p->value1)
            break;
        if (!actor_regs[p->value1] || actor_regs[p->value1]->GetType() != ACTOR_AIRPLANE)
            break;
        // The speed in km/h, in m/s
        ((C_airplane *)actor_regs[p->value1])->Start(p->value2, p->value * (1.0f / 3.6f));
        break;
    }
    case CMD_GETACTORSDIST:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[2] < 0 || num_float_regs <= p[2])
            break;
        float_regs[p[2]] = 0.0f;
        if (p[0] < 0 || num_actor_regs <= p[0] || !actor_regs[p[0]])
            break;
        if (p[1] < 0 || num_actor_regs <= p[1] || !actor_regs[p[1]])
            break;
        float_regs[p[2]] = (actor_regs[p[0]]->GetFrame()->GetWorldPos() -
            actor_regs[p[1]]->GetFrame()->GetWorldPos()).Magnitude();
        // The fourth parameter: negative, at least 0.1, where the second actor has unknown_5e
        if (p[3] && actor_regs[p[1]]->GetUnknown5e())
        {
            if (float_regs[p[2]] < .1f)
                float_regs[p[2]] = .1f;
            float_regs[p[2]] *= -1.0f;
        }
        break;
    }
    case CMD_GETCARDAMAGE:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_float_regs <= p[1] || p[1] < 0)
            break;
        float_regs[p[1]] = 0.0f;
        if (num_actor_regs <= p[0] || p[0] < 0)
            break;
        if (!actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
            break;
        float_regs[p[1]] = ((C_car *)actor_regs[p[0]])->ShowCarEnergy();
        break;
    }
    case CMD_HUMAN_ADDWEAPON:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
            break;
        C_human *human = (C_human *)actor_regs[p[0]];
        S_GameItem item;
        item.id = (ushort)p[1];
        item.unknown_4 = p[2];
        item.unknown_8 = p[3];
        human->inventory.Insert(&item, false);
        human->ChangeWeaponModel();
        if (human->GetType() == ACTOR_ENTITY)
            ((C_entity *)human)->WeaponChoose(human->inventory.GetItemInRightHand(NULL));
        break;
    }
    case CMD_HUMAN_CANADDWEAPON:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
            break;
        if (num_float_regs <= p[2] || p[2] < 0)
            break;
        if (((C_human *)actor_regs[p[0]])->inventory.HaveSpaceFor(p[1], true))
            float_regs[p[2]] = 1.0f;
        else
            float_regs[p[2]] = 0.0f;
        break;
    }
    case CMD_HUMAN_DELWEAPON:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
            break;
        C_human *human = (C_human *)actor_regs[p[0]];
        S_GameItem item;
        item.id = (ushort)p[1];
        human->inventory.Remove(&item, NULL);
        human->ChangeWeaponModel();
        break;
    }
    case CMD_HUMAN_ACTIVATEWEAPON:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
            break;
        C_human *human = (C_human *)actor_regs[p[0]];
        // What the selection drops out of the inventory
        std::vector<S_GameItem> dropped;
        human->inventory.SelectByID(p[1], &dropped);
        human->ChangeWeaponModel();
        if (dropped.size())
            human->DropOutItems(dropped);
        if (human->GetType() == ACTOR_ENTITY)
            ((C_entity *)human)->WeaponChoose(p[1]);
        break;
    }
    case CMD_HUMAN_ANYWEAPONINHAND:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
            break;
        if (num_float_regs <= p[1] || p[1] < 0)
            break;
        float_regs[p[1]] =
            (float)(((C_human *)actor_regs[p[0]])->inventory.HaveSomeWeapon(true, ITEMF_WEAPON) ? 1 : 0);
        break;
    }
    case CMD_HUMAN_ANYWEAPONININVENTORY:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
            break;
        if (num_float_regs <= p[1] || p[1] < 0)
            break;
        float_regs[p[1]] =
            (float)(((C_human *)actor_regs[p[0]])->inventory.HaveSomeWeapon(false, ITEMF_WEAPON) ? 1 : 0);
        break;
    }
    case CMD_HUMAN_ISWEAPON:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[1] < 0 || num_float_regs <= p[1])
            break;
        float_regs[p[1]] = 0.0f;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
            break;
        float_regs[p[1]] = (float)(((C_human *)actor_regs[p[0]])->inventory.IsIn(p[2], false) ? 1 : 0);
        break;
    }
    case CMD_PRELOADMODEL:
    {
        // Loaded once, so that the model cache keeps it
        I3D_model *mod = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
        if (!mod)
            break;
        model_cache.Open(mod, (char *)instructions[cur_command].params, 0, NULL, NULL, NULL);
        mod->Release();
        break;
    }
    case CMD_LOADDIFFERENCES:
        act_mission->GetGame()->LoadDiffData((char *)instructions[cur_command].params);
        break;
    case CMD_CLEARDIFFERENCES:
        act_mission->GetGame()->SetUnknown2ad9();
        break;
    case CMD_BLOCK_BEGIN:
        in_block = true;
        block_mode = (uchar)*(int *)instructions[cur_command].params;
        break;
    case CMD_BLOCK_END:
        in_block = false;
        block_mode = BLOCK_MODE_NONE;
        break;
    case CMD_SETAIPRIORITY:
    {
        // 0 to 100, the script's value 1 to 0
        float *f = (float *)instructions[cur_command].params;
        if (*f > 100.0f)
            *f = 100.0f;
        else if (*f < 0.0f)
            *f = 0.0f;
        SetAIPriority((100.0f - *f) * .01f);
        break;
    }
    case CMD_SETEVENT:
    {
        // The event of an AI human's or a detector's script; where it can't begin now, the label
        S_event_params *p = (S_event_params *)instructions[cur_command].params;
        if (num_actor_regs <= p->value1 || p->value1 < 0 || !actor_regs[p->value1])
            break;
        bool ok = true;
        if (actor_regs[p->value1]->GetType() == ACTOR_ENTITY)
            ok = ((C_entity *)actor_regs[p->value1])->SetEvent(p->value2);
        else if (actor_regs[p->value1]->GetType() == ACTOR_DETECTOR)
            ok = ((C_detector *)actor_regs[p->value1])->SetEvent(p->value2);
        if (ok)
            break;
        NextCommand(p->name);
        return;
    }
    case CMD_EVENT_USE_CB:
        if (!gosub_stack.size())
            break;
        if (*(int *)instructions[cur_command].params)
            gosub_stack.back() |= GOSUB_EVENT_TIME;
        else
            gosub_stack.back() &= ~GOSUB_EVENT_TIME;
        break;
    case CMD_ENEMY_CHANGEANIM:
    {
        // The script's own actor
        if (actor->GetType() != ACTOR_ENTITY)
            break;
        S_enemy_changeanim_params *p = (S_enemy_changeanim_params *)instructions[cur_command].params;
        if (strlen(p->anim2))
            ((C_human *)actor)->ChangeAnim(p->anim1, p->anim2);
        else
            ((C_human *)actor)->ChangeAnim(p->anim1, NULL);
        break;
    }
    case CMD_HUMAN_CHANGEANIM:
    {
        S_changeanim_params *p = (S_changeanim_params *)instructions[cur_command].params;
        if (num_actor_regs <= p->reg || p->reg < 0 || !actor_regs[p->reg])
            break;
        if (actor_regs[p->reg]->GetType() != ACTOR_PLAYER && actor_regs[p->reg]->GetType() != ACTOR_ENTITY)
            break;
        C_human *human = (C_human *)actor_regs[p->reg];
        if (strlen(p->anim2))
            human->ChangeAnim(p->anim1, p->anim2);
        else
            human->ChangeAnim(p->anim1, NULL);
        break;
    }
    case CMD_DIALOG_BEGIN:
    {
        // The p[1] humans from the register p[0]: each has to be one
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] + p[1] - 1 || p[0] < 0)
            break;
        bool ok = true;
        for (int i = 0; i < p[1]; i++)
        {
            if (!actor_regs[p[0] + i] ||
                actor_regs[p[0] + i]->GetType() != ACTOR_ENTITY && actor_regs[p[0] + i]->GetType() != ACTOR_PLAYER)
            {
                ok = false;
                break;
            }
        }
        if (!ok)
            break;
        act_mission->GetGame()->GetCamera().DialogStart((C_human **)&actor_regs[p[0]], p[1]);
        mafiaIndicators.RemoveFlags(INDF_MAP);
        mafiaIndicators.RemoveFlags(INDF_CROSSHAIR);
        if (act_mission->GetGame()->GetPlayer())
            act_mission->GetGame()->GetPlayer()->SetUnknown598(false);
        break;
    }
    case CMD_DIALOG_END:
        act_mission->GetGame()->GetCamera().DialogStop();
        if (act_mission->GetGame()->GetPlayer())
        {
            ((C_player *)act_mission->GetGame()->GetPlayer())->IndicatorSetAimed();
            act_mission->GetGame()->GetPlayer()->SetUnknown598(true);
        }
        break;
    case CMD_DIALOG_CAMSWITCH:
    {
        // From the first human to the second, in a dialog only
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_actor_regs || p[1] < 0 || p[1] >= num_actor_regs)
            break;
        if (!actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
            break;
        if (!actor_regs[p[1]])
            break;
        if (actor_regs[p[1]]->GetType() != ACTOR_PLAYER && actor_regs[p[1]]->GetType() != ACTOR_ENTITY)
            break;
        if (!act_mission->GetGame()->GetCamera().IsInDialog())
            break;
        act_mission->GetGame()->GetCamera().DialogLook((C_human *)actor_regs[p[0]], (C_human *)actor_regs[p[1]]);
        break;
    }
    case CMD_GAME_NIGHTMISSION:
        act_mission->GetGame()->SetNightMission(*(int *)instructions[cur_command].params != 0);
        break;
    case CMD_HUMAN_LOOKTOACTOR:
    {
        // The human looks at the other's frame (p[2]) or its point 1.7 above it; without the other it stops
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_actor_regs)
            break;
        if (p[1] >= 0 && p[1] < num_actor_regs)
        {
            if (!actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (!actor_regs[p[1]])
                break;
            if (p[2])
                ((C_human *)actor_regs[p[0]])->SetLookFrame(actor_regs[p[1]]->GetFrame());
            else
                ((C_human *)actor_regs[p[0]])->PoseSetPoseNormal(actor_regs[p[1]]->GetFrame()->GetWorldPos() +
                    S_vector(0.0f, 1.7f, 0.0f));
        }
        else
        {
            if (!actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            ((C_human *)actor_regs[p[0]])->PoseNullRotate();
        }
        break;
    }
    case CMD_HUMAN_LOOKTOFRAME:
    {
        // The same with a frame; its else is LOOKTOACTOR's, merged
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_actor_regs)
            break;
        if (p[1] >= 0 && p[1] < num_frame_regs)
        {
            if (!actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (!frame_regs[p[1]])
                break;
            if (p[2])
                ((C_human *)actor_regs[p[0]])->SetLookFrame(frame_regs[p[1]]);
            else
                ((C_human *)actor_regs[p[0]])->PoseSetPoseNormal(frame_regs[p[1]]->GetWorldPos());
        }
        else
        {
            if (!actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            ((C_human *)actor_regs[p[0]])->PoseNullRotate();
        }
        break;
    }
    case CMD_GETMISSIONNUMBER:
    {
        int reg = *(int *)instructions[cur_command].params;
        if (reg < 0 || reg >= num_float_regs)
            break;
        float_regs[reg] = (float)act_mission->GetMissionNumber();
        break;
    }
    case CMD_SETMISSIONNUMBER:
    {
        // A register's or the value, rounded; 0 for a register out of range
        int n = 0;
        S_operand *op = (S_operand *)instructions[cur_command].params;
        if (op->reg < 0)
            n = (int)(op->value + .5f);
        else if (num_float_regs > op->reg)
            n = (int)(float_regs[op->reg] + .5f);
        act_mission->SetMissionNumber(n);
        break;
    }
    case CMD_HUMAN_LINKTOHAND:
    {
        // The model into the human's right or (p[2]) left hand: where it is (p[3]) or at the hand's origin
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
            break;
        if (p[1] < 0 || p[1] >= num_frame_regs)
            break;
        if (!frame_regs[p[1]] || frame_regs[p[1]]->GetType() != FRAME_MODEL)
            break;
        act_mission->GetGame()->AddFramePlacement(frame_regs[p[1]]);
        I3D_frame *hand = p[2] ? ((C_human *)actor_regs[p[0]])->left_hand : ((C_human *)actor_regs[p[0]])->right_hand;
        if (p[3])
            frame_regs[p[1]]->LinkTo(hand, I3DLINK_UPDMATRIX);
        else
        {
            frame_regs[p[1]]->LinkTo(hand, 0);
            frame_regs[p[1]]->SetPos(0.0f, 0.0f, 0.0f);
            frame_regs[p[1]]->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
            frame_regs[p[1]]->Update();
        }
        break;
    }
    case CMD_HUMAN_UNLINKFROMHAND:
    {
        // The model in the human's hand becomes a weapon on the ground, the item p[2] to p[4]; with an item a copy of
        // it, the model unlinked and hidden
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
            break;
        if (p[1] < 0 || p[1] >= num_frame_regs)
            break;
        if (!frame_regs[p[1]] || frame_regs[p[1]]->GetType() != FRAME_MODEL)
            break;
        C_human *human = (C_human *)actor_regs[p[0]];
        if (frame_regs[p[1]]->GetParent() != human->right_hand && frame_regs[p[1]]->GetParent() != human->left_hand)
            break;
        bool duplicate = p[2] > 0;
        I3D_frame *frm = frame_regs[p[1]];
        I3D_frame *weapon = duplicate ? driver->CreateFrame(FRAME_MODEL) : frm;
        if (!weapon)
            break;
        if (duplicate)
        {
            weapon->Duplicate(frm);
            weapon->SetPos(frm->GetPos());
            weapon->SetRot(frm->GetRot());
            weapon->SetScale(frm->GetScale());
            weapon->LinkTo(frm->GetParent(), 0);
            weapon->Update();
        }
        weapon->LinkTo(act_mission->GetScene()->GetPrimarySector(), I3DLINK_UPDMATRIX);
        act_mission->GetScene()->SetFrameSector(weapon);
        C_drop_in_weapon *drop = new C_drop_in_weapon;
        if (drop->Init(weapon))
        {
            S_THING_BOX box;
            box.weight = 10.0f;
            box.unknown_10 = .45f;
            box.velocity = S_vector(0.0f, -.11f, 0.0f);
            drop->InitBox(&box);
            drop->item.id = (ushort)p[2];
            drop->item.unknown_4 = p[3];
            drop->item.unknown_8 = p[4];
            drop->item.unknown_c = NULL;
            act_mission->GetGame()->AddTemporaryActor(drop);
        }
        else
        {
            drop->Release();
            if (duplicate)
            {
                weapon->LinkTo(NULL, 0);
                weapon->Release();
            }
        }
        if (duplicate)
        {
            frm->LinkTo(NULL, 0);
            frm->SetOn(false);
        }
        break;
    }
    case CMD_CREATEWEAPONFROMFRAME:
    {
        // A copy of the model as an object the player can pick up, the item p[1] to p[3]; the model hidden
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_frame_regs)
            break;
        if (!frame_regs[p[0]] || frame_regs[p[0]]->GetType() != FRAME_MODEL)
            break;
        if (p[1] <= 0)
            break;
        I3D_frame *weapon = driver->CreateFrame(FRAME_MODEL);
        if (!weapon)
            break;
        I3D_frame *frm = frame_regs[p[0]];
        weapon->Duplicate(frm);
        weapon->SetPos(frm->GetPos());
        weapon->SetRot(frm->GetRot());
        weapon->SetScale(frm->GetScale());
        weapon->LinkTo(frm->GetParent(), 0);
        weapon->Update();
        weapon->LinkTo(act_mission->GetScene()->GetPrimarySector(), I3DLINK_UPDMATRIX);
        act_mission->GetScene()->SetFrameSector(weapon);
        C_using_object::S_using_object *obj = new C_using_object::S_using_object;
        obj->pos = weapon->GetWorldPos() + S_vector(0.0f, 1.0f, 0.0f);
        obj->unknown_c = .5f;
        obj->unknown_10 = 0;
        obj->flags = USINGF_DELETE | USINGF_FRAME;
        obj->frame = weapon;
        obj->unknown_1c = 0;
        obj->unknown_24 = true;
        // The sector the copy is in
        for (obj->sector = weapon->GetParent(); obj->sector; obj->sector = obj->sector->GetParent())
        {
            if (obj->sector->GetType() == FRAME_SECTOR)
                break;
        }
        obj->item.id = (ushort)p[1];
        obj->item.unknown_4 = p[2];
        obj->item.unknown_8 = p[3];
        obj->item.unknown_c = obj;
        act_mission->GetGame()->GetUsingObject().AddObject(obj);
        act_mission->GetGame()->AddFramePlacement(frame_regs[p[0]]);
        frame_regs[p[0]]->SetOn(false);
        frame_regs[p[0]]->LinkTo(NULL, 0);
        break;
    }
    case CMD_GARAGE_ADDCAR:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg >= 0 && reg < num_actor_regs)
            {
                if (actor_regs[reg] && actor_regs[reg]->GetType() == ACTOR_CAR)
                    mafiaLoadSave.AddCarByActor((C_car *)actor_regs[reg]);
            }
            else
            {
                C_human *player = act_mission->GetGame()->GetPlayer();
                if (player && player->GetCar() && player->GetCar()->GetType() == ACTOR_CAR)
                    mafiaLoadSave.AddCarByActor(player->GetCar());
            }
        }
        break;
    case CMD_GARAGE_DELCAR:
        {
            int reg = *(int *)instructions[cur_command].params;
            C_car *car;
            if (reg >= 0 && reg < num_actor_regs)
            {
                if (!actor_regs[reg] || actor_regs[reg]->GetType() != ACTOR_CAR)
                    break;
                car = (C_car *)actor_regs[reg];
            }
            else
            {
                C_human *player = act_mission->GetGame()->GetPlayer();
                if (!player || !player->GetCar() || player->GetCar()->GetType() != ACTOR_CAR)
                    break;
                car = player->GetCar();
            }
            mafiaLoadSave.RemoveCar(car->GetModelName(), car->GetActualLevel());
        }
        break;
    case CMD_GARAGE_ADDCARIDX:
        {
            int *p = (int *)instructions[cur_command].params;
            mafiaLoadSave.AddCarByIdx(p[0], p[1], p[2]);
        }
        break;
    case CMD_GARAGE_GENERATECARS:
        mafiaLoadSave.GenerateCars();
        break;
    case CMD_GARAGE_RELEASECARS:
        mafiaLoadSave.ReleaseCars();
        break;
    case CMD_GARAGE_CARMANAGER:
        mafiaLoadSave.CarManager();
        break;
    case CMD_PLAYER_LOCKCONTROLS:
        if (act_mission->GetGame()->GetPlayer())
        {
            ((C_player *)act_mission->GetGame()->GetPlayer())->LockControls(
                *(int *)instructions[cur_command].params != 0);
            act_mission->GetGame()->GetPlayer()->Do_Breath();
        }
        break;
    case CMD_PERSON_PLAYANIM:
        {
            S_playanim_params *a = (S_playanim_params *)instructions[cur_command].params;
            if (a->reg < 0 || num_actor_regs <= a->reg || !actor_regs[a->reg])
                break;
            if (actor_regs[a->reg]->GetType() != ACTOR_PLAYER && actor_regs[a->reg]->GetType() != ACTOR_ENTITY)
                break;
            int result = ((C_human *)actor_regs[a->reg])->Do_PlayAnim(a->anim, a->simple, a->unknown_4c);
            if (a->result_reg < 0 || num_float_regs <= a->result_reg)
                break;
            float_regs[a->result_reg] = (float)result;
        }
        break;
    case CMD_PERSON_STOPANIM:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg < 0 || reg >= num_actor_regs || !actor_regs[reg])
                break;
            if (actor_regs[reg]->GetType() != ACTOR_PLAYER && actor_regs[reg]->GetType() != ACTOR_ENTITY)
                break;
            ((C_human *)actor_regs[reg])->Do_StopAnim();
        }
        break;
    case CMD_GUNSHOP_MENU:
        if (act_mission->GetGame()->GetPlayer())
        {
            GM_Menu::ExecuteMenu(new GM_GunShop(&act_mission->GetGame()->GetPlayer()->inventory), false, true);
            mafiaIndicators.ConsoleClear();
            act_mission->GetGame()->GetPlayer()->ChangeWeaponModel();
        }
        break;
    case CMD_HUMAN_SET8SLOT:
        {
            int *p = (int *)instructions[cur_command].params;
            // The register's bound is one too high, as PS2's
            if (num_actor_regs <= p[0] - 1 || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            std::vector<S_GameItem> items;
            ((C_human *)actor_regs[p[0]])->inventory.SetCanHaveBigWeapon(p[1] != 0, &items);
            if (items.size())
                ((C_human *)actor_regs[p[0]])->DropOutItems(items);
        }
        break;
    case CMD_ENEMY_FORCESCRIPT:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg < 0 || reg >= num_actor_regs || !actor_regs[reg] || actor_regs[reg]->GetType() != ACTOR_ENTITY)
                break;
            ((C_entity *)actor_regs[reg])->ForceScript(false);
        }
        break;
    case CMD_COMPAREOWNERWITHEX:
        {
            char *label = NULL;
            S_compareowner_params *c = (S_compareowner_params *)instructions[cur_command].params;
            if (num_actor_regs > c->value1 && c->value1 >= 0 && actor_regs[c->value1] && num_actor_regs > c->value2 &&
                c->value2 >= 0 && (actor_regs[c->value1]->GetType() == ACTOR_PLAYER ||
                actor_regs[c->value1]->GetType() == ACTOR_ENTITY))
            {
                label = c->name2;
                C_car *car = ((C_human *)actor_regs[c->value1])->GetCar();
                if (car && car->GetType() == ACTOR_CAR && ((C_human *)actor_regs[c->value1])->unknown_40c != 9 &&
                    ((C_human *)actor_regs[c->value1])->seat > -1)
                    car = NULL;
                if (car == actor_regs[c->value2])
                    label = c->name1;
            }
            NextCommand(label);
        }
        return;
    case CMD_HUMAN_GETPROPERTY:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            C_human *human = (C_human *)actor_regs[p[0]];
            if (human->max_properties[p[2]] == 0.0f)
                float_regs[p[1]] = 0.0f;
            else
                float_regs[p[1]] = human->properties[p[2]] / human->max_properties[p[2]] * 100.0f;
            if (p[2] == HUMAN_PROPERTY_ENERGY && !human->GetUnknown5d())
                float_regs[p[1]] = 0.0f;
        }
        break;
    case CMD_HUMAN_SETPROPERTY:
        {
            S_setproperty_params *s = (S_setproperty_params *)instructions[cur_command].params;
            if (num_actor_regs <= s->reg || s->reg < 0 || !actor_regs[s->reg])
                break;
            if (actor_regs[s->reg]->GetType() != ACTOR_PLAYER && actor_regs[s->reg]->GetType() != ACTOR_ENTITY)
                break;
            float value = s->value;
            if (num_float_regs > s->value_reg && s->value_reg >= 0)
                value = float_regs[s->value_reg];
            C_human *human = (C_human *)actor_regs[s->reg];
            bool dead = human->properties[HUMAN_PROPERTY_ENERGY] < .01f;
            human->properties[s->property] = value * .01f * human->max_properties[s->property];
            // Out of energy now: the human dies of a hit
            if (!dead && human->properties[HUMAN_PROPERTY_ENERGY] < .01f)
            {
                S_vector pos = human->GetFrame()->GetWorldPos();
                S_vector dir = human->GetFrame()->GetWorldDir();
                S_vector back = -dir;
                human->properties[HUMAN_PROPERTY_ENERGY] = 1.0f;
                if (human->car && human->car->GetType() == ACTOR_CAR)
                    human->Hit(HIT_UNKNOWN_5, back, pos, dir, 10.0f, NULL, 5, NULL);
                else
                    human->Hit(HIT_UNKNOWN_6, back, pos, dir, 10.0f, NULL, 5, NULL);
            }
            if (human == act_mission->GetGame()->GetPlayer())
            {
                int lives = (int)(human->properties[HUMAN_PROPERTY_ENERGY] /
                    human->max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
                if (!human->GetUnknown5e())
                    lives = Max(lives, 1);
                mafiaIndicators.PlayerSetLives(lives);
            }
            else if (human->wingman > -1)
            {
                int lives = (int)(human->properties[HUMAN_PROPERTY_ENERGY] /
                    human->max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
                if (!human->GetUnknown5e())
                    lives = Max(lives, 1);
                mafiaIndicators.PlayerSetWingmanLives(human->wingman, lives);
            }
        }
        break;
    case CMD_DISABLECOLLS:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg < 0 || reg >= num_frame_regs || !frame_regs[reg])
                break;
            std::vector<g_collision_header *> colls;
            uint num = gifCollision.FindCollsByFrame(frame_regs[reg], colls);
            for (uint i = 0; i < num; i++)
                colls[i]->flags |= COLLHF_DISABLED;
        }
        break;
    case CMD_HUMAN_GETOWNER:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || num_actor_regs <= p[1] || p[1] < 0)
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            C_human *human = (C_human *)actor_regs[p[0]];
            actor_regs[p[1]] = human->GetCar();
            if (human->GetCar() && human->GetCar()->GetType() == ACTOR_CAR && human->unknown_40c != 9 &&
                human->seat > -1)
                actor_regs[p[1]] = NULL;
        }
        break;
    case CMD_HUMAN_FORCE_SETTOCAR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || num_actor_regs <= p[0] || !actor_regs[p[0]] || p[1] < 0 || num_actor_regs <= p[1] ||
                !actor_regs[p[1]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (actor_regs[p[1]]->GetType() != ACTOR_CAR)
                break;
            ((C_human *)actor_regs[p[0]])->Intern_UseCar((C_car *)actor_regs[p[1]], p[2]);
        }
        break;
    case CMD_CAR_SETDESTROYMOTOR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
                break;
            ((C_car *)actor_regs[p[0]])->SetDestroyMotor(p[1] != 0);
        }
        break;
    case CMD_ACTOR_SETDIR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (num_frame_regs <= p[1] || p[1] < 0 || !frame_regs[p[1]])
                break;
            if (!actor_regs[p[0]]->GetFrame())
                break;
            S_vector dir = frame_regs[p[1]]->GetWorldPos() - actor_regs[p[0]]->GetFrame()->GetWorldPos();
            dir.Normalize();
            actor_regs[p[0]]->SetDir(dir);
            actor_regs[p[0]]->GetFrame()->SetDir(actor_regs[p[0]]->GetDir(), 0.0f);
            actor_regs[p[0]]->GetFrame()->Update();
            if (actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                C_car *car = (C_car *)actor_regs[p[0]];
                car->Deactivate(0);
                car->ResetCar();
            }
            if (actor_regs[p[0]]->GetFrame())
                act_mission->GetScene()->SetFrameSectorPos(actor_regs[p[0]]->GetFrame(),
                    actor_regs[p[0]]->GetFrame()->GetWorldPos() + S_vector(0.0f, .5f, 0.0f));
        }
        break;
    case CMD_ACTOR_SETPOS:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (num_frame_regs <= p[1] || p[1] < 0 || !frame_regs[p[1]])
                break;
            if (!actor_regs[p[0]]->GetFrame())
                break;
            actor_regs[p[0]]->SetPos(frame_regs[p[1]]->GetWorldPos());
            actor_regs[p[0]]->GetFrame()->SetPos(actor_regs[p[0]]->GetPos());
            actor_regs[p[0]]->GetFrame()->Update();
            if (actor_regs[p[0]]->GetFrame())
                act_mission->GetScene()->SetFrameSector(actor_regs[p[0]]->GetFrame());
            if (actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                C_car *car = (C_car *)actor_regs[p[0]];
                car->Deactivate(0);
                car->ResetCar();
            }
            // VC6 merges this statement with ACTOR_SETPLACEMENT's last one
            if (actor_regs[p[0]]->GetFrame())
                act_mission->GetScene()->SetFrameSectorPos(actor_regs[p[0]]->GetFrame(),
                    actor_regs[p[0]]->GetFrame()->GetWorldPos() + S_vector(0.0f, .5f, 0.0f));
        }
        break;
    case CMD_ACTOR_SETPLACEMENT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (num_frame_regs <= p[1] || p[1] < 0 || !frame_regs[p[1]])
                break;
            // A human in a car gets out of it there
            if (actor_regs[p[0]]->GetType() == ACTOR_PLAYER || actor_regs[p[0]]->GetType() == ACTOR_ENTITY)
            {
                C_human *human = (C_human *)actor_regs[p[0]];
                if (human->GetCar() && human->GetCar()->GetType() == ACTOR_CAR)
                {
                    human->Intern_FromCar(frame_regs[p[1]]);
                    break;
                }
            }
            if (!actor_regs[p[0]]->GetFrame())
                break;
            actor_regs[p[0]]->SetPos(frame_regs[p[1]]->GetWorldPos());
            actor_regs[p[0]]->SetDir(frame_regs[p[1]]->GetWorldDir());
            actor_regs[p[0]]->GetFrame()->SetPos(actor_regs[p[0]]->GetPos());
            actor_regs[p[0]]->GetFrame()->SetDir(actor_regs[p[0]]->GetDir(), 0.0f);
            actor_regs[p[0]]->GetFrame()->Update();
            if (actor_regs[p[0]]->GetFrame())
                act_mission->GetScene()->SetFrameSector(actor_regs[p[0]]->GetFrame());
            if (actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                C_car *car = (C_car *)actor_regs[p[0]];
                car->Deactivate(0);
                car->ResetCar();
            }
            if (actor_regs[p[0]]->GetType() == ACTOR_PLAYER || actor_regs[p[0]]->GetType() == ACTOR_ENTITY)
            {
                ((C_human *)actor_regs[p[0]])->unknown_464 = 0;
                ((C_human *)actor_regs[p[0]])->anim_machine.Reset();
            }
            if (actor_regs[p[0]]->GetFrame())
                act_mission->GetScene()->SetFrameSectorPos(actor_regs[p[0]]->GetFrame(),
                    actor_regs[p[0]]->GetFrame()->GetWorldPos() + S_vector(0.0f, .5f, 0.0f));
        }
        break;
    case CMD_GARAGE_ENABLESTEAL:
        mafiaLoadSave.EnableSteal(*(char *)instructions[cur_command].params);
        break;
    case CMD_SETTIMEOUTEVENT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]])
            {
                if (actor_regs[p[0]]->GetType() == ACTOR_DETECTOR)
                    act_mission->GetGame()->SetTimeOutEvent((C_detector *)actor_regs[p[0]], p[1]);
                else if (actor_regs[p[0]]->GetType() == ACTOR_ENTITY)
                    act_mission->GetGame()->SetTimeOutEvent((C_entity *)actor_regs[p[0]], p[1]);
            }
            else
                act_mission->GetGame()->SetTimeOutEvent(NULL, 0);
        }
        break;
    case CMD_SETPLAYERFIREEVENT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]])
            {
                if (actor_regs[p[0]]->GetType() == ACTOR_DETECTOR)
                    act_mission->GetGame()->SetPlayerFireEvent((C_detector *)actor_regs[p[0]], p[1]);
                else if (actor_regs[p[0]]->GetType() == ACTOR_ENTITY)
                    act_mission->GetGame()->SetPlayerFireEvent((C_entity *)actor_regs[p[0]], p[1]);
            }
            else
                act_mission->GetGame()->SetPlayerFireEvent(NULL, 0);
        }
        break;
    case CMD_EMITPARTICLE:
        {
            int *p = (int *)instructions[cur_command].params;
            // Only this effect with the particles off
            if (act_mission->GetOptions().GetParticles() < 1 && p[1] != EFFECT_UNKNOWN_5A)
                break;
            if (num_frame_regs <= p[0] || p[0] < 0 || !frame_regs[p[0]])
                break;
            int particle = act_mission->GetEffects()->CreateParticle(p[1], S_vector(0.0f, 0.0f, 0.0f),
                S_vector(0.0f, 0.0f, 1.0f), 0, frame_regs[p[0]]);
            if (cutscene.IsPlaying())
                act_mission->GetGame()->AddCutsceneParticle(particle);
            if (num_float_regs <= p[2] || p[2] < 0)
                break;
            float_regs[p[2]] = (float)particle;
        }
        break;
    case CMD_STOPPARTICLE:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (num_float_regs <= reg || reg < 0)
                break;
            act_mission->GetEffects()->DestroyParticle((int)float_regs[reg]);
        }
        break;
    case CMD_FINDNEARACTOR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || num_actor_regs <= p[0] || !actor_regs[p[0]] || !actor_regs[p[0]]->GetFrame())
                break;
            if (p[1] < 0 || num_actor_regs <= p[1])
                break;
            float best = FLT_MAX;
            actor_regs[p[1]] = NULL;
            S_vector pos = actor_regs[p[0]]->GetFrame()->GetWorldPos();
            std::vector<C_actor *> &actors = C_actor::GetGlobalActors();
            for (int i = actors.size(); i--;)
            {
                if (actors[i]->GetType() == p[2] && actors[i]->GetFrame() && actors[i]->GetUnknown4() != 2 &&
                    actors[i] != actor)
                {
                    float dist = (actors[i]->GetFrame()->GetWorldPos() - pos).Square();
                    if (dist < best)
                    {
                        actor_regs[p[1]] = actors[i];
                        best = dist;
                    }
                }
            }
        }
        break;
    // Fades out and waits; in the dark it ends the mission, or with value1 0 asks the player through the failure menu
    // (its text value2) and plays the game over music while the menu runs
    case CMD_ENDOFMISSION:
    {
        if (!mafiaIndicators.FadeInOutGetState())
            return;
        if (mafiaIndicators.FadeInOutIsDark())
        {
            S_event_params *e = (S_event_params *)instructions[cur_command].params;
            if (e->value1)
            {
                act_mission->GetGame()->SetUnknown48(1);
                act_mission->GetGame()->SetUnknown3665(true);
            }
            else
            {
                act_mission->GetGame()->StopAllSounds();
                switch (GM_Menu::ExecuteMenu(new GM_GameFailed(e->value2), true, false))
                {
                case GAMEFAILED_RESULT_EXIT:
                    act_mission->GetGame()->SetUnknown48(1);
                    break;
                case GAMEFAILED_RESULT_4FC:
                    act_mission->GetGame()->SetUnknown48(2);
                    break;
                case MAINMENU_LOAD_GAME:
                case GAMEFAILED_RESULT_RETRY:
                    act_mission->GetGame()->SetUnknown48(3);
                    break;
                }
            }
            mafiaIndicators.FadeInOut(false, 0, 0);
            break;
        }
        mafiaIndicators.FadeInOut(true, 1000, 0);
        S_event_params *e = (S_event_params *)instructions[cur_command].params;
        if (strlen(e->name))
            act_mission->GetGame()->Play3DSound(NULL, e->name, I3DSOUND_AMBIENT,
                act_mission->GetScene()->GetActiveCamera()->GetWorldPos(), 20.0f, 20.0f, 1.0f, false, .001f, false);
        act_mission->GetGame()->IShowFadeDownAll(1000);
        int stream = act_mission->GetGame()->CreateIShow(0);
        IShow *show = act_mission->GetGame()->GetIShow(stream);
        if (!show)
            return;
        // STRING: GAME 0x6343b8
        act_mission->GetGame()->OpenIShow("sounds\\music\\13 - Game Over.mp3", stream);
        show->Play();
        act_mission->GetGame()->IShowFade(stream, 0.0f, 1.0f, 1000);
        return;
    }
    // The detector's or the entity's script gets the event when the player sounds the horn; without one the fire
    // event is cleared, not the horn's (PS2's the same)
    case CMD_SETPLAYERHORNEVENT:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]])
        {
            if (actor_regs[p[0]]->GetType() == ACTOR_DETECTOR)
                act_mission->GetGame()->SetPlayerHornEvent((C_detector *)actor_regs[p[0]], p[1]);
            else if (actor_regs[p[0]]->GetType() == ACTOR_ENTITY)
                act_mission->GetGame()->SetPlayerHornEvent((C_entity *)actor_regs[p[0]], p[1]);
        }
        else
            act_mission->GetGame()->SetPlayerFireEvent(NULL, 0);
        break;
    }
    case CMD_SETPLAYERWFALLEVENT:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]])
        {
            if (actor_regs[p[0]]->GetType() == ACTOR_DETECTOR)
                act_mission->GetGame()->SetPlayerWFallEvent((C_detector *)actor_regs[p[0]], p[1]);
            else if (actor_regs[p[0]]->GetType() == ACTOR_ENTITY)
                act_mission->GetGame()->SetPlayerWFallEvent((C_entity *)actor_regs[p[0]], p[1]);
        }
        else
            act_mission->GetGame()->SetPlayerWFallEvent(NULL, 0);
        break;
    }
    case CMD_HUMAN_HOLSTER:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (reg >= 0 && reg < num_actor_regs && actor_regs[reg] &&
            (actor_regs[reg]->GetType() == ACTOR_PLAYER || actor_regs[reg]->GetType() == ACTOR_ENTITY))
            ((C_human *)actor_regs[reg])->Do_Holster();
        break;
    }
    case CMD_CAMERA_LOCK:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_frame_regs > p[0] && p[0] >= 0 && frame_regs[p[0]])
            act_mission->GetGame()->GetCamera().LockAt(frame_regs[p[0]]->GetWorldPos(),
                frame_regs[p[0]]->GetWorldDir());
        if (act_mission->GetGame()->GetUnknown2b14() < 6)
            act_mission->GetGame()->SetCameraRotRepair();
        act_mission->GetGame()->RecomputeLightCache();
        break;
    }
    case CMD_CAMERA_UNLOCK:
        act_mission->GetGame()->GetCamera().SetCutscene(false);
        act_mission->GetGame()->RecomputeLightCache();
        break;
    case CMD_SETMISSIONNAMEID:
        mafiaLoadSave.SetMissionNameID(((int *)instructions[cur_command].params)[0]);
        break;
    // The human (p[0]) becomes the HUD's wingman p[2] with the name p[1]; his health shows at least 1 while he lives
    case CMD_WINGMAN_SETINDICATOR:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] >= 0 && num_actor_regs > p[0] && actor_regs[p[0]] &&
            (actor_regs[p[0]]->GetType() == ACTOR_PLAYER || actor_regs[p[0]]->GetType() == ACTOR_ENTITY) &&
            p[2] >= 0 && p[2] < MAX_WINGMEN)
        {
            C_human *human = (C_human *)actor_regs[p[0]];
            human->SetWingman(p[2], p[1]);
            mafiaIndicators.PlayerSetWingmanName(p[2], p[1]);
            int lives = (int)(human->properties[HUMAN_PROPERTY_ENERGY] / human->max_properties[HUMAN_PROPERTY_ENERGY] * 100.0f);
            if (!human->GetUnknown5e())
                lives = Max(lives, 1);
            mafiaIndicators.PlayerSetWingmanLives(p[2], lives);
            switch (p[2])
            {
            case 0:
                mafiaIndicators.AddFlags(INDF_WINGMAN1);
                break;
            case 1:
                mafiaIndicators.AddFlags(INDF_WINGMAN2);
                break;
            case 2:
                mafiaIndicators.AddFlags(INDF_WINGMAN3);
                break;
            case 3:
                mafiaIndicators.AddFlags(INDF_WINGMAN4);
                break;
            }
        }
        break;
    }
    case CMD_WINGMAN_DELINDICATOR:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] &&
            (actor_regs[p[0]]->GetType() == ACTOR_PLAYER || actor_regs[p[0]]->GetType() == ACTOR_ENTITY))
        {
            ((C_human *)actor_regs[p[0]])->SetWingman(-1, 0);
            switch (p[1])
            {
            case 0:
                mafiaIndicators.RemoveFlags(INDF_WINGMAN1);
                break;
            case 1:
                mafiaIndicators.RemoveFlags(INDF_WINGMAN2);
                break;
            case 2:
                mafiaIndicators.RemoveFlags(INDF_WINGMAN3);
                break;
            case 3:
                mafiaIndicators.RemoveFlags(INDF_WINGMAN4);
                break;
            }
        }
        break;
    }
    // A model of a file into the frame register, which the game deletes with the mission; NULL where it doesn't load
    case CMD_MODEL_CREATE:
    {
        S_name_params *n = (S_name_params *)instructions[cur_command].params;
        if (n->reg >= 0 && n->reg < num_frame_regs)
        {
            frame_regs[n->reg] = driver->CreateFrame(FRAME_MODEL);
            if (model_cache.Open((I3D_model *)frame_regs[n->reg], n->name, 0, NULL, NULL, NULL) == I3D_OK)
            {
                act_mission->GetGame()->AddCreatedFrameToVector(frame_regs[n->reg]);
                // STRING: GAME 0x6343a8
                frame_regs[n->reg]->SetName("created_model");
            }
            else
            {
                frame_regs[n->reg]->Release();
                frame_regs[n->reg] = NULL;
            }
        }
        break;
    }
    // Only a model MODEL_CREATE created
    case CMD_MODEL_DESTROY:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (reg >= 0 && reg < num_frame_regs && act_mission->GetGame()->DelCreatedFrameToVector(frame_regs[reg]))
        {
            frame_regs[reg]->LinkTo(NULL, 0);
            frame_regs[reg]->Release();
            frame_regs[reg] = NULL;
        }
        break;
    }
    case CMD_MODEL_PLAYANIM:
    {
        S_name_params *n = (S_name_params *)instructions[cur_command].params;
        if (n->reg >= 0 && n->reg < num_frame_regs && frame_regs[n->reg] &&
            frame_regs[n->reg]->GetType() == FRAME_MODEL)
        {
            I3D_animation_set *anim = NULL;
            anim_cache.Create(n->name, &anim, NULL, NULL);
            if (anim)
            {
                anim->SetOptions(I3DANIMOP_LOOP);
                ((I3D_model *)frame_regs[n->reg])->SetAnimation(anim, 0, 1.0f, 0);
                act_mission->AddAnimModel((I3D_model *)frame_regs[n->reg], true);
                anim->Release();
            }
        }
        break;
    }
    case CMD_MODEL_STOPANIM:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] >= 0 && p[0] < num_frame_regs && frame_regs[p[0]] && frame_regs[p[0]]->GetType() == FRAME_MODEL)
        {
            ((I3D_model *)frame_regs[p[0]])->StopAnimation(0);
            act_mission->DelAnimModel((I3D_model *)frame_regs[p[0]]);
        }
        break;
    }
    case CMD_HUMAN_CHANGEMODEL:
    {
        S_name_params *n = (S_name_params *)instructions[cur_command].params;
        if (n->reg >= 0 && n->reg < num_actor_regs && actor_regs[n->reg] &&
            (actor_regs[n->reg]->GetType() == ACTOR_ENTITY || actor_regs[n->reg]->GetType() == ACTOR_PLAYER))
            ((C_human *)actor_regs[n->reg])->_intern_ChangeModel(n->name, false);
        break;
    }
    case CMD_AUTOSAVEGAME:
    {
        int *p = (int *)instructions[cur_command].params;
        mafiaLoadSave.SaveGameFromScript(p[0], false, p[1]);
        break;
    }
    case CMD_AUTOSAVEGAMEFULL:
    {
        int *p = (int *)instructions[cur_command].params;
        mafiaLoadSave.SaveGameFromScript(p[0], true, p[1]);
        break;
    }
    case CMD_HUMAN_DEATH:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] >= 0 && p[0] < num_actor_regs && actor_regs[p[0]] &&
            (actor_regs[p[0]]->GetType() == ACTOR_ENTITY || actor_regs[p[0]]->GetType() == ACTOR_PLAYER))
            act_mission->GetGame()->DeathActorAdd(actor_regs[p[0]]);
        break;
    }
    // The sound frame plays the sound table's sound p[1], looped
    case CMD_PLAYSOUNDEX:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_frame_regs > p[0] && p[0] >= 0 && frame_regs[p[0]] && frame_regs[p[0]]->GetType() == FRAME_SOUND)
        {
            I3D_sound *snd = (I3D_sound *)frame_regs[p[0]];
            if (sound_table.ConstructSound(snd, p[1]))
            {
                snd->SetLoop(true);
                snd->SetOn(true, true);
            }
        }
        break;
    }
    case CMD_STOPSOUND:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (num_frame_regs > reg && reg >= 0 && frame_regs[reg] && frame_regs[reg]->GetType() == FRAME_SOUND)
            ((I3D_sound *)frame_regs[reg])->SetOn(false, true);
        break;
    }
    case CMD_GETSOUNDTIME:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_frame_regs > p[0] && p[0] >= 0 && frame_regs[p[0]] && frame_regs[p[0]]->GetType() == FRAME_SOUND &&
            num_float_regs > p[1] && p[1] >= 0)
            float_regs[p[1]] = (float)((I3D_sound *)frame_regs[p[0]])->GetPlayTime();
        break;
    }
    // The human says the speech p[1] on a free dubbing channel, kept in p[3], and the command waits until it is said
    case CMD_HUMAN_TALK:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[3] == -1)
        {
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] &&
                (actor_regs[p[0]]->GetType() == ACTOR_ENTITY || actor_regs[p[0]]->GetType() == ACTOR_PLAYER) &&
                !actor_regs[p[0]]->GetUnknown5e())
            {
                p[3] = DabingGetFreeChanel();
                DabingSetHuman(p[3], (C_human *)actor_regs[p[0]]);
                DabingSpeak(p[3], p[1], true, p[2]);
                act_mission->GetGame()->AddHumanTalk((C_human *)actor_regs[p[0]], p[3]);
                return;
            }
        }
        else
        {
            if (DabingIsSpeaking(p[3]))
                return;
            act_mission->GetGame()->DelHumanTalk((C_human *)actor_regs[p[0]], p[3]);
        }
        p[3] = -1;
        break;
    }
    // The entity's action button's object (the AB): taken away, or put there again
    case CMD_HUMAN_ERASEAB:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (num_actor_regs > reg && reg >= 0 && actor_regs[reg] && actor_regs[reg]->GetType() == ACTOR_ENTITY)
        {
            C_entity *ent = (C_entity *)actor_regs[reg];
            ent->DelUsingObject();
            ent->ab_created = false;
        }
        break;
    }
    case CMD_HUMAN_CREATEAB:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (num_actor_regs > reg && reg >= 0 && actor_regs[reg] && actor_regs[reg]->GetType() == ACTOR_ENTITY)
        {
            C_entity *ent = (C_entity *)actor_regs[reg];
            ent->DelUsingObject();
            ent->AddUsingObject();
            ent->ab_created = true;
        }
        break;
    }
    case CMD_POLICEMANAGER_ON:
    {
        int *p = (int *)instructions[cur_command].params;
        act_mission->GetGame()->GetShvestky()->SetActive(true, p[0], p[1]);
        break;
    }
    // The police takes the mission's human of that frame name, and his car if he drives one
    case CMD_POLICEMANAGER_ADD:
    {
        char *name = (char *)instructions[cur_command].params;
        std::vector<C_actor *> &actors = act_mission->GetActors();
        int i = actors.size();
        while (i--)
        {
            if ((actors[i]->GetType() == ACTOR_ENTITY || actors[i]->GetType() == ACTOR_PLAYER) &&
                actors[i]->GetFrame() && !_stricmp(actors[i]->GetFrame()->GetName(), name))
            {
                act_mission->GetGame()->GetShvestky()->AddPerson((C_human *)actors[i]);
                if (((C_human *)actors[i])->GetCar() && ((C_human *)actors[i])->GetCar()->GetType() == ACTOR_CAR)
                    act_mission->GetGame()->GetShvestky()->DriveCar((C_human *)actors[i],
                        ((C_human *)actors[i])->GetCar());
                break;
            }
        }
        break;
    }
    case CMD_POLICEMANAGER_DEL:
    {
        char *name = (char *)instructions[cur_command].params;
        std::vector<C_actor *> &actors = act_mission->GetActors();
        int i = actors.size();
        while (i--)
        {
            if ((actors[i]->GetType() == ACTOR_ENTITY || actors[i]->GetType() == ACTOR_PLAYER) &&
                actors[i]->GetFrame() && !_stricmp(actors[i]->GetFrame()->GetName(), name))
            {
                act_mission->GetGame()->GetShvestky()->DelPerson((C_human *)actors[i]);
                break;
            }
        }
        break;
    }
    case CMD_CAR_ENABLEUS:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
            ((C_car *)actor_regs[p[0]])->SetEnterProtect(!p[1]);
        break;
    }
    // Opens the car's door value2 by value percent
    case CMD_CAR_SETDOOROPEN:
    {
        S_float_int_params *s = (S_float_int_params *)instructions[cur_command].params;
        if (s->value1 >= 0 && num_actor_regs > s->value1 && actor_regs[s->value1] &&
            actor_regs[s->value1]->GetType() == ACTOR_CAR)
        {
            C_car *car = (C_car *)actor_regs[s->value1];
            int door = s->value2;
            if (door >= 0 && door < car->GetNumUnknownCa0())
            {
                float open = s->value * .01f;
                if (open < 0.0f)
                    open = 0.0f;
                else if (open > 1.0f)
                    open = 1.0f;
                car->OpenDoor(door, open, true);
            }
        }
        break;
    }
    // The human's target to fire at, none without the frame
    case CMD_HUMAN_SETFIRETARGET:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 &&
            ((actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_ENTITY) ||
            (actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_PLAYER)))
        {
            C_human *human = (C_human *)actor_regs[p[0]];
            if (num_frame_regs > p[1] && p[1] >= 0 && frame_regs[p[1]])
                human->SetFireTarget(frame_regs[p[1]]);
            else
                human->SetFireTarget(NULL);
        }
        break;
    }
    case CMD_ISCARUSABLE:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR &&
            num_float_regs > p[1] && p[1] >= 0)
            float_regs[p[1]] = ((C_car *)actor_regs[p[0]])->IsCarUsable((uint)-1);
        break;
    }
    case CMD_AIRPLANE_ISDESTROYED:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] &&
            actor_regs[p[0]]->GetType() == ACTOR_AIRPLANE && num_float_regs > p[1] && p[1] >= 0)
            float_regs[p[1]] = ((C_airplane *)actor_regs[p[0]])->IsDestroyed();
        break;
    }
    // Opens the door by value2 percent
    case CMD_DOOR_SETOPENPOSE:
    {
        S_int_float_params *s = (S_int_float_params *)instructions[cur_command].params;
        if (num_actor_regs > s->value1 && s->value1 >= 0 && actor_regs[s->value1] &&
            actor_regs[s->value1]->GetType() == ACTOR_DOOR)
        {
            float open = s->value2 * .01f;
            if (open < 0.0f)
                open = 0.0f;
            else if (open > 1.0f)
                open = 1.0f;
            ((C_door *)actor_regs[s->value1])->SetOpenAngle(open);
        }
        break;
    }
    case CMD_HUMAN_RETURNFROMPANIC:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_ENTITY)
            ((C_entity *)actor_regs[p[0]])->ReturnFromPanic(p[1]);
        break;
    }
    // In km/h
    case CMD_CAR_GETSPEED:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR &&
            num_float_regs > p[1] && p[1] >= 0)
            float_regs[p[1]] = ((C_car *)actor_regs[p[0]])->GetSpeed() * 3.6f;
        break;
    }
    case CMD_CAR_BREAKMOTOR:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
            ((C_car *)actor_regs[p[0]])->SetBreakMotor(p[1]);
        break;
    }
    // The player's inventory and values kept between missions (the game's stacks)
    case CMD_INVENTORY_PUSH:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (num_actor_regs > reg && reg >= 0 && actor_regs[reg] &&
            (actor_regs[reg]->GetType() == ACTOR_ENTITY || actor_regs[reg]->GetType() == ACTOR_PLAYER))
            act_mission->GetGame()->InventoryStackPush((C_human *)actor_regs[reg]);
        break;
    }
    case CMD_INVENTORY_POP:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (num_actor_regs > reg && reg >= 0 && actor_regs[reg] &&
            (actor_regs[reg]->GetType() == ACTOR_ENTITY || actor_regs[reg]->GetType() == ACTOR_PLAYER))
        {
            act_mission->GetGame()->InventoryStackPop((C_human *)actor_regs[reg]);
            ((C_human *)actor_regs[reg])->ChangeWeaponModel();
        }
        break;
    }
    case CMD_CHARACTER_PUSH:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (num_actor_regs > reg && reg >= 0 && actor_regs[reg] &&
            (actor_regs[reg]->GetType() == ACTOR_ENTITY || actor_regs[reg]->GetType() == ACTOR_PLAYER))
            act_mission->GetGame()->CharacterStackPush((C_human *)actor_regs[reg]);
        break;
    }
    case CMD_CHARACTER_POP:
    {
        int reg = ((int *)instructions[cur_command].params)[0];
        if (num_actor_regs > reg && reg >= 0 && actor_regs[reg] &&
            (actor_regs[reg]->GetType() == ACTOR_ENTITY || actor_regs[reg]->GetType() == ACTOR_PLAYER))
            act_mission->GetGame()->CharacterStackPop((C_human *)actor_regs[reg]);
        break;
    }
    case CMD_HUMAN_THROWITEM:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] &&
                (actor_regs[p[0]]->GetType() == ACTOR_ENTITY || actor_regs[p[0]]->GetType() == ACTOR_PLAYER))
            {
                C_human *human = (C_human *)actor_regs[p[0]];
                if (human->inventory.IsIn(p[1], false))
                {
                    std::vector<S_GameItem> items;
                    S_GameItem item;
                    item.id = p[1];
                    human->inventory.Remove(&item, &items);
                    if (items.size())
                    {
                        human->DropOutItems(items);
                        human->ChangeWeaponModel();
                    }
                }
            }
        }
        break;
    case CMD_FUCKINGBOX_ADD:
        {
            S_name_params *n = (S_name_params *)instructions[cur_command].params;
            I3D_frame *frm = act_mission->GetScene()->FindFrame(n->name, ENUMF_ALL);
            if (frm)
                act_mission->GetGame()->BoxPlacementAdd(frm, n->reg);
        }
        break;
    case CMD_FUCKINGBOX_RECOMPILE:
        act_mission->GetGame()->BoxPlacementRecompile();
        break;
    case CMD_FUCKINGBOX_ADD_DEST:
        {
            S_name_params *n = (S_name_params *)instructions[cur_command].params;
            I3D_frame *frm = act_mission->GetScene()->FindFrame(n->name, ENUMF_ALL);
            if (frm)
                act_mission->GetGame()->BoxPlacementDestAdd(frm, n->reg);
        }
        break;
    case CMD_FUCKINGBOX_GETNUMBOX:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_float_regs > p[1] && p[1] >= 0)
                float_regs[p[1]] = (float)act_mission->GetGame()->BoxPlacement_GetBox(p[0]);
        }
        break;
    case CMD_FUCKINGBOX_GETNUMDEST:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_float_regs > p[1] && p[1] >= 0)
                float_regs[p[1]] = (float)act_mission->GetGame()->BoxPlacement_GetBoxDest(p[0]);
        }
        break;
    case CMD_FUCKINGBOX_MOVE:
        act_mission->GetGame()->BoxPlacement_MoveBoxToDest(*(int *)instructions[cur_command].params);
        break;
    case CMD_AIRPLANE_START2:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && num_actor_regs > i && actor_regs[i] && actor_regs[i]->GetType() == ACTOR_AIRPLANE)
                ((C_airplane *)actor_regs[i])->Start2();
        }
        break;
    case CMD_WAGON_SETEVENT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] >= 0 && num_actor_regs > p[0] && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_WAGON)
                ((C_wagon *)actor_regs[p[0]])->SetEvent(this, p[1]);
        }
        break;
    case CMD_WAGON_GETLASTNODE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] >= 0 && num_actor_regs > p[0] && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_WAGON &&
                num_float_regs > p[1] && p[1] >= 0)
                float_regs[p[1]] = (float)((C_wagon *)actor_regs[p[0]])->GetLastNode();
        }
        break;
    case CMD_CAR_SETSPECCOLL:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] >= 0 && num_actor_regs > p[0] && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                if (p[1])
                {
                    if (!((C_car *)actor_regs[p[0]])->GetSpecialDynamicColls())
                    {
                        ((C_car *)actor_regs[p[0]])->SetSpecialDynamicColls(true);
                        ((C_car *)actor_regs[p[0]])->LockVehicle(true);
                        act_mission->GetGame()->BoxRecompileBoxForCar((C_car *)actor_regs[p[0]], true);
                    }
                }
                else if (((C_car *)actor_regs[p[0]])->GetSpecialDynamicColls())
                {
                    ((C_car *)actor_regs[p[0]])->SetSpecialDynamicColls(false);
                    ((C_car *)actor_regs[p[0]])->LockVehicle(false);
                    ((C_car *)actor_regs[p[0]])->SetVelocity(S_vector(0.0f, 0.0f, 0.0f));
                    ((C_car *)actor_regs[p[0]])->SetUnknown420(S_vector(0.0f, 0.0f, 0.0f));
                    act_mission->GetGame()->BoxRecompileBoxForCar((C_car *)actor_regs[p[0]], false);
                }
            }
        }
        break;
    case CMD_CAR_GETSPECCOLL:
        {
            // VC6 shares the stores with IFPLAYERSTEALCAR's identical ones (1.0 there, 0 here)
            int *p = (int *)instructions[cur_command].params;
            if (p[0] >= 0 && num_actor_regs > p[0] && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR &&
                p[1] >= 0 && num_float_regs > p[1])
            {
                if (((C_car *)actor_regs[p[0]])->GetSpecialDynamicColls())
                    float_regs[p[1]] = 1.0f;
                else
                    float_regs[p[1]] = 0.0f;
            }
        }
        break;
    case CMD_MISSION_OBJECTIVES:
        mafiaLoadSave.ObjectivesAdd(*(int *)instructions[cur_command].params);
        break;
    case CMD_MISSION_OBJECTIVESREMOVE:
        mafiaLoadSave.ObjectivesRemove(*(int *)instructions[cur_command].params);
        break;
    case CMD_MISSION_OBJECTIVESCLEAR:
        mafiaLoadSave.ObjectivesClear();
        break;
    case CMD_RACING_MISSION6_INIT:
        {
            int *p = (int *)instructions[cur_command].params;
            RacingMisse06(p[0], p[1], p[2], p[3]);
        }
        break;
    case CMD_RACING_MISSION6_START:
        // Starts the race and waits for its end: the command stays until the game's state is RACE6_STATE_END
        if (act_mission->GetGame()->GetUnknown2edc() == RACE6_STATE_NONE)
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && i < num_float_regs)
                act_mission->GetGame()->SetUnknown2ee0(&float_regs[i]);
            act_mission->GetGame()->SetUnknown2edc(true);
            return;
        }
        if (act_mission->GetGame()->GetUnknown2edc() != RACE6_STATE_END)
            return;
        break;
    case CMD_CAR_EXPLOSION:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && num_actor_regs > i && actor_regs[i] && actor_regs[i]->GetType() == ACTOR_CAR)
                ((C_car *)actor_regs[i])->CarExplosion(0);
        }
        break;
    case CMD_HUMAN_FROMCAR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] >= 0 && num_actor_regs > p[0] && actor_regs[p[0]] &&
                (actor_regs[p[0]]->GetType() == ACTOR_ENTITY || actor_regs[p[0]]->GetType() == ACTOR_PLAYER) &&
                p[1] >= 0 && num_frame_regs > p[1] && frame_regs[p[1]])
            {
                C_human *human = (C_human *)actor_regs[p[0]];
                if (human->GetCar() && human->GetCar()->GetType() == ACTOR_CAR)
                {
                    // The car reset where it is, its wheels' collisions kept
                    C_car *car = human->GetCar();
                    bool *colls = new bool[car->GetNumWheels()];
                    car->GetCarWheelColl(colls, car->GetNumWheels());
                    car->Deactivate(0);
                    car->Reset(0.0f, true);
                    car->SetCarWheelColl(colls, car->GetNumWheels());
                    car->Tick(1);
                    human->Intern_FromCar(frame_regs[p[1]]);
                }
            }
        }
        break;
    case CMD_CAR_REMOVE_DRIVER:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && num_actor_regs > i && actor_regs[i] && actor_regs[i]->GetType() == ACTOR_CAR)
                act_mission->GetGame()->GetSledovacka()->RemoveCar((C_car *)actor_regs[i]);
        }
        break;
    case CMD_SETMODELTOCAR:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && num_frame_regs > i && frame_regs[i] && frame_regs[i]->GetType() == FRAME_MODEL)
            {
                // The model the frame is in, the nearest above it up to its sector, takes it from the scene
                I3D_frame *frm = frame_regs[i]->GetParent();
                while (frm && frm->GetType() != FRAME_MODEL && frm->GetType() != FRAME_SECTOR)
                    frm = frm->GetParent();
                if (frm && frm->GetType() == FRAME_MODEL)
                {
                    act_mission->GetGame()->BoxPlacement_DelInfo(frame_regs[i]);
                    ((I3D_model *)frm)->AddFrame(frame_regs[i]);
                    act_mission->GetScene()->DeleteFrame(frame_regs[i]);
                }
            }
        }
        break;
    case CMD_GETENEMYAISTATE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] >= 0 && num_actor_regs > p[0] && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_ENTITY &&
                p[1] >= 0 && num_float_regs > p[1])
            {
                uint state = ((C_entity *)actor_regs[p[0]])->GetAIState();
                if (state >= ENTITY_STATE_10000)
                    state -= ENTITY_STATE_10000;
                if (state >= ENTITY_STATE_666)
                    state -= ENTITY_STATE_666;
                float_regs[p[1]] = (float)state;
            }
        }
        break;
    case CMD_GET_PM_STATE:
        {
            // p[2] is a bit of the police's state of the human, 1 where it is set
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && num_float_regs > p[1] && p[1] >= 0)
            {
                float_regs[p[1]] = 0.0f;
                if ((actor_regs[p[0]]->GetType() == ACTOR_PLAYER || actor_regs[p[0]]->GetType() == ACTOR_ENTITY) &&
                    (p[2] & act_mission->GetGame()->GetShvestky()->GetHumanStatus((C_human *)actor_regs[p[0]])))
                    float_regs[p[1]] = 1.0f;
            }
        }
        break;
    case CMD_GET_PM_CRASHTIME:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && num_float_regs > p[1] && p[1] >= 0)
            {
                float_regs[p[1]] = 0.0f;
                if (actor_regs[p[0]]->GetType() == ACTOR_PLAYER || actor_regs[p[0]]->GetType() == ACTOR_ENTITY)
                    float_regs[p[1]] =
                        (float)act_mission->GetGame()->GetShvestky()->GetCrashTime((C_human *)actor_regs[p[0]]);
            }
        }
        break;
    case CMD_USE_LIGHTCACHE:
        act_mission->GetGame()->UseLightCacheSystem();
        break;
    case CMD_STREAM_CREATE:
        {
            S_name_params *n = (S_name_params *)instructions[cur_command].params;
            if (n->reg >= 0 && n->reg < num_float_regs)
            {
                int show = act_mission->GetGame()->CreateIShow(0);
                float_regs[n->reg] = (float)show;
                act_mission->GetGame()->OpenIShow(n->name, show);
            }
        }
        break;
    case CMD_STREAM_PLAY:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && i < num_float_regs)
            {
                IShow *show = act_mission->GetGame()->GetIShow((int)float_regs[i]);
                if (show)
                {
                    show->Play();
                    show->SetVolume(mafiaLoadSave.settings.music_volume);
                    act_mission->GetGame()->IShowStoreVolume((int)float_regs[i], 1.0f);
                    act_mission->GetGame()->CityMusicOff();
                }
            }
        }
        break;
    case CMD_STREAM_PAUSE:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && i < num_float_regs)
            {
                IShow *show = act_mission->GetGame()->GetIShow((int)float_regs[i]);
                if (show)
                    show->Pause();
            }
        }
        break;
    case CMD_STREAM_STOP:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && i < num_float_regs)
            {
                IShow *show = act_mission->GetGame()->GetIShow((int)float_regs[i]);
                if (show)
                {
                    show->Stop();
                    show->SetCurrentPos(0);
                }
            }
        }
        break;
    case CMD_STREAM_DESTROY:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && i < num_float_regs)
                act_mission->GetGame()->DestroyIShow((int)float_regs[i]);
        }
        break;
    case CMD_STREAM_CONNECT:
        {
            // The video's first region's texture on the first face group of the object's first level
            int *p = (int *)instructions[cur_command].params;
            if (num_float_regs > p[0] && p[0] >= 0 && num_frame_regs > p[1] && p[1] >= 0 && frame_regs[p[1]] &&
                frame_regs[p[1]]->GetType() == FRAME_VISUAL &&
                ((I3D_visual *)frame_regs[p[1]])->GetVisualType() == I3D_VISUAL_OBJECT)
            {
                I3D_mesh_object *mesh = ((I3D_object *)frame_regs[p[1]])->GetMesh();
                if (mesh->NumLODs())
                {
                    I3D_mesh_level *level = mesh->GetLOD(0);
                    if (level->NumFGroups())
                    {
                        I3D_face_group *fg = level->GetFGroup(0);
                        IShow *show = act_mission->GetGame()->GetIShow((int)float_regs[p[0]]);
                        if (show && show->GetRegionCnt())
                        {
                            S_show_region *regions = show->GetRegions();
                            fg->GetMaterial()->SetTexture(regions->texture);
                        }
                    }
                }
            }
        }
        break;
    case CMD_STREAM_FADEVOL:
        {
            S_fade_params *f = (S_fade_params *)instructions[cur_command].params;
            if (num_float_regs > f->value1 && f->value1 >= 0)
                act_mission->GetGame()->IShowFade((int)float_regs[f->value1], f->value3, f->value4, f->value2);
        }
        break;
    case CMD_STREAM_SETLOOP:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_float_regs > p[0] && p[0] >= 0)
                act_mission->GetGame()->IShowLoop((int)float_regs[p[0]], p[1]);
        }
        break;
    case CMD_SETTANKHITCOUNT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
                ((C_car *)actor_regs[p[0]])->SetTankHitCount(p[1]);
        }
        break;
    case CMD_SETNOANIMHIT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] &&
                (actor_regs[p[0]]->GetType() == ACTOR_PLAYER || actor_regs[p[0]]->GetType() == ACTOR_ENTITY))
                ((C_human *)actor_regs[p[0]])->SetNoAnimHit(p[1]);
        }
        break;
    case CMD_CARLIGHT_MAIN:
        {
            // 1 on, 2 on with the main lights' projectors, other values off
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                uint projectors = ((C_car *)actor_regs[p[0]])->GetProjectors() & ~VLIGHT_PROJECTOR_MAIN;
                switch (p[1])
                {
                case 1:
                    ((C_car *)actor_regs[p[0]])->SetMainLights(true);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors);
                    ((C_car *)actor_regs[p[0]])->SetUnknown217c(false);
                    break;
                case 2:
                    ((C_car *)actor_regs[p[0]])->SetMainLights(true);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors | VLIGHT_PROJECTOR_MAIN);
                    ((C_car *)actor_regs[p[0]])->SetUnknown217c(false);
                    break;
                default:
                    ((C_car *)actor_regs[p[0]])->SetMainLights(false);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors);
                    ((C_car *)actor_regs[p[0]])->SetUnknown217c(true);
                }
            }
        }
        break;
    case CMD_CARLIGHT_LIGHT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                uint projectors = ((C_car *)actor_regs[p[0]])->GetProjectors() & ~VLIGHT_PROJECTOR_LIGHT;
                switch (p[1])
                {
                case 1:
                    ((C_car *)actor_regs[p[0]])->SetLight(true);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors);
                    break;
                case 2:
                    ((C_car *)actor_regs[p[0]])->SetLight(true);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors | VLIGHT_PROJECTOR_LIGHT);
                    break;
                default:
                    ((C_car *)actor_regs[p[0]])->SetLight(false);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors);
                }
            }
        }
        break;
    case CMD_CARLIGHT_INDIC_L:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                uint projectors = ((C_car *)actor_regs[p[0]])->GetProjectors() & ~VLIGHT_PROJECTOR_INDICATOR;
                switch (p[1])
                {
                case 1:
                    ((C_car *)actor_regs[p[0]])->SetLightsUnknown8(false);
                    ((C_car *)actor_regs[p[0]])->SetIndicatorL(true);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors);
                    break;
                case 2:
                    ((C_car *)actor_regs[p[0]])->SetLightsUnknown8(false);
                    ((C_car *)actor_regs[p[0]])->SetIndicatorL(true);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors | VLIGHT_PROJECTOR_INDICATOR);
                    break;
                default:
                    ((C_car *)actor_regs[p[0]])->SetLightsUnknown8(true);
                    ((C_car *)actor_regs[p[0]])->SetIndicatorL(false);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors);
                }
            }
        }
        break;
    case CMD_CARLIGHT_INDIC_R:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                uint projectors = ((C_car *)actor_regs[p[0]])->GetProjectors() & ~VLIGHT_PROJECTOR_INDICATOR;
                switch (p[1])
                {
                case 1:
                    ((C_car *)actor_regs[p[0]])->SetLightsUnknown8(false);
                    ((C_car *)actor_regs[p[0]])->SetIndicatorR(true);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors);
                    break;
                case 2:
                    ((C_car *)actor_regs[p[0]])->SetLightsUnknown8(false);
                    ((C_car *)actor_regs[p[0]])->SetIndicatorR(true);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors | VLIGHT_PROJECTOR_INDICATOR);
                    break;
                default:
                    ((C_car *)actor_regs[p[0]])->SetLightsUnknown8(true);
                    ((C_car *)actor_regs[p[0]])->SetIndicatorR(false);
                    ((C_car *)actor_regs[p[0]])->SetProjectors(projectors);
                }
            }
        }
        break;
    case CMD_CARLIGHT_INDIC_OFF:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
            {
                if (p[1])
                    ((C_car *)actor_regs[p[0]])->SetLightsUnknown8(true);
                else
                {
                    ((C_car *)actor_regs[p[0]])->SetLightsUnknown8(false);
                    ((C_car *)actor_regs[p[0]])->SetIndicatorL(false);
                    ((C_car *)actor_regs[p[0]])->SetIndicatorR(false);
                }
            }
        }
        break;
    case CMD_CAR_MUSTSTEAL:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR)
                ((C_car *)actor_regs[p[0]])->SetMustSteal(p[1]);
        }
        break;
    case CMD_GARAGE_CANSTEAL:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR &&
                num_float_regs > p[1] && p[1] >= 0)
                float_regs[p[1]] =
                    mafiaLoadSave.CanSteal((uchar *)((C_car *)actor_regs[p[0]])->GetModelName()) ? 1 : 0;
        }
        break;
    case CMD_CAR_INWATER:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs > p[0] && p[0] >= 0 && actor_regs[p[0]] && actor_regs[p[0]]->GetType() == ACTOR_CAR &&
                num_float_regs > p[1] && p[1] >= 0)
                float_regs[p[1]] = ((C_car *)actor_regs[p[0]])->in_water ? 1 : 0;
        }
        break;
    case CMD_CAR_SETPROJECTOR:
        {
            S_vector_params *v = (S_vector_params *)instructions[cur_command].params;
            if (num_actor_regs > v->reg && v->reg >= 0 && actor_regs[v->reg] &&
                actor_regs[v->reg]->GetType() == ACTOR_CAR)
                ((C_car *)actor_regs[v->reg])->SetProjectorSize(VLIGHT_PROJECTOR_MAIN, S_vector(v->x, v->y, v->z));
        }
        break;
    case CMD_HUMAN_FORCEFALL:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i >= 0 && i < num_actor_regs && actor_regs[i] &&
                (actor_regs[i]->GetType() == ACTOR_PLAYER || actor_regs[i]->GetType() == ACTOR_ENTITY))
                ((C_human *)actor_regs[i])->unknown_418 += .1f;
        }
        break;
    case CMD_CREATE_PHYSICALOBJECT:
        {
            S_fade_params *p = (S_fade_params *)instructions[cur_command].params;
            if (num_frame_regs <= p->value1 || p->value1 < 0 || !frame_regs[p->value1])
                break;
            C_public_physics *phys = new C_public_physics;
            if (phys->Init(frame_regs[p->value1]))
            {
                S_THING_BOX box;
                box.weight = Max(p->value4, 1.0f);
                box.unknown_10 = .45f;
                box.velocity = S_vector(0.0f, -p->value3, 0.0f);
                phys->InitBox(&box);
                act_mission->GetGame()->AddTemporaryActor(phys);
                if (p->value2)
                    phys->ExternalImpuls(S_vector(0.0f, box.weight * .01f, 0.0f),
                        S_vector(rnd() * 2.0f - 1.0f, rnd() * 2.0f - 1.0f, rnd() * 2.0f - 1.0f));
            }
            else
                phys->Release();
        }
        break;
    case CMD_WEATHER_RESET:
        act_mission->GetScene()->WeatherSystemReset();
        break;
    case CMD_WEATHER_SETPARAM:
        {
            S_weather_params *w = (S_weather_params *)instructions[cur_command].params;
            switch (w->param)
            {
            case WEATHER_ON: case WEATHER_SECTORS: case WEATHER_DUMMIES: case WEATHER_COLORH: case WEATHER_COLORL:
            case WEATHER_MAX_CNT: case WEATHER_MODE:
                if (w->value_reg == -1)
                    act_mission->GetScene()->SetWeatherSystemParam((WS_PARAM)w->param, w->value.i);
                else if (w->value_reg >= 0 && w->value_reg < num_float_regs)
                    act_mission->GetScene()->SetWeatherSystemParam((WS_PARAM)w->param,
                        (uint)float_regs[w->value_reg]);
                if (w->param == WEATHER_ON || w->param == WEATHER_MAX_CNT)
                    act_mission->GetGame()->OsefujPocasi(true, w->param);
                break;
            case WEATHER_SPEED: case WEATHER_LEN: case WEATHER_WIDTH: case WEATHER_MAX_DIST: case WEATHER_MAX_HEIGHT:
            case WEATHER_DIR_X: case WEATHER_DIR_Y: case WEATHER_DIR_Z:
                // The float's bits
                if (w->value_reg == -1)
                    act_mission->GetScene()->SetWeatherSystemParam((WS_PARAM)w->param, w->value.i);
                else if (w->value_reg >= 0 && w->value_reg < num_float_regs)
                {
                    float f = float_regs[w->value_reg];
                    act_mission->GetScene()->SetWeatherSystemParam((WS_PARAM)w->param, *(uint *)&f);
                }
                break;
            }
        }
        break;
    case CMD_GETTICKTIME:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            float_regs[p[0]] = (float)time;
        }
        break;
    case CMD_GETGAMETIME:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg < 0 || reg >= num_float_regs)
                break;
            float_regs[reg] = (float)act_mission->GetGame()->GetGameTime();
        }
        break;
    case CMD_POLICEMANAGER_SETSPEED:
        {
            // In km/h
            float *f = (float *)instructions[cur_command].params;
            act_mission->GetGame()->GetShvestky()->SetSpeed(f[0] * (1.0f / 3.6f), f[1] * (1.0f / 3.6f));
        }
        break;
    case CMD_GARAGE_ADDLASTSTOLEN:
        mafiaLoadSave.GarageAddLastStolen();
        break;
    case CMD_GARAGE_SHOW:
        mafiaLoadSave.GarageShowResults();
        break;
    case CMD_GETACTIVEPLAYER:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg < 0 || reg >= num_actor_regs)
                break;
            actor_regs[reg] = act_mission->GetGame()->GetPlayer();
        }
        break;
    case CMD_GETANGLEACTORTOACTOR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || num_actor_regs <= p[0] || !actor_regs[p[0]])
                break;
            if (p[1] < 0 || num_actor_regs <= p[1] || !actor_regs[p[1]])
                break;
            if (p[2] < 0 || num_float_regs <= p[2])
                break;
            float_regs[p[2]] = actor_regs[p[0]]->GetWorldDir().AngleTo(
                actor_regs[p[1]]->GetWorldPos() - actor_regs[p[0]]->GetWorldPos());
            float_regs[p[2]] *= 180.0f / PI;
        }
        break;
    case CMD_POLICEMANAGER_FORCEARREST:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            act_mission->GetGame()->GetShvestky()->ForceArrest((C_human *)actor_regs[p[0]], p[1] != 0, p[2] != 0);
        }
        break;
    case CMD_SETCITYTRAFFICVISIBLE:
        {
            int *p = (int *)instructions[cur_command].params;
            act_mission->GetGame()->SetTrafficVisible(p[0] != 0);
        }
        break;
    case CMD_CONSOLE_ADDTEXT:
        // The parameter is read after the colour is pushed: the call's argument
        mafiaIndicators.ConsoleAddTextByID(*(int *)instructions[cur_command].params, 0xffffff);
        break;
    case CMD_CAR_SETSPEED:
        {
            // In km/h
            S_int_float_params *p = (S_int_float_params *)instructions[cur_command].params;
            if (num_actor_regs <= p->value1 || p->value1 < 0 || !actor_regs[p->value1])
                break;
            if (actor_regs[p->value1]->GetType() != ACTOR_CAR)
                break;
            C_car *car = (C_car *)actor_regs[p->value1];
            car->Deactivate(0);
            car->Reset(p->value2 * (1.0f / 3.6f), true);
            if (act_mission->GetGame()->GetCamera().GetCar() == car)
                act_mission->GetGame()->GetCamera().SetCar(car);
        }
        break;
    case CMD_GET_PM_FIRETIME:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = 0.0f;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            float_regs[p[1]] = (float)act_mission->GetGame()->GetShvestky()->GetFireTime((C_human *)actor_regs[p[0]]);
        }
        break;
    case CMD_GET_PM_NUMPREDATORS:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = 0.0f;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            float_regs[p[1]] =
                (float)act_mission->GetGame()->GetShvestky()->PocetNasranych((C_human *)actor_regs[p[0]]);
        }
        break;
    case CMD_SHOWCARDAMAGE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_CAR)
                break;
            ((C_car *)actor_regs[p[0]])->SetShowDamage(p[1] != 0);
        }
        break;
    case CMD_CAMERA_SETRANGE:
        {
            float *f = (float *)instructions[cur_command].params;
            act_mission->GetScene()->GetActiveCamera()->SetRange(f[0], f[1]);
        }
        break;
    case CMD_ACTOR_DUPLICATE:
        {
            // A copy of a model actor's model, from the cache, its name ending in D; a plain actor with the third
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (num_actor_regs <= p[1] || p[1] < 0)
                break;
            actor_regs[p[1]] = NULL;
            I3D_frame *frm = actor_regs[p[0]]->GetFrame();
            if (!frm)
                break;
            if (frm->GetType() != FRAME_MODEL)
                break;
            I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            const char *name = ((I3D_model *)frm)->GetFileName();
            const char *s = strrchr(name, '\\');
            if (s)
                name = s + 1;
            if (model_cache.Open(model, name, 0, NULL, NULL, NULL) != I3D_OK)
            {
                model->Release();
                break;
            }
            const char *frm_name = frm->GetName();
            char *new_name = new char[strlen(frm_name) + 2];
            strcpy(new_name, frm_name);
            // STRING: GAME 0x6343a4
            strcat(new_name, "D");
            model->SetName(new_name);
            model->LinkTo(frm->GetParent(), 0);
            model->SetPos(frm->GetPos());
            model->SetRot(frm->GetRot());
            model->Update();
            C_actor *act = act_mission->CreateActor(p[2] ? ACTOR_ACTOR : actor_regs[p[0]]->GetType());
            if (act)
            {
                if (act->Init(model))
                {
                    act->SetOwnFrame(true);
                    act_mission->GetGame()->AddTemporaryActor(act);
                    actor_regs[p[1]] = act;
                }
                else
                    act->Release();
            }
            else
                model->Release();
        }
        break;
    case CMD_ACTOR_DELETE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]])
                break;
            act_mission->GetGame()->RemoveTemporaryActor(actor_regs[p[0]]);
        }
        break;
    case CMD_CAR_FORCESTOP:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg < 0 || reg >= num_actor_regs || !actor_regs[reg])
                break;
            if (actor_regs[reg]->GetType() != ACTOR_CAR)
                break;
            C_car *car = (C_car *)actor_regs[reg];
            car->Deactivate(0);
            car->Reset(0.0f, true);
        }
        break;
    case CMD_HUMAN_RESET:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg < 0 || reg >= num_actor_regs || !actor_regs[reg])
                break;
            if (actor_regs[reg]->GetType() != ACTOR_PLAYER && actor_regs[reg]->GetType() != ACTOR_ENTITY)
                break;
            ((C_human *)actor_regs[reg])->ResetActions();
        }
        break;
    case CMD_HUMAN_HAVEBOX:
        {
            // Whether the human carries a box, and the box's index, -1 without one
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            C_human *human = (C_human *)actor_regs[p[0]];
            float_regs[p[1]] = human->box ? 1.0f : 0.0f;
            if (num_float_regs <= p[2] || p[2] < 0)
                break;
            if (human->box)
                float_regs[p[2]] = (float)human->box_index;
            else
                float_regs[p[2]] = -1.0f;
        }
        break;
    case CMD_HUMAN_HAVEBODY:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = ((C_human *)actor_regs[p[0]])->dead_body ? 1.0f : 0.0f;
        }
        break;
    case CMD_HUMAN_STOPTALK:
        {
            int reg = *(int *)instructions[cur_command].params;
            if (reg < 0 || reg >= num_actor_regs || !actor_regs[reg])
                break;
            if (actor_regs[reg]->GetType() != ACTOR_PLAYER && actor_regs[reg]->GetType() != ACTOR_ENTITY)
                break;
            act_mission->GetGame()->DeathHumanTalk((C_human *)actor_regs[reg]);
        }
        break;
    case CMD_ENEMY_BLOCK:
        act_mission->GetGame()->EntityBlock(true, false);
        break;
    case CMD_ENEMY_UNBLOCK:
        act_mission->GetGame()->EntityBlock(false, true);
        break;
    case CMD_GET_REMOTE_FLOAT:
        {
            // A register of another actor's script: a script object's or an AI human's (both derive from C_program)
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] >= num_actor_regs || !actor_regs[p[1]])
                break;
            switch (actor_regs[p[1]]->GetType())
            {
            case ACTOR_DETECTOR:
                float_regs[p[0]] = ((C_detector *)actor_regs[p[1]])->GetFloatReg(p[2]);
                break;
            case ACTOR_ENTITY:
                float_regs[p[0]] = ((C_entity *)actor_regs[p[1]])->GetFloatReg(p[2]);
                break;
            }
        }
        break;
    case CMD_GET_REMOTE_FRAME:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs)
                break;
            if (p[1] < 0 || p[1] >= num_actor_regs || !actor_regs[p[1]])
                break;
            switch (actor_regs[p[1]]->GetType())
            {
            case ACTOR_DETECTOR:
                frame_regs[p[0]] = ((C_detector *)actor_regs[p[1]])->GetFrameReg(p[2]);
                break;
            case ACTOR_ENTITY:
                frame_regs[p[0]] = ((C_entity *)actor_regs[p[1]])->GetFrameReg(p[2]);
                break;
            }
        }
        break;
    case CMD_GET_REMOTE_ACTOR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs)
                break;
            if (p[1] < 0 || p[1] >= num_actor_regs || !actor_regs[p[1]])
                break;
            switch (actor_regs[p[1]]->GetType())
            {
            case ACTOR_DETECTOR:
                actor_regs[p[0]] = ((C_detector *)actor_regs[p[1]])->GetActorReg(p[2]);
                break;
            case ACTOR_ENTITY:
                actor_regs[p[0]] = ((C_entity *)actor_regs[p[1]])->GetActorReg(p[2]);
                break;
            }
        }
        break;
    case CMD_SET_REMOTE_FLOAT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] >= num_actor_regs || !actor_regs[p[1]])
                break;
            switch (actor_regs[p[1]]->GetType())
            {
            case ACTOR_DETECTOR:
                ((C_detector *)actor_regs[p[1]])->SetFloatReg(p[2], float_regs[p[0]]);
                break;
            case ACTOR_ENTITY:
                ((C_entity *)actor_regs[p[1]])->SetFloatReg(p[2], float_regs[p[0]]);
                break;
            }
        }
        break;
    case CMD_SET_REMOTE_FRAME:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs)
                break;
            if (p[1] < 0 || p[1] >= num_actor_regs || !actor_regs[p[1]])
                break;
            switch (actor_regs[p[1]]->GetType())
            {
            case ACTOR_DETECTOR:
                ((C_detector *)actor_regs[p[1]])->SetFrameReg(p[2], frame_regs[p[0]]);
                break;
            case ACTOR_ENTITY:
                ((C_entity *)actor_regs[p[1]])->SetFrameReg(p[2], frame_regs[p[0]]);
                break;
            }
        }
        break;
    case CMD_SET_REMOTE_ACTOR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs)
                break;
            if (p[1] < 0 || p[1] >= num_actor_regs || !actor_regs[p[1]])
                break;
            switch (actor_regs[p[1]]->GetType())
            {
            case ACTOR_DETECTOR:
                ((C_detector *)actor_regs[p[1]])->SetActorReg(p[2], actor_regs[p[0]]);
                break;
            case ACTOR_ENTITY:
                ((C_entity *)actor_regs[p[1]])->SetActorReg(p[2], actor_regs[p[0]]);
                break;
            }
        }
        break;
    case CMD_GETACTORFRAME:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (num_frame_regs <= p[1] || p[1] < 0)
                break;
            frame_regs[p[1]] = actor_regs[p[0]]->GetFrame();
        }
        break;
    case CMD_ZATMYSE:
        {
            // Czech for "darken": the screen fades out, or with 0 in, over the time
            int *p = (int *)instructions[cur_command].params;
            if (p[0])
                mafiaIndicators.FadeInOut(true, p[1], 0);
            else
                mafiaIndicators.FadeInOut(false, p[1], 0);
        }
        break;
    case CMD_CARDAMAGEVISIBLE:
        {
            int on = *(int *)instructions[cur_command].params;
            C_player::SetUnknown6478b0(on != 0);
            if (on)
            {
                if (act_mission->GetGame()->GetPlayer() && act_mission->GetGame()->GetPlayer()->GetCar() &&
                    act_mission->GetGame()->GetPlayer()->GetCar()->GetType() == ACTOR_CAR)
                    mafiaIndicators.AddFlags(INDF_UNKNOWN_10000);
            }
            else
                mafiaIndicators.RemoveFlags(INDF_UNKNOWN_10000);
        }
        break;
    case CMD_IFPLAYERSTEALCAR:
        {
            // VC6 shares the else with CAR_INWATER's, and that case's 1 with this one's
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_CAR)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            if (!act_mission->GetGame()->GetPlayer())
                break;
            if (act_mission->GetGame()->GetPlayer()->stealing_car == actor_regs[p[0]])
                float_regs[p[1]] = 1.0f;
            else
                float_regs[p[1]] = 0.0f;
        }
        break;
    case CMD_INTRO_SUBTITLE_ADD:
        {
            int *p = (int *)instructions[cur_command].params;
            mafiaIndicators.IntroSubtitlesAdd(p[0], p[1]);
        }
        break;
    case CMD_MATH_COS:
        {
            // In degrees, as the arc functions' results
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            float_regs[p[0]] = (float)cos(float_regs[p[0]] * (PI / 180.0f));
        }
        break;
    case CMD_MATH_SIN:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            float_regs[p[0]] = (float)sin(float_regs[p[0]] * (PI / 180.0f));
        }
        break;
    case CMD_MATH_ACOS:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            float_regs[p[0]] = (float)acos(float_regs[p[0]]) * (180.0f / PI);
        }
        break;
    case CMD_MATH_ASIN:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            float_regs[p[0]] = (float)asin(float_regs[p[0]]) * (180.0f / PI);
        }
        break;
    case CMD_MATH_ATAN:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            float_regs[p[0]] = (float)atan(float_regs[p[0]]) * (180.0f / PI);
        }
        break;
    case CMD_MATH_ABS:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            float_regs[p[0]] = (float)fabs(float_regs[p[0]]);
        }
        break;
    case CMD_FRM_GETDIR:
        {
            // Into three float registers from the second parameter on
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            S_vector dir = frame_regs[p[0]]->GetDir();
            float_regs[p[1]] = dir.x;
            float_regs[p[1] + 1] = dir.y;
            float_regs[p[1] + 2] = dir.z;
        }
        break;
    case CMD_FRM_SETDIR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            frame_regs[p[0]]->SetDir(S_vector(float_regs[p[1]], float_regs[p[1] + 1], float_regs[p[1] + 2]), 0.0f);
            frame_regs[p[0]]->Update();
        }
        break;
    case CMD_FRM_GETPOS:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            S_vector pos = frame_regs[p[0]]->GetPos();
            float_regs[p[1]] = pos.x;
            float_regs[p[1] + 1] = pos.y;
            float_regs[p[1] + 2] = pos.z;
        }
        break;
    case CMD_FRM_SETPOS:
        {
            // A frame at the top of the hierarchy or in a sector gets the sector of its new position
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            frame_regs[p[0]]->SetPos(S_vector(float_regs[p[1]], float_regs[p[1] + 1], float_regs[p[1] + 2]));
            frame_regs[p[0]]->Update();
            if (!frame_regs[p[0]]->GetParent() || frame_regs[p[0]]->GetParent()->GetType() == FRAME_SECTOR)
                act_mission->GetScene()->SetFrameSector(frame_regs[p[0]]);
        }
        break;
    case CMD_FRM_GETSCALE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            S_vector s = frame_regs[p[0]]->GetScale();
            float_regs[p[1]] = s.x;
            float_regs[p[1] + 1] = s.y;
            float_regs[p[1] + 2] = s.z;
        }
        break;
    case CMD_FRM_SETSCALE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            S_vector s(float_regs[p[1]], float_regs[p[1] + 1], float_regs[p[1] + 2]);
            frame_regs[p[0]]->SetScale(s);
            frame_regs[p[0]]->Update();
        }
        break;
    case CMD_VECT_COPY:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            float_regs[p[0]] = float_regs[p[1]];
            float_regs[p[0] + 1] = float_regs[p[1] + 1];
            float_regs[p[0] + 2] = float_regs[p[1] + 2];
        }
        break;
    case CMD_VECT_MUL_VECT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            float_regs[p[0]] *= float_regs[p[1]];
            float_regs[p[0] + 1] *= float_regs[p[1] + 1];
            float_regs[p[0] + 2] *= float_regs[p[1] + 2];
        }
        break;
    case CMD_VECT_MUL_QUAT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            // A reference: the original takes the vector's address before the rotation's matrix is made
            S_vector &v = *(S_vector *)&float_regs[p[0]];
            v *= ((S_quat *)&float_regs[p[1]])->RotationMatrix();
        }
        break;
    case CMD_VECT_MUL_MATRIX:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 15 >= num_float_regs)
                break;
            S_vector v(float_regs[p[0]], float_regs[p[0] + 1], float_regs[p[0] + 2]);
            v *= *(S_matrix *)&float_regs[p[1]];
            float_regs[p[0]] = v.x;
            float_regs[p[0] + 1] = v.y;
            float_regs[p[0] + 2] = v.z;
        }
        break;
    case CMD_VECT_ADD_VECT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            float_regs[p[0]] += float_regs[p[1]];
            float_regs[p[0] + 1] += float_regs[p[1] + 1];
            float_regs[p[0] + 2] += float_regs[p[1] + 2];
        }
        break;
    case CMD_VECT_SUB_VECT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            float_regs[p[0]] -= float_regs[p[1]];
            float_regs[p[0] + 1] -= float_regs[p[1] + 1];
            float_regs[p[0] + 2] -= float_regs[p[1] + 2];
        }
        break;
    case CMD_VECT_CROSS:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            S_vector v1(float_regs[p[0]], float_regs[p[0] + 1], float_regs[p[0] + 2]);
            S_vector v2(float_regs[p[1]], float_regs[p[1] + 1], float_regs[p[1] + 2]);
            S_vector v = v1.Cross(v2);
            float_regs[p[0]] = v.x;
            float_regs[p[0] + 1] = v.y;
            float_regs[p[0] + 2] = v.z;
        }
        break;
    case CMD_VECT_MAGNITUDE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] >= num_float_regs)
                break;
            S_vector v(float_regs[p[0]], float_regs[p[0] + 1], float_regs[p[0] + 2]);
            float_regs[p[1]] = v.Magnitude();
        }
        break;
    case CMD_VECT_ANGLETO:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            if (p[2] < 0 || p[2] >= num_float_regs)
                break;
            S_vector v1(float_regs[p[0]], float_regs[p[0] + 1], float_regs[p[0] + 2]);
            S_vector v2(float_regs[p[1]], float_regs[p[1] + 1], float_regs[p[1] + 2]);
            float_regs[p[2]] = v1.AngleTo(v2);
        }
        break;
    case CMD_VECT_NORMALIZE:
        {
            // The register in a local: the original doesn't read it again after the stores
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i + 2 >= num_float_regs)
                break;
            S_vector v(float_regs[i], float_regs[i + 1], float_regs[i + 2]);
            // __forceinline, the one call Process inlines
            v.Normalize();
            float_regs[i] = v.x;
            float_regs[i + 1] = v.y;
            float_regs[i + 2] = v.z;
        }
        break;
    case CMD_VECT_INVERSE:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i + 2 >= num_float_regs)
                break;
            float_regs[i] = -float_regs[i];
            float_regs[i + 1] = -float_regs[i + 1];
            float_regs[i + 2] = -float_regs[i + 2];
        }
        break;
    case CMD_VECT_SET:
        {
            S_vector_params *p = (S_vector_params *)instructions[cur_command].params;
            if (p->reg < 0 || p->reg + 2 >= num_float_regs)
                break;
            float_regs[p->reg] = p->x;
            float_regs[p->reg + 1] = p->y;
            float_regs[p->reg + 2] = p->z;
        }
        break;
    case CMD_VECT_MUL_SCL:
        {
            S_reg_operand_params *p = (S_reg_operand_params *)instructions[cur_command].params;
            if (p->reg < 0 || p->reg + 2 >= num_float_regs)
                break;
            if (p->value_reg == -1)
            {
                float_regs[p->reg] *= p->value;
                float_regs[p->reg + 1] *= p->value;
                float_regs[p->reg + 2] *= p->value;
            }
            else
            {
                if (p->value_reg < 0 || p->value_reg >= num_float_regs)
                    break;
                float_regs[p->reg] *= float_regs[p->value_reg];
                float_regs[p->reg + 1] *= float_regs[p->value_reg];
                float_regs[p->reg + 2] *= float_regs[p->value_reg];
            }
        }
        break;
    case CMD_VECT_ROTATE_2D:
        {
            // The angle's register isn't checked
            S_reg_operand_params *p = (S_reg_operand_params *)instructions[cur_command].params;
            if (p->reg < 0 || p->reg + 2 >= num_float_regs)
                break;
            S_vector v(float_regs[p->reg], float_regs[p->reg + 1], float_regs[p->reg + 2]);
            if (p->value_reg == -1)
                RotateDir(v, p->value);
            else
                RotateDir(v, float_regs[p->value_reg]);
            float_regs[p->reg] = v.x;
            float_regs[p->reg + 1] = v.y;
            float_regs[p->reg + 2] = v.z;
        }
        break;
    case CMD_COLL_TESTLINE:
        {
            // flt[p[0]] 1 for a hit, the line from flt[p[1]] along flt[p[2]], the hit's position and normal into
            // flt[p[3]] and flt[p[4]]
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            if (p[2] < 0 || p[2] + 2 >= num_float_regs)
                break;
            if (p[3] < 0 || p[3] + 2 >= num_float_regs)
                break;
            if (p[4] < 0 || p[4] + 2 >= num_float_regs)
                break;
            S_vector from(float_regs[p[1]], float_regs[p[1] + 1], float_regs[p[1] + 2]);
            S_vector dir(float_regs[p[2]], float_regs[p[2] + 1], float_regs[p[2] + 2]);
            S_vector pos, normal;
            if (gifCollision.TestLineV(from, dir, &pos, &normal, COLF_UNKNOWN_2, 0))
            {
                float_regs[p[0]] = 1.0f;
                float_regs[p[3]] = pos.x;
                float_regs[p[3] + 1] = pos.y;
                float_regs[p[3] + 2] = pos.z;
                float_regs[p[4]] = normal.x;
                float_regs[p[4] + 1] = normal.y;
                float_regs[p[4] + 2] = normal.z;
            }
            else
                float_regs[p[0]] = 0.0f;
        }
        break;
    case CMD_VLVP:
        {
            // Whether the police have seen the player lately. The else's store is tail-merged with a later case's.
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_float_regs)
                break;
            if (act_mission->GetGame()->GetPlayer())
                float_regs[i] = AIManager.police.PanoveMateOciNaZmrdoviNeboCumitePoBabach(
                    act_mission->GetGame()->GetPlayer()) ? 1.0f : 0.0f;
            else
                float_regs[i] = 0.0f;
        }
        break;
    case CMD_MATRIX_COPY:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 15 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 15 >= num_float_regs)
                break;
            for (int i = 16; i--;)
                float_regs[p[0] + i] = float_regs[p[1] + i];
        }
        break;
    case CMD_MATRIX_MUL:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 15 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 15 >= num_float_regs)
                break;
            *(S_matrix *)&float_regs[p[0]] *= *(S_matrix *)&float_regs[p[1]];
        }
        break;
    case CMD_QUAT_COPY:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            for (int i = 4; i--;)
                float_regs[p[0] + i] = float_regs[p[1] + i];
        }
        break;
    case CMD_QUAT_GETROTMATRIX:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 15 >= num_float_regs)
                break;
            // The destination's address is taken before the call
            S_matrix &m = *(S_matrix *)&float_regs[p[1]];
            m = ((S_quat *)&float_regs[p[0]])->RotationMatrix();
        }
        break;
    case CMD_QUAT_ADD:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            S_quat &q = *(S_quat *)&float_regs[p[0]];
            q = q + *(S_quat *)&float_regs[p[1]];
        }
        break;
    case CMD_QUAT_SUB:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            S_quat &q = *(S_quat *)&float_regs[p[0]];
            q = q - *(S_quat *)&float_regs[p[1]];
        }
        break;
    case CMD_QUAT_MUL_QUAT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            *(S_quat *)&float_regs[p[0]] *= *(S_quat *)&float_regs[p[1]];
        }
        break;
    case CMD_QUAT_ROTBYMATRIX:
        {
            // The rotated quaternion is thrown away, as the original's
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 15 >= num_float_regs)
                break;
            ((S_quat *)&float_regs[p[0]])->RotateByMatrix(*(S_matrix *)&float_regs[p[1]]);
        }
        break;
    case CMD_QUAT_MUL_SCL:
        {
            S_reg_operand_params *p = (S_reg_operand_params *)instructions[cur_command].params;
            if (p->reg < 0 || p->reg + 3 >= num_float_regs)
                break;
            if (p->value_reg == -1)
            {
                S_quat &q = *(S_quat *)&float_regs[p->reg];
                q = q * p->value;
            }
            else
            {
                if (p->value_reg < 0 || p->value_reg >= num_float_regs)
                    break;
                S_quat &q = *(S_quat *)&float_regs[p->reg];
                q = q * float_regs[p->value_reg];
            }
        }
        break;
    case CMD_MATRIX_ZERO:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i + 15 >= num_float_regs)
                break;
            ((S_matrix *)&float_regs[i])->Zero();
        }
        break;
    case CMD_MATRIX_IDENTITY:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i + 15 >= num_float_regs)
                break;
            ((S_matrix *)&float_regs[i])->Identity();
        }
        break;
    case CMD_MATRIX_INVERSE:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i + 15 >= num_float_regs)
                break;
            ((S_matrix *)&float_regs[i])->Invert();
        }
        break;
    case CMD_QUAT_INVERSE:
        {
            // The quaternion negated
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i + 3 >= num_float_regs)
                break;
            S_quat &q = *(S_quat *)&float_regs[i];
            q = q * -1.0f;
        }
        break;
    case CMD_QUAT_NORMALIZE:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i + 3 >= num_float_regs)
                break;
            ((S_quat *)&float_regs[i])->Normalize();
        }
        break;
    case CMD_QUAT_MAKE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            if (p[2] < 0 || p[2] >= num_float_regs)
                break;
            ((S_quat *)&float_regs[p[0]])->Make(*(S_vector *)&float_regs[p[1]], float_regs[p[2]]);
        }
        break;
    case CMD_QUAT_SETDIR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            if (p[2] < 0 || p[2] >= num_float_regs)
                break;
            ((S_quat *)&float_regs[p[0]])->SetDir(*(S_vector *)&float_regs[p[1]], float_regs[p[2]]);
        }
        break;
    case CMD_QUAT_EXTRACT:
        {
            // The axis and the angle
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            if (p[2] < 0 || p[2] >= num_float_regs)
                break;
            ((S_quat *)&float_regs[p[0]])->Inverse(*(S_vector *)&float_regs[p[1]], float_regs[p[2]]);
        }
        break;
    case CMD_QUAT_DOT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            if (p[2] < 0 || p[2] >= num_float_regs)
                break;
            float_regs[p[2]] = ((S_quat *)&float_regs[p[0]])->Dot(*(S_quat *)&float_regs[p[1]]);
        }
        break;
    case CMD_QUAT_SLERP:
        {
            // As the original's: the third register tested from above first, the fourth only from above
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 3 >= num_float_regs)
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            if (p[2] + 3 >= num_float_regs || p[2] < 0)
                break;
            if (p[3] >= num_float_regs)
                break;
            S_quat &q = *(S_quat *)&float_regs[p[0]];
            q = ((S_quat *)&float_regs[p[1]])->Slerp(*(S_quat *)&float_regs[p[2]], float_regs[p[3]], false);
        }
        break;
    case CMD_FRM_GETROT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            *(S_quat *)&float_regs[p[1]] = frame_regs[p[0]]->GetRot();
        }
        break;
    case CMD_FRM_SETROT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            frame_regs[p[0]]->SetRot(*(S_quat *)&float_regs[p[1]]);
            frame_regs[p[0]]->Update();
        }
        break;
    case CMD_FRM_GETWORLDDIR:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            *(S_vector *)&float_regs[p[1]] = frame_regs[p[0]]->GetWorldDir();
        }
        break;
    case CMD_FRM_GETWORLDPOS:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            *(S_vector *)&float_regs[p[1]] = frame_regs[p[0]]->GetWorldPos();
        }
        break;
    case CMD_FRM_GETWORLDSCALE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs)
                break;
            *(S_vector *)&float_regs[p[1]] = frame_regs[p[0]]->GetWorldScale();
        }
        break;
    case CMD_FRM_GETWORLDROT:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 3 >= num_float_regs)
                break;
            *(S_quat *)&float_regs[p[1]] = frame_regs[p[0]]->GetWorldRot();
        }
        break;
    case CMD_FRM_GETLOCALMATRIX:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 15 >= num_float_regs)
                break;
            *(S_matrix *)&float_regs[p[1]] = frame_regs[p[0]]->GetLocalMatrix();
        }
        break;
    case CMD_FRM_GETWORLDMATRIX:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (p[1] < 0 || p[1] + 15 >= num_float_regs)
                break;
            *(S_matrix *)&float_regs[p[1]] = frame_regs[p[0]]->GetMatrix();
        }
        break;
    case CMD_CTRL_READ:
        {
            // The control isn't checked from above
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || num_float_regs <= p[0])
                break;
            if (p[1] < 0)
                break;
            float *values;
            controls.GetValues(values, 0);
            float_regs[p[0]] = values[p[1]];
        }
        break;
    case CMD_CTRL_READEX:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || num_float_regs <= p[0] || p[1] < 0)
            break;
        float *list;
        controls.GetValues(list, CONTROLS_PRESSED);
        float_regs[p[0]] = list[p[1]];
    }
        break;
    case CMD_GETFRAMEFROMACTOR:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_frame_regs || p[1] < 0 || p[1] >= num_actor_regs || !actor_regs[p[1]])
            break;
        frame_regs[p[0]] = actor_regs[p[1]]->GetFrame();
    }
        break;
    case CMD_ACTORUPDATEPLACEMENT:
    {
        int i = *(int *)instructions[cur_command].params;
        if (i < 0 || i >= num_actor_regs || !actor_regs[i] || !actor_regs[i]->GetFrame())
            break;
        actor_regs[i]->SetPos(actor_regs[i]->GetFrame()->GetPos());
        actor_regs[i]->SetDir(actor_regs[i]->GetFrame()->GetDir());
    }
        break;
    case CMD_RACING_AUTOINVISIBLE:
        act_mission->GetGame()->AddInvisibleActor(actor);
        break;
    case CMD_CAR_INVISIBLE:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
            break;
        C_car *car = (C_car *)actor_regs[p[0]];
        car->SetInvisible(p[1] != 0);
        car->invisible = p[1] != 0;
        car->SetEnterProtect(p[1] != 0);
    }
        break;
    case CMD_CAR_SWITCHSHOWENERGY:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
            break;
        ((C_car *)actor_regs[p[0]])->SetShowEnergy(p[1]);
    }
        break;
    case CMD_POLICEITCHFORPLAYER:
    {
        int i = *(int *)instructions[cur_command].params;
        if (i < 0 || i >= num_float_regs)
            break;
        if (act_mission->GetGame()->GetPlayer())
            float_regs[i] = (act_mission->GetGame()->GetShvestky()->PocetNasranychZivych(
                act_mission->GetGame()->GetPlayer()) <= 0 ? AIManager.police.PanoveHrajeteSiNaPoldy() :
                !act_mission->GetGame()->GetShvestky()->NeviDoKterePiceVlezl(act_mission->GetGame()->GetPlayer())) ?
                1.0f : 0.0f;
        else
            float_regs[i] = 0.0f;
    }
        break;
    case CMD_NOANIMPRELOAD:
        act_mission->GetGame()->SetAnimPreload(false);
        break;
    case CMD_DESTROY_PHYSICALOBJECT:
    {
        int i = *(int *)instructions[cur_command].params;
        if (i < 0 || i >= num_frame_regs || !frame_regs[i])
            break;
        std::vector<C_actor *> &actors = C_actor::GetGlobalActors();
        for (int n = actors.size(); n--; )
        {
            if (actors[n]->GetType() == ACTOR_PUBLIC_PHYSICS && actors[n]->GetFrame() == frame_regs[i])
            {
                act_mission->GetGame()->RemoveTemporaryActor(actors[n]);
                break;
            }
        }
    }
        break;
    case CMD_SETNULLACTOR:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_actor_regs)
            break;
        actor_regs[p[0]] = NULL;
    }
        break;
    case CMD_SETNULLFRAME:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_frame_regs)
            break;
        frame_regs[p[0]] = NULL;
    }
        break;
    case CMD_SETFREERIDE:
        mafiaLoadSave.FreerideEnable(*(int *)instructions[cur_command].params);
        break;
    case CMD_COMPAREACTORS:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_actor_regs || p[1] < 0 || p[1] >= num_actor_regs || p[2] < 0 ||
            p[2] >= num_float_regs)
            break;
        if (actor_regs[p[0]] == actor_regs[p[1]])
            float_regs[p[2]] = 1.0f;
        else
            float_regs[p[2]] = 0.0f;
    }
        break;
    case CMD_COMPAREFRAMES:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_frame_regs || p[1] < 0 || p[1] >= num_frame_regs || p[2] < 0 ||
            p[2] >= num_float_regs)
            break;
        if (frame_regs[p[0]] == frame_regs[p[1]])
            float_regs[p[2]] = 1.0f;
        else
            float_regs[p[2]] = 0.0f;
    }
        break;
    case CMD_ENABLEMAP:
        mafiaIndicators.MapEnable(*(int *)instructions[cur_command].params != 0);
        break;
    case CMD_FRAMEISTELEPHONE:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_frame_regs <= p[0] || p[0] < 0 || num_float_regs <= p[1] || p[1] < 0)
            break;
        float_regs[p[1]] = act_mission->GetGame()->IsTelephoneExist(frame_regs[p[0]]) ? 1.0f : 0.0f;
    }
        break;
    case CMD_CAR_CALM:
    {
        int i = *(int *)instructions[cur_command].params;
        if (num_actor_regs <= i || i < 0 || !actor_regs[i] || actor_regs[i]->GetType() != ACTOR_CAR ||
            !((C_car *)actor_regs[i])->IsCarUsable(1))
            break;
        ((C_car *)actor_regs[i])->CalmVehicle(20);
    }
        break;
    case CMD_BRIDGE_SHUTDOWN:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_BRIDGE)
            break;
        ((C_bridge *)actor_regs[p[0]])->ShutDown(p[1] != 0);
    }
        break;
    case CMD_SETTRAFFSECTORSND:
    {
        I3D_frame *frm = act_mission->GetScene()->FindFrame((const char *)instructions[cur_command].params,
            ENUMF_SECTOR);
        if (frm)
            C_Vehicle::AddSectorForSounds((I3D_sector *)frm);
    }
        break;
    case CMD_CITYMUSIC_OFF:
        act_mission->GetGame()->CityMusicPaused(true);
        break;
    case CMD_CITYMUSIC_ON:
        act_mission->GetGame()->CityMusicPaused(false);
        break;
    case CMD_GETACTIVECAMERA:
    {
        int i = *(int *)instructions[cur_command].params;
        if (i < 0 || i >= num_frame_regs)
            break;
        frame_regs[i] = act_mission->GetScene()->GetActiveCamera();
    }
        break;
    case CMD_FRM_GETPARENT:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_frame_regs <= p[1] || p[1] < 0)
            break;
        frame_regs[p[1]] = NULL;
        if (num_frame_regs <= p[0] || p[0] < 0 || !frame_regs[p[0]])
            break;
        frame_regs[p[1]] = frame_regs[p[0]]->GetParent();
    }
        break;
    case CMD_ISPOINTINSECTOR:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] + 2 >= num_float_regs || p[1] < 0 || p[1] >= num_frame_regs || p[2] < 0 ||
            p[2] >= num_float_regs)
            break;
        if (!frame_regs[p[1]] || frame_regs[p[1]]->GetType() != FRAME_SECTOR)
            break;
        if (((I3D_sector *)frame_regs[p[1]])->CheckPoint(*(S_vector *)&float_regs[p[0]]))
            float_regs[p[2]] = 1.0f;
        else
            float_regs[p[2]] = 0.0f;
    }
        break;
    case CMD_ISPOINTINAREA:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] + 2 >= num_float_regs || p[1] < 0 || p[1] >= num_frame_regs || p[2] < 0 ||
            p[2] >= num_float_regs)
            break;
        if (!frame_regs[p[1]] || frame_regs[p[1]]->GetType() != FRAME_AREA)
            break;
        if (((I3D_area *)frame_regs[p[1]])->CheckPoint(*(S_vector *)&float_regs[p[0]]))
            float_regs[p[2]] = 1.0f;
        else
            float_regs[p[2]] = 0.0f;
    }
        break;
    case CMD_SOUND_GETVOLUME:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_float_regs <= p[1] || p[1] < 0 || num_frame_regs <= p[0] || p[0] < 0 || !frame_regs[p[0]] ||
            frame_regs[p[0]]->GetType() != FRAME_SOUND)
            break;
        float_regs[p[1]] = ((I3D_sound *)frame_regs[p[0]])->GetVolume();
    }
        break;
    case CMD_SOUND_SETVOLUME:
    {
        S_reg_operand_params *s = (S_reg_operand_params *)instructions[cur_command].params;
        if (s->reg < 0 || s->reg >= num_frame_regs || !frame_regs[s->reg] ||
            frame_regs[s->reg]->GetType() != FRAME_SOUND)
            break;
        I3D_sound *snd = (I3D_sound *)frame_regs[s->reg];
        float vol = 0.0f;
        if (s->value_reg == -1)
            vol = s->value;
        else if (s->value_reg >= 0 && s->value_reg < num_float_regs)
            vol = float_regs[s->value_reg];
        if (vol < 0.0f)
            vol = 0.0f;
        else if (vol > 1.0f)
            vol = 1.0f;
        snd->SetVolume(vol);
    }
        break;
    case CMD_RACING_CHANGE_MODEL:
    {
        if (!CRacing)
            break;
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
            break;
        C_human *human = (C_human *)actor_regs[p[0]];
        for (int n = CRacing->GetUnknown298(); n--; )
        {
            if (CRacing->GetOpponents()[n]->GetUnknown60() == p[1])
            {
                human->_intern_ChangeModel(mafiaCarDatabase.GetPlayerModel(CRacing->GetOpponents()[n]->unknown_20),
                    false);
                break;
            }
        }
    }
        break;
    case CMD_CITYCACHING_TICK:
        if (!base_cache.IsOpen())
            break;
        base_cache.Tick(CITYCACHING_TICK_TIME);
        act_mission->FinishTimeTransparentObjects();
        break;
    case CMD_ACTORLIGHTNESSON:
        act_mission->GetGame()->SetActorLightness(true);
        break;
    case CMD_CAR_LOCK:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
            break;
        C_car *car = (C_car *)actor_regs[p[0]];
        car->Lock(p[1] != 0);
        if (p[1] && actor->GetType() == ACTOR_ENTITY)
            ((C_entity *)actor)->locked_car = car;
    }
        break;
    case CMD_CAR_LOCK_ALL:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
            break;
        C_car *car = (C_car *)actor_regs[p[0]];
        car->LockVehicle(p[1] != 0);
        if (p[1] && actor->GetType() == ACTOR_ENTITY)
            ((C_entity *)actor)->locked_vehicle = car;
    }
        break;
    case CMD_SETFILMMUSIC:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_float_regs <= p[0] || p[0] < 0)
            break;
        act_mission->GetGame()->SetFilmMusic((int)float_regs[p[0]]);
    }
        break;
    case CMD_GETFILMMUSIC:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_float_regs <= p[0] || p[0] < 0)
            break;
        float_regs[p[0]] = (float)act_mission->GetGame()->GetFilmMusic(p[1]);
    }
        break;
    case CMD_HUMAN_GETSEATIDX:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || num_float_regs <= p[1] || p[1] < 0)
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
            break;
        C_human *human = (C_human *)actor_regs[p[0]];
        if (human->GetCar())
        {
            if (human->GetCar()->GetType() == ACTOR_CAR)
                float_regs[p[1]] = (float)human->seat;
            else
                float_regs[p[1]] = -1.0f;
        }
        else
            float_regs[p[1]] = -1.0f;
    }
        break;
    case CMD_PM_SHOWSYMBOL:
        if (act_mission->GetGame()->GetShvestky()->GetUnknown0())
            break;
        switch (*(int *)instructions[cur_command].params)
        {
        case 1:
            mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
            mafiaIndicators.SetSymbol(1, 1.0f);
            break;
        case 2:
            mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
            mafiaIndicators.SetSymbol(2, 1.0f);
            break;
        case 3:
            mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
            mafiaIndicators.SetSymbol(3, 1.0f);
            break;
        case 4:
            mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
            mafiaIndicators.SetSymbol(4, 1.0f);
            break;
        case 5:
            mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
            mafiaIndicators.SetSymbol(1, .4f);
            break;
        case 6:
            mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
            mafiaIndicators.SetSymbol(2, .4f);
            break;
        case 7:
            mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
            mafiaIndicators.SetSymbol(3, .4f);
            break;
        case 8:
            mafiaIndicators.AddFlags(INDF_UNKNOWN_4000000);
            mafiaIndicators.SetSymbol(4, .4f);
            break;
        default:
            mafiaIndicators.RemoveFlags(INDF_UNKNOWN_4000000);
            break;
        }
        break;
    case CMD_PM_SHOWPROGRESS:
        if (act_mission->GetGame()->GetShvestky()->GetUnknown0())
            break;
        if (*(int *)instructions[cur_command].params)
            mafiaIndicators.AddFlags(INDF_UNKNOWN_10);
        else
            mafiaIndicators.RemoveFlags(INDF_UNKNOWN_10);
        break;
    case CMD_PM_SETPROGRESS:
    {
        if (act_mission->GetGame()->GetShvestky()->GetUnknown0())
            break;
        int *p = (int *)instructions[cur_command].params;
        if (num_float_regs <= p[0] || p[0] < 0)
            break;
        float progress = float_regs[p[0]];
        if (progress < 0.0f)
            progress = 0.0f;
        else if (progress > 1.0f)
            progress = 1.0f;
        mafiaIndicators.SetUnknown46e8(progress);
    }
        break;
    case CMD_HUMAN_ENTERTOTRUCK:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]] || p[1] < 0 || p[1] >= num_actor_regs ||
            !actor_regs[p[1]])
            break;
        if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
            break;
        if (actor_regs[p[1]]->GetType() != ACTOR_CAR || actor_regs[p[1]]->GetUnknown14() != ACTORSUB_CAR_EX)
            break;
        C_human *human = (C_human *)actor_regs[p[0]];
        C_car_ex *car = (C_car_ex *)actor_regs[p[1]];
        if (car->CanUseTruck())
            human->_intern_UseCarTruck(car, p[2] != 0);
    }
        break;
    case CMD_FLOATREG_PUSH:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_float_regs)
            break;
        act_mission->GetGame()->FloatStackPush(float_regs[p[0]]);
    }
        break;
    case CMD_FLOATREG_POP:
    {
        int *p = (int *)instructions[cur_command].params;
        if (p[0] < 0 || p[0] >= num_float_regs)
            break;
        act_mission->GetGame()->FloatStackPop(&float_regs[p[0]]);
    }
        break;
    case CMD_AIRPLANESHOWDAMAGE:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] ||
            actor_regs[p[0]]->GetType() != ACTOR_AIRPLANE)
            break;
        ((C_airplane *)actor_regs[p[0]])->ShowDamageIndicator(p[1] != 0);
    }
        break;
    case CMD_SETNPCKILLEVENT:
    {
        S_event_params *e = (S_event_params *)instructions[cur_command].params;
        if (num_actor_regs <= e->value1 || e->value1 < 0 || !actor_regs[e->value1])
            break;
        C_program *prog;
        if (actor_regs[e->value1]->GetType() == ACTOR_ENTITY)
            prog = (C_entity *)actor_regs[e->value1];
        else if (actor_regs[e->value1]->GetType() == ACTOR_DETECTOR)
            prog = (C_detector *)actor_regs[e->value1];
        else
            break;
        act_mission->GetGame()->AddNPCKillEvent(prog, e->value2, e->name);
    }
        break;
    case CMD_STREAM_SETPOS:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_float_regs <= p[0] || p[0] < 0 || num_float_regs <= p[1] || p[1] < 0)
            break;
        IShow *show = act_mission->GetGame()->GetIShow((int)float_regs[p[0]]);
        if (show)
            show->SetCurrentPos((int)float_regs[p[1]]);
    }
        break;
    case CMD_STREAM_GETPOS:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_float_regs <= p[0] || p[0] < 0 || num_float_regs <= p[1] || p[1] < 0)
            break;
        IShow *show = act_mission->GetGame()->GetIShow((int)float_regs[p[0]]);
        if (show)
        {
            uint pos;
            show->GetCurrentPos(&pos);
            float_regs[p[1]] = (float)pos;
        }
    }
        break;
    case CMD_CAR_GETSEATCOUNT:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || num_float_regs <= p[1] || p[1] < 0 ||
            actor_regs[p[0]]->GetType() != ACTOR_CAR)
            break;
        float_regs[p[1]] = (float)((C_car *)actor_regs[p[0]])->GetNumUnknownCa0();
    }
        break;
    case CMD_FRM_SETALPHA:
    {
        int *p = (int *)instructions[cur_command].params;
        if (num_frame_regs <= p[0] || p[0] < 0 || !frame_regs[p[0]] || num_float_regs <= p[1] || p[1] < 0 ||
            frame_regs[p[0]]->GetType() != FRAME_VISUAL)
            break;
        I3D_object *obj = (I3D_object *)frame_regs[p[0]];
        switch (obj->GetVisualType())
        {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_BILLBOARD:
        case I3D_VISUAL_SINGLEMESH:
        case I3D_VISUAL_LENSF:
        case I3D_VISUAL_MORPH:
        case I3D_VISUAL_SINGLEMORPH:
        {
            float alpha = float_regs[p[1]];
            if (alpha < 0.0f)
                alpha = 0.0f;
            else if (alpha > 1.0f)
                alpha = 1.0f;
            obj->SetTransparency(alpha);
        }
            break;
        }
    }
        break;
    case CMD_HUMAN_THROWGRENADE:
    {
        S_grenade_params *g = (S_grenade_params *)instructions[cur_command].params;
        if (num_actor_regs <= g->reg || g->reg < 0 || !actor_regs[g->reg])
            break;
        if (actor_regs[g->reg]->GetType() != ACTOR_PLAYER && actor_regs[g->reg]->GetType() != ACTOR_ENTITY)
            break;
        S_vector dir = actor_regs[g->reg]->GetFrame()->GetWorldDir();
        dir.y = 0.0f;
        dir.Normalize();
        S_vector axis(dir.z, 0.0f, -dir.x);
        dir *= g->value2;
        S_quat q;
        // 45, not -45, below -45, as the original has it
        float angle = g->value1;
        if (angle < -45.0f)
            angle = 45.0f;
        else if (angle > 80.0f)
            angle = 80.0f;
        q.Make(axis, angle * PI / 180.0f);
        dir *= q.RotationMatrix();
        ((C_human *)actor_regs[g->reg])->Do_ThrowGrenade(dir);
    }
        break;
    case CMD_CAR_UNBREAKABLE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
                break;
            ((C_car *)actor_regs[p[0]])->SetUnbreakable(p[1] ? CARF_UNBREAKABLE : 0);
        }
        break;
    case CMD_PHOBJ_IMPULS:
        {
            // The impulse's vectors are three float registers each: the second parameter's is the second vector
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]] ||
                actor_regs[p[0]]->GetType() != ACTOR_BOX_NEW)
                break;
            if (p[1] < 0 || p[1] + 2 >= num_float_regs || p[2] < 0 || p[2] + 2 >= num_float_regs)
                break;
            ((C_PhThing *)actor_regs[p[0]])->ExternalImpuls(
                S_vector(float_regs[p[2]], float_regs[p[2] + 1], float_regs[p[2] + 2]),
                S_vector(float_regs[p[1]], float_regs[p[1] + 1], float_regs[p[1] + 2]));
        }
        break;
    case CMD_WEATHER_PREPAREBUFFER:
        act_mission->GetGame()->SetUnknown3638(*(int *)instructions[cur_command].params);
        break;
    case CMD_SUBTITLE:
        mafiaLoadSave.ExecuteCredits();
        break;
    case CMD_VERSION_IS_GERMANY: case CMD_VERSION_IS_EDITOR:
        {
            // Neither version: 0
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_float_regs)
                break;
            float_regs[p[0]] = 0.0f;
        }
        break;
    case CMD_GARAGE_NEZAHAZUJ:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_actor_regs || !actor_regs[i] || actor_regs[i]->GetType() != ACTOR_CAR)
                break;
            mafiaLoadSave.CarCantGetOut((C_car *)actor_regs[i]);
        }
        break;
    case CMD_DAN_INTERNAL_1:
        {
            // A Thunderbird at a place of the city for the developers, put into the game as a temporary actor
            I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            if (!model)
                break;
            // STRING: GAME 0x634398
            model->SetName("cheat_car");
            // STRING: GAME 0x634384
            if (model_cache.Open(model, "thunderbird00.i3d", 0, NULL, NULL, NULL) == I3D_OK)
            {
                C_car *car = new C_car;
                if (car)
                {
                    model->SetPos(-841.0f, -6.5f, -637.0f);
                    model->SetDir(S_vector(1.0f, 0.0f, 0.0f), 0.0f);
                    model->Update();
                    act_mission->GetScene()->SetFrameSector(model);
                    if (car->Init(model))
                    {
                        car->GameInit();
                        car->SetUnknown220c(true);
                        act_mission->GetGame()->InternTempActAdd(car);
                        car->CalmVehicle(100);
                        car->SetUnknown65(true);
                    }
                }
            }
            model->Release();
        }
        break;
    case CMD_POLICE_SUPPORT:
        {
            // The register's test lets an index out of range through to the actor's test, as PS2's
            S_float_int_params *s = (S_float_int_params *)instructions[cur_command].params;
            if ((s->value1 < 0 || num_actor_regs <= s->value1) && !actor_regs[s->value1])
                break;
            if (actor_regs[s->value1]->GetType() != ACTOR_PLAYER && actor_regs[s->value1]->GetType() != ACTOR_ENTITY)
                break;
            act_mission->GetGame()->GetShvestky()->DalsiChalpiDoPartie((C_human *)actor_regs[s->value1],
                s->value2 ? 0 : 4, s->value);
        }
        break;
    case CMD_PLAY_AVI_INTRO:
        PlayLogo("intro", 800.0f, 440.0f);
        break;
    case CMD_CAMERA_GETFOV:
        {
            // In degrees
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_float_regs)
                break;
            float_regs[i] = act_mission->GetScene()->GetActiveCamera()->GetFOV() * 180.0f / PI;
        }
        break;
    case CMD_CAMERA_SETFOV:
        {
            // In degrees; only the register's upper bound is tested
            S_operand *o = (S_operand *)instructions[cur_command].params;
            int reg = o->reg;
            if (reg == -1)
                mafiaIndicators.SetCameraFOV(act_mission->GetScene()->GetActiveCamera(), o->value * PI / 180.0f);
            else if (num_float_regs > reg)
                mafiaIndicators.SetCameraFOV(act_mission->GetScene()->GetActiveCamera(), float_regs[reg] * PI / 180.0f);
        }
        break;
    case CMD_DOOR_GETSTATE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_DOOR)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)((C_door *)actor_regs[p[0]])->GetState();
        }
        break;
    case CMD_CAR_GETMAXLEVELS:
        {
            // The actor has to be a door, as PS2's (the original's mistake, the car's command after the door's)
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_DOOR)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)((C_car *)actor_regs[p[0]])->GetLevelsCount();
        }
        break;
    case CMD_CAR_GETACTLEVEL:
        {
            // A door again, as PS2's
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_DOOR)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)((C_car *)actor_regs[p[0]])->GetActualLevel();
        }
        break;
    case CMD_CAR_SETACTLEVEL:
        {
            S_reg_operand_params *s = (S_reg_operand_params *)instructions[cur_command].params;
            if (s->reg < 0 || s->reg >= num_actor_regs || !actor_regs[s->reg] ||
                actor_regs[s->reg]->GetType() != ACTOR_CAR)
                break;
            C_car *car = (C_car *)actor_regs[s->reg];
            int level = 0;
            if (s->value_reg == -1)
                level = (int)s->value;
            else if (s->value_reg >= 0 && s->value_reg < num_float_regs)
                level = (int)float_regs[s->value_reg];
            car->SetActualLevel(level);
        }
        break;
    case CMD_FREERIDE_SCOREON:
        act_mission->GetGame()->ScoreSetOn(*(int *)instructions[cur_command].params != 0);
        break;
    case CMD_FREERIDE_SCOREGET:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_float_regs)
                break;
            float_regs[i] = (float)act_mission->GetGame()->GetScore();
        }
        break;
    case CMD_FREERIDE_SCORESET:
        {
            // Only the register's upper bound is tested; one out of range sets 0
            S_operand *o = (S_operand *)instructions[cur_command].params;
            float score = 0.0f;
            if (o->reg == -1)
                score = o->value;
            else if (num_float_regs > o->reg)
                score = float_regs[o->reg];
            act_mission->GetGame()->ScoreSet((int)(score + .5f));
        }
        break;
    case CMD_FREERIDE_SCOREADD:
        {
            S_operand *o = (S_operand *)instructions[cur_command].params;
            float score = 0.0f;
            if (o->reg == -1)
                score = o->value;
            else if (num_float_regs > o->reg)
                score = float_regs[o->reg];
            act_mission->GetGame()->ScoreSet(act_mission->GetGame()->GetScore() + (int)(score + .5f));
        }
        break;
    case CMD_TAXIDRIVER_ENABLE:
        C_traffic_generator::EnableTaxiDriver(*(int *)instructions[cur_command].params != 0);
        break;
    case CMD_HUMAN_RETURNTOTRAFF:
        {
            // A copy of the entity's model goes back to the pedestrians, the entity out of the game
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_actor_regs || !actor_regs[i] || actor_regs[i]->GetType() != ACTOR_ENTITY)
                break;
            I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            if (model)
            {
                model->Duplicate(actor_regs[i]->GetFrame());
                model->SetPos(actor_regs[i]->GetFrame()->GetPos());
                model->SetRot(actor_regs[i]->GetFrame()->GetRot());
                model->Update();
                I3D_frame *parent = actor_regs[i]->GetFrame()->GetParent();
                model->LinkTo(parent, 0);
                C_traffic_generator::AddFreePerson(model, NULL, false);
            }
            act_mission->GetGame()->GetShvestky()->DelDoRiciActor(actor_regs[i]);
            act_mission->GetGame()->RemoveTemporaryActor(actor_regs[i]);
        }
        break;
    case CMD_TIMER_GETINTERVAL:
        {
            // 0 without the clock
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_float_regs)
                break;
            if (mafiaIndicators.IsShown(INDF_TIMER))
            {
                uint interval;
                mafiaIndicators.TimerGetInterval(&interval);
                float_regs[i] = (float)interval;
            }
            else
                float_regs[i] = 0.0f;
        }
        break;
    case CMD_TIMER_SETINTERVAL:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_float_regs)
                break;
            if (!mafiaIndicators.IsShown(INDF_TIMER))
                break;
            mafiaIndicators.TimerSetInterval((int)float_regs[i]);
        }
        break;
    case CMD_HUMAN_WAITTOREADY:
        {
            // Stays on the command until the human is ready
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_actor_regs || !actor_regs[i])
                break;
            if (actor_regs[i]->GetType() != ACTOR_PLAYER && actor_regs[i]->GetType() != ACTOR_ENTITY)
                break;
            if (!((C_human *)actor_regs[i])->IsReady())
                return;
        }
        break;
    case CMD_FRM_GETNUMCHILDREN:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_frame_regs <= p[0] || p[0] < 0 || !frame_regs[p[0]])
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)frame_regs[p[0]]->NumChildren();
        }
        break;
    case CMD_FRM_GETCHILD:
        {
            // The register holds NULL unless the child exists
            S_reg_operand_params *s = (S_reg_operand_params *)instructions[cur_command].params;
            if (s->reg < 0 || s->reg >= num_frame_regs)
                break;
            I3D_frame *frm = frame_regs[s->reg];
            if (!frm)
                break;
            frame_regs[s->reg] = NULL;
            int i = -1;
            if (s->value_reg == -1)
                i = (int)s->value;
            else if (s->value_reg >= 0 && s->value_reg < num_float_regs)
                i = (int)float_regs[s->value_reg];
            if (i < 0 || i >= frm->NumChildren())
                break;
            frame_regs[s->reg] = frm->GetChild(i);
        }
        break;
    case CMD_ACTOR_ADDDORICI:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]])
                break;
            act_mission->GetGame()->GetShvestky()->AddDoRiciActor(actor_regs[p[0]]);
        }
        break;
    case CMD_CAR_REPAIR:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_actor_regs || !actor_regs[i] || actor_regs[i]->GetType() != ACTOR_CAR)
                break;
            ((C_car *)actor_regs[i])->_RepairPosition(true);
        }
        break;
    case CMD_POLICE_SPEED_FACTOR:
        {
            // 0 to 1; only the register's upper bound is tested
            S_operand *o = (S_operand *)instructions[cur_command].params;
            float f = 0.0f;
            if (o->reg == -1)
                f = o->value;
            else if (num_float_regs > o->reg)
                f = float_regs[o->reg];
            if (f < 0.0f)
                f = 0.0f;
            if (f > 1.0f)
                f = 1.0f;
            AIManager.police.speed_factor = f;
        }
        break;
    case CMD_SUBTITLE_ADD:
        mafiaIndicators.IntroSubtitlesAddSimple(*(int *)instructions[cur_command].params);
        break;
    case CMD_CARTRIDGE_INVALIDATE:
        act_mission->GetGame()->ClearCartridges();
        break;
    case CMD_NPC_SHUTUP:
        C_traffic_generator::SetUnknown63d728(*(int *)instructions[cur_command].params == 0);
        break;
    case CMD_FREERIDE_ENABLECAR:
        mafiaLoadSave.FreerideEnableCar(*(char *)instructions[cur_command].params);
        break;
    case CMD_GETCARLINENUMFROMTABLE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)mafiaCarDatabase.GetIdxByModel(((C_car *)actor_regs[p[0]])->GetModelName());
        }
        break;
    case CMD_ENEMY_GETHOSTILESTATE:
        {
            // 1 while the entity's base job is the hostile one
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]] ||
                actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = 0.0f;
            ai_action *job = ((C_entity *)actor_regs[p[0]])->action_manager.GetBaseJob();
            if (job && job->type == AIACTION_UNKNOWN_1E && job->unknown_20->unknown_38 == AIACTION20_UNKNOWN_A)
                float_regs[p[1]] = 1.0f;
        }
        break;
    case CMD_HUMAN_SHUTUP:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_actor_regs <= p[0] || p[0] < 0 || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_ENTITY && actor_regs[p[0]]->GetType() != ACTOR_PLAYER)
                break;
            ((C_human *)actor_regs[p[0]])->SetShutUp(p[1] != 0);
        }
        break;
    case CMD_GROUP_DISBAND:
        {
            // The group of the float register's number goes apart in steps, the second parameter the step (0 from
            // DecodeParams): 0 stops the members' fights' cars, keeping the last one in the third parameter, 1 marks
            // the members without a car or not unknown_5d until none is left, 2 frees the members, that car and the
            // group's unknown_40. The first two steps stay on the command. The register's test is unsigned, as PS2's.
            int *p = (int *)instructions[cur_command].params;
            if ((uint)p[0] >= (uint)num_float_regs)
                break;
            ai_group *g = AIManager.groups.GetGroup((uint)float_regs[p[0]]);
            if (!g)
                break;
            switch (p[1])
            {
            case 0:
                {
                    for (int i = g->members.size() - 1; i >= 0; i--)
                    {
                        // A local: the element's operator[] comes before the constants' pushes
                        C_entity *e = g->members[i].entity;
                        uint unknown;
                        ai_fight_followship *party = AIManager.fight.GetParty(e, NULL, &unknown);
                        if (party && party->unknown_30 == 1)
                        {
                            p[2] = (int)party->car;
                            act_mission->GetGame()->GetSledovacka()->SetActionStatus(party->car,
                                C_sledovacka::STATUS_UNKNOWN_2);
                        }
                    }
                    p[1] = 1;
                }
                return;
            case 1:
                {
                    int count = g->members.size();
                    for (int i = g->members.size() - 1; i >= 0; i--)
                    {
                        C_entity *e = g->members[i].entity;
                        if (!e->GetCar() || !e->GetUnknown5d())
                        {
                            e->SetUnknownF50Bit2(true);
                            count--;
                        }
                    }
                    if (!count)
                        p[1] = 2;
                }
                return;
            case 2:
                {
                    int i;
                    for (i = g->members.size() - 1; i >= 0; i--)
                    {
                        C_entity *e = g->members[i].entity;
                        e->GetLogs()->Clear();
                        e->GetSensors()->Init(e);
                        e->GetVisLogs()->Init(e);
                        e->action_manager.Clear(false);
                        e->SetUnknownF50Bit2(false);
                        AIManager.fight.LeaveParty(e, NULL);
                        AIManager.groups.AddFreeActor(e);
                    }
                    if (p[2])
                        AIManager.groups.AddFreeActor((C_actor *)p[2]);
                    for (i = g->unknown_40.size() - 1; i >= 0; i--)
                        AIManager.groups.AddFreeActor((C_actor *)g->unknown_40[i]);
                }
                break;
            }
        }
        break;
    case CMD_PUMPER_CANWORK:
        {
            int *p = (int *)instructions[cur_command].params;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)C_pumper::ScriptCanPump(p[0]);
        }
        break;
    case CMD_CAR_DISABLE_UO:
        {
            // Protects a seat of the car against getting in (or lifts it)
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
                break;
            C_car *car = (C_car *)actor_regs[p[0]];
            int seat = p[1];
            if (car->GetNumUnknownCa0() <= seat || seat < 0)
                break;
            car->SetEnterProtect1(seat, p[2] != 0);
        }
        break;
    case CMD_INVENTORY_CLEAR:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_actor_regs || !actor_regs[i])
                break;
            if (actor_regs[i]->GetType() != ACTOR_PLAYER && actor_regs[i]->GetType() != ACTOR_ENTITY)
                break;
            ((C_human *)actor_regs[i])->inventory.Clear();
            ((C_human *)actor_regs[i])->ChangeWeaponModel();
        }
        break;
    case CMD_GETLASTSAVENUM:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_float_regs)
                break;
            float_regs[i] = (float)mafiaLoadSave.SaveGameGetSelectedNumber();
        }
        break;
    case CMD_FREERIDE_TRAFFSETUP:
        mafiaLoadSave.FreerideSetupFromScript();
        break;
    case CMD_POCZKURVENYCHBEDENCAR:
        {
            // The boxes put on the car (PS2 C_game::BoxPlacementPoPiciNaZkurvenemAute, Czech and rude for that)
            int *p = (int *)instructions[cur_command].params;
            if (p[1] < 0 || p[1] >= num_float_regs)
                break;
            if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]] || actor_regs[p[0]]->GetType() != ACTOR_CAR)
                break;
            float_regs[p[1]] = (float)act_mission->GetGame()->BoxPlacementPoPiciNaZkurvenemAute(
                (C_car *)actor_regs[p[0]], p[2]);
        }
        break;
    case CMD_VECT_ANGLETO_2D:
        {
            // The angle between the x and z of two vectors of three float registers; the result's register is tested
            // against 0 only, the second vector's again in its place (as PS2's)
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] + 2 >= num_float_regs || p[1] < 0 || p[1] + 2 >= num_float_regs || p[2] < 0 ||
                p[1] >= num_float_regs)
                break;
            S_vector2 a(float_regs[p[0]], float_regs[p[0] + 2]);
            S_vector2 b(float_regs[p[1]], float_regs[p[1] + 2]);
            float_regs[p[2]] = AngleTo2(a, b);
        }
        break;
    case CMD_HUMAN_GETACTANIMID:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)((C_human *)actor_regs[p[0]])->anim_id;
        }
        break;
    case CMD_CAR_SETAXIS:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_actor_regs || !actor_regs[i] || actor_regs[i]->GetType() != ACTOR_CAR)
                break;
            ((C_car *)actor_regs[i])->SetAxis(1);
        }
        break;
    case CMD_FRM_ISON:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_frame_regs || !frame_regs[p[0]])
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = frame_regs[p[0]]->IsOn() ? 1.0f : 0.0f;
        }
        break;
    case CMD_HUMAN_GETITEMINRHAND:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)((C_human *)actor_regs[p[0]])->inventory.GetItemInRightHand(NULL);
        }
        break;
    case CMD_GET_PM_HUMANSTATE:
        {
            int *p = (int *)instructions[cur_command].params;
            if (p[0] < 0 || p[0] >= num_actor_regs || !actor_regs[p[0]])
                break;
            if (actor_regs[p[0]]->GetType() != ACTOR_PLAYER && actor_regs[p[0]]->GetType() != ACTOR_ENTITY)
                break;
            if (num_float_regs <= p[1] || p[1] < 0)
                break;
            float_regs[p[1]] = (float)act_mission->GetGame()->GetShvestky()->JakPicusNaTomJest(
                (C_human *)actor_regs[p[0]]);
        }
        break;
    case CMD_M7_ZASTAVZKURVENEPOLISE:
        AIManager.police.ZastavMotorizovaneJednotky();
        break;
    case CMD_ISVALIDTAXIPASSENGER:
        {
            int i = *(int *)instructions[cur_command].params;
            if (i < 0 || i >= num_float_regs)
                break;
            float_regs[i] = act_mission->GetGame()->IsValidTaxiPassenger() ? 1.0f : 0.0f;
        }
        break;
    }
    NextCommand(NULL);
}

// FUNCTION: GAME 0x47bb40
inline void S_help::SwitchLightMap(I3D_frame *frm, int level)
{
    if (frm->GetType() == FRAME_VISUAL && ((I3D_visual *)frm)->GetVisualType() == I3D_VISUAL_LIT_OBJECT &&
        ((I3D_lit_object *)frm)->GetLevel(level).constructed)
        ((I3D_lit_object *)frm)->SetActiveLevel(level);
    for (I3D_frame *child = frm->m_pChild; child; child = child->m_pNext)
    {
        if (child->GetType() != FRAME_SECTOR)
            SwitchLightMap(child, level);
    }
}
