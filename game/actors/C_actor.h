#pragma once

#include "Main.h"

#include "I3D/I3D.h"

class C_chunk;

// The actors (PS2 C_actor). Only what the mission uses so far.
// The PC's vtable (0x623248) has 38 functions, PS2's 27: the PC's first 16 come before the destructor, where PS2 has
// AI, Update and NetDirect; from the destructor on the PC's are PS2's in PS2's order. The first 16 are placeholders.
class C_actor
{
public:
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
    // Deletes the actor (PS2 name)
    virtual void Release();
    // Takes the actor's frame from the scene: false if it can't be the actor (PS2 name)
    virtual bool Init(I3D_frame *frame);
    virtual void SaveData(C_chunk *chunk);
    // The actor's data of the mission's scene2.bin (PS2 name)
    virtual void LoadData(C_chunk *chunk);

    // C_mission::OpenBin sets it for the actors of the mission's file (PC only, the name is ours)
    // FUNCTION: GAME 0x547220
    void SetUnknown66()
    {
        unknown_66 = true;
    }

    uchar unknown_4[0x62];
    bool unknown_66;                    // 0x66

    // The actors that exist: the constructor counts up, the destructor down, and C_mission::Open clears it (PS2 name)
    static int global_refs;
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
