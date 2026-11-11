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
// A human (DabingCutscene takes it as one; the name is ours, by value: no class of C_mission::CreateActor's has it)
#define ACTOR_UNKNOWN_7 7
#define ACTOR_RAILWAY 8
#define ACTOR_DYN_OBJECT 9
#define ACTOR_BOTTLE 0xa
// C_game::NewExplosion hits it as a traffic car, whatever the distance (the name is ours, by value: no class of
// C_mission::CreateActor's has it)
#define ACTOR_UNKNOWN_B 0xb
#define ACTOR_TRAFFIC_CAR 0xc
#define ACTOR_GRENADE 0xe               // PS2 C_grenade's constructor; C_human::MovementIntern pushes it as a bottle
#define ACTOR_STONE 0xf                 // PC only, C_game::Init's 2stone.i3d
#define ACTOR_CARTRIDGE 0x10            // C_game::Init's cartridges
#define ACTOR_TRAFFIC_GENERATOR 0x12
#define ACTOR_DROP_OUT 0x11             // PS2 C_DropOut's constructor, a C_PhThing's (TickShoot takes its material)
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

// The actors' states (C_actor::act_state, PS2's C_actor::E_act_state; the names are ours, after the script's
// ACT_SETSTATE, which sets them for its ACT_STATE_ACTIVE, ACT_STATE_INACTIVE and ACT_STATE_NONE): C_game::Tick ticks
// the active and the inactive actors, and one in ACTSTATE_NONE leaves it only through Rebirth
#define ACTSTATE_ACTIVE 0
#define ACTSTATE_INACTIVE 1
#define ACTSTATE_NONE 2

// C_actor::number's top bit: InternMissionInit sets it in the numbers of the actors it carries into the new
// mission, whose own actors count from 1 (the name is ours)
#define ACTOR_NUMBER_CARRIED 0x80000000

// Hit's types (PS2's C_actor::Hit takes an E_hit_type; the names are ours, after their callers, by value where none
// passes it): the script's HUMAN_SETPROPERTY hits a human whose energy runs out with HIT_IN_CAR or HIT_LANDING
#define HIT_SHOT 0                      // C_game::TickShoot's
#define HIT_STROKE 1                    // C_human::Hit's stroke (HitStroke)
#define HIT_EXPLOSION 2                 // C_game::NewExplosion's, a train's or a plane's: a fall above 100 (Hit)
#define HIT_FIRE 3                      // C_fire::Tick's: the hands and legs lose energy too (C_human::Hit)
#define HIT_DOG_BITE 4                  // C_dog's: a leg, blood and SHOTPARTICLE_BODY (C_human::Hit)
#define HIT_IN_CAR 5                    // C_car's damage to its seats' owners: only C_human::HitInCar counts it
#define HIT_LANDING 6                   // C_human::Hit's landing (HitLanding)
#define HIT_AIRPLANE 7                  // C_airplane::AI's: no energy left, a fall and HITPARTICLE_AIRPLANE
#define HIT_CAR 8                       // a car run into a human (C_human::Collision): a fall aside above 3
#define HIT_UNKNOWN_9 9

// C_actor::act_flags (the names are ours): the cutscene's marks, the actor played and recorded (the names are guesses),
// which SetCutscenePlay and SetCutsceneRecord ask for (4, 8) and UpdateCutsceneFlags makes the marks (1, 2): Tick
// leaves out AI while the actor is played or asked to be, C_human::SetCutsceneLink notes a link only while recorded
#define ACTF_CUTSCENE_PLAYED 0x1
#define ACTF_CUTSCENE_RECORDED 0x2
#define ACTF_CUTSCENE_PLAY 0x4
#define ACTF_CUTSCENE_RECORD 0x8
// A cutscene's actor: set on a cutscene's actors while it plays (C_recorder::SetState), cleared after; C_game::Init's
// search for the player skips the players that have it
#define ACTF_CUTSCENE 0x10

// A human's link to another actor, for the cutscene's recording (the names are ours), 0x48 bytes:
// C_human::SetCutsceneLink fills it while the human is recorded, and C_human::RecordKey records the kind and the
// actor (by its frame's name, an item of the cutscene's anim_names) into its next record, which C_human::PlaceAtTime
// plays back through SetCutsceneLink
struct S_cutscene_link
{
    bool is_new;                        // 0x0, set by SetCutsceneLink, cleared by the record that took it
    int kind;                           // 0x4, 1 to 15 (4 bits in the record)
    char name[0x24];                    // 0x8, the actor's frame's name, "" for none
    S_vector pos;                       // 0x2c, the human's frame's position and rotation then
    S_quat rot;                         // 0x38
};

// An actor's record (C_actor::AddToCutscene gives its size; the names are ours), 0x5c bytes: a key of a spline through
// the frame's positions, with its tangents and the ease of the position and of the rotation into the key and out of it
// (Ease's, 3D Studio's keys'), and an animation of the cutscene. PlayStep, PlaceAtTime and GetRecordPos move the
// frame between two keys by Hermite's basis functions, GetRecordRot slerps the rotation without the ease.
struct S_actor_record
{
    int time;                           // 0x0, S_cutscene_record's
    int type;                           // 0x4
    S_vector pos;                       // 0x8, the frame's position
    S_quat rot;                         // 0x14, the frame's rotation
    S_vector tangent_out;               // 0x24, the spline's tangent towards the next key
    S_vector tangent_in;                // 0x30, its tangent from the previous key
    uchar unknown_3c[0xc];              // 0x3c
    float ease_to;                      // 0x48, the position's ease into the key
    float ease_from;                    // 0x4c, and out of it
    float rot_ease_to;                  // 0x50, the same for the rotation
    float rot_ease_from;                // 0x54
    // 0x58: the animation (ACTOR_REC_ANIM), its flags (ACTOR_REC_ANIMATED ...), and from bit ACTOR_REC_ANIM_TIME_SHIFT
    // on its time when the record was made, in 40 ms
    uint anim;
};

// S_actor_record::anim (the names are ours): the animation, an index of the cutscene's animations (GetAnimByIndex's),
// played on a model's first channel where ACTOR_REC_ANIMATED is set, looping or once; the frame stays at the record's
// placement up to the next key (no blending), and a camera's record puts the scene's camera where the frame is
#define ACTOR_REC_ANIM 0x7ff
#define ACTOR_REC_ANIMATED 0x1000
#define ACTOR_REC_LOOP 0x2000
#define ACTOR_REC_CAMERA 0x4000
#define ACTOR_REC_NO_BLEND 0x8000
#define ACTOR_REC_ANIM_TIME_SHIFT 16

// The saved game's data of an actor (C_actor::SaveGameSave's; the names are ours), 0xd bytes, packed; a car's models
// follow it (SaveGameSave). PS2's has the same fields, 0x10 bytes: version, number and num_models as ints, then the
// bytes.
#pragma pack(push, 1)
struct S_actor_savegame
{
    uchar version;                      // 0x0, ACTOR_SAVEGAME_VERSION
    uchar act_state;                    // 0x1
    int number;                         // 0x2, which SaveGameLoadGuid reads alone
    bool alive;                         // 0x6
    bool dead;                          // 0x7
    bool frame_on;                      // 0x8, the frame's FRMFLAGS_ON, false without a frame
    int num_models;                     // 0x9, the car's models saved after it
};
#pragma pack(pop)

// The version of the saved game's data SaveGameSave writes, the only one SaveGameLoad and SaveGameLoadGuid take (the
// name is ours)
#define ACTOR_SAVEGAME_VERSION 3

// The actors (PS2 C_actor), 0x70 bytes (PS2's 0x80).
// The PC's vtable (0x623248) has 38 functions, PS2's 27: the PC's first 13 come before AI, Update and NetDirect, PS2's
// first three; from those on the PC's are PS2's in PS2's order, but for PS2's last two (ChangeSceneInit and
// ChangeSceneDone). The first 13 are PC only, the cutscenes', on the actor's records (the names are ours; their
// parameters and results by the original's code). Each actor class declares the slots its vtable overrides.
// VTABLE: GAME 0x623248
class C_actor
{
public:
    C_actor();

    // The record made for the time: its type, -1 for none (always here)
    virtual int RecordKey(int time);
    virtual void PlayRecord(int time);
    virtual void PlayStep(int time);
    virtual bool PlaceAtTime(int time);
    virtual S_vector GetRecordPos(int time);
    virtual S_quat GetRecordRot(int time);
    // ACTF_CUTSCENE_*: ask for the marks or take them back (0x5c27b0, 0x5c27d0), the asked ones made marks (0x5c27f0)
    virtual void SetCutsceneRecord(bool on);
    virtual void SetCutscenePlay(bool on);
    virtual void UpdateCutsceneFlags();
    virtual bool AddToCutscene();
    // The size of the records up to the last key, and of a block's (their bytes, without the blocks' headers)
    virtual int GetRecordsSize();
    virtual int GetBlockRecordsSize(uchar *block);
    // Empty here (the folded ret, 0x404990)
    virtual void DumpRecords() {}
    // Adds a record of the type to the records (they are the actor's own) and returns it; it starts a new block where
    // the last one is full (PC only, 0x5c2730 in the records' file; the name is ours)
    void *AddRecord(C_cutscene_records *rec, int type);
    // A new block of records (0x5c2700, the name is ours), linked after prev where there is one: it doesn't use
    // the actor, but AddRecord calls it with the actor in ecx and its argument on the stack, as a member
    uchar *NewRecordBlock(uchar *prev);
    // The actor's frame of a tick (PS2 names): its own decisions, unless a cutscene plays the actor (ACTF_CUTSCENE_*),
    // or what the network says instead where net_direct is set, then the update. Empty here, as PS2's (the original's
    // slots point at the folded ret 4, 0x403db0).
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
    // The actor's data of the mission's scene2.bin, written and read (PS2 names): true for done (the overrides
    // return false where the data isn't theirs, C_car's, C_dyn_object's LoadData)
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    // PS2 names, the types PS2's; the save functions return true (mov al, 1)
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    // An actor in a saved game, by its frame's name and its type, NULL for none (PS2 names; static, the arguments in
    // registers): the save returns true too
    static int Actor_SaveGameGetSize(C_actor *act);
    static bool Actor_SaveGameSave(void **buf, C_actor *act);
    static C_actor *Actor_SaveGameLoad(void **buf);
    // The actor's number from a saved game's record (PS2 name), which G_LoadSaveClass::SaveGameLoad calls
    void SaveGameLoadGuid(void **buf);
    // The actor taken into the game and out of it (PS2 names), C_game::Init's and Done's; C_mission::FrmActBuff_ClearAll
    // calls GameDone before releasing a kept actor
    virtual void GameInit();
    virtual void GameDone();
    // PS2 names from here on, the types PS2's but where noted: empty or constant here, each slot a folded copy, but for
    // GetBoundPoints2D (C_actor.cpp)
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

    // Sets the actor's state and tells the game of the change, but for an actor in ACTSTATE_NONE, which stays in it
    // (PS2 name, its parameter a C_actor::E_act_state)
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
    // Saves the frame's placement as GameInit does (PS2 name)
    void InternalPlacementReset();
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
    // The frames the actors' possible collisions test against, in posscoll_frame, with a radius each (PS2 names; the
    // parameters' types are guesses)
    bool IsPossibleCollAll(S_vector v1, S_vector v2, float f, S_vector &out, C_actor **coll);
    static void AddFrameForPossibleColl(I3D_frame *frm, float radius);
    static void DelFrameForPossibleColl(I3D_frame *frm);
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
    // 0x8, what a cutscene keeps for the actor (C_recorder::CreateRecords'), 0 from the constructor's body (the name is ours,
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
    // A frame the actors' possible collisions test against, with its radius (PS2 name, 8 bytes; the members' names
    // are ours)
    struct S_posscoll_frame
    {
        I3D_frame *frame;               // 0x0
        float radius;                   // 0x4
    };
    // The frames AddFrameForPossibleColl and DelFrameForPossibleColl keep, which C_game::Init clears (PS2 name)
    static std::vector<S_posscoll_frame> posscoll_frame;

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
// ?OnClose@GM_Menu@@UAEHXZ

// The other base functions the linker folded with others of the same code, by name after the copies our linker keeps
// (foldfix): DuplicateFrom's empty ret 4 of the out-of-line kind, GetActorSpeed, GetMaterialID's return of 0,
// IsPossibleColl's and Collision's returns of false
// SYNTHETIC: GAME 0x4ab130
// ?DuplicateFrom@C_actor@@UAEXPAV1@@Z

// SYNTHETIC: GAME 0x4091f0
// ?GetActorSpeed@C_actor@@UAEXAAUS_vector@@@Z

// SYNTHETIC: GAME 0x409210
// ?OnTick@GM_Menu@@UAEHI@Z

// SYNTHETIC: GAME 0x407590
// ?IsPossibleColl@C_actor@@UAE_NPAV1@US_vector@@1MAAMPAPAV1@@Z

// SYNTHETIC: GAME 0x4aa450
// ?Collision@C_actor@@UAE_NPAV1@ABUS_vector@@11MPAVI3D_frame@@@Z

// The derived classes' constant overrides the linker folded the same way: Hit's return of false, returns of true (in
// the class and out of line, one argument) and of 0 (out of line, none)
// SYNTHETIC: GAME 0x4bd260
// ?Hit@C_actor@@UAE_NHABUS_vector@@00MPAV1@IPAVI3D_frame@@@Z

// SYNTHETIC: GAME 0x4aa440
// ?LoadData@C_wagon@@UAE_NPAVC_chunk@@@Z

// SYNTHETIC: GAME 0x4aa490
// ?SaveData@C_actor@@UAE_NPAVC_chunk@@@Z

// SYNTHETIC: GAME 0x4aa430
// ?vfunc_53@C_recorder@@UAEHXZ

// GetActFlags's and GetFrameActor's copies, folded with g_indicators.cpp's vectors' begin and end: by name, after the
// copy our linker keeps (foldfix)
// FUNCTION: GAME 0x604d80
// ?GetRefCount@@YIHPBVI3D_frame@@@Z

// FUNCTION: GAME 0x604d90
// ?GetFrameActor@@YIPAPAVC_actor@@PAVI3D_frame@@@Z

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

// _anim_info_'s ids: the low nine bits are the animation (ANIM_BASE), at most one bit above them its set (ANIM_SET_*),
// in which PersonAnim plays the walks, the stances and the corners' animations of the ids up to ANIM_SET_LAST (the
// names are ours, after the sets' files)
#define ANIM_BASE 0x1ff
#define ANIM_SET_PISTOL 0x200           // "pis": a pistol aimed (breath02a.i3d, walk2.i3d, ...)
#define ANIM_SET_RIFLE 0x400            // "puska": a rifle aimed
#define ANIM_SET_SMG 0x800              // "sapik": the Thompson or a shotgun aimed (the corners' "brok", "sap")
#define ANIM_SET_TWO_HANDED 0x1000      // a two-handed weapon not aimed
#define ANIM_SET_CROUCH 0x2000          // "drep": crouched
#define ANIM_SET_CROUCH_PISTOL 0x4000   // "pis drep": crouched, a pistol aimed
#define ANIM_SET_CROUCH_RIFLE 0x8000    // "puska drep": crouched with a two-handed weapon
#define ANIM_SET_LAST 0x5d
// _anim_info_'s last entry's id (the name is ours)
#define ANIM_INFO_END 0xffff
// The animations by their base ids, those the humans' code uses (the names are the files', the lowest set's or what
// the sets' files share, without '!', '?' and the extension; JUMPRX1's X is its '?')
#define ANIM_BREATH01A 0x1              // "breath01a.i3d"
#define ANIM_BREATH01B 0x2              // "breath01b.i3d"
#define ANIM_BREATH01C 0x3              // "breath01c.i3d"
#define ANIM_BREATH01D 0x4              // "breath01d.i3d"
#define ANIM_RUN1 0x5                   // "run1.i3d"
#define ANIM_WALK1 0x6                  // "walk1.i3d"
#define ANIM_BACK1 0x7                  // "back1.i3d"
#define ANIM_LEFT1 0x8                  // "left1.i3d"
#define ANIM_RIGHT1 0x9                 // "right1.i3d"
#define ANIM_TURN1 0xa                  // "turn1.i3d"
#define ANIM_STRAFL1 0xb                // "strafL1.i3d"
#define ANIM_STRAFR1 0xc                // "strafR1.i3d"
#define ANIM_STRAFRL1 0xd               // "strafRL1.i3d"
#define ANIM_STRAFRR1 0xe               // "strafRR1.i3d"
#define ANIM_RUNL1 0xf                  // "runL1.i3d"
#define ANIM_RUNR1 0x10                 // "runR1.i3d"
#define ANIM_WALKL1 0x11                // "walkL1.i3d"
#define ANIM_WALKR1 0x12                // "walkR1.i3d"
#define ANIM_BACKL1 0x13                // "backL1.i3d"
#define ANIM_BACKR1 0x14                // "backR1.i3d"
#define ANIM_ENM_KLID_STAT_A 0x15       // "enm klid stat A.i3d"
#define ANIM_ENM_KLID_STAT_B 0x16       // "enm klid stat B.i3d"
#define ANIM_ENM_KLID_STAT_C 0x17       // "enm klid stat C.i3d"
#define ANIM_ENM_KLID_WALK 0x18         // "enm klid walk.i3d"
#define ANIM_ENM_KLID_BACK 0x19         // "enm klid back.i3d"
#define ANIM_ENM_KLID_LEFT 0x1a         // "enm klid left.i3d"
#define ANIM_ENM_KLID_RIGHT 0x1b        // "enm klid right.i3d"
#define ANIM_ENM_KLID_STRAFL 0x1c       // "enm klid strafL.i3d"
#define ANIM_ENM_KLID_STRAFR 0x1d       // "enm klid strafR.i3d"
#define ANIM_ENM_KLID_WALKL 0x1e        // "enm klid walkL.i3d"
#define ANIM_ENM_KLID_WALKR 0x1f        // "enm klid walkR.i3d"
#define ANIM_ENM_KLID_BACKL 0x20        // "enm klid backL.i3d"
#define ANIM_ENM_KLID_BACKR 0x21        // "enm klid backR.i3d"
#define ANIM_ENM_STREH_STAT_A 0x22      // "enm streh pis stat A.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_STAT_B 0x23      // "enm streh pis stat B.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_STAT_C 0x24      // "enm streh pis stat C.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_WALK 0x25        // "enm streh pis walk.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_BACK 0x26        // "enm streh pis back.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_LEFT 0x27        // "enm streh pis left.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_RIGHT 0x28       // "enm streh pis right.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_STRAFL 0x29      // "enm streh pis strafL.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_STRAFR 0x2a      // "enm streh pis strafR.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_WALKL 0x2b       // "enm streh pis walkL.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_WALKR 0x2c       // "enm streh pis walkR.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_BACKL 0x2d       // "enm streh pis backL.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_ENM_STREH_BACKR 0x2e       // "enm streh pis backR.i3d" (set 0x200; 0x400, 0x800)
#define ANIM_PANIKA_LEKNUTI01L 0x2f     // "panika leknuti01L.i3d"
#define ANIM_PANIKA_LEKNUTI02L 0x33     // "panika leknuti02L.i3d"
#define ANIM_X_PANIKA_DREP_DOLU 0x3d    // "x panika drep dolu.i3d"
#define ANIM_X_PANIKA_DREP_STATIV 0x3e  // "x panika drep stativ.i3d"
#define ANIM_X_PANIKA_NA4_DOLU 0x40     // "x panika na4 dolu.i3d"
#define ANIM_X_PANIKA_NA4_STATIV 0x41   // "x panika na4 stativ.i3d"
#define ANIM_X_PANIKA_LEH_DOLU 0x43     // "x panika leh dolu.i3d"
#define ANIM_X_PANIKA_LEH_STATIV 0x44   // "x panika leh stativ.i3d"
#define ANIM_X_PANIKA_LEH_NAHORU 0x45   // "x panika leh nahoru.i3d"
#define ANIM_PANIKA_RUN01A 0x46         // "panika run01a.i3d"
#define ANIM_PANIKA_RUN03A 0x48         // "panika run03a.i3d"
#define ANIM_PANIKA_RUN02A 0x4a         // "panika run02a.i3d"
#define ANIM_ENM_ROH_STOJ_STAT_L 0x4e   // "enm roh stoj stat L.i3d"
#define ANIM_ENM_ROH_STOJ_STAT_R 0x4f   // "enm roh stoj stat R.i3d"
#define ANIM_ENM_ROH_STOJ_NAHL_L 0x50   // "enm roh stoj nahl L.i3d"
#define ANIM_ENM_ROH_STOJ_NAHL_R 0x51   // "enm roh stoj nahl R.i3d"
#define ANIM_ENM_ROH_DREP_NAHL_T 0x52   // "enm roh drep nahl T.i3d" (set 0x2000; 0x4000, 0x8000)
#define ANIM_ENM_ROH_FIRE01_L 0x53      // "enm roh pis stoj fire01 L.i3d" (set 0x200; 0x800, 0x1000, 0x4000, 0x8000)
#define ANIM_ENM_ROH_SAP_STOJ_FIRE01_L 0x54 // "enm roh sap stoj fire01 L.i3d" (set 0x800; 0x1000)
#define ANIM_ENM_ROH_FIRE01_R 0x55      // "enm roh pis stoj fire01 R.i3d" (set 0x200; 0x800, 0x1000, 0x4000, 0x8000)
#define ANIM_ENM_ROH_SAP_STOJ_FIRE01_R 0x56 // "enm roh sap stoj fire01 R.i3d" (set 0x800; 0x1000)
#define ANIM_ENM_ROH_DREP_FIRE01_T 0x57 // "enm roh pis drep fire01 T.i3d" (set 0x4000; 0x8000)
#define ANIM_ENM_ROH_FIRE02_L 0x58      // "enm roh pis stoj fire02 L.i3d" (set 0x200; 0x800, 0x1000, 0x4000, 0x8000)
#define ANIM_ENM_ROH_SAP_STOJ_FIRE02_L 0x59 // "enm roh sap stoj fire02 L.i3d" (set 0x800; 0x1000)
#define ANIM_ENM_ROH_FIRE02_R 0x5a      // "enm roh pis stoj fire02 R.i3d" (set 0x200; 0x800, 0x1000, 0x4000, 0x8000)
#define ANIM_ENM_ROH_SAP_STOJ_FIRE02_R 0x5b // "enm roh sap stoj fire02 R.i3d" (set 0x800; 0x1000)
#define ANIM_ENM_ROH_DREP_FIRE02_T 0x5c // "enm roh pis drep fire02 T.i3d" (set 0x4000; 0x8000)
// Not in _anim_info_ (the name is ours, by value): the AI's log and PersonAnim take it for a jump with 0x5f and 0x60
#define ANIM_UNKNOWN_5E 0x5e
#define ANIM_JUMP1 0x5f                 // "jump1.i3d"
#define ANIM_JUMPRX1 0x60               // "jumpR?1.i3d"
#define ANIM_JUMPSTAT 0x61              // "jumpstat?.i3d"
#define ANIM_JUMPL1 0x62                // "jumpL1.i3d"
#define ANIM_JUMPR1 0x63                // "jumpR1.i3d"
#define ANIM_JUMPL3 0x64                // "jumpL3.i3d"
#define ANIM_JUMPR3 0x65                // "jumpR3.i3d"
#define ANIM_PLACHTENI 0x66             // "!plachteni.i3d"
#define ANIM_DOSKOK 0x67                // "!doskok.i3d"
#define ANIM_NA_BEDNU_06M 0x68          // "na bednu 06m.i3d"
#define ANIM_NA_BEDNU_1M 0x69           // "na bednu 1m.i3d"
#define ANIM_SMRT_PAD_DOZADU 0x6a       // "smrt pad dozadu.I3D"
#define ANIM_SMRT_PAD_VLEVO 0x6b        // "smrt pad vlevo.I3D"
#define ANIM_SMRT_PAD_VPRAVO 0x6c       // "smrt pad vpravo.I3D"
#define ANIM_SMRT_PAD_END_A 0x6d        // "smrt pad end A.I3D"
#define ANIM_SMRT_PAD_END_B 0x6e        // "smrt pad end B.I3D"
#define ANIM_SMRT_PAD_END_VSTAVANI 0x6f // "smrt pad end vstavani.I3D"
#define ANIM_SMRT_PAD_DOPREDU_A 0x70    // "smrt pad dopredu A.i3d"
#define ANIM_SMRT_PAD_DOPREDU_B 0x71    // "smrt pad dopredu B.i3d"
#define ANIM_SMRT_PAD_DOPREDU_C 0x72    // "smrt pad dopredu C.i3d"
#define ANIM_SMRT_STOJ_ZADA_A 0x83      // "smrt stoj zada A.I3D"
#define ANIM_SMRT_STOJ_BRICHO_A 0x86    // "smrt stoj bricho A.i3d"
#define ANIM_SMRT_STOJ_HLAVA_ZEZADU 0x89 // "smrt stoj hlava zezadu.i3d"
#define ANIM_SMRT_STOJ_HLAVA_ZEPREDU 0x8a // "smrt stoj hlava zepredu.I3D"
#define ANIM_SMRT_DREP_ZADA_A 0x8c      // "smrt drep zada A.I3D"
#define ANIM_SMRT_DREP_BRICHO_A 0x8e    // "smrt drep bricho A.I3D"
#define ANIM_SMRT_DREP_HLAVA_ZEZADU 0x90 // "smrt drep hlava zezadu.I3D"
#define ANIM_SMRT_DREP_HLAVA_ZEPREDU 0x91 // "smrt drep hlava zepredu.I3D"
#define ANIM_SMRT_STOJ_BRICHO_T_PAD 0x93 // "smrt stoj bricho T pad.I3D"
#define ANIM_SMRT_STOJ_BRICHO_T_PLAZENI 0x94 // "smrt stoj bricho T plazeni.I3D"
#define ANIM_SMRT_STOJ_BRICHO_T_STATIV 0x95 // "smrt stoj bricho T stativ.I3D"
#define ANIM_SMRT_STOJ_BRICHO_T_END 0x96 // "smrt stoj bricho T end.I3D"
#define ANIM_SMRT_DREP_ZADA_T_END 0xa2  // "smrt drep zada T end.I3D"
#define ANIM_HOD_VRCHEM_HOD 0xa3        // "hod vrchem hod.i3d"
#define ANIM_NOSENI_MRTVOLY_TOM_NAHOZENI 0xa7 // "NoseniMrtvolyTomNahozeni.i3d"
#define ANIM_NOSENI_MRTVOLY_TOM_SHOZENI 0xa8 // "NoseniMrtvolyTomShozeni.i3d"
#define ANIM_BOJ_BASB_Z_LH 0xa9         // "boj basb z LH.i3d"
#define ANIM_BOJ_BASB_Z_RH 0xac         // "boj basb z RH.i3d"
#define ANIM_BOJ_HPT_BASB_LH 0xae       // "boj HPT basb LH.i3d"
#define ANIM_BOJ_NUZ_BOD 0xb2           // "boj nuz bod.i3d"
#define ANIM_BOJ_NUZ_KOMBO 0xb5         // "boj nuz kombo.i3d"
#define ANIM_BOJ_HPT_NUZ_BOD 0xb6       // "boj HPT nuz bod.i3d"
#define ANIM_BOJ_BOX_L_DIRECT 0xb9      // "boj box L direct.i3d"
#define ANIM_BOJ_BOX_L_HAK_SPODEK 0xbc  // "boj box L hak spodek.i3d"
#define ANIM_BOJ_HPT_BOX_L_DIRECT 0xbf  // "boj HPT box L direct.i3d"
#define ANIM_ZASAH_SM_RAMENO_FR 0xc4    // "zasah Sm rameno FR.i3d"
#define ANIM_ZASAH_SM_RAMENO_BR 0xc5    // "zasah Sm rameno BR.i3d"
#define ANIM_ZASAH_SM_RAMENO_FL 0xc6    // "zasah Sm rameno FL.i3d"
#define ANIM_ZASAH_SM_RAMENO_BL 0xc7    // "zasah Sm rameno BL.i3d"
#define ANIM_ZASAH_SM_NOHA_R 0xc8       // "zasah Sm noha R.i3d"
#define ANIM_ZASAH_SM_NOHA_L 0xc9       // "zasah Sm noha L.i3d"
#define ANIM_ZASAH_SM_BRICHO 0xca       // "zasah Sm bricho.i3d"
#define ANIM_ZASAH_SM_ZADA 0xcb         // "zasah Sm zada.i3d"
#define ANIM_ZASAH_SM_HLAVA_F 0xcc      // "zasah Sm hlava F.i3d" and "zasah Lg hlava F.i3d" of the same id
#define ANIM_ZASAH_SM_HLAVA_B 0xcd      // "zasah Sm hlava B.i3d" and "zasah Lg hlava B.i3d" of the same id
#define ANIM_ZASAH_LG_RAMENO_FR 0xce    // "zasah Lg rameno FR.i3d"
#define ANIM_ZASAH_LG_RAMENO_BR 0xcf    // "zasah Lg rameno BR.i3d"
#define ANIM_ZASAH_LG_RAMENO_FL 0xd0    // "zasah Lg rameno FL.i3d"
#define ANIM_ZASAH_LG_RAMENO_BL 0xd1    // "zasah Lg rameno BL.i3d"
#define ANIM_ZASAH_LG_NOHA_R 0xd2       // "zasah Lg noha R.i3d"
#define ANIM_ZASAH_LG_NOHA_L 0xd3       // "zasah Lg noha L.i3d"
// Not in _anim_info_, whose "zasah Lg hlava F.i3d" has the light hit's 0xcc: HitGun's heavy hit in the head from the
// front (the name is a guess, after the file)
#define ANIM_ZASAH_LG_HLAVA_F 0xd4
#define ANIM_ZASAH_LG_HLAVA_B 0xd5      // from behind ("zasah Lg hlava B.i3d" has 0xcd)
#define ANIM_ZASAH_DREP_RAMENO_FR 0xd6  // "zasah Drep rameno FR.i3d"
#define ANIM_ZASAH_DREP_RAMENO_BR 0xd7  // "zasah Drep rameno BR.i3d"
#define ANIM_ZASAH_DREP_RAMENO_FL 0xd8  // "zasah Drep rameno FL.i3d"
#define ANIM_ZASAH_DREP_RAMENO_BL 0xd9  // "zasah Drep rameno BL.i3d"
#define ANIM_ZASAH_DREP_NOHA_R 0xda     // "zasah Drep noha R.i3d"
#define ANIM_ZASAH_DREP_NOHA_L 0xdb     // "zasah Drep noha L.i3d"
#define ANIM_ZASAH_DREP_BRICHO 0xdc     // "zasah Drep bricho.i3d"
#define ANIM_ZASAH_DREP_ZADA 0xdd       // "zasah Drep zada.i3d"
#define ANIM_ZASAH_SM_DREP_HLAVA_F 0xde // "zasah Sm Drep hlava F.i3d"
#define ANIM_ZASAH_SM_DREP_HLAVA_B 0xdf // "zasah Sm Drep hlava B.i3d"
#define ANIM_ZASAH_LG_DREP_HLAVA_F 0xe0 // "zasah Lg Drep hlava F.i3d"
#define ANIM_ZASAH_LG_DREP_HLAVA_B 0xe1 // "zasah Lg Drep hlava B.i3d"
#define ANIM_ZASAH_HPT_NOGUNF 0xe2      // "zasah HPT nogunF.i3d"
#define ANIM_ZASAH_HPT_PISTOL1F 0xe4    // "zasah HPT pistol1F.i3d"
#define ANIM_ZASAH_HPT_PUSKA1F 0xe6     // "zasah HPT puska1F.i3d"
#define ANIM_ZASAH_HPT_SAPIKF 0xe8      // "zasah HPT sapikF.i3d"
#define ANIM_ZASAH_HPT_PUSKA2F 0xea     // "zasah HPT puska2F.i3d"
#define ANIM_BOJ_ZASAH_HL_D 0xec        // "boj zasah hl D.i3d"
#define ANIM_BOJ_ZASAH_HL_L 0xed        // "boj zasah hl L.i3d"
#define ANIM_BOJ_ZASAH_HL_R 0xee        // "boj zasah hl R.i3d"
#define ANIM_BOJ_ZASAH_BR_L 0xef        // "boj zasah br L.i3d"
#define ANIM_BOJ_ZASAH_BR_R 0xf0        // "boj zasah br R.i3d"
#define ANIM_BOJ_BOX_ROZKROK 0xf1       // "boj box rozkrok.i3d"
#define ANIM_BOJ_ZASAH_ROZKROK_SM 0xf2  // "boj zasah rozkrok sm.i3d"
#define ANIM_BOJ_BOX_HLAVICKA 0xf3      // "boj box hlavicka.i3d"
#define ANIM_BOJ_ZASAH_HLAVICKA 0xf4    // "boj zasah hlavicka.i3d"
#define ANIM_KRADENI_AUTA01 0xf5        // "KradeniAuta01.i3d"
#define ANIM_AUTO_RIDIC_STATIV 0xf6     // "AutoRidicStativ.i3d"
#define ANIM_FORMULE_STATIV 0xf7        // "FormuleStativ.i3d"
#define ANIM_AUTO_SPOL_STATIV 0xf8      // "AutoSpolStativ.i3d"
#define ANIM_AUTO_RIDIC_VOLANT 0xf9     // "AutoRidicVolant.i3d"
#define ANIM_FORMULE_VOLANT 0xfa        // "FormuleVolant.i3d"
#define ANIM_AUTO_FIRE01_L_ON 0xfb      // "Auto fire01 L On.i3d"
#define ANIM_AUTO_FIRE01_L_OFF 0xfc     // "Auto fire01 L Off.i3d"
#define ANIM_AUTO_FIRE01_L 0xfd         // "Auto fire01 L.i3d"
#define ANIM_AUTO_FIRE03_ON 0x107       // "Auto fire03 On.i3d"
#define ANIM_AUTO_FIRE03_OFF 0x108      // "Auto fire03 Off.i3d"
#define ANIM_AUTO_FIRE03_L 0x109        // "Auto fire03 L.i3d"
#define ANIM_SMRT_AUTO_RIDIC01 0x10a    // "smrt auto ridic01.i3d"
#define ANIM_SMRT_AUTO_SPOL_IN01 0x10b  // "smrt auto spol IN01.i3d"
#define ANIM_SMRT_AUTO_SPOL_IN02 0x10c  // "smrt auto spol IN02.i3d"
#define ANIM_SMRT_AUTO_SPOL_OUTL 0x10d  // "smrt auto spol OutL.i3d"
#define ANIM_SMRT_AUTO_SPOL_OUTR 0x10e  // "smrt auto spol OutR.i3d"
#define ANIM_AUTO_SM_NAS_FL 0x10f       // "AutoSmNas FL.i3d"
#define ANIM_AUTO_LEZENI_Z_VRAKU 0x12f  // "Auto lezeni z vraku.i3d"
#define ANIM_AUTO_PRELEZ_L_TO_R 0x130   // "AutoPrelezLtoR.i3d"
#define ANIM_AUTO_PRELEZ_R_TO_L 0x131   // "AutoPrelezRtoL.i3d"
#define ANIM_VYHOZ_MAN_FRONT_L 0x132    // "VyhozManFrontL.i3d"
#define ANIM_VYHOZ_MAN_BACK_L 0x133     // "VyhozManBackL.i3d"
#define ANIM_VYHOZ_FRONT_L 0x134        // "VyhozFrontL.i3d"
#define ANIM_VYHOZ_FRONT_R 0x135        // "VyhozFrontR.i3d"
#define ANIM_VYHOZ_BACK_L 0x136         // "VyhozBackL.i3d"
#define ANIM_VYHOZ_BACK_R 0x137         // "VyhozBackR.i3d"
#define ANIM_VYHOZ_MRT_FRONT_L 0x138    // "VyhozMrtFrontL.i3d"
#define ANIM_VYHOZ_MRT_FRONT_R 0x139    // "VyhozMrtFrontR.i3d"
#define ANIM_VYHOZ_MRT_BACK_L 0x13a     // "VyhozMrtBackL.i3d"
#define ANIM_VYHOZ_MRT_BACK_R 0x13b     // "VyhozMrtBackR.i3d"
#define ANIM_FORMULE_NASTUP_L 0x13c     // "FormuleNastupL.i3d"
#define ANIM_FORMULE_VYSTUP_L 0x13d     // "FormuleVystupL.i3d"
#define ANIM_FORMULE_VYSTUP_R 0x13f     // "FormuleVystupR.i3d"
#define ANIM_SALINA_NASTUP 0x140        // "salina nastup.i3d"
#define ANIM_SALINA_VYSTUP 0x141        // "salina vystup.i3d"
#define ANIM_USKOK_RIGHT_FRONT 0x142    // "Uskok Right Front.I3D"
#define ANIM_USKOK_LEFT_BACK 0x144      // "Uskok Left Back.I3D"
#define ANIM_USKOK_FRONT_LEFT 0x146     // "Uskok Front Left.I3D"
#define ANIM_USKOK_BACK_RIGHT 0x148     // "Uskok Back Right.I3D"
#define ANIM_USKOK_FRONT_RIGHT 0x149    // "Uskok Front Right.I3D"

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
// Turns the vector about the y axis (PS2 names); RotateDir makes it horizontal and normalizes it after
void RotateDir(S_vector &v, float angle);
void RotateDirNoNorm(S_vector &v, float angle);
// Builds the animations' index and sorts it (PS2 C_actor.cpp), once, for InitSystem
void InitAnimIndex();
// Frees the index, at CloseSystem
void DoneAnimIndex();
// The index of the animation of that id in _anim_info_, -1 for none (PS2 name), a binary search of _anim_index_
int GetAnimIndex(uint id);
