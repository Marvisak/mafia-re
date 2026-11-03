#pragma once

#include "actors/C_actor.h"
#include "actors/C_animations.h"
#include "actors/g_inventory.h"
#include "tools.h"

class C_bridge;
class C_car;
class C_car_ex;
class I3D_sound;
class C_human;
class C_traffic_element;

// The bones the shots test against (PS2 C_ShotSkeleton; its file's name isn't known, PC 0x5d6f80-0x5d767c), 0x130 bytes
// as far as C_human shows
class C_ShotSkeleton
{
public:
    // The line from from along dir, of that length squared: the bone hit, 0 for none (PS2 name; the parameters' names
    // and the result are ours)
    int TestCollision(const S_vector &from, const S_vector &dir, float dist2);

    // Empty, in the class: C_human's constructor and destructor unwind the skeleton through the linker's folded
    // empty function of that kind (0x404990), and construct nothing
    ~C_ShotSkeleton()
    {
    }
    // Takes the bones of the human's model (PS2 name), which GameInit and _intern_ChangeModel call
    void Init(I3D_frame *frm);

    uchar unknown_0[0x130];
};

// The humans' properties, max_properties' and properties' indices: _human_property_'s (C_program.cpp), whose names
// the script's HUMAN_GETPROPERTY and HUMAN_SETPROPERTY take (the name is ours)
#define HUMAN_PROPERTY_STRENGTH 0       // "Strenght"
#define HUMAN_PROPERTY_ENERGY 1         // "Energy", the health
#define HUMAN_PROPERTY_ENERGY_HAND_L 2  // "EnergyHandL"
#define HUMAN_PROPERTY_ENERGY_HAND_R 3  // "EnergyHandR"
#define HUMAN_PROPERTY_ENERGY_LEG_L 4   // "EnergyLegL"
#define HUMAN_PROPERTY_ENERGY_LEG_R 5   // "EnergyLegR"

// The body parts a hit takes (C_human::Hit's part, the bone C_ShotSkeleton::TestCollision gives a shot; the names are
// ours): Hit takes a shot's damage from the energy of the hand or leg too, 0.3 of it for a hand and 0.5 for a leg, and
// 1.5 times it from the energy for the head
#define HIT_PART_HAND_R 1
#define HIT_PART_HAND_L 2
#define HIT_PART_LEG_R 3
#define HIT_PART_LEG_L 4
#define HIT_PART_BODY 5
#define HIT_PART_HEAD 6

// C_human::human_type's values (the names are ours, by value where the meaning isn't known): Hit leaves out the type 2
// humans while the game's GAMEF_INVULNERABLE is set; the police (C_shvestky::TickDoRiciActors dismisses the type 4
// entities) give the score for a kill without a bonus
#define HUMAN_TYPE_UNKNOWN_2 2
#define HUMAN_TYPE_POLICE 4

// C_human::unknown_94's bit with which an entity gets EVENT_HIT_FROM_PLAYER when the player hits it (the name is ours)
#define HUMANF_EVENT_HIT_FROM_PLAYER 0x2

// The effects' particle of Hit's type 7 at the human's frame (the name is ours, by value)
#define HITPARTICLE_UNKNOWN_28 0x28

// The sound table's sound of a human a car or a train runs into (C_human::Collision; the name is ours)
#define SOUND_RUN_OVER 0xb8
#define HUMAN_PROPERTY_REACTIONS 6      // "Reactions"
#define HUMAN_PROPERTY_SPEED 7          // "Speed", which the walks' speeds are multiplied by
#define HUMAN_PROPERTY_AGGRESIVITY 8    // "Aggresivity"
#define HUMAN_PROPERTY_INTELLIGENCE 9   // "Intelligence"
#define HUMAN_PROPERTY_SHOOTING 10      // "Shooting"
#define HUMAN_PROPERTY_SIGHT 11         // "Sight"
#define HUMAN_PROPERTY_HEARING 12       // "Hearing"
#define HUMAN_PROPERTY_DRIVING 13       // "Driving"
#define HUMAN_PROPERTY_MASS 14          // "Mass"
// The 16th float, which no name reaches: the constructor and the old versions' readers make it 0.5 (the name is
// ours, by index)
#define HUMAN_PROPERTY_15 15

// The version of the mission's character data SaveData writes and LoadData reads (scene2.bin; the name is ours)
#define HUMAN_DATA_VERSION 6
// The version of the saved game's data SaveGameSave writes, the only one SaveGameLoad takes (the name is ours)
#define HUMAN_SAVEGAME_VERSION 6

// One of C_human's five entries at 0x23c (PS2 0x3dc), 0x14 bytes (the names are placeholders)
struct S_unknown_23c
{
    int unknown_0;                      // 0x0, 3: Update ends the animation with the human's action
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    bool unknown_c;                     // 0xc
    uchar unknown_d[3];                 // 0xd
    int unknown_10;                     // 0x10
};

// A step of what a weapon's fire does (PS2's _fire_action_*_ lists, file statics of C_human.cpp; the struct's and the
// members' names are ours): each list ends with FIRE_ACTION_END
struct S_fire_action
{
    int action;                         // 0x0
    int time;                           // 0x4, PS2's reads a ushort
};
#define FIRE_ACTION_END 3               // the lists' last step (the name is ours)

// gameItemTable's item 1 (g_material.h), which ChangeState puts on a dead body's using objects, with the values 0x7dc
// and 0x7dd where a weapon has its ammunition (the name is ours, by value)
#define GAMEITEM_UNKNOWN_1 1

// What a human made of a pedestrian of the traffic keeps of it (PS2 0x750; the struct's and the members' names are
// ours), which GameDone deletes: the pedestrian is put back at the human's place, and the frame isn't the scene's
struct S_human_traffic
{
    bool traffic_frame;                 // 0x0, the frame is the pedestrian's model: GameDone releases it
    C_traffic_element *element;         // 0x4, the pedestrian, NULL for none
};

// What ai_loc keeps of a special of the AI's segments the human was in (PS2 ai_seg_stat, ai_loc's vector's), 0x10
// bytes: AddSegment's (the names are ours)
struct ai_seg_stat
{
    uint type;                          // 0x0, the special's type (check_seg::GetSpecial's)
    uint time;                          // 0x4, the game time it was last found
    int count;                          // 0x8, how often it was found
    void *special;                      // 0xc, the special (check_seg::GetSpecial's data), which AddSegment finds it by
};

// ai_loc::flags (the names are guesses): set by each update, the node found and the position in one of its
// connections, or the node only
#define AILOC_UPDATED 0x1
#define AILOC_NODE 0x2
#define AILOC_CONN 0x10

// Where a human is on the walking paths' web and in the AI's segments (PS2 ai_loc, whose methods are at the start of
// PS2's C_human.cpp, as the PC's), 0x34 bytes, C_human's at 0x724 (the members' names are ours)
class ai_loc
{
public:
    S_vector pos;                       // 0x0, the position of the last update
    uint node;                          // 0xc, the web's node there (ai_wayfind::GetPosition's)
    uint node2;                         // 0x10, the other node of the connection the position is in
    uint conn;                          // 0x14, that connection's index among node's
    uint flags;                         // 0x18, AILOC_*
    uint time;                          // 0x1c, the game time of the last update
    std::vector<ai_seg_stat> segs;      // 0x20, the specials the human was in lately
    C_actor *actor;                     // 0x30, Init's

    // PS2 names
    void Init(C_actor *act);
    void Update();
    void AddSegment(uint type, void *special);
    void SegListUpdate();
};

// A human's position and direction at a time (PS2 ai_blob, 0x30 bytes, its vectors 0x10; PS2's has an operator new of
// its own, at the start of its C_human.cpp, the PC's original calls the global one), 0x24 bytes: ai_blob_log keeps the
// last 16 of a human, and the fights' Tick tells the members of a party in a car about its target with one, each
// member's C_entity::Event getting a copy
struct ai_blob
{
    S_vector pos;                       // 0x0, a metre above the frame's position in the log; the target's position
    S_vector dir;                       // 0xc, from the last position in the log, or the frame's direction
    uint time;                          // 0x18, the game's time
    uint flags;                         // 0x1c, AIBLOB_*, 0 from the fights
    C_actor *actor;                     // 0x20, the human (Blob's owner), the fights' target
};

// ai_blob::flags (the names are guesses, by value but the crouch): the human's state when he was logged
#define AIBLOB_UNKNOWN_1 0x1            // animation 5
#define AIBLOB_CROUCHING 0x2            // C_human::crouching
#define AIBLOB_UNKNOWN_4 0x4            // C_human::unknown_1ec
#define AIBLOB_UNKNOWN_8 0x8            // animations 0x5e and 0x5f; 0x60 sets 9

// The last 16 positions of a human, logged at most every 500 ms (PS2 ai_blob_log, whose methods are at the start of
// PS2's C_human.cpp, as the PC's), 0x244 bytes, C_human's at 0x758; Main.cpp and jp_ai_entity.cpp use them too (the
// members' names are ours)
class ai_blob_log
{
public:
    ai_blob blobs[16];                  // 0x0
    int last;                           // 0x240, the last one's (mod 16), -1 for none (Clear's)

    // PS2 names
    void Blob(void *owner);
    // A copy of the one logged that many before the last, NULL if none: the caller deletes it (0x56ee60)
    ai_blob *GetBlob(uint back);
    // Empties the log (PS2 name, 0x56eeb0)
    void Clear();
};

// C_human::anim_set (the names are guesses, after the animations': "enm klid" is the enemy's calm stance, "panika"
// the panic's)
#define HUMAN_ANIMSET_CALM 1
#define HUMAN_ANIMSET_2 2
#define HUMAN_ANIMSET_PANIC 3

// The height of the ground below pos, from a metre above it down 6 metres, relative to pos: 0 for none (PS2 names, in
// PS2's C_human.cpp after ai_blob_log; the second leaves the moving objects out). The player's file calls them too.
float _get_delta_y_(S_vector &pos);
float _get_delta_nodyn_y_(S_vector &pos);

// SYNTHETIC: GAME 0x56f330
// ??1ai_logs@@QAE@XZ

// The saved game's data of a human, after C_actor's (C_human::SaveGameSave's and SaveGameLoad's), 0x17e bytes, packed:
// the members the fields copy, in their order but for the properties, each field with its member's name (PS2's has the
// same fields, the bytes and shorts last; the struct's name is ours). action_state and unknown_410 are saved as words.
#pragma pack(push, 1)
struct S_human_savegame
{
    uchar version;                      // 0x0, HUMAN_SAVEGAME_VERSION
    S_vector pos;                       // 0x1, C_actor's
    S_vector dir;                       // 0xd, C_actor's
    int anim_id;                        // 0x19, which SaveGameLoad puts into next_anim_id
    int anim_set;                     // 0x1d
    int seat;                           // 0x21
    bool crouching;                     // 0x25
    bool aimed;                         // 0x26
    bool anim_left;                   // 0x27
    bool unknown_1ec;                   // 0x28
    S_vector unknown_200;               // 0x29
    int unknown_218;                    // 0x35
    bool unknown_21c;                   // 0x39
    int unknown_220;                    // 0x3a
    int unknown_228;                    // 0x3e
    bool unknown_22c;                   // 0x42
    bool unknown_22d;                   // 0x43
    float tilt;                         // 0x44
    bool unknown_2b0;                   // 0x48
    S_vector unknown_2b4;               // 0x49
    S_vector unknown_2c0;               // 0x55
    S_vector unknown_2cc;               // 0x61
    ushort action_state;                // 0x6d
    ushort unknown_410;                 // 0x6f
    float fall;                         // 0x71
    S_vector unknown_420;               // 0x75
    float unknown_42c;                  // 0x81
    S_vector unknown_430;               // 0x85
    float unknown_43c;                  // 0x91
    S_vector unknown_440;               // 0x95
    int unknown_44c;                    // 0xa1
    int unknown_450;                    // 0xa5
    int agony_anim;                    // 0xa9
    int unknown_45c;                    // 0xad
    int unknown_460;                    // 0xb1
    int unknown_464;                    // 0xb5
    S_vector unknown_468;               // 0xb9
    int unknown_584;                    // 0xc5
    int idle_time;                    // 0xc9
    int breath_time;                    // 0xcd
    uchar unknown_d1[0x14];             // 0xd1, neither function touches it (PS2's struct has the gap too)
    float properties[16];               // 0xe5
    float max_properties[16];           // 0x125
    int ground_material;                    // 0x165
    int wingman;                        // 0x169
    uint wingman_name;                  // 0x16d
    int bleed_count;                    // 0x171
    int bleed_time;                    // 0x175
    ushort unknown_6ec;                 // 0x179
    ushort unknown_6ee;                 // 0x17b
    uchar unknown_6f0;                  // 0x17d
};
#pragma pack(pop)

// The humans (PS2 C_human, C_human.cpp), the base of the player and of the AI's C_entity, 0xa9c bytes. Its file
// (0x56b180-0x58e34c) starts with the PC only functions of the cutscenes and their helpers, then the constructor, then
// PS2's functions in PS2's order. The members not named yet are placeholders by offset, typed by how the file's code
// reads them.
// VTABLE: GAME 0x6256f8
class C_human : public C_actor
{
public:
    // The corners a human looks, fires and breathes around (PS2 C_human::E_corner_view; the values aren't known yet)
    enum E_corner_view
    {
        CORNER_0,
        CORNER_1,
        CORNER_2,               // crouched only
    };

    // The cutscenes' functions (PC only; the slots' names are C_actor's)
    virtual bool vfunc_9();
    virtual int vfunc_0(int a);
    virtual void vfunc_1(int a);
    virtual void vfunc_2(int a);
    virtual bool vfunc_3(int a);
    virtual void vfunc_12();
    virtual S_vector vfunc_4(int a);
    virtual S_vector vfunc_5(int a);
    // A callback the animations call, which GameInit and _intern_ChangeModel pass (PC only, 0x56d130; four arguments on
    // the stack)
    static uint __stdcall Unknown56d130(I3D_frame *frm, LS3D_CALLBACK_MESSAGE message, uint param1, uint param2);

    C_human();
    virtual ~C_human();
    virtual bool Init(I3D_frame *frm);
    virtual void DuplicateFrom(C_actor *act);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void Death();
    void Extern_AI_Update(uint time);
    virtual void SoundsChange(int i);
    virtual void AI(uint time);
    // Gets into the car, the railway's wagon (PS2 names)
    void Intern_UseCar(bool b);
    void Intern_UseRailway(bool b);
    // Gets into the truck, HUMAN_ENTERTOTRUCK's
    void _intern_UseCarTruck(C_car_ex *car, bool unknown);
    // PS2's takes the vector by reference, the PC's four dwords
    void ThrowFromCarTruck(S_vector dir, float f);
    virtual void Update(uint time);
    void ChangeDeadAnim();
    void PersonAnim(float f, bool b1, bool b2);
    // Use_Actor's (PS2 name)
    void ShiftAnimation(int note);
    // The linker's folded empty function of the out-of-line kind (0x4ab130)
    virtual void NetDirect(uint time);
    virtual void ChangeState();
    virtual void GameInit();
    virtual void GameDone();
    // The human's own collisions (PS2 names)
    void CreateDynColls();
    void EraseDynColls();
    void DeactivateColls(bool b);
    void ActivateColls(int i);
    void UpdateDynColl();
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    // The hit's animations (PS2 names), inline on the PC: Hit has them expanded, HitGun three times
    inline void HitLanding();
    bool HitInCar(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act, uint u,
        I3D_frame *frm);
    inline void HitAgony(const S_vector &dir);
    inline void HitStroke(const S_vector &dir, uint part);
    inline void HitGun(const S_vector &dir, uint part, bool heavy);
    inline void HitGunHPT(const S_vector &dir, uint part, bool heavy);
    inline void HitFall(const S_vector &dir, uint part);
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll);
    // PC only, which vfunc_3 calls (0x578e20)
    void Unknown578e20(int a, int b);
    // The poses (PS2 names); the PC's take the vectors by value, PS2's by reference
    void PoseInterpolate(uint time);
    void PoseSetPoseNormal(S_vector pos);
    void PoseSetPoseAimed(S_vector pos);
    void PoseNullRotate();
    void PoseValidate(bool valid);
    // PS2 name; static, as the PC's two arguments in registers show
    static int GetAnimIDForDeath(int i, bool b, const S_vector &v1, const S_vector &v2);
    void Movement(uint time);
    void MovementIntern(uint time);
    // Nonzero if the move hits (the PC's tests eax)
    int TestMoveColl(const S_vector &from, const S_vector &to, S_vector &result);
    void DynSetNextCollOwnerMult();
    int TestMoveCollInOwner(const S_vector &from, const S_vector &to, S_vector &result);
    void Do_StopAnim();
    void Do_Breath();
    // The walks (PS2 names)
    void Go_Front(bool b);
    void Go_Back();
    void Go_Left(bool b);
    void Go_Right(bool b);
    void Go_FrontLeft(bool b);
    void Go_FrontRight(bool b);
    void Go_BackLeft();
    void Go_BackRight();
    // The animations replaced, the second NULL where the script gives none
    void ChangeAnim(char *anim1, char *anim2);
    // PS2 name (PS2's takes six bools, the PC's four dwords on the stack besides the two in registers)
    static int GetAnimIDForUseCar(bool b1, bool b2, bool b3, bool b4, bool b5, bool b6);
    // Gets into the car's seat
    void Intern_UseCar(C_car *car, int seat);
    // Gets out of the car the human is in (PS2 name), which G_LoadSaveClass::GarageGameDone asks of the garage cars'
    // owners with the car's frame
    void Intern_FromCar(I3D_frame *frm);
    void RepairWeaponPosition();
    void ChangeWeaponModel();
    void Do_ChangeWeapon(int i, bool b);
    void Do_Aimed(bool aim);
    void Do_Crouched(bool crouch);
    void InventoryTruncate();
    // Drops the items out of the inventory
    bool DropOutItems(std::vector<S_GameItem> &items);
    // PS2 names, Czech: "so I've found that highest bridge", the car functions, "the crappy climbing out of the car in
    // the morning" and its "throwing up" one
    // The bridge the human's car is on, within 100 m, NULL for none (Czech: "so I've found that highest bridge")
    C_bridge *TozNaselJsemTenNejvyssiMost();
    bool Can_DropOutFromCar_Free(C_car *car, int i);
    bool Can_DropOutFromCar(int i);
    bool Can_EnterToCar(C_car *car, int i, bool b);
    void ZasrateLezeniZAuta_ve2rano();
    void ZasrateVybliknutiZAuta_ve3rano();
    void Do_ClimbInCarLR();
    bool Use_Actor(C_actor *act, int a, int b, int c);
    // The weapons (PS2 names)
    void Do_Shoot(bool b, S_vector *dir);
    bool Do_ThrowGrenade(S_vector &dir);
    // PS2's takes a bool, the PC's nothing
    bool Can_SpecialStroke();
    // The kicks between the legs and the head-butt at the human: false where he can't be hit so
    bool Do_Castrate(C_human *human);
    bool Do_HardHead(C_human *human);
    void Shooting();
    void Do_Reload();
    void Do_Holster();
    void Do_WeaponDrop();
    int Do_PlayAnim(const char *anim, bool simple, bool unknown);
    virtual bool GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm);
    // PS2 names; the PC's Do_Jump takes nothing, PS2's two bools
    void Do_Jump();
    bool Do_Climb();
    void Do_SideJump(const S_vector &v1, const S_vector &v2);
    // The model changed for the named one
    void _intern_ChangeModel(char *name, bool unknown);
    // The humans' force feedback effects, which C_game's Init and Done create and free when the profile has them (PC
    // only, the names are ours)
    static void InitFFEffects();
    static void DoneFFEffects();
    void _intern_ForceDeath();
    void Do_ShockStart(int i);
    void Do_ShockDown(int i);
    void Do_ShockUp();
    void Do_ShockStumble();
    virtual void cbCollTester(tDynamicCollObject *obj);
    void Do_ThrowCocotFromCar(C_car *car, int i);
    void _intern_ThrowMeFromCar();
    // The box and the dead body carried (PS2 names)
    void Do_FuckingBox(I3D_frame *frm);
    // False where the box can't be put down here (the return's meaning is a guess)
    bool Do_FuckingBoxCancel();
    void Do_DeadBody(I3D_frame *frm);
    // PS2 name, German: "my little car in the air"
    void EineMeineKleineAutoInLuft(C_car *car);
    // PS2's takes an int, the PC's nothing
    bool Can_Aimed();
    bool Do_CornerBreath(E_corner_view view);
    bool Do_CornerView(E_corner_view view);
    bool Do_CornerFire(E_corner_view view);
    bool Do_CornerFireAimed(E_corner_view view);
    void Force_CanWorking();
    void Turn_Turn();
    // The human taken out of the game and back into it (PS2 names), which C_mission's FrmActBuff_StoreActor and
    // FrmActBuff_GetActor call for a kept human
    void Activate();
    void Deactivate();
    void NewScream(uint id);
    void Do_FuckingBoxDrop();
    void Do_DeadBodyDrop();
    void ResetActions();
    // The death's position (PS2 names), with three helpers of the PC's RecompileDeathPos (0x58bf30, 0x58c4f0,
    // 0x58c980)
    void RecompileDeathPos();
    void Unknown58bf30(int a);
    void Unknown58c4f0(int a);
    void Unknown58c980(int a, int b, int c, int d, int e, int f, int g, int h, int i);
    void ProcessDeathPos(uint time);
    // Whether the player or an entity is in the car or stealing it (PS2 name), which
    // C_traffic_generator::TryGoToAssHole asks
    static bool IsThisCarStealing(C_car *car);
    void FreeHand();
    // The weapons' effects (PS2 names; Czech: "without the magazine")
    void BezZasobnikuWeaponEffect(int i, I3D_model *model);
    void StartWeaponEffect(int i);
    void TickWeaponEffect(uint time);
    void ResetWeaponEffects();
    void FakeWeaponEffect(I3D_frame *frm, int i, bool b);
    // Hides or shows a driver's legs (PS2 name): empty on the PC
    static void HideLegs(I3D_model *model, bool hide);
    // Whether the shooter's shot can hit the human (PS2 name; the parameters' names are ours)
    static bool Can_Hit_FilterHumanType(C_human *shooter, C_human *human);
    // The human's speed (0x51af20, in jp_ai_entity.cpp's range)
    virtual void GetActorSpeed(S_vector &speed);

    // The shot's line against the human's bones (C_ShotSkeleton::TestCollision), the line by value (the name is ours:
    // TickShoot's call copies both vectors first, as an inline function taking them by value does)
    int TestShotCollision(S_vector from, S_vector dir, float dist2)
    {
        return shot_skeleton.TestCollision(from, dir, dist2);
    }

    int anim_id;                        // 0x70 (PS2 0x200), HUMAN_GETACTANIMID's (the name is ours)
    // 0x74 (PS2 0x204), the animation PersonAnim plays next, as anim_id (the name is ours)
    int next_anim_id;
    // 0x78 (PS2 0x208), HUMAN_ANIMSET_*: the set of animations PersonAnim takes the stances and walks of (the
    // name is ours)
    int anim_set;
    // 0x7c, the animations ChangeAnim replaced, which PersonAnim looks the id up in first, the last first (the name
    // is ours)
    std::vector<S_anim_info> anims;
    float unknown_8c;                   // 0x8c
    float unknown_90;                   // 0x90
    int unknown_94;                     // 0x94
    C_car *car;                         // 0x98, the car the human is in (the name is ours)
    // 0x9c (PS2 0x228), the car the human is getting into or was thrown out of: PS2's Use_Actor sets it to the car
    // it uses, _intern_ThrowMeFromCar to the car, the car functions clear it; C_shvestky::TickDoRiciActors keeps a car
    // in the police's faces while a player or an entity has it here or in car (the name is ours)
    C_car *using_car;
    int unknown_a0;                     // 0xa0
    // The cars the script's CAR_LOCK and CAR_LOCK_ALL lock to the human (the names are ours)
    C_car *locked_car;                  // 0xa4
    C_car *locked_vehicle;              // 0xa8
    // 0xac (PS2 0x238), the seat in the car most likely, -1 for none (the name is a guess)
    int seat;
    int unknown_b0;                     // 0xb0
    int unknown_b4;                     // 0xb4
    int unknown_b8;                     // 0xb8
    // 0xbc (PS2 0x250), whose Reset the script's ACTOR_SETPLACEMENT calls (the name is ours)
    C_animations_machine_de_luxe anim_machine;
    bool crouching;                     // 0x1e4 (PS2 0x390), the camera drops while it is set (the name is ours)
    // 0x1e5 (PS2 0x391), the weapon aimed: PS2's Do_Aimed sets it to its argument, Do_Shoot and the corner fire set
    // it, a reload or a weapon change clears it (the name is ours, after Do_Aimed)
    bool aimed;
    // 0x1e6 (PS2 0x392), the side of the animations whose name has a '?': L while it is set, else R (the name is ours)
    bool anim_left;
    uchar unknown_1e7;                  // 0x1e7
    int unknown_1e8;                    // 0x1e8
    bool unknown_1ec;                   // 0x1ec
    bool unknown_1ed;                   // 0x1ed
    uchar unknown_1ee[2];               // 0x1ee
    // What the weapon's fire does (PS2 0x39c, 0x3a0 and 0x3a4; the names are ours): GameInit, Activate and
    // ResetWeaponEffects set _fire_action_ready_ and clear the step and its time
    const S_fire_action *fire_action;   // 0x1f0
    int fire_step;                      // 0x1f4
    int fire_time;                      // 0x1f8
    bool unknown_1fc;                   // 0x1fc
    bool unknown_1fd;                   // 0x1fd
    bool unknown_1fe;                   // 0x1fe
    uchar unknown_1ff;                  // 0x1ff
    S_vector unknown_200;               // 0x200
    int unknown_20c;                    // 0x20c
    int unknown_210;                    // 0x210
    int unknown_214;                    // 0x214
    int unknown_218;                    // 0x218, saved as a dword
    bool unknown_21c;                   // 0x21c
    uchar unknown_21d[3];               // 0x21d
    int unknown_220;                    // 0x220
    uchar unknown_224[4];               // 0x224
    int unknown_228;                    // 0x228
    bool unknown_22c;                   // 0x22c
    bool unknown_22d;                   // 0x22d
    bool unknown_22e;                   // 0x22e
    bool unknown_22f;                   // 0x22f
    bool unknown_230;                   // 0x230
    uchar unknown_231[3];               // 0x231
    int unknown_234;                    // 0x234
    // 0x238 (PS2 0x3d8), the human's look up or down, which PS2's C_player::AI_walk moves: the camera's tilt for the
    // player (C_game::Tick; the name is ours, after the camera's)
    float tilt;
    // 0x23c (PS2 0x3dc), five entries of 0x14 bytes, the one unknown_2a0 indexes: Update ends the animation with the
    // action of its unknown_0 3, else sets action_state 6
    S_unknown_23c unknown_23c[5];
    int unknown_2a0;                    // 0x2a0
    // The poses of the model's frames "back1", "back2" and "neck" (PS2 0x444, 0x448 and 0x44c), which GameInit
    // creates and GameDone releases (the names are ours)
    I3D_pose *pose_back1;               // 0x2a4
    I3D_pose *pose_back2;               // 0x2a8
    I3D_pose *pose_neck;                // 0x2ac
    bool unknown_2b0;                   // 0x2b0
    uchar unknown_2b1[3];               // 0x2b1
    S_vector unknown_2b4;               // 0x2b4
    S_vector unknown_2c0;               // 0x2c0
    S_vector unknown_2cc;               // 0x2cc
    I3D_frame *look_frame;              // 0x2d8 (PS2 0x454), SetLookFrame's (the name is ours)
    C_ShotSkeleton shot_skeleton;       // 0x2dc (PS2 0x458; the name is ours)
    // 0x40c (PS2 0x588), what the human does, which PS2's Go_*, Do_*, Hit* and car functions set and Movement switches
    // on: 1, 2 and 9 are ready, 9 keeps the car (the name is ours)
    int action_state;
    int unknown_410;                    // 0x410
    int unknown_414;                    // 0x414
    // 0x418 (PS2 0x594), the human's fall, 0 while he stands: HUMAN_FORCEFALL adds 0.1, PS2's Movement starts one at
    // 0.05 and makes it 60 below a height of -30 (the name is ours)
    float fall;
    bool unknown_41c;                   // 0x41c
    uchar unknown_41d[3];               // 0x41d
    S_vector unknown_420;               // 0x420
    float unknown_42c;                  // 0x42c (PS2 0x59c), the turn's speed: Update turns dir by it times the time
    S_vector unknown_430;               // 0x430 (PS2 0xd0), cleared by Turn_Turn
    float unknown_43c;                  // 0x43c (PS2 0x5a0), cleared by Turn_Turn
    S_vector unknown_440;               // 0x440
    int unknown_44c;                    // 0x44c
    int unknown_450;                    // 0x450
    bool killed_by_shot;                // 0x454 (PS2 0x5ac), set by Hit for a death by HIT_SHOT (the name is ours)
    uchar unknown_455[3];               // 0x455
    // 0x458 (PS2 0x5b0), the agony's animation, 0 to 12 by 4, added to 0x93 for its first (PS2's HitAgony sets it), -1
    // for none: Hit plays its death on a hit in the agony (the name is ours)
    int agony_anim;
    int unknown_45c;                    // 0x45c, saved as a dword
    int unknown_460;                    // 0x460
    int unknown_464;                    // 0x464 (PS2 0x5bc), ACTOR_SETPLACEMENT clears it
    // 0x468, the position the animation's track moves the human from: PersonAnim sets it with unknown_464 1, Update
    // moves it to where the collisions let him go
    S_vector unknown_468;
    uchar unknown_474[4];               // 0x474
    C_car *stealing_car;                // 0x478 (PS2 0x5c4), IFPLAYERSTEALCAR's (the name is ours)
    int shadow;                         // 0x47c (PS2 0x5c8), C_game::AddFrameForShadow's (the name is ours)
    G_Inventory inventory;              // 0x480 (the name is ours)
    // The weapons' models, FRAME_MODELs named "weapon" (PS2 0x5f0 and 0x5f4): Update turns them off in a car, Hit
    // on in action_state 10 (the names are ours)
    I3D_model *weapon_model;            // 0x564
    I3D_model *weapon_model2;           // 0x568
    // The frames the hands hold things by, LINKTOHAND's (PS2 0x5f8 and 0x5fc; the names are ours)
    I3D_frame *right_hand;              // 0x56c
    I3D_frame *left_hand;               // 0x570
    // FRAME_TARGETs GameInit links to the left and the right hand, NULL without the hand (PS2 0x600 and 0x604;
    // the names are ours)
    I3D_frame *left_hand_target;        // 0x574
    I3D_frame *right_hand_target;       // 0x578
    // 0x57c (PS2 0x608), the model's frame "base", LightnessAdd's, whose world position Update gives the human's
    // frame's sector (the name is ours)
    I3D_frame *base_frame;
    bool unknown_580;                   // 0x580
    bool unknown_581;                   // 0x581
    uchar unknown_582[2];               // 0x582
    int unknown_584;                    // 0x584
    // The model's targets "targetE" and "targetN" (PS2 0x614 and 0x618), which Init turns off as C_actor::Init does
    // and GameDone links back to the model (the names are ours, after the frames')
    I3D_frame *target_e;                // 0x588
    I3D_frame *target_n;                // 0x58c
    // 0x590 (PS2 0x61c), the time to the next idle animation: PersonAnim sets 5 to 10 s at breath01a (1), -1 at the
    // others; Update plays one of the next three breaths when it runs out (the name is ours)
    int idle_time;
    // 0x594 (PS2 0x620), the breath's time left: PersonAnim sets it from the length of the breaths 2 to 4, -1 at
    // the others; while it isn't below 0, an animation less than 4 ids below anim_id stays (the name is ours)
    int breath_time;
    // 0x598 (PS2 0x624), whether the human plays his idle animations most likely: cleared while a record (RECLOADFULL)
    // plays or a dialog runs, set again after; PS2's C_player::GameInit sets it, and its Update starts one of three
    // at random when a timer runs out and it is set (the name is a guess)
    bool idle_anims;
    uchar unknown_599[3];               // 0x599
    int unknown_59c;                    // 0x59c
    int unknown_5a0;                    // 0x5a0
    uchar unknown_5a4[0xc];             // 0x5a4
    S_vector unknown_5b0;               // 0x5b0
    uchar unknown_5bc[0x24];            // 0x5bc
    bool unknown_5e0;                   // 0x5e0
    uchar unknown_5e1[3];               // 0x5e1
    int unknown_5e4;                    // 0x5e4
    int unknown_5e8;                    // 0x5e8
    int unknown_5ec;                    // 0x5ec
    float unknown_5f0;                  // 0x5f0, the length of the car's shot animation (Update)
    float unknown_5f4;                  // 0x5f4, 0.5 from Update's car shot animation
    // 0x5f8 (PS2 0x65c), the model's name before _intern_ChangeModel changed it, new[]'s: GameDone and Deactivate
    // change it back and free it (the name is ours)
    char *orig_model;
    // 0x5fc (PS2 0x660), the human's type, a single bit of 11, 0 for none: PS2's Can_Hit_FilterHumanType takes the
    // two humans' bit numbers into a table, DETECTOR_INRANGE counts those with a bit of the detector's (the name is
    // ours, after Can_Hit_FilterHumanType)
    uint human_type;
    // The properties' highest values and the current ones, HUMAN_PROPERTY_* (PS2 0x664 and 0x6a4; the names are ours):
    // the 15 _human_property_ names in 16 floats each
    float max_properties[16];           // 0x600
    float properties[16];               // 0x640, which C_game::Init caps at max_properties
    int unknown_680;                    // 0x680, 0xc from the constructor
    bool shut_up;                       // 0x684 (PS2 0x6e8), HUMAN_SHUTUP's (the name is ours)
    uchar unknown_685[3];               // 0x685
    // The human's own collisions, which its shots leave out (the names are ours)
    tDynamicCollObject **colls;         // 0x688 (PS2 0x6ec)
    I3D_frame **coll_frames;            // 0x68c (PS2 0x6f0), the frames the collisions follow (the name is ours)
    int num_colls;                      // 0x690 (PS2 0x6f4)
    bool colls_active;                  // 0x694 (PS2 0x6f8), ActivateColls sets it (the name is ours)
    uchar unknown_695[3];               // 0x695
    std::vector<tDynamicCollObject *> unknown_698;  // 0x698, collisions the tests leave out besides colls
    std::vector<tDynamicCollObject *> unknown_6a8;  // 0x6a8, DynSetNextCollOwnerMult's list of both
    // The human's sounds (FRAME_SOUNDs linked to the model, SoundsChange's)
    I3D_sound *unknown_6b8;             // 0x6b8, 1.5 m high for the player, 0.1 m for the others
    int unknown_6bc;                    // 0x6bc
    I3D_sound *unknown_6c0;             // 0x6c0, 1 m high, of priority 0x60
    I3D_sound *unknown_6c4;             // 0x6c4, 1 m high
    I3D_sound *unknown_6c8;             // 0x6c8, 1 m high
    // 0x6cc, the voice, NewScream's: 1.7 m high, at the speech volume (the name is ours)
    I3D_sound *voice;
    int unknown_6d0;                    // 0x6d0
    int ground_material;                // 0x6d4, the surface's under the human, MAT_* (the name is ours)
    // The HUD's wingman indicator, -1 for none, and its name, 0 to 7 (PS2 0x734 and 0x738; the names are ours)
    int wingman;                        // 0x6d8
    uint wingman_name;                  // 0x6dc
    I3D_frame *fire_target;             // 0x6e0, HUMAN_SETFIRETARGET's, NULL for none (the name is ours)
    // The drops of blood left and the time to the next (the names are ours): Update adds them every 0.5 to 2.5 s
    int bleed_count;                    // 0x6e4
    int bleed_time;                     // 0x6e8
    ushort unknown_6ec;                 // 0x6ec
    ushort unknown_6ee;                 // 0x6ee
    uchar unknown_6f0;                  // 0x6f0, 6 from cbCollTester
    uchar unknown_6f1[3];               // 0x6f1
    S_human_traffic *traffic;           // 0x6f4 (PS2 0x750), NULL for a human not made of a pedestrian (ours)
    // What the human carries (PS2 0x754, 0x758 and 0x75c, Do_FuckingBox's and Do_DeadBody's; the names are ours)
    I3D_frame *box;                     // 0x6f8
    int box_index;                      // 0x6fc
    I3D_frame *dead_body;               // 0x700
    // 0x704 (PS2 0x760), the dead whose body (dead_body) the human carries, whose animation Update ticks and
    // ChangeDeadAnim sets (the name is ours)
    C_human *dead_body_human;
    int dead_body_time;                 // 0x708 (PS2 0x764), the ms the body's animation goes on after the drop (ours)
    ushort unknown_70c;                 // 0x70c
    uchar unknown_70e[2];               // 0x70e
    S_vector unknown_710;               // 0x710
    float unknown_71c;                  // 0x71c
    bool no_anim_hit;                   // 0x720 (PS2 0x770), SetNoAnimHit's (the name is ours)
    uchar unknown_721[3];               // 0x721
    ai_loc loc;                         // 0x724 (PS2 0x780), which GameInit and Activate start (the name is ours)
    ai_blob_log blob_log;               // 0x758 (PS2 0x7c0), which GameInit and Activate clear (the name is ours)
    int unknown_99c;                    // 0x99c
    ushort unknown_9a0;                 // 0x9a0
    uchar unknown_9a2[2];               // 0x9a2
    // The time (GetTickCount's) and the scene's render number of the last hit that played an animation: a hit within
    // 200 ms of it or in the same frame plays none (PS2 0xad8 and 0xadc; the names are ours)
    uint last_hit_time;                 // 0x9a4
    int last_hit_render;                // 0x9a8
    float unknown_9ac;                  // 0x9ac
    float unknown_9b0;                  // 0x9b0
    float unknown_9b4;                  // 0x9b4
    float unknown_9b8;                  // 0x9b8
    float unknown_9bc;                  // 0x9bc
    int unknown_9c0;                    // 0x9c0
    // The using objects of the dead body (the name is ours): ChangeState fills them for a dead human
    C_using_object::S_using_object using_objects[2];  // 0x9c4
    bool ab_created;                    // 0xa34 (PS2 0xb80), HUMAN_CREATEAB sets it, HUMAN_ERASEAB clears it (ours)
    bool unknown_a35;                   // 0xa35
    bool unknown_a36;                   // 0xa36
    uchar unknown_a37;                  // 0xa37
    S_vector unknown_a38;               // 0xa38
    S_vector unknown_a44;               // 0xa44
    int unknown_a50;                    // 0xa50
    int unknown_a54;                    // 0xa54
    int unknown_a58;                    // 0xa58
    int unknown_a5c;                    // 0xa5c
    int unknown_a60;                    // 0xa60
    float unknown_a64;                  // 0xa64
    float unknown_a68;                  // 0xa68
    int unknown_a6c;                    // 0xa6c
    I3D_frame *unknown_a70;             // 0xa70 (PS2 0xba4), unlinked and released by GameDone
    C_actor *killer;                    // 0xa74 (PS2 0xba8), whom the death camera looks at (the name is ours)
    // 0xa78 (PS2 0xbac), the car's speed limiter: no limit (1000) without it, the HUD's INDF_SPEED_LIMITER with it
    // (the name is ours)
    bool speed_limiter;
    // 0xa79 (PS2 0xbad), a kill of the human by the player adds 500 to the game's score (Hit, HitInCar; the
    // name is a guess)
    bool score_bonus;
    bool unknown_a7a;                   // 0xa7a
    uchar unknown_a7b[0xd];             // 0xa7b
    int unknown_a88;                    // 0xa88
    float unknown_a8c;                  // 0xa8c, 1 from GameInit
    int unknown_a90;                    // 0xa90
    int unknown_a94;                    // 0xa94
    bool unknown_a98;                   // 0xa98
    bool unknown_a99;                   // 0xa99
    bool unknown_a9a;                   // 0xa9a
    uchar unknown_a9b;                  // 0xa9b

    // The properties' highest values a new human has (PS2's constructor's, in its order; the helper is ours, see the
    // constructor)
    void SetDefaultProperties()
    {
        max_properties[HUMAN_PROPERTY_STRENGTH] = 0.7f;
        max_properties[HUMAN_PROPERTY_ENERGY] = 100.0f;
        max_properties[HUMAN_PROPERTY_ENERGY_HAND_L] = 20.0f;
        max_properties[HUMAN_PROPERTY_ENERGY_HAND_R] = 20.0f;
        max_properties[HUMAN_PROPERTY_ENERGY_LEG_L] = 20.0f;
        max_properties[HUMAN_PROPERTY_ENERGY_LEG_R] = 20.0f;
        max_properties[HUMAN_PROPERTY_REACTIONS] = 0.7f;
        max_properties[HUMAN_PROPERTY_SPEED] = 1.0f;
        max_properties[HUMAN_PROPERTY_AGGRESIVITY] = 0.6f;
        max_properties[HUMAN_PROPERTY_INTELLIGENCE] = 0.8f;
        max_properties[HUMAN_PROPERTY_SHOOTING] = 0.7f;
        max_properties[HUMAN_PROPERTY_SIGHT] = 1.0f;
        max_properties[HUMAN_PROPERTY_HEARING] = 1.0f;
        max_properties[HUMAN_PROPERTY_DRIVING] = 0.8f;
        max_properties[HUMAN_PROPERTY_MASS] = 80.0f;
        max_properties[HUMAN_PROPERTY_15] = 0.5f;
    }

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b7a0
    void SetLookFrame(I3D_frame *frm)
    {
        look_frame = frm;
    }
    // FUNCTION: GAME 0x47b7b0
    C_car *GetCar() const
    {
        return car;
    }
    // FUNCTION: GAME 0x47b7c0
    void SetWingman(int index, uint name)
    {
        wingman = index;
        wingman_name = name;
    }
    // FUNCTION: GAME 0x47b7e0
    void SetFireTarget(I3D_frame *frm)
    {
        fire_target = frm;
    }
    // FUNCTION: GAME 0x47b7f0
    void SetNoAnimHit(bool on)
    {
        no_anim_hit = on;
    }
    // FUNCTION: GAME 0x47b800
    void SetIdleAnims(bool value)
    {
        idle_anims = value;
    }
    // Whether the human can go on, HUMAN_WAITTOREADY's (PS2 has the same test inline)
    // FUNCTION: GAME 0x47b810
    bool IsReady() const
    {
        return alive && (action_state == 1 || action_state == 2 || action_state == 9) &&
            !anim_machine.IsBlocking();
    }
    // FUNCTION: GAME 0x47b850
    void SetShutUp(bool b)
    {
        shut_up = b;
    }

    // The animation set played to its last 250 ms before the human goes on, waiting (PS2 name; inline on the PC,
    // where no copy of it is left)
    void SetWaitAnim()
    {
        PersonAnim(0.0f, true, false);
        uint time, length;
        anim_machine.GetActiveAnimTime(time, length);
        if (length > 250)
        {
            unknown_44c = length - 250;
            unknown_430.Zero();
            unknown_43c = 0.0f;
            action_state = 5;
        }
    }
};


// SYNTHETIC: GAME 0x56f1f0
// C_human::`scalar deleting destructor'
