// The scripts (PS2 C_program.cpp; the PC's file is most likely the one at 0x460a50-0x47c31c, whose strings are the
// script language's, docs/game-files.md, and which starts with the changes' initializer). Written so far: the
// constructor, the destructor and LoadSourceCode, which C_mission::OpenBin and Close call, the patch's changes,
// LoadPatchFile, the helper it reads their strings through, and UnloadPatchFiles, TryChangeProgramData with
// EqualFinalPath, and SetSourceCode, which LoadSourceCode calls; Init and Done, which the mission's and the game's Init
// and Done call, the compiler Init calls (Compile, DecodeInstruction with its table of the commands' names, and
// DecodeParams with its helpers, ExtrudeParams to DecodeIf, and its names' tables), CallSubroutine and CallProcess,
// DecompileCommand. Not written: DuplicateProgramFrom (0x4617d0, between Done and ExtrudeParams), which the detector's
// and the AI humans' Duplicate call. Stubs: SaveSourceCode, the save functions and Process, the interpreter (a switch
// of 396 commands, about 50 KB of code, 0x46d3b0-0x4799b9). The menu mission's script (dim_flt, stream_create,
// stream_play, stream_setloop: the menu's music) now compiles and needs Process.
// Open: the changes' destructor (0x460a90, registered by 0x460a80) tests and sets a guard byte (0x63d774) of its own,
// as C_animations_machine_de_luxe::tracks' does. Compile: the original stores the line's end pointer to its stack home
// at the top of the scan loop (ours after it), and its slot and the error count's are swapped. DecodeInstruction: VC6
// schedules the table's stores differently (the stores themselves are the original's).

#include "mission/C_program.h"

#include <stdlib.h>
#include <string.h>

#include "C_chunk.h"
#include "actors/C_actor.h"
#include "mission/Mission.h"
#include "stub.h"
#include "ui/g_loadsave.h"

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
    unknown_float_60(.5f)
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
    unknown_float_60 = .5f;
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
        unknown_float_60 = 0;
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
            a->unknown_40 = -1;
            if (num > 2)
                a->unknown_40 = atoi(p[2]);
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

// FUNCTION: GAME 0x46cbc0
void C_program::CallSubroutine()
{
    cur_command = 0;
    gosub_stack.clear();
    actor = NULL;
    run_through = true;
    CallProcess(1);
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

// Stops CallProcess's loop
// STUB: GAME 0x46d3b0
void C_program::Process(uint time)
{
    StubLog("C_program::Process");
    run_through = false;
}

#pragma auto_inline(on)
