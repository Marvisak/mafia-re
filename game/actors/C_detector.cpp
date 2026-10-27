// The script objects (PS2 C_detector.cpp; the PC's file is the one at 0x432870-0x433fdc, docs/game-files.md), whose
// constructor (0x432880) comes first. Written so far: the constructor and Init, which C_mission::CreateActor and
// OpenBin call, the destructor, LoadData, which OpenBin calls next, and GameInit and GameDone, which C_game::Init and
// Done call, with EraseColls (0x433ae0). Not written: the rest; the vtable's DuplicateFrom (0x4329c0) and its save
// functions are stubs.

#include "actors/C_detector.h"

#include "C_chunk.h"
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
    unknown_d8 = 0;
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
    unknown_dc = false;
    unknown_dd = false;
    unknown_de = false;
    unknown_df = false;
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
void C_detector::LoadData(C_chunk *chunk)
{
    uchar unknown;
    chunk->Read(&unknown, sizeof(unknown));
    chunk->Read(&unknown_d8, sizeof(unknown_d8));
    LoadSourceCode(chunk);
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
