#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3D.h"

class C_chunk;

// The actors' types (C_actor::type), which the mission's scene2.bin gives each actor and C_mission::CreateActor creates
// a class for (the names are ours, after the classes; PS2's are the same values but for the three PC only classes)
#define ACTOR_ACTOR 1
#define ACTOR_PLAYER 2
#define ACTOR_CAR 4
#define ACTOR_DETECTOR 5
#define ACTOR_DOOR 6
#define ACTOR_RAILWAY 8
#define ACTOR_DYN_OBJECT 9
#define ACTOR_BOTTLE 0xa
#define ACTOR_TRAFFIC_CAR 0xc
#define ACTOR_TRAFFIC_GENERATOR 0x12
#define ACTOR_BRIDGE 0x14
#define ACTOR_DOG 0x15
#define ACTOR_AIRPLANE 0x16
#define ACTOR_SEAGULL 0x17
#define ACTOR_TURNOUT 0x18
#define ACTOR_PUMPER 0x19
#define ACTOR_ENTITY 0x1b
#define ACTOR_RACECAM 0x1c
#define ACTOR_SCATTER 0x1d
#define ACTOR_WAGON 0x1e
#define ACTOR_IRENKA 0x1f
#define ACTOR_PUBLIC_PHYSICS 0x20
#define ACTOR_ACTOR_SHOT 0x21
#define ACTOR_CLOCK 0x22
#define ACTOR_BOX_NEW 0x23
#define ACTOR_CAR_EX 0x24
#define ACTOR_RAIL_GENERATOR 0x26

// The actors (PS2 C_actor), 0x70 bytes (PS2's 0x80). Only what the mission uses so far.
// The PC's vtable (0x623248) has 38 functions, PS2's 27: the PC's first 16 come before the destructor, where PS2 has
// AI, Update and NetDirect; from the destructor on the PC's are PS2's in PS2's order. The first 16 are placeholders.
// Not written yet: the destructor and the virtual functions declared here but Init and Release, which the actors'
// classes need, are stubs (C_actor.cpp).
// VTABLE: GAME 0x623248
class C_actor
{
public:
    C_actor();

    virtual void vfunc_0();
    virtual void vfunc_1();
    virtual void vfunc_2();
    virtual void vfunc_3();
    virtual void vfunc_4();
    virtual void vfunc_5();
    virtual void vfunc_6();
    virtual void vfunc_7();
    virtual void vfunc_8();
    virtual void vfunc_9();
    virtual void vfunc_10();
    virtual void vfunc_11();
    virtual void vfunc_12();
    virtual void vfunc_13();
    virtual void vfunc_14();
    virtual void vfunc_15();
    virtual ~C_actor();
    // Releases a reference, and deletes the actor with the last one (PS2 name); the references left
    // FUNCTION: GAME 0x4091d0
    virtual ushort Release()
    {
        if (--ref)
            return ref;
        delete this;
        return 0;
    }
    // Takes the actor's frame from the scene: false if it can't be the actor, where the frame has another one (PS2
    // name). It gives the actor the mission's next number and turns the frame's targets off.
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    // The actor's data of the mission's scene2.bin (PS2 name)
    virtual void LoadData(C_chunk *chunk);
    // PS2 names, the types PS2's; the save functions return true (mov al, 1)
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    // C_mission::FrmActBuff_ClearAll calls it before releasing a kept actor
    virtual void GameDone();

    // C_mission::OpenBin sets it for the actors of the mission's file (PC only, the name is ours)
    // FUNCTION: GAME 0x547220
    void SetUnknown66()
    {
        unknown_66 = true;
    }

    // Adds the actor to global_actors, unless it is there already (PS2 name)
    static void AddActorToGlobal(C_actor *act);

    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8, 0 from the constructor's body
    ushort ref;                         // 0xc, Release's references, 1 at first (the name is ours)
    int type;                           // 0x10, ACTOR_*, ACTOR_ACTOR from the constructor
    int unknown_14;                     // 0x14, 1 at first
    // 0x18, the mission's count of actors when Init took the actor's frame (C_mission::unknown_8; the name is ours)
    int number;
    int unknown_1c;                     // 0x1c, 0 from Init
    bool unknown_20;                    // 0x20
    uchar unknown_21[0x3f];             // 0x21
    int unknown_60;                     // 0x60
    uchar unknown_64[2];                // 0x64
    bool unknown_66;                    // 0x66
    I3D_frame *frame;                   // 0x68, Init's (the name is ours)
    // 0x6c, false from Init; the destructor releases the frame where it is set (the name is ours)
    bool own_frame;
    uchar unknown_6d[3];                // 0x6d

    // The actors that exist: the constructor counts up, the destructor down, and C_mission::Open clears it (PS2 name)
    static int global_refs;
    // Every actor, which the constructor adds itself to (PS2 name, its gif::vector<C_actor *, 8>)
    static std::vector<C_actor *> global_actors;
};

// A frame's actor (the names are ours): the frame's user data is the address of a pointer to it, allocated for it.
// PS2 inlines both.
// FUNCTION: GAME 0x604d90
inline C_actor **GetFrameActor(I3D_frame *frm)
{
    return (C_actor **)frm->GetData();
}

// Gives the frame the pointer, freeing the one it had; a pointer to NULL is freed too
// FUNCTION: GAME 0x547130
inline void SetFrameActor(I3D_frame *frm, C_actor **actor)
{
    if ((C_actor **)frm->m_UserData != actor)
        delete (C_actor **)frm->m_UserData;
    if (actor && !*actor)
    {
        delete actor;
        actor = NULL;
    }
    frm->m_UserData = (uint)actor;
}

// An animation's file and its id (the struct's name is ours)
struct S_anim_file
{
    const char *name;
    uint id;
};

// The actors' animations by id (PS2 name), not in the ids' order, ended by an id of 0xffff
extern S_anim_file _anim_info_[];

// An id and its animation's place in _anim_info_ (the struct's name is ours)
struct S_anim_index
{
    uint id;
    int index;
};

// _anim_info_'s ids in ascending order (PS2 names), which InitAnimIndex builds
extern S_anim_index *_anim_index_;
extern int _anim_index_count_;

// Builds the animations' index and sorts it (PS2 C_actor.cpp), once, for InitSystem
void InitAnimIndex();
// Frees the index, at CloseSystem
void DoneAnimIndex();
