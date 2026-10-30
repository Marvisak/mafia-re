#pragma once

#include "actors/C_PhThings.h"

// The pieces a crash breaks off (PS2 C_crash_obj, C_crash_obj.cpp), a physical thing of type 0x1a, 0x3c0 bytes: its
// constructor (0x431b40, vtable 0x623918) is called by a helper of the file's (0x4311d0) that walks a model's frames.
// Only the count and DestroyOuterMostCrashObject so far, which C_game::Tick uses.
class C_crash_obj : public C_PhThing
{
public:
    virtual ~C_crash_obj();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    // The base's: the original's jumps there
    // The original's: 0x432090
    virtual void Update(uint time)
    {
        C_PhThing::Update(time);
    }
    // The base's: the original's jumps there, a copy the linker folded
    // The original's: 0x443860
    virtual void NetDirect(uint time)
    {
        C_PhThing::NetDirect(time);
    }
    virtual bool Init(I3D_frame *frm);
    // The base's: the original's jumps there, a copy the linker folded
    // The original's: 0x443860
    virtual void DuplicateFrom(C_actor *act)
    {
        C_PhThing::DuplicateFrom(act);
    }
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual int GetMaterialID(I3D_frame *frm);
    // Of the game's temporary actors, the crash object farthest from the camera gets flag 0x8000 at its 0x38c (PS2
    // name): C_game::Tick calls it while there are more than 20
    static void DestroyOuterMostCrashObject();

    uchar unknown_38c[0x34];            // 0x38c

    // How many there are in the game (PS2 name): GameInit (0x432460) counts it up, GameDone (0x432470) down, as PS2's,
    // and C_game::Init clears it
    static int count;
};

// SYNTHETIC: GAME 0x431bb0
// C_crash_obj::`scalar deleting destructor'
