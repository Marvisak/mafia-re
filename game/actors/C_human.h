#pragma once

#include "actors/C_actor.h"
#include "actors/C_animations.h"
#include "actors/C_ShotSkeleton.h"
#include "actors/g_inventory.h"
#include "tools.h"

class C_bridge;
struct g_collision_header;
class C_car;
class C_car_ex;
class I3D_sound;
class C_human;
class C_traffic_element;

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

// C_human::human_type's values (the names are ours; the player's is 1, C_player::Init's): the player's friends most
// likely (a guess: Hit spares them with the player under GAMEF_INVULNERABLE; they shoot the player only as their
// angriest enemy); the police (C_shvestky::TickDoRiciActors dismisses type 4) give a kill's score without a bonus
#define HUMAN_TYPE_FRIEND 2
#define HUMAN_TYPE_POLICE 4
// The types _intern_ThrowMeFromCar gives a traffic's driver it makes an entity of, armed and unarmed (the names
// are ours, by value)
#define HUMAN_TYPE_UNKNOWN_40 0x40
#define HUMAN_TYPE_UNKNOWN_80 0x80

// C_human::human_flags's bits (the names are ours): PersonAnim starts every other walk animation with its f .5, the
// bit marking the walk between (the name is a guess); an entity gets EVENT_HIT_FROM_PLAYER when the player hits it
// (C_entity's constructor sets it by its program)
#define HUMANF_WALK_HALF 0x1
#define HUMANF_EVENT_HIT_FROM_PLAYER 0x2

// The effects' particle of the airplane's hit (Hit's HIT_AIRPLANE) at the human's frame (the name is ours)
#define HITPARTICLE_AIRPLANE 0x28

// The sound table's sound of a human a car or a train runs into (C_human::Collision; the name is ours)
#define SOUND_RUN_OVER 0xb8
// The sound table's sound of a weapon taken out, put away or thrown away (C_human::Do_ChangeWeapon; the name is
// ours)
#define SOUND_WEAPON_CHANGE 0xc2
// The sound table's sounds ChangeWeaponModel gives pump_sound for the pump shotgun and for the two rifles, the
// weapons with a pump or a bolt (the names are guesses)
#define SOUND_SHOTGUN_PUMP 0xd7
#define SOUND_RIFLE_BOLT 0xbe
// MovementIntern's sounds (the names are ours): a drowning's splash and the landing after a fall
#define SOUND_WATER_SPLASH 0xec
#define SOUND_LANDING 0xc0
// MovementIntern's particles of the effects, the landing's also NOTE_FALL_UPPER's at "back2" (the names are ours)
#define PARTICLE_WATER_SPLASH 0x25
#define PARTICLE_LANDING 0x4b
// The sound table's sound of a car stolen (Movement's HUMAN_ACTION_STEAL; the name is ours)
#define SOUND_CAR_STOLEN 0x87
// The sound table's car_unlock, which loops at the player while he steals a car (C_player::Can_UseCar; the name
// is ours)
#define SOUND_CAR_UNLOCK 0x86
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

// C_human::voice_type's, S_voice_def::sounds' index (the names are ours): the constructor's, and the first of the
// three male and of the three female voices _intern_ThrowMeFromCar gives a traffic's driver it makes an entity of
#define HUMAN_VOICE_DEFAULT 0xc
#define HUMAN_VOICE_TRAFFIC_MALE 0x1a
#define HUMAN_VOICE_TRAFFIC_FEMALE 0x1d
// NewScream's voices, gameVoices' entries from 2 (voices.def; the names are ours): a heavy hit in a leg, the hits'
// seven cries from VOICE_HIT, the deaths' two from VOICE_DEATH, the agony's three from VOICE_AGONY, and the seven
// a driver swears with, thrown out of his car, from VOICE_SWEAR
#define VOICE_HIT_LEG 0x54
#define VOICE_HIT 0x55
#define VOICE_DEATH 0x5c
#define VOICE_AGONY 0x5e
#define VOICE_SWEAR 0xa1
// The player's two lines when the engine he starts has no fuel (C_player::AI_drive; the name is a guess)
#define VOICE_NO_FUEL 0x89
// The first of the player's three remarks on a car he can't steal or hijack (C_player's Can_UseCar and
// Do_AB_OwnerNULL; the name is a guess)
#define VOICE_CANT_STEAL 0x84
// The police's calls by the player's offences (human_notify_callback's NOTE_POLICE_SCREAM_1 to 3: 0x98 to 0x9a
// for one, 0x9b to 0x9d for two, 0xa2, 0x9c and 0xa3 for three; the police's voices have the same file in 0xa2
// as in 0x98) and NOTE_SCREAM_88's (the names are ours, by value)
#define VOICE_UNKNOWN_88 0x88
#define VOICE_UNKNOWN_98 0x98
#define VOICE_UNKNOWN_99 0x99
#define VOICE_UNKNOWN_9A 0x9a
#define VOICE_UNKNOWN_9B 0x9b
#define VOICE_UNKNOWN_9C 0x9c
#define VOICE_UNKNOWN_9D 0x9d
#define VOICE_UNKNOWN_A2 0xa2
#define VOICE_UNKNOWN_A3 0xa3

// The version of the mission's character data SaveData writes and LoadData reads (scene2.bin; the name is ours)
#define HUMAN_DATA_VERSION 6
// The version of the saved game's data SaveGameSave writes, the only one SaveGameLoad takes (the name is ours)
#define HUMAN_SAVEGAME_VERSION 6

// C_human::action_state's values, what the human does, which Movement switches on (the names are ours): IsReady
// takes STAND, WALK and CAR
#define HUMAN_ACTION_STAND 1            // on foot, no action: Movement starts a fall while test_ground is set
#define HUMAN_ACTION_WALK 2             // walking (PS2's Go_*), and slowing down after the walk (Update)
#define HUMAN_ACTION_FALL 3             // falling (MovementIntern) until he lands
#define HUMAN_ACTION_TRACK 4            // an animation whose track moves him (PersonAnim, Do_PlayAnim) until it ends
#define HUMAN_ACTION_WAIT 5             // waiting action_time out, still moved by last_velocity (SetWaitAnim's)
#define HUMAN_ACTION_STEPS 6            // the animation steps, anim_steps: the hits, the agony and the deaths
#define HUMAN_ACTION_AGONY 7            // the agony, action_time long, then agony_count more or the death
#define HUMAN_ACTION_DEAD 8             // dead: ACTSTATE_NONE once dead_time is out
#define HUMAN_ACTION_CAR 9              // seated in the car: only then the script's car tests count him in it
#define HUMAN_ACTION_STEAL 10           // stealing stealing_car (C_player), action_time more
#define HUMAN_ACTION_JUMP 11            // a jump (Do_Jump) until he lands or falls

// C_human::vehicle_action's values, the use of a vehicle an animation's end finishes (the names are ours): into
// the car's seat and out of it (Use_Actor), into the rail vehicle and out of it (Use_Actor, at the end of
// HUMAN_ACTION_WAIT), thrown out of the car (_intern_ThrowMeFromCar), over to the other seat (Do_ClimbInCarLR)
#define HUMAN_VEHICLE_CAR_IN 1
#define HUMAN_VEHICLE_CAR_OUT 2
#define HUMAN_VEHICLE_RAILWAY_IN 3
#define HUMAN_VEHICLE_RAILWAY_OUT 4
#define HUMAN_VEHICLE_CAR_THROWN_OUT 5
#define HUMAN_VEHICLE_SEAT_CHANGE 6

// C_human::next_car_action's values, what the human does once seated in the car (the names are ours): over to the
// other seat of the pair (Do_ClimbInCarLR, after Use_Actor's getting in with c), out of the car (Use_Actor, after
// the climb over to a seat he can get out at); at the seat it gets HUMAN_CAR_ACTION_DUE added, which AI waits for
#define HUMAN_CAR_ACTION_CLIMB_OVER 1
#define HUMAN_CAR_ACTION_GET_OUT 2
#define HUMAN_CAR_ACTION_DUE 100

// S_anim_step::type, what the step does (the names are ours): waits wait ms out, plays the animation anim_id, goes
// on with the agony (HUMAN_ACTION_AGONY), ends the steps (Movement's HUMAN_ACTION_STEPS)
#define HUMAN_STEP_WAIT 0
#define HUMAN_STEP_ANIM 1
#define HUMAN_STEP_AGONY 2
#define HUMAN_STEP_END 3
// S_anim_step::flags (the names are ours): the animation plays once, not looped (PersonAnim's second bool, which
// goes on to C_animations_machine_de_luxe::SetAnim's third, where ChangeDeadAnim gives Do_PlayAnim !loop; the name
// is a guess), and the step waits the animation's length after it
#define HUMAN_STEPF_ONCE 0x1
#define HUMAN_STEPF_WAIT 0x2

// One of C_human's animation steps (anim_steps, PS2 0x3dc), 0x14 bytes, which the hits, the agony and the deaths
// fill in and Movement plays in HUMAN_ACTION_STEPS (the names are ours)
struct S_anim_step
{
    int type;                           // 0x0, HUMAN_STEP_*
    int anim_id;                        // 0x4, HUMAN_STEP_ANIM's animation, as C_human::next_anim_id
    int wait;                           // 0x8, HUMAN_STEP_WAIT's time left in ms
    bool stance;                        // 0xc, PersonAnim's first bool: in the weapon's and the crouch's stance
    uchar unknown_d[3];                 // 0xd
    int flags;                          // 0x10, HUMAN_STEPF_*
};

// A step of a weapon's fire (g_material.h)
struct S_fire_action;

// The sounds of a strike that hits (C_human::AI; the names are ours, after the sound table's "Body" of "Hit punch",
// "Hit knife" and "Hit baseball bat"): a human's by the weapon in the right hand, the fists and the knuckleduster,
// the knife, crowbar, bottle and sword, the others; a car's; the bat's from behind ("omraceni", stunning)
#define SOUND_STRIKE_PUNCH 0x93
#define SOUND_STRIKE_KNIFE 0xc4
#define SOUND_STRIKE_BAT 0x16
#define SOUND_STRIKE_CAR 0x17
#define SOUND_STRIKE_BACK 0xf0


// The weapons' effects, StartWeaponEffect's argument, the animations' notes of the same ids (the names are ours): the
// moves are also effect_step's values while TickWeaponEffect runs them
#define WEAPON_EFFECT_OPEN 0x49         // the revolver's cylinder and the shotgun's barrels open, the clip drops out
#define WEAPON_EFFECT_CLOSE 0x4a        // they close, the new clip is in
#define WEAPON_EFFECT_PUMP 0x4b         // the pump-action's foregrip back and forth, the rifles' bolt sound
#define WEAPON_EFFECT_SHELLS 0x4c       // the revolver's six empty shells
#define WEAPON_EFFECT_NEW_CLIP 0x4d     // the new clip in the left hand
#define WEAPON_EFFECT_SHOT 0x50         // the slide back and forth (Shooting, with ammunition left)
#define WEAPON_EFFECT_LAST_SHOT 0x51    // the slide back, open (Shooting, the last shot)

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

// ai_blob::flags (the names are ours, after the animations and the members): the human's state when he was logged
#define AIBLOB_RUNNING 0x1              // run1.i3d (5), and the running jump jumpR?1.i3d (0x60)
#define AIBLOB_CROUCHING 0x2            // C_human::crouching
#define AIBLOB_FIRING 0x4               // C_human::fire_held
#define AIBLOB_JUMPING 0x8              // the jumps: jump1.i3d (0x5f), 0x5e, and with AIBLOB_RUNNING jumpR?1.i3d

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

// C_human::car_stance (the names are ours): seated, the seat's own animation; the driver's hands on the wheel,
// whose animation follows the car's steering; aimed out of the window, Movement's car shots
#define HUMAN_CARSTANCE_SEATED 0
#define HUMAN_CARSTANCE_STEERING 1
#define HUMAN_CARSTANCE_AIMED 2

// C_human::ActivateColls' modes (the names are ours): the upper sphere, and the lower one while the human is
// alive; the upper one only, the lower one only, both while he is alive
#define HUMAN_COLLS_ALL 0
#define HUMAN_COLLS_UPPER 1
#define HUMAN_COLLS_LOWER 2
// C_human::Use_Actor's how (the names are ours): into the car's seat or the rail vehicle, out of it (any other)
#define HUMAN_USE_IN 1
#define HUMAN_USE_OUT 2
// C_human::Do_ChangeWeapon's how (the names are ours, after the animations "gun stoj on off.i3d" and "gun stoj
// zahozeni.i3d"): the weapon taken out or put away, thrown away
#define HUMAN_WEAPON_ON_OFF 0
#define HUMAN_WEAPON_THROW_AWAY 1

// The height of the ground below pos, from a metre above it down 6 metres, relative to pos: 0 for none (PS2 names, in
// PS2's C_human.cpp after ai_blob_log; the second leaves the moving objects out). The player's file calls them too.
float _get_delta_y_(S_vector &pos);
float _get_delta_nodyn_y_(S_vector &pos);

// SYNTHETIC: GAME 0x56f330
// ??1ai_logs@@QAE@XZ

// The saved game's data of a human, after C_actor's (C_human::SaveGameSave's and SaveGameLoad's), 0x17e bytes, packed:
// the members the fields copy, in their order but for the properties, each field with its member's name (PS2's has the
// same fields, the bytes and shorts last; the struct's name is ours). action_state and vehicle_action are saved as
// words.
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
    bool fire_held;                   // 0x28
    S_vector throw_dir;                 // 0x29
    int throw_time;                    // 0x35
    bool throw_swung;                   // 0x39
    int strike_time;                    // 0x3a
    int strike_index;                    // 0x3e
    bool guarding;                      // 0x42
    bool sniper_mode;                   // 0x43
    float tilt;                         // 0x44
    bool pose_moving;                   // 0x48
    S_vector pose_back1_dir;            // 0x49
    S_vector pose_back2_dir;            // 0x55
    S_vector pose_neck_dir;             // 0x61
    ushort action_state;                // 0x6d
    ushort vehicle_action;              // 0x6f
    float fall;                         // 0x71
    S_vector velocity;               // 0x75
    float turn_speed;                   // 0x81
    S_vector last_velocity;             // 0x85
    float velocity_ramp;                // 0x91
    S_vector ground_normal;               // 0x95
    int action_time;                    // 0xa1
    int dead_time;                      // 0xa5
    int agony_anim;                    // 0xa9
    int agony_count;                    // 0xad
    int last_fire_time;                 // 0xb1
    int track_test;                     // 0xb5
    S_vector track_test_pos;            // 0xb9
    int wagon;                          // 0xc5
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
    short track_steps_max;              // 0x179
    short track_steps;                  // 0x17b
    uchar track_texture;                // 0x17d
};
#pragma pack(pop)

// A human's record of a cutscene (the names are ours): types 1 to 3 are 0x28, 0x2c and 0x38 bytes (AddToCutscene),
// type 2 and 3 with unknown_28, type 3 with pose_pos too
struct S_human_record
{
    int time;                           // 0x0, S_cutscene_record's
    int type;                           // 0x4
    S_vector pos;                       // 0x8, the frame's position
    S_quat rot;                         // 0x14, the frame's rotation
    // 0x24: the animation (HUMAN_REC_ANIM) and its flags (HUMAN_REC_ANIM_FLAGS), the link's actor and kind
    // (HUMAN_REC_LINK_*), the pose (HUMAN_REC_POSE_SHIFT)
    uint anim;
    uint unknown_28;                    // 0x28, types 2 and 3: an animation's time, in 40 ms (0xfff)
    S_vector pose_pos;                  // 0x2c, type 3: unknown_a7c, the pose's
};

// C_human::pose_flags (the names are ours): a pose set since the last record (all three pose functions), the aimed pose
// (PoseSetPoseAimed sets it, PoseSetPoseNormal and PoseNullRotate clear it), the null rotation (PoseNullRotate sets
// it, PoseSetPoseNormal and PoseSetPoseAimed clear it), and 8, which forces a record too
#define HUMAN_POSEF_NEW 0x1
#define HUMAN_POSEF_AIMED 0x2
#define HUMAN_POSEF_NULL_ROTATE 0x4
#define HUMAN_POSEF_UNKNOWN_8 0x8

// S_human_record::anim (the names are ours)
#define HUMAN_REC_ANIM 0x7ff
#define HUMAN_REC_ANIM_FLAGS 0xf800
// The flags (the names are guesses): the "base" interpolators' animation of the other channel cleared, the cutscene
// animation's .tck played too (tck_on), looping, the animation machine reset first for the human's own animation
// and the other channel's blend (cutscene_blend) turned around for a cutscene's, and PlaceAtTime places the frame as
// the record has it, without blending to the next
#define HUMAN_REC_ANIM_BASE 0x800
#define HUMAN_REC_ANIM_TCK 0x1000
#define HUMAN_REC_ANIM_LOOP 0x2000
#define HUMAN_REC_ANIM_RESET 0x4000
#define HUMAN_REC_ANIM_NO_BLEND 0x8000
// The animations from it on are the cutscene's (C_recorder::GetAnimByIndex's, less it), those below it the human's
#define HUMAN_REC_ANIM_CUTSCENE 0x400
// The link's actor (an item of the cutscene's anim_names, less 0x100; HUMAN_REC_LINK_NONE for none) and kind
#define HUMAN_REC_LINK_ID_SHIFT 16
#define HUMAN_REC_LINK_NONE 0xff
#define HUMAN_REC_LINK_KIND_SHIFT 24
// The pose flags (HUMAN_POSEF_*) the record was made with, but HUMAN_POSEF_UNKNOWN_8
#define HUMAN_REC_POSE_SHIFT 28
// S_human_record::type (the names are ours, after vfunc_1's and vfunc_3's records timed and posed): the frame and
// the animation, with the animation's time too (unknown_28), and with the pose's position too
#define HUMAN_REC_TYPE_PLAIN 1
#define HUMAN_REC_TYPE_TIMED 2
#define HUMAN_REC_TYPE_POSED 3

// The spheres of a human moved together (PS2 tgcPanak2, Czech "panak", a little man; PS2's has its members in another
// order), 0x8c bytes, g_collision::TestPanak2's and TestPanak2InVehicle's, which C_human's TestMoveColl and
// TestMoveCollInOwner fill (the members' names are ours; here, as g_colls.h has no S_vector)
struct tgcPanak2
{
    float radius[3];                    // 0x0, the spheres' radii, the legs' first
    // The spheres' centres: three members, as an array of S_vector would be constructed (the original calls no
    // vector constructor iterator)
    S_vector legs;                      // 0xc
    S_vector body;                      // 0x18
    S_vector head;                      // 0x24
    S_vector dir;                       // 0x30, the move
    uint flags2;                        // 0x3c, TestSphereB's second flags (COLF2_*)
    uint flags;                         // 0x40, TestSphereB's flags (COLF_*)
    S_vector pos_out;                   // 0x44, where the legs' sphere stops, for a hit
    S_vector normal;                    // 0x50, the surface's normal there
    uchar unknown_5c[4];                // 0x5c
    int hits[10];                       // 0x60, TestPanak2's: the collision hit after each pass, 0 for none yet
    int last_pass;                      // 0x88, TestPanak2's: the index of its last pass
};

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

    // The cutscenes' functions (PC only, C_actor's slots), on the human's records (S_human_record): AddToCutscene
    // gives their sizes, RecordKey returns the record's type, PlayRecord starts the current record's animation,
    // PlaceAtTime places the last record's link and pose too, DumpRecords writes them to "<name>.dump"
    virtual bool AddToCutscene();
    virtual int RecordKey(int time);
    virtual void PlayRecord(int time);
    virtual void PlayStep(int time);
    virtual bool PlaceAtTime(int time);
    virtual void DumpRecords();
    virtual S_vector GetRecordPos(int time);
    virtual S_quat GetRecordRot(int time);

    C_human();
    virtual ~C_human();
    virtual bool Init(I3D_frame *frm);
    virtual void DuplicateFrom(C_actor *act);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
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
    // Notes a link for the cutscene's recording (PC only, which PlaceAtTime calls; the name is ours)
    void SetCutsceneLink(C_actor *act, int kind);
    // The poses (PS2 names); the PC's take the vectors by value, PS2's by reference
    void PoseInterpolate(uint time);
    // False where the poses can't turn to the position (in a car, without the neck, behind the human)
    bool PoseSetPoseNormal(S_vector pos);
    bool PoseSetPoseAimed(S_vector pos);
    void PoseNullRotate();
    void PoseValidate(bool valid);
    // PS2 name; static, as the PC's two arguments in registers show
    static int GetAnimIDForDeath(int i, bool b, const S_vector &v1, const S_vector &v2);
    void Movement(uint time);
    void MovementIntern(uint time);
    // The collision the move runs into, a g_collision_header as TestLineV's int (MovementIntern reads its material), 0
    // for none
    int TestMoveColl(const S_vector &from, const S_vector &dir, S_vector &result);
    // Inline (defined so in the .cpp): MovementIntern has it expanded, TestMoveColl calls its copy
    void DynSetNextCollOwnerMult();
    int TestMoveCollInOwner(const S_vector &from, const S_vector &dir, S_vector &result);
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
    void Do_ChangeWeapon(int how, bool hide);
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
    bool ZasrateLezeniZAuta_ve2rano();
    void ZasrateVybliknutiZAuta_ve3rano();
    // Over to the free seat beside his own in the car: false where he can't
    bool Do_ClimbInCarLR();
    // The actor used (how 1: into a car's seat b or a rail vehicle; another how: out of it)
    void Use_Actor(C_actor *act, int how, int b, int c);
    // Empty on the PC (PS2 name): Use_Actor's calls go to the empty function the linker folded it into
    void UniversalUse();
    // The weapons (PS2 names)
    bool Do_Shoot(bool b, S_vector *dir);
    bool Do_ThrowGrenade(S_vector &dir);
    // PS2's takes a bool, the PC's nothing
    C_human *Can_SpecialStroke();
    // The kicks between the legs and the head-butt at the human: false where he can't be hit so
    bool Do_Castrate(C_human *human);
    bool Do_HardHead(C_human *human);
    void Shooting();
    // Do_Shoot's strike animation (PS2 name), inline on the PC
    inline void SetFreeBoxingAnim(uint id);
    // Shooting's shot from a car's seat (the name is ours), inline
    inline void ShootingInCar();
    void Do_Reload();
    void Do_Holster();
    void Do_WeaponDrop();
    int Do_PlayAnim(const char *anim, bool simple, bool unknown);
    virtual bool GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm);
    // PS2 names; the PC's Do_Jump takes nothing, PS2's two bools
    void Do_Jump();
    bool Do_Climb();
    void Do_SideJump(const S_vector &pos2, const S_vector &dir2);
    // The model changed for the named one
    void _intern_ChangeModel(char *name, bool restore);
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
    // False where the seat can't be reached
    bool Do_ThrowCocotFromCar(C_car *car, int seat);
    void _intern_ThrowMeFromCar();
    // The box and the dead body carried (PS2 names)
    // True where the box was taken or put down (the frame's, NULL to put it down)
    bool Do_FuckingBox(I3D_frame *frm);
    // False where the box can't be put down here (the return's meaning is a guess)
    bool Do_FuckingBoxCancel();
    // The same for a dead body
    bool Do_DeadBody(I3D_frame *frm);
    // PS2 name, German: "my little car in the air"
    void EineMeineKleineAutoInLuft(C_car *wreck);
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
    // An object picked up (PS2 name and parameter, C_player::Do_AB_OwnerNULL passes its position): empty, the
    // linker's folded empty function of the out-of-line kind (0x4ab130)
    void PickObject(S_vector &pos);
    void ResetActions();
    // The death's position (PS2 names): RecompileDeathPos settles the body onto the ground through three static
    // helpers of C_human.cpp, ProcessDeathPos moves it there
    void RecompileDeathPos();
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
    // The human's speed, the walk's (an inline function: the linker kept jp_ai_entity.cpp's copy)
    // FUNCTION: GAME 0x51af20
    virtual void GetActorSpeed(S_vector &speed)
    {
        speed = velocity;
    }

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
    // 0x8c, the scale PersonAnim stretches the climbing animations' tracks by: along x over the car's seats
    // (0x130 and 0x131: Do_ClimbInCarLR's seats' distance against the track's mixing point), along y onto a wall or a
    // box (0x68 and 0x69: Do_Climb's height above the feet, divided by 0.6 for the low climb; the name is ours)
    float climb_scale;
    // 0x90, the scale of the climb's track along x: 2.5 times the distance to the wall, at least 1 (Do_Climb;
    // the name is ours)
    float climb_reach;
    int human_flags;                    // 0x94, HUMANF_* (the name is ours)
    C_car *car;                         // 0x98, the car the human is in (the name is ours)
    // 0x9c (PS2 0x228), the car the human is getting into or was thrown out of: PS2's Use_Actor sets it to the car
    // it uses, _intern_ThrowMeFromCar to the car, the car functions clear it; C_shvestky::TickDoRiciActors keeps a car
    // in the police's faces while a player or an entity has it here or in car (the name is ours)
    C_car *using_car;
    // 0xa0 (PS2 0x22c), the car the human last got into (Use_Actor, Intern_UseCar), kept after he leaves it: the
    // notes open and close its door after using_car's and car's (the name is ours)
    C_car *last_car;
    // The cars the script's CAR_LOCK and CAR_LOCK_ALL lock to the human (the names are ours)
    C_car *locked_car;                  // 0xa4
    C_car *locked_vehicle;              // 0xa8
    // 0xac (PS2 0x238), the seat in the car most likely, -1 for none (the name is a guess)
    int seat;
    // 0xb0, the car whose empty tank the player last complained about (C_player::AI_drive), cleared on the
    // way into or out of a car (the name is ours)
    C_car *no_fuel_car;
    // 0xb4, HUMAN_CAR_ACTION_*, what the human does once seated in the car, 0 for nothing (Use_Actor's c gives the
    // climb over): Update and Intern_UseCar add HUMAN_CAR_ACTION_DUE at the seat, AI does it on its next tick (it
    // tests car_action_time -= time, not its sign; the name is ours)
    int next_car_action;
    int car_action_time;                // 0xb8, 250 ms for next_car_action, -1 from GameInit (the name is ours)
    // 0xbc (PS2 0x250), whose Reset the script's ACTOR_SETPLACEMENT calls (the name is ours)
    C_animations_machine_de_luxe anim_machine;
    bool crouching;                     // 0x1e4 (PS2 0x390), the camera drops while it is set (the name is ours)
    // 0x1e5 (PS2 0x391), the weapon aimed: PS2's Do_Aimed sets it to its argument, Do_Shoot and the corner fire set
    // it, a reload or a weapon change clears it (the name is ours, after Do_Aimed)
    bool aimed;
    // 0x1e6 (PS2 0x392), the side of the animations whose name has a '?': L while it is set, else R (the name is ours)
    bool anim_left;
    uchar unknown_1e7;                  // 0x1e7
    // 0x1e8, the right hand's weapon's kind, S_item_def::kind (ChangeWeaponModel; the name is ours)
    int weapon_kind;
    // 0x1ec, the shot or strike held: Do_Shoot sets it on the press and clears it on the release (the name is ours)
    bool fire_held;
    // 0x1ed, the fire the animation's notes hold: NOTE_UNKNOWN_1A sets it with fire_held, NOTE_UNKNOWN_1B clears
    // both; Shooting shoots behind the human only while it is set (the name is ours)
    bool note_fire;
    uchar unknown_1ee[2];               // 0x1ee
    // What the weapon's fire does (PS2 0x39c, 0x3a0 and 0x3a4; the names are ours): GameInit, Activate and
    // ResetWeaponEffects set _fire_action_ready_ and clear the step and its time
    const S_fire_action *fire_action;   // 0x1f0
    int fire_step;                      // 0x1f4
    int fire_time;                      // 0x1f8
    // 0x1fc, the reload under way: AI sets it at the fire's FIRE_ACTION_RELOAD step, and at the fire's end reloads
    // the weapon and clears it (the name is ours)
    bool reloading;
    // 0x1fd, the reload Do_Reload asks for: the fire's reload step runs though the clip isn't empty, and no
    // cartridge is thrown out; cleared at the fire's end and by a shot (the name is ours)
    bool reload_forced;
    // 0x1fe, the pump shotgun loaded a shell at a time (Do_Reload, or Shooting's last shell): AI keeps the weapon
    // aimed and reloads again at each fire's end while a shell fits, and the left hand's target lets go of the
    // weapon meanwhile; a shot or a strike started ends it (the name is ours)
    bool loading_shells;
    uchar unknown_1ff;                  // 0x1ff
    S_vector throw_dir;                 // 0x200, Do_ThrowGrenade's, the throw's direction (the name is ours)
    I3D_frame *cartridge_frame;         // 0x20c, where the weapon throws its cartridges out (NewCartidge's; the name is ours)
    // 0x210, ChangeWeaponModel's: Movement turns the car shots' animation by the angle from it to unknown_200 (the
    // name is a guess)
    I3D_frame *shot_frame;
    // 0x214, the number in the property of the weapon's frame "muzzle", one more in a night mission, -1 without one
    // (ChangeWeaponModel; the name is ours)
    int muzzle;
    // 0x218, saved as a dword, the time a throw started: the release's force grows from 400 to 2000 ms (Do_Shoot;
    // the name is ours)
    int throw_time;
    bool throw_swung;                   // 0x21c, a throw swung (Do_Shoot's thrown weapons; the name is ours)
    uchar unknown_21d[3];               // 0x21d
    int strike_time;                    // 0x220, the time of the strike series' last strike (Do_Shoot; ours)
    // 0x224, the bat's backswing ("boj basb naprah HPT.i3d") started by Do_Shoot, which plays it once a strike is held
    // over 200 ms (the name is ours)
    bool backswing;
    uchar unknown_225[3];               // 0x225
    // 0x228, the strike's place in its series, 0 to 3, -1 after the series ended, which AI's strike doubles the damage
    // at (the name is ours)
    int strike_index;
    // 0x22c, the melee guard played: the fists', the knife's or the bat's "boj HPT ... kryt.i3d" (Czech: guard),
    // which Update plays as the hands' extended animation while aimed; Do_Aimed(false) ends it, SetFreeBoxingAnim's
    // strike takes its place (the name is ours)
    bool guarding;
    // 0x22d, the player's sniper mode, which C_player::SetSniperMode sets and clears: Shooting shoots from the
    // camera while it is set (the name is ours)
    bool sniper_mode;
    // 0x22e (PS2 0x3ce), MovementIntern tests the ground below even without a move while it is set: set where no
    // ground was found or the ground is a moving object's, cleared on static ground and in a vehicle; Update and
    // GameInit set it (the name is ours)
    bool test_ground;
    // 0x22f (PS2 0x3cf), set by MovementIntern's water death (MAT_WATER_DEATH), with the energy 0; GameInit clears
    // it (the name is ours)
    bool drowned;
    // 0x230, set while a weapon change runs: Do_ChangeWeapon does nothing while it is set, the animations' note
    // NOTE_CHANGE_WEAPON changes the model (the name is ours)
    bool changing_weapon;
    uchar unknown_231[3];               // 0x231
    // 0x234, the strike animation's time left: SetFreeBoxingAnim sets its set's end time, Update counts it down (the
    // name is ours)
    int strike_anim_time;
    // 0x238 (PS2 0x3d8), the human's look up or down, which PS2's C_player::AI_walk moves: the camera's tilt for the
    // player (C_game::Tick; the name is ours, after the camera's)
    float tilt;
    // 0x23c (PS2 0x3dc), the animation steps HUMAN_ACTION_STEPS plays from the first: an animation's end goes on
    // with them, or at a HUMAN_STEP_END with the human's own end of it (Update; the name is ours)
    S_anim_step anim_steps[5];
    int anim_step;                      // 0x2a0, the step of anim_steps HUMAN_ACTION_STEPS plays (the name is ours)
    // The poses of the model's frames "back1", "back2" and "neck" (PS2 0x444, 0x448 and 0x44c), which GameInit
    // creates and GameDone releases (the names are ours)
    I3D_pose *pose_back1;               // 0x2a4
    I3D_pose *pose_back2;               // 0x2a8
    I3D_pose *pose_neck;                // 0x2ac
    // 0x2b0, the poses are still turning: PoseInterpolate clears it and sets it again while one is short of its
    // direction, Update calls it only while it is set, PoseNullRotate sets it (the name is ours)
    bool pose_moving;
    uchar unknown_2b1[3];               // 0x2b1
    // The directions the poses turn to (pose_back1's, pose_back2's and pose_neck's), PoseInterpolate's: (0, 0, 1)
    // from PoseNullRotate (the names are ours)
    S_vector pose_back1_dir;            // 0x2b4
    S_vector pose_back2_dir;            // 0x2c0
    S_vector pose_neck_dir;             // 0x2cc
    I3D_frame *look_frame;              // 0x2d8 (PS2 0x454), SetLookFrame's (the name is ours)
    C_ShotSkeleton shot_skeleton;       // 0x2dc (PS2 0x458; the name is ours)
    // 0x40c (PS2 0x588), HUMAN_ACTION_*, what the human does, which PS2's Go_*, Do_*, Hit* and car functions set and
    // Movement switches on (the name is ours)
    int action_state;
    // 0x410, HUMAN_VEHICLE_*, the use of a vehicle an animation's end finishes, 0 for none: set where the getting
    // in or out starts, finished by Update, at once by Hit and C_player::LockControls (the name is ours)
    int vehicle_action;
    // 0x414 (PS2 0x590), the collision the last move (TestMoveColl) ran into, NULL for none (the name is ours)
    g_collision_header *move_coll;
    // 0x418 (PS2 0x594), the human's fall, 0 while he stands: HUMAN_FORCEFALL adds 0.1, PS2's Movement starts one at
    // 0.05 and makes it 60 below a height of -30 (the name is ours)
    float fall;
    // 0x41c (PS2 0x598), the fall started in a jump (HUMAN_ACTION_JUMP): the landing hurts only above 18, else 8, and
    // less (the name is ours)
    bool jump_fall;
    uchar unknown_41d[3];               // 0x41d
    // 0x420 (PS2 0xc0), the walk's velocity per second: MovementIntern moves by it, Update ramps it (the name is
    // ours)
    S_vector velocity;
    // 0x42c (PS2 0x59c), the turn's speed in radians a second: Update turns dir by it times the time, the side
    // walks and Do_Breath stop it (the name is ours)
    float turn_speed;
    // 0x430 (PS2 0xd0), the walk's last velocity, which Update keeps while he walks: once he stops MovementIntern
    // moves him on by it times velocity_ramp, and HUMAN_ACTION_WAIT by it alone (Use_Actor's 1.5 along the
    // frame's direction); Turn_Turn and the actions clear it (the name is ours)
    S_vector last_velocity;
    // 0x43c (PS2 0x5a0), the walk's speed from 0 to 1, which Update ramps up while he walks (velocity scaled by
    // it) and down after; a shot multiplies it by 0.3, Turn_Turn and the actions clear it (the name is ours)
    float velocity_ramp;
    // 0x440 (PS2 0xe0), the ground's normal the collision tests give, which the move is projected along (the name
    // is ours)
    S_vector ground_normal;
    // 0x44c, the action's time left in ms: HUMAN_ACTION_WAIT's, HUMAN_ACTION_AGONY's and HUMAN_ACTION_STEAL's (the
    // name is ours)
    int action_time;
    // 0x450, the time HUMAN_ACTION_DEAD waits before ACTSTATE_NONE: the death's animation in a car (HitInCar), 500
    // from GameInit, 1500 from Activate (the name is ours)
    int dead_time;
    bool killed_by_shot;                // 0x454 (PS2 0x5ac), set by Hit for a death by HIT_SHOT (the name is ours)
    uchar unknown_455[3];               // 0x455
    // 0x458 (PS2 0x5b0), the agony's animation, 0 to 12 by 4, added to 0x93 for its first (PS2's HitAgony sets it), -1
    // for none: Hit plays its death on a hit in the agony (the name is ours)
    int agony_anim;
    // 0x45c, saved as a dword, the agonies after this one: HitAgony sets 5 to 12, Movement plays another while it
    // counts down, then the death (the name is ours)
    int agony_count;
    // 0x460, the game time of the human's last shot or throw (Shooting, ShootingInCar, Do_Shoot's throws,
    // Do_ThrowGrenade), which ai_logs takes for a shot within 250 ms (0x4f6b10; the name is ours)
    int last_fire_time;
    // 0x464 (PS2 0x5bc), 1 while Update tests the move of a track PersonAnim plays (but the car's and the climbs')
    // from track_test_pos against the collisions, until the track ends; ACTOR_SETPLACEMENT clears it (the name is
    // ours)
    int track_test;
    // 0x468, where the track's move is tested from: PersonAnim sets it to pos with track_test, Update tests the move
    // from it to pos and puts both where the collisions let him go (the name is ours)
    S_vector track_test_pos;
    // 0x474, the time stealing the car takes, which the thief indicator's bar shows action_time of (the name is
    // ours)
    int steal_time;
    C_car *stealing_car;                // 0x478 (PS2 0x5c4), IFPLAYERSTEALCAR's (the name is ours)
    int shadow;                         // 0x47c (PS2 0x5c8), C_game::AddFrameForShadow's (the name is ours)
    G_Inventory inventory;              // 0x480 (the name is ours)
    // The weapons' models, FRAME_MODELs named "weapon" (PS2 0x5f0 and 0x5f4): Update turns them off in a car, Hit
    // on in HUMAN_ACTION_STEAL (the names are ours)
    I3D_model *weapon_model;            // 0x564
    I3D_model *weapon_model2;           // 0x568
    // The frames the hands hold things by, LINKTOHAND's (PS2 0x5f8 and 0x5fc; the names are ours)
    I3D_frame *right_hand;              // 0x56c
    I3D_frame *left_hand;               // 0x570
    // FRAME_TARGETs GameInit links to the left and the right hand, NULL without the hand (PS2 0x600 and 0x604;
    // the names are ours)
    I3D_target *left_hand_target;       // 0x574
    I3D_target *right_hand_target;      // 0x578
    // 0x57c (PS2 0x608), the model's frame "base", LightnessAdd's, whose world position Update gives the human's
    // frame's sector (the name is ours)
    I3D_frame *base_frame;
    // 0x580, the weapon out of its place in the hands: the reload's notes NOTE_WEAPON_FROM_TARGET and
    // NOTE_WEAPON_TO_LEFT_HAND, and Do_Reload's shell by shell, take it out of the left hand's target or into the
    // left hand; RepairWeaponPosition, which PersonAnim and the shots call while it is set, puts it back and clears
    // it, and C_game::RecPrepareHumans sets it to have it put back (the name is ours)
    bool weapon_displaced;
    // 0x581, a reload's weapon effects play: the animations' notes start the effects but the pump
    // (WEAPON_EFFECT_PUMP) only while it is set; Do_Reload sets it, and Shooting at the last round with rounds in
    // reserve; the fire's end and whatever resets the weapon effects (ResetWeaponEffects) clear it (the name is
    // ours)
    bool reload_effects;
    uchar unknown_582[2];               // 0x582
    // 0x584, the rail vehicle's wagon the human is in, from 1, 0 for the vehicle itself (the name is ours): Intern_UseRailway
    // links him to its frame and puts his collisions into its
    int wagon;
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
    // The frames a strike hits with and their positions of the last tick, while striking is set: AI tests the line
    // each moved along against the collisions; human_notify_callback's NOTE_WEAPON_SOUND takes them and sets striking
    // (the names are ours)
    int num_strike_frames;              // 0x59c
    I3D_frame *strike_frames[4];        // 0x5a0
    S_vector strike_pos[4];             // 0x5b0
    bool striking;                      // 0x5e0, cleared by the strike's first hit
    uchar unknown_5e1[3];               // 0x5e1
    // The shots from a car's seat (Update, Movement's action_state 9; the names are ours): the stance in the seat,
    // HUMAN_CARSTANCE_*; the time left of its change, an animation Update ticks the frame through (250 ms after
    // getting in, the aiming's animation less 250 ms, HitInCar's death): Movement changes nothing while it isn't
    // below 0, Update counts it down and plays the stance's own animation where it runs out; the animation
    // SetAnimTime moves by hand (anim_machine.anim_index: the steering wheel's by the steering, the aimed one's by
    // the angle to the target), its length, and the aim's place in it, 0 to 1 (0.5 ahead, which it turns to at
    // 0.003 a ms)
    int car_stance;                     // 0x5e4
    int car_stance_time;                // 0x5e8
    int car_anim_index;                 // 0x5ec
    float car_anim_length;              // 0x5f0
    float car_aim;                      // 0x5f4
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
    // 0x680, the human's voice, the index of the voices' sounds (S_voice_def::sounds) he says: 0xc from the
    // constructor, saved with the mission's data (the name is ours)
    int voice_type;
    bool shut_up;                       // 0x684 (PS2 0x6e8), HUMAN_SHUTUP's (the name is ours)
    uchar unknown_685[3];               // 0x685
    // The human's own collisions, which its shots leave out (the names are ours)
    tDynamicCollObject **colls;         // 0x688 (PS2 0x6ec)
    I3D_frame **coll_frames;            // 0x68c (PS2 0x6f0), the frames the collisions follow (the name is ours)
    int num_colls;                      // 0x690 (PS2 0x6f4)
    bool colls_active;                  // 0x694 (PS2 0x6f8), ActivateColls sets it (the name is ours)
    uchar unknown_695[3];               // 0x695
    // 0x698, the collisions the move's tests leave out besides colls: the AI's ghosts' (C_entity's ai_ghost at
    // 0x1268, PS2's ai_ghost_actor items: an actor, its collision, a time and a radius), which C_entity puts here
    // whenever the ghosts change (0x52f920); C_entity's GameInit and 0x507b50 empty it (the name is ours)
    std::vector<tDynamicCollObject *> ghost_colls;
    // 0x6a8, the collisions the next test leaves out, the list g_collision::DynSetNextCollOwnerMult takes: colls
    // and ghost_colls (DynSetNextCollOwnerMult), a car's (the name is ours, after the function)
    std::vector<tDynamicCollObject *> next_coll_owners;
    // The human's sounds (FRAME_SOUNDs linked to the model, SoundsChange's)
    // 0x6b8, the steps' sound, 1.5 m high for the player, 0.1 m for the others, and the game time of the last step
    // (the names are ours)
    I3D_sound *step_sound;
    uint step_time;
    I3D_sound *weapon_sound;            // 0x6c0, the weapon's sound (NOTE_WEAPON_SOUND), 1 m high (ours)
    // 0x6c4, the weapon's reload sound (S_item_def::reload_sound, the sound table's "Reload" group), 1 m high,
    // which the fire's FIRE_ACTION_RELOAD plays (the name is ours)
    I3D_sound *reload_sound;
    // 0x6c8, the pump shotgun's or the rifles' bolt sound (SOUND_SHOTGUN_PUMP, SOUND_RIFLE_BOLT), 1 m high, which
    // WEAPON_EFFECT_PUMP plays (the name is ours)
    I3D_sound *pump_sound;
    // 0x6cc, the voice, NewScream's: 1.7 m high, at the speech volume (the name is ours)
    I3D_sound *voice;
    int voice_priority;                 // 0x6d0, the voice's priority (S_voice_def's), -1 for none (ours)
    int ground_material;                // 0x6d4, the surface's under the human, MAT_* (the name is ours)
    // The HUD's wingman indicator, -1 for none, and its name, 0 to 7 (PS2 0x734 and 0x738; the names are ours)
    int wingman;                        // 0x6d8
    uint wingman_name;                  // 0x6dc
    I3D_frame *fire_target;             // 0x6e0, HUMAN_SETFIRETARGET's, NULL for none (the name is ours)
    // The drops of blood left and the time to the next (the names are ours): Update adds them every 0.5 to 2.5 s
    int bleed_count;                    // 0x6e4
    int bleed_time;                     // 0x6e8
    // The steps that leave a track (cbCollTester: 10 of the blood's), the count they started from, which fades the
    // tracks, and the track's texture (C_game::track_textures' index; 6 from cbCollTester; the names are ours)
    short track_steps_max;              // 0x6ec
    short track_steps;                  // 0x6ee
    uchar track_texture;                // 0x6f0
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
    // How far the human got in the last 100 ms (the player) or 500 ms (the others) against how far he should
    // have, which Movement measures (the names are ours): the time since the last measure, the position there,
    // and the ratio, averaged with the last, 1 where he shouldn't have moved
    short move_time;                    // 0x70c
    uchar unknown_70e[2];               // 0x70e
    S_vector move_pos;                  // 0x710
    float move_ratio;                   // 0x71c
    bool no_anim_hit;                   // 0x720 (PS2 0x770), SetNoAnimHit's (the name is ours)
    uchar unknown_721[3];               // 0x721
    ai_loc loc;                         // 0x724 (PS2 0x780), which GameInit and Activate start (the name is ours)
    ai_blob_log blob_log;               // 0x758 (PS2 0x7c0), which GameInit and Activate clear (the name is ours)
    // 0x99c, the number (C_game::Play3DSound's) of the car stealing's sound, the sound table's car_unlock (0x86),
    // which the player's stealing starts (C_player, 0x5955f0) and AI, Hit and Movement stop; -1 from
    // C_player::GameInit (the name is ours)
    int steal_sound;
    // 0x9a0, the scream areas the human is in, 1 for none, which GameInit and Activate set: NewScream says
    // nothing out of the camera's (C_game::camera_scream_areas; the name is ours)
    ushort scream_areas;
    uchar unknown_9a2[2];               // 0x9a2
    // The time (GetTickCount's) and the scene's render number of the last hit that played an animation: a hit within
    // 200 ms of it or in the same frame plays none (PS2 0xad8 and 0xadc; the names are ours)
    uint last_hit_time;                 // 0x9a4
    int last_hit_render;                // 0x9a8
    float shots;                        // 0x9ac, the shots fired, the pellets for a shotgun (Shooting; ours)
    float hits;                         // 0x9b0, the shots that hit, AI's hit_ratio's numerator (the name is ours)
    // shots and hits at the counts' last restart: every 5000 ms (ratio_time) AI restarts them from the shots and
    // hits since the restart before, plus 1 each, so hit_ratio covers the last 5 to 10 seconds (the names are ours)
    float prev_shots;                   // 0x9b4
    float prev_hits;                    // 0x9b8
    float hit_ratio;                    // 0x9bc, hits / shots, which AI computes (the name is ours)
    int ratio_time;                     // 0x9c0, the time to the counts' next restart, 5000 ms (the name is ours)
    // The using objects of the dead body (the name is ours): ChangeState fills them for a dead human
    C_using_object::S_using_object using_objects[2];  // 0x9c4
    bool ab_created;                    // 0xa34 (PS2 0xb80), HUMAN_CREATEAB sets it, HUMAN_ERASEAB clears it (ours)
    // 0xa35, out of a car (Use_Actor) only the upper sphere's collision (DYNCOLLF_UPPER) is active and
    // UpdateDynColl doesn't move the lower one; the next animation (PersonAnim) activates it (ActivateColls(2))
    // and clears it; GameInit and Activate clear it (the name is ours)
    bool lower_coll_off;
    // 0xa36, the dead body's position is settled (RecompileDeathPos sets it; Update then plays ProcessDeathPos, else
    // calls RecompileDeathPos); GameInit and Activate clear it (the name is ours)
    bool death_pos_set;
    uchar unknown_a37;                  // 0xa37
    // The dead body's move onto the ground, which ProcessDeathPos plays (the names are ours): the frame's position
    // RecompileDeathPos starts from, the move from there, and the time in it of its length in ms (-1 for none,
    // 1 where the death has no animation)
    S_vector death_pos;                 // 0xa38
    S_vector death_move;                // 0xa44
    int death_time;                     // 0xa50
    int death_length;                   // 0xa54
    // 0xa58, while above 0 the human's Update only counts it down and AI does nothing: nothing sets it above 0 (PS2
    // alike), -1 from GameInit and Activate (the name is a guess)
    int freeze_time;
    I3D_frame *effect_frame;            // 0xa5c (PS2 0xb90), the weapon's part the effect moves (the name is ours)
    int effect_step;                    // 0xa60 (PS2 0xb94), WEAPON_EFFECT_* of the move, 0 for none (ours)
    // 0xa64 and 0xa68 (PS2 0xb98, 0xb9c), the move's position and its speed a ms (the names are ours)
    float effect_pos;
    float effect_speed;
    int pump_time;                      // 0xa6c (PS2 0xba0), the time to the shotgun's pump, -1 for none (ours)
    // 0xa70 (PS2 0xba4), the new clip in the left hand while the weapon is reloaded (WEAPON_EFFECT_NEW_CLIP),
    // a copy of the weapon's "clip"; unlinked and released by GameDone and ResetWeaponEffects (the name is ours)
    I3D_frame *hand_clip;
    C_actor *killer;                    // 0xa74 (PS2 0xba8), whom the death camera looks at (the name is ours)
    // 0xa78 (PS2 0xbac), the car's speed limiter: no limit (1000) without it, the HUD's INDF_SPEED_LIMITER with it
    // (the name is ours)
    bool speed_limiter;
    // 0xa79 (PS2 0xbad), a kill of the human by the player adds 500 to the game's score (Hit, HitInCar; the
    // name is a guess)
    bool score_bonus;
    // 0xa7a, the driver of a traffic car Do_ThrowCocotFromCar made an entity, a temporary actor
    // (C_game::AddTemporaryActor): the police's 0x49ede0 frees such entities among do_rici_actors (the name is
    // ours)
    bool temp_driver;
    uchar unknown_a7b;                  // 0xa7b
    // 0xa7c, the position the last pose was set to (PoseSetPoseNormal's, PoseSetPoseAimed's), which RecordKey
    // records (the name is ours)
    S_vector pose_pos;
    // 0xa88, HUMAN_POSEF_*: what the poses set, for the cutscene's recording (RecordKey); 0 from GameInit (the name
    // is ours)
    int pose_flags;
    // 0xa8c, the blend of the model's channel 0 for the cutscene's animations, channel 1 gets the rest: PlayStep
    // moves it 0.004 a ms towards the channel cutscene_channel_0 selects; 1 from GameInit (the name is ours)
    float cutscene_blend;
    // 0xa90, the time into the cutscene animation's .tck: PlayStep advances it,
    // PlaceAtTime, GetRecordPos and GetRecordRot set it (the name is ours)
    int tck_time;
    // 0xa94, the first time step PlayStep plays after PlayRecord started an animation (the time since its record),
    // -1 once taken (the name is ours)
    int cutscene_step;
    // 0xa98, PlayStep blends the channels (cutscene_blend) while it is set: PlayRecord sets it, false from GameInit
    // (the name is ours)
    bool cutscene_blending;
    // 0xa99, the cutscene's current animation is on the model's channel 0, else on 1: PlayRecord switches them for
    // each new animation; true from GameInit (the name is ours)
    bool cutscene_channel_0;
    // 0xa9a, the cutscene animation's .tck moves the frame (HUMAN_REC_ANIM_TCK with a .tck loaded; the name is ours)
    bool tck_on;
    uchar unknown_a9b;                  // 0xa9b

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
        return alive && (action_state == HUMAN_ACTION_STAND || action_state == HUMAN_ACTION_WALK ||
            action_state == HUMAN_ACTION_CAR) && !anim_machine.IsBlocking();
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
            action_time = length - 250;
            last_velocity.Zero();
            velocity_ramp = 0.0f;
            action_state = HUMAN_ACTION_WAIT;
        }
    }
};


// The notes of a human's animations (PS2 name), the callback of the model's "notify" frame, which GameInit and
// _intern_ChangeModel set (a free function on PS2 too)
uint __stdcall human_notify_callback(I3D_frame *frm, LS3D_CALLBACK_MESSAGE message, uint param1, uint param2);

// The notes, human_notify_callback's param1 (the names are ours, by value where what they do doesn't say more): the
// weapons' effects are WEAPON_EFFECT_*'s
#define NOTE_STEP_R 1                   // a step of the right foot: its sound, and its track while the steps leave one
#define NOTE_STEP_L 2                   // of the left
#define NOTE_FALL_LOWER 8               // SOUND_FALL_LOWER at the human
#define NOTE_DOOR_OPEN 0x16             // the door of the human's car open
#define NOTE_DOOR_CLOSE 0x18            // closed
#define NOTE_SKIP_DOOR 0x56             // Use_Actor's ShiftAnimation: the getting in past an open door (a guess)
#define NOTE_FIRE_HOLD 0x1a             // sets fire_held and note_fire (the name is a guess)
#define NOTE_FIRE_RELEASE 0x1b          // clears them (the name is a guess)
#define NOTE_WEAPON_TO_TARGET 0x1e      // the weapon into the left hand's target
#define NOTE_WEAPON_FROM_TARGET 0x1f    // out of it
#define NOTE_WEAPON_TO_LEFT_HAND 0x20   // out of it into the left hand
#define NOTE_WEAPON_TO_RIGHT_HAND 0x21  // into the right hand
#define NOTE_CHANGE_WEAPON 0x24         // ChangeWeaponModel while changing_weapon is set
#define NOTE_WEAPON_SOUND 0x28          // the weapon's sound (S_item_def::unknown_54)
#define NOTE_THROW 0x29                 // the grenade or the Molotov cocktail thrown
#define NOTE_STAGE_BLEND_END 0x35       // the animation machine's stage blend (blend) to its end (a guess)
#define NOTE_CAR_SIT 0x50               // SOUND_CAR_SIT, or SOUND_SPORTCAR_SIT in the Formula
#define NOTE_FALL_UPPER 0x52            // SOUND_FALL_UPPER, and PARTICLE_LANDING at "back2"
// The police's screams, by the offences of the player (C_shvestky::PocetPrestupku)
#define NOTE_POLICE_SCREAM_1 0x5f
#define NOTE_POLICE_SCREAM_2 0x60
#define NOTE_POLICE_SCREAM_3 0x61
#define NOTE_SCREAM_88 0x62             // NewScream(0x88)

// The sound table's sounds the notes play at the human (the names are ours, after the table's "postava - in/out
// car", "in/out sportcar", "pad na zem (horni)" and "(dolni)": a character's fall to the ground, the upper body's
// and the legs'), and the one Do_DeadBody plays where the camera is near ("player - zvedani mrtvol", picking up
// the dead)
#define SOUND_DEAD_BODY_PICKUP 0x9f
#define SOUND_CAR_SIT 0xb9
#define SOUND_FALL_UPPER 0xbb
#define SOUND_FALL_LOWER 0xbc
#define SOUND_SPORTCAR_SIT 0xed

// SYNTHETIC: GAME 0x56f1f0
// C_human::`scalar deleting destructor'

// TEMPLATE: GAME 0x58e2f0
// std::vector<tDynamicCollObject *,std::allocator<tDynamicCollObject *> >::clear

// The points' insert, which GetBoundPoints2D's inlined push_backs call: the original kept the copy at 0x4eaa00 of its
// folded group (the vectors of 8-byte elements)
// TEMPLATE: GAME 0x4eaa00
// ?insert@?$vector@US_vector2@@V?$allocator@US_vector2@@@std@@@std@@QAEXPAUS_vector2@@IABU3@@Z

// The animations' file names PersonAnim and RecompileDeathPos build in a static buffer each: VC6's PDB names both
// buff, so the markers are by name and pair with ours in address order (RecompileDeathPos's first)
// GLOBAL: GAME 0x647300
// buff
// GLOBAL: GAME 0x647380
// buff
