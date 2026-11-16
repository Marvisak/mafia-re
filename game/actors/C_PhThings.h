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

// A physical thing's wheel (PS2 S_THING_WHEEL, InitWheel's; the members' names are ours), 0x20 bytes, PS2's with the
// velocity first. C_box_new's GameInit fills it from its scene data.
struct S_THING_WHEEL
{
    float radius;                       // 0x0, InitWheel's 0x260, whose square times the weight it divides by
    float unknown_4;                    // 0x4, InitWheel's 0x25c
    float weight;                       // 0x8, InitWheel's 0x210
    S_vector velocity;                  // 0xc, InitWheel's 0x220
    float unknown_18;                   // 0x18, InitWheel's 0x238, as the box's unknown_10
    float unknown_1c;                   // 0x1c, InitWheel's 0x264
};

// The physical things (PS2 C_PhThing, C_PhThings.cpp), the base of the public physics, of C_box_new and of
// C_drop_in_weapon, 0x38c bytes. Only the constructor so far, which the derived classes' constructors call, and the
// functions the script and the derived classes call, as stubs.
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
    // The same for a wheel (C_box_new's)
    void InitWheel(S_THING_WHEEL *wheel);
    // An impulse at the position
    void ExternalImpuls(const S_vector &impuls, const S_vector &pos);
    // Forgets an actor (PS2 name), which C_game::InvalidateActor calls
    void InvalidateActor(C_actor *act);

    uchar unknown_70[0x50];             // 0x70
    uint unknown_c0;                    // 0xc0, 0x80000 for the models C_human::FreeHand drops
    uchar unknown_c4[0x184];            // 0xc4
    // 0x248, the weight's force, (0, -weight, 0) times gravity from InitBox and InitWheel; C_box_new's GameInit sets
    // (0, -1, 0) before them (the name is ours)
    S_vector gravity_force;
    float gravity;                      // 0x254, 10 from the constructor (the name is ours)
    uchar unknown_258[0xf0];            // 0x258
    // 0x348, the sound of the thing hitting a surface: an index of its material's sounds from S_material_def's 0x130
    // (grenade_sound the first), -1 for none; the items C_human drops by their kind, 8 for a clip (the name is ours)
    int hit_sound;
    int material;                       // 0x34c, an index of materialList, the shots' sound's (the name is ours)
    uchar unknown_350[0x28];            // 0x350
    // 0x378, true from the constructor: a hit cuts its sound short to play it again; false lets it finish (the name is
    // ours)
    bool hit_sound_restart;
    uchar unknown_379[0x13];            // 0x379
};

// SYNTHETIC: GAME 0x459f70
// C_PhThing::`scalar deleting destructor'
