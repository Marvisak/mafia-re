// The objects scattered around a frame (PC only, the file's name is ours, after the class; the PC's file is the one
// at 0x58e350-0x58f15c, docs/game-files.md), which starts with the constructor. Not written yet: the constructor,
// which C_mission::CreateActor calls, is a stub.

#include "actors/C_scatter.h"

#include "collision/g_c_glob.h"

#include "stub.h"

// One of the scattered objects (0x74 bytes, the elements of the scatter's vector at 0x74; the names are ours, by what
// AI does with them): its collision, whose owner is the scatter (DYNCOLL_UNKNOWN_47), then how its frame leans. AI moves
// dir by the force and by its last move, sets the frame's direction by it, and makes the force the spring's pull back to
// upright; a line's hit adds a push to the force (LineHit).
struct S_scatter_object
{
    tDynamicCollObject coll;            // 0x0
    S_vector dir;                       // 0x48, the up direction the frame leans to, its y kept 1 (a guess)
    S_vector last_dir;                  // 0x54, the one before AI's last move (a guess)
    S_vector force;                     // 0x60
    float stiffness;                    // 0x6c, the spring's, random from the creation (a guess)
    I3D_frame *frame;                   // 0x70, the copy of the frame's object
};

// STUB: GAME 0x58e350
C_scatter::C_scatter()
{
    StubLog("C_scatter::C_scatter");
}

// STUB: GAME 0x58e3c0
C_scatter::~C_scatter()
{
    StubLog("C_scatter::~C_scatter");
}

// The linker's folded copy (0x4aa490)
bool C_scatter::SaveGameSave(void **buf)
{
    return true;
}

// The linker's folded copy (0x4aa490)
bool C_scatter::SaveGameLoad(void **buf)
{
    return true;
}

// STUB: GAME 0x58e3f0
bool C_scatter::Init(I3D_frame *frm)
{
    StubLog("C_scatter::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x58e420
bool C_scatter::SaveData(C_chunk *chunk)
{
    StubLog("C_scatter::SaveData");
    return C_actor::SaveData(chunk);
}

// STUB: GAME 0x58e470
bool C_scatter::LoadData(C_chunk *chunk)
{
    StubLog("C_scatter::LoadData");
    return C_actor::LoadData(chunk);
}

// STUB: GAME 0x58e550
void C_scatter::DuplicateFrom(C_actor *act)
{
    StubLog("C_scatter::DuplicateFrom");
    C_actor::DuplicateFrom(act);
}

// STUB: GAME 0x58e590
void C_scatter::GameInit()
{
    StubLog("C_scatter::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x58e9d0
void C_scatter::GameDone()
{
    StubLog("C_scatter::GameDone");
    C_actor::GameDone();
}

// A line test hit one of the objects (LineInDynamicsInnerDyn's): the line's direction in x and z, times 100, pushes it,
// and the force's x and z are kept to 5 at most (both scaled down by the larger one)
// FUNCTION: GAME 0x58ea30
void C_scatter::LineHit(tDynamicCollObject *obj, tCLineParam &p)
{
    S_scatter_object *o = (S_scatter_object *)obj;
    o->force += S_vector(p.dir.x, 0.0f, p.dir.z) * 100.0f;
    float m = (float)fabs(o->force.x);
    if (m > 5.0f)
    {
        float f = 5.0f / m;
        o->force.x *= f;
        o->force.z *= f;
    }
    m = (float)fabs(o->force.z);
    if (m > 5.0f)
    {
        float f = 5.0f / m;
        o->force.x *= f;
        o->force.z *= f;
    }
}

// STUB: GAME 0x58ead0
void C_scatter::AI(uint time)
{
    StubLog("C_scatter::AI");
    C_actor::AI(time);
}
