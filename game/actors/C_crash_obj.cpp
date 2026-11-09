// The pieces crashes break off (PS2 C_crash_obj.cpp; the PC's file is the one at 0x430ba0-0x43286c, after
// C_cartridge.cpp as the names sort, docs/game-files.md): the helpers that make them first, then the constructor
// (0x431b40) and PS2's functions in PS2's order, GameInit (0x432460), GameDone (0x432470), SetModelFileName (0x4326d0)
// and DestroyOuterMostCrashObject among them. Written so far: the count; DestroyOuterMostCrashObject, which
// C_game::Tick calls, is a stub.

#include "actors/C_crash_obj.h"

#include "stub.h"

// GLOBAL: GAME 0x63d6d4
int C_crash_obj::count;

// STUB: GAME 0x430c50
bool CreateCrashActors(I3D_frame *frm, S_vector &pos, S_vector &dir, float power, char *name, bool unknown1,
    uchar unknown2, int unknown3, bool unknown4, bool unknown5)
{
    StubLog("CreateCrashActors");
    return false;
}

// STUB: GAME 0x431710
void CreateCrashModel(I3D_frame *frm, S_vector &pos, S_vector &dir, float power)
{
    StubLog("CreateCrashModel");
}

// STUB: GAME 0x432740
void C_crash_obj::DestroyOuterMostCrashObject()
{
    StubLog("C_crash_obj::DestroyOuterMostCrashObject");
}

// Declared, empty: it stores the vtable again before the base's
// The original's: 0x431bd0
C_crash_obj::~C_crash_obj()
{
}

// The original's: 0x431be0
void C_crash_obj::AI(uint time)
{
    StubLog("C_crash_obj::AI");
    C_PhThing::AI(time);
}

// The original's: 0x4320a0
int C_crash_obj::SaveGameGetSize()
{
    StubLog("C_crash_obj::SaveGameGetSize");
    return C_PhThing::SaveGameGetSize();
}

// The original's: 0x4320b0
bool C_crash_obj::SaveGameSave(void **buf)
{
    StubLog("C_crash_obj::SaveGameSave");
    return C_PhThing::SaveGameSave(buf);
}

// The original's: 0x4320d0
bool C_crash_obj::SaveGameLoad(void **buf)
{
    StubLog("C_crash_obj::SaveGameLoad");
    return C_PhThing::SaveGameLoad(buf);
}

// The original's: 0x4320f0
bool C_crash_obj::Init(I3D_frame *frm)
{
    StubLog("C_crash_obj::Init");
    return C_PhThing::Init(frm);
}

// The original's: 0x432460
void C_crash_obj::GameInit()
{
    StubLog("C_crash_obj::GameInit");
    C_PhThing::GameInit();
}

// The original's: 0x432470
void C_crash_obj::GameDone()
{
    StubLog("C_crash_obj::GameDone");
    C_PhThing::GameDone();
}

// The original's: 0x4324c0
bool C_crash_obj::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_crash_obj::Collision");
    return C_PhThing::Collision(act, v1, v2, v3, f, frm);
}

// The original's: 0x4325f0
bool C_crash_obj::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_crash_obj::Hit");
    return C_PhThing::Hit(type, v1, v2, v3, f, act, u, frm);
}

// The original's: 0x432870
int C_crash_obj::GetMaterialID(I3D_frame *frm)
{
    StubLog("C_crash_obj::GetMaterialID");
    return C_PhThing::GetMaterialID(frm);
}
