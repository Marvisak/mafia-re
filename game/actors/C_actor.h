#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3D.h"

// GetProgram's vector of C_program::S_instruction
#include "mission/C_program.h"

class C_chunk;
class C_cutscene_records;
struct tDynamicCollObject;

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
#define ACTOR_UNKNOWN_E 0xe               // C_human::MovementIntern pushes it as a bottle
#define ACTOR_STONE 0xf                 // PC only, C_game::Init's 2stone.i3d
#define ACTOR_CARTRIDGE 0x10            // C_game::Init's cartridges
#define ACTOR_TRAFFIC_GENERATOR 0x12
#define ACTOR_UNKNOWN_11 0x11            // a C_PhThing's (TickShoot takes its material)
#define ACTOR_DON_OCAS 0x13
#define ACTOR_BRIDGE 0x14
#define ACTOR_DOG 0x15
#define ACTOR_AIRPLANE 0x16
#define ACTOR_SEAGULL 0x17
#define ACTOR_TURNOUT 0x18
#define ACTOR_PUMPER 0x19
#define ACTOR_CRASH_OBJ 0x1a              // C_crash_obj's
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

// C_actor::subtype: 1 from C_actor's constructor (the names are ours)
#define ACTORSUB_CAR_EX 3               // a C_car_ex's, whose type stays ACTOR_CAR
#define ACTORSUB_HUMAN 2                // C_human's constructor

// The actors' states (C_actor::act_state, PS2's C_actor::E_act_state; the names are ours, by value)
#define ACTSTATE_0 0
#define ACTSTATE_1 1                    // the script's ACT_SETSTATE INACTIVE
#define ACTSTATE_2 2

// Hit's types (PS2's C_actor::Hit takes an E_hit_type; the names are ours, by value): the script's HUMAN_SETPROPERTY
// hits a human whose energy runs out with them, in a car and on foot
#define HIT_SHOT 0                      // C_game::TickShoot's
#define HIT_STROKE 1                    // C_human::Hit's stroke (HitStroke)
#define HIT_UNKNOWN_2 2                 // C_human::Hit: a fall above 100, a heavy hit on a random part above 50
#define HIT_UNKNOWN_3 3                 // C_human::Hit: the hands and legs lose energy too
#define HIT_UNKNOWN_4 4                 // C_human::Hit: a leg, blood and SHOTPARTICLE_BODY
#define HIT_UNKNOWN_5 5
#define HIT_LANDING 6                   // C_human::Hit's landing (HitLanding)
#define HIT_UNKNOWN_7 7                 // C_human::Hit: no energy left, a fall and a particle
#define HIT_UNKNOWN_8 8                 // C_human::Hit: a fall, aside, for a direction longer than 3
#define HIT_UNKNOWN_9 9

// C_actor::act_flags (the names are ours, by value where the meaning isn't known): Tick leaves out AI where 1 or 4 is
// set (vfunc_7 sets and clears both)
#define ACTF_UNKNOWN_1 0x1
#define ACTF_UNKNOWN_4 0x4
// Set with 8 while a cutscene records the actor (vfunc_6, which C_recorder's CUTSCENE_STATE_2 sets for the second
// list's actors): only then C_human::SetCutsceneLink notes a link for the recording
#define ACTF_UNKNOWN_2 0x2
// A cutscene's actor: set on a cutscene's actors while it plays (C_recorder::SetState), cleared after; C_game::Init's
// search for the player skips the players that have it
#define ACTF_CUTSCENE 0x10

// A human's link to another actor, for the cutscene's recording (the names are ours), 0x48 bytes:
// C_human::SetCutsceneLink fills it while the human is recorded, and C_human::vfunc_0 records the kind and the actor
// (by its frame's name, an item of the cutscene's anim_names) into its next record, which C_human::vfunc_3 plays back
// through SetCutsceneLink
struct S_cutscene_link
{
    bool is_new;                        // 0x0, set by SetCutsceneLink, cleared by the record that took it
    int kind;                           // 0x4, 1 to 15 (4 bits in the record)
    char name[0x24];                    // 0x8, the actor's frame's name, "" for none
    S_vector pos;                       // 0x2c, the human's frame's position and rotation then
    S_quat rot;                         // 0x38
};

// The actors (PS2 C_actor), 0x70 bytes (PS2's 0x80). Only what the mission uses so far.
// The PC's vtable (0x623248) has 38 functions, PS2's 27: the PC's first 13 come before AI, Update and NetDirect, PS2's
// first three; from those on the PC's are PS2's in PS2's order, but for PS2's last two (ChangeSceneInit and
// ChangeSceneDone). The first 13 are PC only, most of them the cutscenes' (they read records): placeholders, their
// parameters and results by the original's code (the types are guesses).
// Not written yet: the virtual functions declared here but the destructor, Init, Release, GameInit and GameDone, which
// the actors' classes need, are stubs (C_actor.cpp). Each actor class declares the slots its vtable overrides.
// VTABLE: GAME 0x623248
class C_actor
{
public:
    C_actor();

    virtual int vfunc_0(int a);         // -1 here
    virtual void vfunc_1(int a);
    virtual void vfunc_2(int a);
    virtual bool vfunc_3(int a);
    virtual S_vector vfunc_4(int a);
    virtual S_quat vfunc_5(int a);
    // Set or clear the flags' 8 and 2 and their 4 and 1 (0x5c27b0, 0x5c27d0); the cutscenes call them
    virtual void vfunc_6(bool on);
    virtual void vfunc_7(bool on);
    virtual void vfunc_8();
    virtual bool vfunc_9();
    virtual void *vfunc_10();
    virtual void *vfunc_11(int a);
    // Empty here (the folded ret, 0x404990)
    virtual void vfunc_12() {}
    // Adds a record of the type to the records (they are the actor's own) and returns it; it starts a new block where
    // the last one is full (PC only, 0x5c2730 in the records' file; the name is ours)
    void *AddRecord(C_cutscene_records *rec, int type);
    // The actor's frame of a tick (PS2 names): its own decisions, unless ACTF_UNKNOWN_1 or 4 stops them, or what the
    // network says instead where net_direct is set, then the update. Empty here, as PS2's (the original's slots point
    // at the folded ret 4, 0x403db0).
    virtual void AI(uint time) {}
    virtual void Update(uint time) {}
    virtual void NetDirect(uint time) {}
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
    // An actor in a saved game, by its frame's name, NULL for none (PS2 names; static, the arguments in
    // registers)
    static int Actor_SaveGameGetSize(C_actor *act);
    static void Actor_SaveGameSave(void **buf, C_actor *act);
    static C_actor *Actor_SaveGameLoad(void **buf);
    // The actor taken into the game and out of it (PS2 names), C_game::Init's and Done's; C_mission::FrmActBuff_ClearAll
    // calls GameDone before releasing a kept actor
    virtual void GameInit();
    virtual void GameDone();
    // PS2 names from here on, the types PS2's but where noted: empty or constant here, each slot a folded copy, but for
    // GetBoundPoints2D (a stub)
    virtual void ChangeState() {}
    // The actor's speed, none here
    virtual void GetActorSpeed(S_vector &speed)
    {
        speed.Zero();
    }
    // The actor's death, which Tick runs once (empty, its slot the folded empty function of the out-of-line kind,
    // 0x4aabe0)
    virtual void Death();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm)
    {
        return false;
    }
    // A hit of the type, HIT_* (PS2 name, the parameters' names are ours): false, as PS2's (the original's slot points
    // at a folded copy, 0x4bd260)
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm)
    {
        return false;
    }
    virtual int GetMaterialID(I3D_frame *frm)
    {
        return 0;
    }
    // The PC's takes ten dwords, PS2's six parameters, the two vectors references: taken here by value (a guess)
    virtual bool IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll)
    {
        return false;
    }
    virtual bool GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm);
    // The actor's script's instructions, NULL without one (PS2 name), which the script's CALL runs: a copy the linker
    // folded with every return of NULL, annotated by name after the class
    virtual std::vector<C_program::S_instruction> *GetProgram()
    {
        return NULL;
    }
    virtual void cbCollTester(tDynamicCollObject *obj) {}
    // The actor's sounds changed (PS2 name), which C_game::StopAllSounds calls with 0: empty here, as PS2's (the
    // original's slot points at a folded empty ret 4)
    virtual void SoundsChange(int) {}

    // C_mission::OpenBin sets from_mission for the actors of the mission's file (PC only, the name is ours)
    // FUNCTION: GAME 0x547220
    void SetFromMission()
    {
        from_mission = true;
    }

    // Sets the actor's state; a change from any but ACTSTATE_2 tells the game (PS2 name, its parameter a
    // C_actor::E_act_state)
    void SetActState(int state);
    // The state (the name is ours, after SetActState): C_game::ChangeActState reads it through it, which leaves the
    // first push_back's insert out of line there, as the original
    int GetActState() const
    {
        return act_state;
    }
    // PS2 name, which C_mission::FrmActBuff_IsActiveActor calls for a kept actor
    void Rebirth();
    // Puts the frame back where GameInit found it (PS2 name), which C_mission's FrmActBuff functions call for an actor
    // they keep
    void ResetPlacement();
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
    // The actor's frame (PS2 name), which C_game::Tick runs for the mission's actors and the script for a car it puts on
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

    int act_flags;                      // 0x4, ACTF_* (the name is ours, after act_state)
    // 0x8, what a cutscene keeps for the actor (C_recorder::vfunc_5's), 0 from the constructor's body (the name is ours,
    // after the type)
    C_cutscene_records *records;
    ushort ref;                         // 0xc, Release's references, 1 at first (the name is ours)
    int type;                           // 0x10, ACTOR_*, ACTOR_ACTOR from the constructor
    int subtype;                        // 0x14, ACTORSUB_*, 1 at first (the name is ours)
    // 0x18, the mission's count of actors when Init took the actor's frame (C_mission::last_actor_number; the name is ours)
    int number;
    int act_state;                      // 0x1c, ACTSTATE_*, 0 from Init (PS2 SetActState's; the name is ours)
    // 0x20 (PS2 0x60), false from the constructor: Tick runs NetDirect instead of AI where it is set (the name is ours,
    // after NetDirect)
    bool net_direct;
    uchar unknown_21[3];                // 0x21
    S_vector pos;                       // 0x24, the actor's position (the name is ours)
    S_vector dir;                       // 0x30, its direction (the name is ours)
    // The frame's placement when GameInit took the actor into the game, which GameDone puts back (the names are ours)
    S_vector saved_pos;                 // 0x3c, the local position
    S_quat saved_rot;                   // 0x48
    I3D_frame *saved_parent;            // 0x58
    bool saved_on;                      // 0x5c
    // The actor's life (PS2 0x61 to 0x63; the names are ours): alive from GameInit till its end, dead once Tick has run
    // Death for it (HUMAN_GETPROPERTY gives an actor not alive no energy, the HUD's lives stay at least 1 for one not
    // dead), and whether Tick found its model or visual rendered in the scene's last frame
    bool alive;                         // 0x5d, true from GameInit
    bool dead;                          // 0x5e, false from GameInit
    bool rendered;                      // 0x5f, false from GameInit
    // 0x60, a link to another actor that the recording takes into the human's next record (C_human::SetCutsceneLink's;
    // the name is ours)
    S_cutscene_link *cutscene_link;
    // 0x64, GameInit gave the frame its pointer to the actor, which GameDone takes back (the name is ours)
    bool frame_actor_set;
    // 0x65, set by GameInit for a plain actor (ACTOR_ACTOR), by C_clock's and by C_detector's for a script without
    // instructions
    bool unknown_65;
    // 0x66, false from the constructor, set for the actors of the mission's file (SetFromMission), whose cars
    // C_game::Done keeps (the name is ours)
    bool from_mission;
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
    int GetSubtype() const
    {
        return subtype;
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
    // Folded with C_mission::GetMissionName's copy (Mission.h)
    // FUNCTION: GAME 0x47afb0 FOLDED
    I3D_frame *GetFrame() const
    {
        return frame;
    }
    // FUNCTION: GAME 0x47afc0
    bool GetDead() const
    {
        return dead;
    }
    // Its copy is among C_sledovacka.cpp's
    // FUNCTION: GAME 0x4a9990
    bool GetRendered() const
    {
        return rendered;
    }
    // FUNCTION: GAME 0x47afd0
    bool GetAlive() const
    {
        return alive;
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
    int GetActFlags() const
    {
        return act_flags;
    }
};

// SYNTHETIC: GAME 0x405d70
// C_actor::`scalar deleting destructor'

// GetProgram's folded group (0x409220, which c_clock's vtable shares), named after the copy our linker keeps (foldfix)
// SYNTHETIC: GAME 0x409220
// ?SaveGameGetSize@C_racecam@@UAEHXZ

// The other base functions the linker folded with others of the same code, by name after the copies our linker keeps
// (foldfix): DuplicateFrom's empty ret 4 of the out-of-line kind, GetActorSpeed, GetMaterialID's return of 0,
// IsPossibleColl's and Collision's returns of false
// SYNTHETIC: GAME 0x4ab130
// ?Update@C_rail_generator@@UAEXI@Z

// SYNTHETIC: GAME 0x4091f0
// ?GetActorSpeed@C_actor@@UAEXAAUS_vector@@@Z

// SYNTHETIC: GAME 0x409210
// ?GetMaterialID@C_actor@@UAEHPAVI3D_frame@@@Z

// SYNTHETIC: GAME 0x407590
// ?IsPossibleColl@C_actor@@UAE_NPAV1@US_vector@@1MAAMPAPAV1@@Z

// SYNTHETIC: GAME 0x4aa450
// ?Collision@C_actor@@UAE_NPAV1@ABUS_vector@@11MPAVI3D_frame@@@Z

// The derived classes' constant overrides the linker folded the same way: Hit's return of false, returns of true (in
// the class and out of line, one argument) and of 0 (out of line, none)
// SYNTHETIC: GAME 0x4bd260
// ?Hit@C_actor@@UAE_NHABUS_vector@@00MPAV1@IPAVI3D_frame@@@Z

// SYNTHETIC: GAME 0x4aa440
// ?SaveGameLoad@C_bridge@@UAE_NPAPAX@Z

// SYNTHETIC: GAME 0x4aa490
// ?SaveGameLoad@C_clock@@UAE_NPAPAX@Z

// SYNTHETIC: GAME 0x4aa430
// ?vfunc_53@C_recorder@@UAEHXZ

// GetActFlags's and GetFrameActor's copies, folded with g_indicators.cpp's vectors' begin and end: by name, after the
// copy our linker keeps (foldfix)
// FUNCTION: GAME 0x604d80
// ?begin@?$vector@UG_MIIntroSubtitles@@V?$allocator@UG_MIIntroSubtitles@@@std@@@std@@QAEPAUG_MIIntroSubtitles@@XZ

// FUNCTION: GAME 0x604d90
// ?end@?$vector@UG_MIIntroSubtitles@@V?$allocator@UG_MIIntroSubtitles@@@std@@@std@@QAEPAUG_MIIntroSubtitles@@XZ

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

// An animation's file and its id (PS2 S_anim_info, 8 bytes): _anim_info_'s entries, and the animations a human replaces
// them with (C_human::anims, PS2's vector<S_anim_info>)
struct S_anim_info
{
    char *name;                         // 0x0, a literal in _anim_info_, new[]'s in C_human::anims
    uint id;
};

// The actors' animations by id (PS2 name), not in the ids' order, ended by an id of 0xffff
extern S_anim_info _anim_info_[];

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
// Unlinks the frame's children (PS2 name, C_actor.cpp's range): C_human::ChangeWeaponModel empties the weapon's model
void UnlinkAllFrameChild(I3D_frame *frm);
// The angle from one 2D direction to the other, from -pi to pi (PS2 name)
float AngleTo2(const S_vector2 &a, const S_vector2 &b);
// Turns the vector about the y axis (PS2 name)
void RotateDir(S_vector &v, float angle);
// Builds the animations' index and sorts it (PS2 C_actor.cpp), once, for InitSystem
void InitAnimIndex();
// Frees the index, at CloseSystem
void DoneAnimIndex();
// The index of the animation of that id in _anim_info_, -1 for none (PS2 name), a binary search of _anim_index_
int GetAnimIndex(uint id);
