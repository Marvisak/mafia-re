#pragma once

#include "actors/C_actor.h"

// A physical thing's box (PS2 S_THING_BOX, InitBox's; the members' names are ours), 0x14 bytes, PS2's in another
// order. Its constructor is the implicit one, which S_vector's empty constructor makes the original call (the folded
// 0x401c60).
struct S_THING_BOX
{
    float weight;                       // 0x0, the script's CREATE_PHYSICALOBJECT gives at least 1 (InitBox's 0x210)
    S_vector velocity;                  // 0x4, (0, -speed, 0) from the scripts (InitBox's 0x220)
    float unknown_10;                   // 0x10, .45 from the scripts (InitBox's 0x238)
};

// The physical things (PS2 C_PhThing, C_PhThings.cpp), the base of the public physics, of C_box_new and of
// C_drop_in_weapon, 0x38c bytes. Only the constructor so far, which the derived classes' constructors call, and the
// functions the script calls, as stubs.
// VTABLE: GAME 0x624088
class C_PhThing : public C_actor
{
public:
    C_PhThing();
    virtual ~C_PhThing();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual void ChangeState();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    // PS2 names, which the script calls; the parameters' names are ours
    // The thing's box, its state set by the velocity
    void InitBox(S_THING_BOX *box);
    // An impulse at the position
    void ExternalImpuls(const S_vector &impuls, const S_vector &pos);

    uchar unknown_70[0x2dc];            // 0x70
    int material;                       // 0x34c, an index of materialList, the shots' sound's (the name is ours)
    uchar unknown_350[0x3c];            // 0x350
};

// SYNTHETIC: GAME 0x459f70
// C_PhThing::`scalar deleting destructor'
