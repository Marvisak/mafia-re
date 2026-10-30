#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3D.h"

// GetProgram's vector of C_program::S_instruction
#include "mission/C_program.h"

class C_chunk;
class C_cutscene_records;

// The actors' types (C_actor::type), which the mission's scene2.bin gives each actor and C_mission::CreateActor creates
// a class for (the names are ours, after the classes; PS2's are the same values but for the three PC only classes)
#define ACTOR_ACTOR 1
#define ACTOR_PLAYER 2
#define ACTOR_HUMAN 3                   // PS2 C_human's constructor
#define ACTOR_CAR 4
#define ACTOR_DETECTOR 5
#define ACTOR_DOOR 6
#define ACTOR_RAILWAY 8
#define ACTOR_DYN_OBJECT 9
#define ACTOR_BOTTLE 0xa
#define ACTOR_TRAFFIC_CAR 0xc
#define ACTOR_STONE 0xf                 // PC only, C_game::Init's 2stone.i3d
#define ACTOR_CARTRIDGE 0x10            // C_game::Init's cartridges
#define ACTOR_TRAFFIC_GENERATOR 0x12
#define ACTOR_DON_OCAS 0x13
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
#define ACTOR_DROP_IN_WEAPON 0x25       // PS2 C_drop_in_weapon's constructor
#define ACTOR_RAIL_GENERATOR 0x26

// C_actor::unknown_14, a subtype: 1 from C_actor's constructor (the names are ours)
#define ACTORSUB_CAR_EX 3               // a C_car_ex's, whose type stays ACTOR_CAR

// The actors' states (C_actor::act_state, PS2's C_actor::E_act_state; the names are ours, by value)
#define ACTSTATE_0 0
#define ACTSTATE_1 1                    // the script's ACT_SETSTATE INACTIVE
#define ACTSTATE_2 2

// Hit's types (PS2's C_actor::Hit takes an E_hit_type; the names are ours, by value): the script's HUMAN_SETPROPERTY
// hits a human whose energy runs out with them, in a car and on foot
#define HIT_UNKNOWN_5 5
#define HIT_UNKNOWN_6 6

// C_actor::unknown_4's flags (the names are ours, by value): the cutscenes set and clear 0x10 on their actors
#define ACTF_UNKNOWN_10 0x10

// The actors (PS2 C_actor), 0x70 bytes (PS2's 0x80). Only what the mission uses so far.
// The PC's vtable (0x623248) has 38 functions, PS2's 27: the PC's first 16 come before the destructor, where PS2 has
// AI, Update and NetDirect; from the destructor on the PC's are PS2's in PS2's order. The first 16 are placeholders.
// Not written yet: the virtual functions declared here but the destructor, Init, Release, GameInit and GameDone, which
// the actors' classes need, are stubs (C_actor.cpp).
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
    // Set or clear the flags' 8 and 2 and their 4 and 1 (0x5c27b0, 0x5c27d0); the cutscenes call them
    virtual void vfunc_6(bool on);
    virtual void vfunc_7(bool on);
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
    // The actor taken into the game and out of it (PS2 names), C_game::Init's and Done's; C_mission::FrmActBuff_ClearAll
    // calls GameDone before releasing a kept actor
    virtual void GameInit();
    virtual void GameDone();
    // The vtable's other slots (0x6c to 0x90): PS2's Death, Collision, IsPossibleColl and the other small ones, in an
    // order not known yet; placeholders but for Hit and GetProgram
    virtual void vfunc_27() {}
    virtual void vfunc_28() {}
    virtual void vfunc_29() {}
    virtual void vfunc_30() {}
    // A hit of the type, HIT_* (PS2 name, the parameters' names are ours): false, as PS2's (the original's slot points
    // at a folded copy, 0x4bd260)
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm)
    {
        return false;
    }
    virtual void vfunc_32() {}
    virtual void vfunc_33() {}
    virtual void vfunc_34() {}
    // The actor's script's instructions, NULL without one (PS2 name), which the script's CALL runs: a copy the linker
    // folded with every return of NULL, annotated by name after the class
    virtual std::vector<C_program::S_instruction> *GetProgram()
    {
        return NULL;
    }
    virtual void vfunc_36(int) {}
    // The actor's sounds changed (PS2 name), which C_game::StopAllSounds calls with 0: empty here, as PS2's (the
    // original's slot points at a folded empty ret 4)
    virtual void SoundsChange(int) {}

    // C_mission::OpenBin sets it for the actors of the mission's file (PC only, the name is ours)
    // FUNCTION: GAME 0x547220
    void SetUnknown66()
    {
        unknown_66 = true;
    }

    // Sets the actor's state; a change from any but ACTSTATE_2 tells the game (PS2 name, its parameter a
    // C_actor::E_act_state)
    void SetActState(int state);
    // PS2 name, which C_mission::FrmActBuff_IsActiveActor calls for a kept actor
    void Rebirth();
    // The type, ACTOR_* (the name is ours): calls VC6 left out of line read it through it, a copy folded with
    // C_mission::GetScene, C_sound_table::GetSounds and GM_Component::GetText
    int GetType() const
    {
        return type;
    }

    // PS2 names: what C_game::Init does to the models it loads for the game (the sheet, the money)
    static void DebustizeFrame(I3D_frame *frm);
    static void NoShadowFrame(I3D_frame *frm);
    // Adds the actor to global_actors, unless it is there already (PS2 name)
    static void AddActorToGlobal(C_actor *act);
    // Takes the actor from global_actors, the last one in its place (PS2 name)
    static void DelActorFromGlobal(C_actor *act);
    // Whether the actor is one of global_actors, so still exists: false for NULL (PC only, the name is ours)
    static bool IsGlobalActor(C_actor *act);
    // A tick of the actor's frame, a model's first (PS2 name), which the script calls for a car it puts on
    void Tick(uint time);
    // The frame's world position, or pos without a frame (PS2 name)
    S_vector GetWorldPos();
    // The same for the direction (PS2 name)
    S_vector GetWorldDir();
    // The position and the direction, by value (the names are ours): the camera copies them before each use, as an inline
    // function returning a copy does, where a member read in place is used there
    // FUNCTION: GAME 0x47af30
    S_vector GetPos() const
    {
        return pos;
    }
    // FUNCTION: GAME 0x47af70
    S_vector GetDir() const
    {
        return dir;
    }

    int unknown_4;                      // 0x4, flags: ACTF_*
    // 0x8, what a cutscene keeps for the actor (C_cutscene::vfunc_5's), 0 from the constructor's body
    C_cutscene_records *unknown_8;
    ushort ref;                         // 0xc, Release's references, 1 at first (the name is ours)
    int type;                           // 0x10, ACTOR_*, ACTOR_ACTOR from the constructor
    int unknown_14;                     // 0x14, ACTORSUB_*, 1 at first
    // 0x18, the mission's count of actors when Init took the actor's frame (C_mission::unknown_8; the name is ours)
    int number;
    int act_state;                      // 0x1c, ACTSTATE_*, 0 from Init (PS2 SetActState's; the name is ours)
    bool unknown_20;                    // 0x20
    uchar unknown_21[3];                // 0x21
    S_vector pos;                       // 0x24, the actor's position (the name is ours)
    S_vector dir;                       // 0x30, its direction (the name is ours)
    // The frame's placement when GameInit took the actor into the game, which GameDone puts back (the names are ours)
    S_vector saved_pos;                 // 0x3c, the local position
    S_quat saved_rot;                   // 0x48
    I3D_frame *saved_parent;            // 0x58
    bool saved_on;                      // 0x5c
    bool unknown_5d;                    // 0x5d, true from GameInit
    bool unknown_5e;                    // 0x5e, false from GameInit
    bool unknown_5f;                    // 0x5f, false from GameInit
    int unknown_60;                     // 0x60
    // 0x64, GameInit gave the frame its pointer to the actor, which GameDone takes back (the name is ours)
    bool frame_actor_set;
    // 0x65, set by GameInit for a plain actor (ACTOR_ACTOR), by C_clock's and by C_detector's for a script without
    // instructions
    bool unknown_65;
    bool unknown_66;                    // 0x66
    I3D_frame *frame;                   // 0x68, Init's (the name is ours)
    // 0x6c, false from Init; the destructor releases the frame where it is set (the name is ours)
    bool own_frame;
    bool unknown_6d;                    // 0x6d
    uchar unknown_6e[2];                // 0x6e

    // The actors that exist: the constructor counts up, the destructor down, and C_mission::Open clears it (PS2 name)
    static int global_refs;
    // Every actor, which the constructor adds itself to (PS2 name, its gif::vector<C_actor *, 8>)
    static std::vector<C_actor *> global_actors;
    // PS2 name (a vector of PS2's S_posscoll_frame), which C_game::Init clears; its type is ours
    static std::vector<uint> posscoll_frame;

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47af20
    int GetUnknown14() const
    {
        return unknown_14;
    }
    // The position and the direction set, by value (the script's)
    // FUNCTION: GAME 0x47af50
    void SetPos(S_vector pos)
    {
        this->pos = pos;
    }
    // FUNCTION: GAME 0x47af90
    void SetDir(S_vector dir)
    {
        this->dir = dir;
    }
    I3D_frame *GetFrame() const
    {
        return frame;
    }
    // FUNCTION: GAME 0x47afc0
    bool GetUnknown5e() const
    {
        return unknown_5e;
    }
    // FUNCTION: GAME 0x47afd0
    bool GetUnknown5d() const
    {
        return unknown_5d;
    }
    // FUNCTION: GAME 0x47afe0
    void SetOwnFrame(bool own)
    {
        own_frame = own;
    }
    // FUNCTION: GAME 0x47aff0
    void SetUnknown65(bool b)
    {
        unknown_65 = b;
    }
    // FUNCTION: GAME 0x47b000
    static std::vector<C_actor *> &GetGlobalActors()
    {
        return global_actors;
    }
    // Its copy is in g_indicators.cpp's range, folded with another class's (by name, after the class)
    int GetUnknown4() const
    {
        return unknown_4;
    }
};

// GetProgram's folded group (0x409220, which c_clock's vtable shares), named after the copy our linker keeps (foldfix)
// SYNTHETIC: GAME 0x409220
// ?OnClose@GM_Menu@@UAEHXZ

// GetUnknown4's and GetFrameActor's copies, folded with g_indicators.cpp's vectors' begin and end: by name, after the
// copy our linker keeps (foldfix)
// FUNCTION: GAME 0x604d80
// ?begin@?$vector@UG_MISubtitleStruct@@V?$allocator@UG_MISubtitleStruct@@@std@@@std@@QAEPAUG_MISubtitleStruct@@XZ

// FUNCTION: GAME 0x604d90
// ?end@?$vector@UG_MISubtitleStruct@@V?$allocator@UG_MISubtitleStruct@@@std@@@std@@QAEPAUG_MISubtitleStruct@@XZ

// A frame's actor (the names are ours): the frame's user data is the address of a pointer to it, allocated for it.
// PS2 inlines both.
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

// A random number from 0 to n - 1, 0 for an n below 1 (PS2 name, C_actor.cpp)
int rnd(int n);
// A random number from 0 to 1 (PS2 name)
float rnd();
// The angle from one 2D direction to the other, from -pi to pi (PS2 name)
float AngleTo2(const S_vector2 &a, const S_vector2 &b);
// Turns the vector about the y axis (PS2 name)
void RotateDir(S_vector &v, float angle);

// Builds the animations' index and sorts it (PS2 C_actor.cpp), once, for InitSystem
void InitAnimIndex();
// Frees the index, at CloseSystem
void DoneAnimIndex();
