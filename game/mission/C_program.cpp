// The scripts (PS2 C_program.cpp; the PC's file is most likely the one at 0x460a50-0x47c31c, whose strings are the
// script language's, docs/game-files.md, and which starts with the changes' initializer). Written so far: the
// constructor, the destructor and LoadSourceCode, which C_mission::OpenBin and Close call, the patch's changes,
// LoadPatchFile, the helper it reads their strings through, and UnloadPatchFiles, TryChangeProgramData with
// EqualFinalPath, and SetSourceCode, which LoadSourceCode calls. Not written: the rest of the scripts.
// Open: the changes' destructor (0x460a90, registered by 0x460a80) tests and sets a guard byte (0x63d774) of its own,
// as C_animations_machine_de_luxe::tracks' does.

#include "mission/C_program.h"

#include <string.h>

#include "C_chunk.h"
#include "actors/C_actor.h"
#include "mission/Mission.h"

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
    unknown_60(.5f)
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
