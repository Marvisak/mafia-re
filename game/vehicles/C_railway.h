#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"
#include "tools.h"

class C_animations_machine_de_luxe;
class C_railway;
class C_web_node;
class C_car;
struct S_road_crossing;

// An entry a rail vehicle at a station has, where a pedestrian boards (PS2 rentry, in the order the PC's fields have):
// the wagon, from 1, 0 for the vehicle itself, the entry's number in it, its place and the direction out of it (the
// names are ours), 0x20 bytes (PS2's 0x30)
struct rentry
{
    uint wagon;
    uint entry;
    S_vector pos;
    S_vector dir;
};

// A row of a vehicle's seats (PS2 rail_seat_row), 0x38 bytes, S_RailSeats::AddRow's (the names are ours)
struct rail_seat_row
{
    S_vector start;                     // 0x0, the first seat's place
    S_vector end;                       // 0xc, the row's other end: the seats lie on the line between
    S_vector dir;                       // 0x18, the direction the seats face, across the row
    uint first_seat;                    // 0x24, the number of its first seat in the vehicle
    uint num_seats;                     // 0x28
    uint wagon;                         // 0x2c, the wagon it is in, from 0
    uint *state;                        // 0x30, each seat's, RSEAT_*
    void **who;                         // 0x34, each seat's passenger that reserved or took it
};

// rail_seat_row::state (the names are ours)
#define RSEAT_FREE 0
#define RSEAT_RESERVED 1
#define RSEAT_TAKEN 2

// The seats of a rail vehicle (PS2 S_RailSeats, C_railway's seats), 8 bytes: its rows, those of the vehicle's own
// duplicated for each wagon; the passengers reserve a seat, sit on it and leave it empty
struct S_RailSeats
{
    uint num_rows;                      // 0x0 (the name is ours)
    rail_seat_row *rows;                // 0x4 (the name is ours)

    // A free seat of the wagon, from a row the counter spreads, on the same side of z = dist as pos if it is given;
    // 0xffff for none (PS2 name)
    uint FindFreeSeat(uint wagon, S_vector *pos, float dist);
    // PS2 names
    int NumFreeSeats(int wagon);
    S_vector GetSeatPos(uint seat);
    S_vector GetSeatDir(uint seat);
    bool EmptySeat(uint seat, void *who);
    bool ReserveSeat(uint seat, void *who);
    bool FullSeat(uint seat, void *who);
    void AddRow(rail_seat_row *row);
    void Duplicate(uint wagons);
    void Init(C_railway *tram);
    void Destroy();
};

// S_rail_human::state (the names are ours, after the update's switch at 0x4845f0): the way in (the entry chosen, the
// walk there and the turn to the door, then the tram's boarding animation or a walk in, linking to the vehicle), the
// seat (found, walked to, sat on: RH_FIND_SEAT goes straight to RH_TO_EXIT for none), the way out the same in
// reverse, back to a pedestrian, then the deaths and the fears. The passenger holds a lock on the vehicle
// (C_railway::Lock) from RH_TO_ENTRY to the end of RH_LINK and from RH_STAND_UP to the end of RH_WALK_AWAY.
#define RH_CHOOSE_ENTRY 0
#define RH_TO_ENTRY 1
#define RH_TURN_TO_DOOR 2
#define RH_BOARD_ANIM 3                 // "salina nastup.i3d", until anim_running clears
#define RH_WALK_IN 4
#define RH_LINK 5
#define RH_FIND_SEAT 6
#define RH_TO_SEAT 7
#define RH_SIT_DOWN 8
#define RH_SITTING 9
#define RH_STAND_UP 10
#define RH_TO_EXIT 11
#define RH_TURN_TO_EXIT 12
#define RH_ALIGHT_ANIM 13               // "salina vystup.i3d", until anim_running clears
#define RH_WALK_OUT 14
#define RH_UNLINK 15
#define RH_WALK_AWAY 16
#define RH_DYING 17                     // the death animation, to its last 250 ms, then RH_DEAD or RH_DRIVER_DEAD
#define RH_DEAD 18
#define RH_DRIVER 19                    // the driver's (RH_DYING ends in RH_DRIVER_DEAD for him)
#define RH_DRIVER_DEAD 20
// DoPanic's, for one sitting and one on foot, until the fear timer passes: back to RH_SITTING, or old_state
#define RH_SCARED_SITTING 21
#define RH_PANIC 22

// The passengers' animations, ranims' indices (PS2 ranims, 0x6345c0 on PC: an index, the file, two flags; the names
// are ours, after the files)
#define RANIM_BREATH 0                  // "breath1.i3d"
#define RANIM_WALK 1                    // "walk1.i3d"
#define RANIM_SALINA_NASTUP 2           // "salina nastup.i3d"
#define RANIM_SALINA_VYSTUP 3           // "salina vystup.i3d"
#define RANIM_SED 4                     // "x sed1.I3D"
#define RANIM_RIGHT 5                   // "right1.i3d"
#define RANIM_LEFT 6                    // "left1.i3d"
#define RANIM_SMRT_STOJ 7               // five deaths on foot, "smrt stoj zada A.I3D" to "smrt stoj zada B.I3D"
#define RANIM_SMRT_ZIDLE 12             // two deaths on the seat, "x smrt zidle01.I3D" and "x smrt zidle02.I3D"
#define RANIM_SALINDRIVER 14            // "salindriver.I3D"
#define RANIM_SED_PANIKA 15             // two, "x sed panika01.I3D" and "x sed panika02.I3D"
#define RANIM_PANIKA_STOJ 17            // six, "panika stoj stativ01a.I3D" to "panika stoj stativ02c.I3D"
#define RANIM_STROJ_TRAM_SMRT 23        // the driver's two deaths, "StrojTramSmrt.I3D" and "StrojTramSmrt02.I3D"
#define RANIM_ZEN_SALSEZ 25             // two, a woman's sitting, "ZenSalsez02.I3D" and "ZenSalsez01.I3D"

// One of the passengers' animations (PS2 ranims' element), 12 bytes (the names are ours)
struct S_rail_anim
{
    int index;                          // 0x0, RANIM_*
    const char *file;                   // 0x4
    bool flag1;                         // 0x8, SetAnim's use_track and load_track
    bool flag2;                         // 0x9, SetAnim's once
};
extern S_rail_anim ranims[27];

// A rail vehicle's passenger (PS2 name; C_railway keeps them in persons, the driver in driver), 0x94 bytes; the
// members' names are ours, after the update's states, Kill's and DoPanic's, the fear timer the latter sets and SetAnim.
struct S_rail_human
{
    int state;                          // 0x00, RH_*
    I3D_frame *frame_model;             // 0x04, the model (AddPerson's)
    C_animations_machine_de_luxe *anim_container;  // 0x08
    int should_keep_sitting;            // 0x0c, AddPerson's rnd(2) + 2
    // 0x10, RANIM_*, the one playing, which SetAnim sets; before the first SetAnim, the entry AddPerson chose (-1 for
    // none), which RH_CHOOSE_ENTRY reads
    int animation_number;
    int new_animation_number;           // 0x14, RANIM_*, the one SetAnim starts
    uint wagon_index;                   // 0x18
    int seat_index;                     // 0x1c
    // Set as the tram's boarding or alighting animation starts, cleared by SetAnim at its end (ANIMTICK_END), which
    // RH_BOARD_ANIM and RH_ALIGHT_ANIM wait for (the name is ours)
    bool anim_running;                  // 0x20
    S_vector target_position;           // 0x24
    S_vector look_target;               // 0x30
    // 0x3c, how long a fear lasts: DoPanic resets it to the time it gets, the update goes back once it is past
    S_timer fear;
    int old_state;                      // 0x44, DoPanic's, which RH_PANIC goes back to
    int old_animation_number;           // 0x48
    S_matrix inverse_global_matrix;     // 0x4c, the collision's inv_matrix points at it
    int unknown_8c;                     // 0x8c
    tDynamicCollObject *collision_data; // 0x90

    // PS2 name
    S_rail_human();
    // Turns the passenger toward the place (PS2 name; the PC passes the vector by value); true while turning
    bool RotateTo(S_vector pos, uint time);
    // Walks the passenger to the place (PS2 name; the PC passes the vector by value); true while walking
    bool MoveTo(S_vector pos, uint time);
    // Starts new_animation_number if it isn't the one playing and runs the animations for the time (PS2 name; the
    // parameter's name is ours, the animation machine's and the frame's Tick get it); the rest of the class calls it
    void SetAnim(uint time);
    // Kills the passenger (PS2 name): a random death animation for its state, the vehicle's lock given back if it was
    // boarding or leaving (Unlock), then RH_DYING
    void Kill(C_railway *tram);
    // PS2 names
    void CreateColl(C_railway *tram);
    void UpdateColl();
    void Done();
    void DoPanic(uint time);
    // Whether the model is a woman's: the code in its "back1" frame's property (PS2 name)
    bool IsWoman();
};

// An entry of a rail vehicle (PS2 rail_entry), 0x18 bytes, rail_entries' (C_railway::Enumerate's): where a passenger
// stands inside and outside the vehicle, in its frame's space (the names are ours)
struct rail_entry
{
    S_vector inner_pos;                 // 0x0
    S_vector outer_pos;                 // 0xc
};

// A vehicle's entries (PS2 rail_entries), 8 bytes: C_railway's entries, the vehicle's own, which each wagon repeats
struct rail_entries
{
    uint num_entries;                   // 0x0 (the name is ours)
    rail_entry *entries;                // 0x4 (the name is ours)

    // PS2 names
    void AddEntry(rail_entry *entry);
    void Destroy();
};

// rail_doors::state (the names are ours): the doors' leaves turned to 0, max_angle, and on their way
#define RDOORS_CLOSED 0
#define RDOORS_OPEN 1
#define RDOORS_CLOSING 2
#define RDOORS_OPENING 3

// The doors of a rail vehicle (PS2 rail_doors, C_railway's doors), 0x4c bytes: two sets of leaves turned about axis,
// one each way (the names are ours)
struct rail_doors
{
    I3D_frame **leaves1;                // 0x0, AddFrame's with true
    I3D_frame **leaves2;                // 0x4, AddFrame's with false
    int state;                          // 0x8, RDOORS_*
    uint num_leaves1;                   // 0xc
    uint num_leaves2;                   // 0x10
    S_vector axis;                      // 0x14
    S_quat rot1;                        // 0x20, leaves1's, about axis by -angle
    S_quat rot2;                        // 0x30, leaves2's, by angle
    float angle;                        // 0x40
    float speed;                        // 0x44
    float max_angle;                    // 0x48

    // PS2 names
    void Open();
    void Close();
    void Place();
    void Tick(int time);
    void AddFrame(I3D_frame *frm, bool first);
    void Init();
    void Destroy();
};

// rail_window::state (the names are ours): broken, whole, cracked by the first hit
#define RWINDOW_BROKEN 0
#define RWINDOW_WHOLE 1
#define RWINDOW_CRACKED 2

// A window of a rail vehicle (PS2 rail_window), 0x14 bytes, rail_glass's (the names are ours)
struct rail_window
{
    int state;                          // 0x0, RWINDOW_*
    I3D_frame *frame;                   // 0x4
    int wagon_num;                      // 0x8, 0 for the vehicle's own, from 1 for the wagons'
    I3D_mesh_object *cracked_mesh;      // 0xc, the copy of the mesh Hit makes, which carries the cracked glass
    I3D_mesh_object *own_mesh;          // 0x10, the frame's own mesh

    // PS2 names
    rail_window() : state(RWINDOW_WHOLE), frame(NULL), wagon_num(-1), cracked_mesh(NULL), own_mesh(NULL) {}
    void Restore(I3D_material *mat);
    void Hit(const S_vector &pos, I3D_material *mat);
};

// The windows of a rail vehicle (PS2 rail_glass, C_railway's glass), 0x14 bytes (the names are ours)
struct rail_glass
{
    int unknown_0;                      // 0x0
    uint num_windows;                   // 0x4
    rail_window *windows;               // 0x8
    // 0xc, the windows' material (the first window's mesh's LOD 0, face group 0), which rail_window::Restore puts back
    I3D_material *orig_material;
    // 0x10, its copy with the cracked glass's textures ("2ft19r.bmp", "2ft19r+.bmp"), rail_window::Hit's
    I3D_material *glass_material;

    // PS2 names
    void GameInit();
    void GameDone();
    void AddWindow(rail_window *window);
};

// raildummy::type (the names are ours), by the dummies' properties: RAIL_ENTRY_<n>_S/E, RAIL_ROW_<n>_S/E,
// RAIL_COLL, RAIL_DOOR_<n>_R/L, [DEFORM GLASS]
#define RDUMMY_ENTRY 0
#define RDUMMY_ROW 1
#define RDUMMY_COLL 2
#define RDUMMY_DOOR 3
#define RDUMMY_GLASS 4

// A dummy of the vehicle's model that C_railway::Enumerate collects (PS2 raildummy), 0x10 bytes, which cbEnumDummies
// and cbEnumDummiesVagons fill in (the members' names are ours)
struct raildummy
{
    int type;                           // 0x0, RDUMMY_*
    int num;                            // 0x4, the number in the name ("RAIL_ENTRY_<num>_S")
    int side;                           // 0x8, 0 for S and R, 1 for E and L
    I3D_frame *frame;                   // 0xc
};

// A light of a rail vehicle (the names are ours), 0x24 bytes, C_railway's lights: where cbEnumLights found the dummy
// and the projector made for it, which the update keeps at the dummy's place
struct S_rail_light
{
    S_vector pos;                       // 0x0, the dummy's, in parent's space
    S_quat rot;                         // 0xc, the dummy's
    I3D_frame *frame;                   // 0x1c, the projector
    I3D_frame *parent;                  // 0x20, the vehicle's frame
};

// What C_railway::GameInit hands cbEnumLights (the name is ours)
struct S_enum_lights
{
    C_railway *tram;                    // 0x0
    I3D_frame *frame;                   // 0x4, the vehicle's
};

// A crossing's semaphore the rails pass (PS2 S_rail_semaphore), 8 bytes: the road crossing and the rail's node nearest
// it, which GameInit marks with WEBF_UNKNOWN_800 (the names are ours)
struct S_rail_semaphore
{
    S_road_crossing *crossing;          // 0x0
    C_web_node *node;                   // 0x4
};

// A node the rail vehicles have stopped at (PS2 S_shedule_item), 8 bytes: when, in ms (timeGetTime; the names are
// ours)
struct S_shedule_item
{
    C_web_node *node;                   // 0x0
    uint time;                          // 0x4

    S_shedule_item(C_web_node *node, uint time) : node(node), time(time) {}
};

// C_railway::rlw_state (the names are ours, after what the states do in AI): driving to the wanted speed, standing
// still at a stop, the doors open for the passengers leaving, then entering, the wait with them open, the wait for
// the schedule, leaving the stop, braking for an obstacle, an unused state, slowing down for a stop, for a signal,
// braking to a halt with the driver shot (Hit; hidden, the vehicle gets him back) and following the vehicle ahead
#define RLW_DRIVE 0
#define RLW_STOPPED 1
#define RLW_LEAVING 2
#define RLW_ENTERING 3
#define RLW_DOORS_OPEN 4
#define RLW_SCHEDULE 5
#define RLW_DEPART 6
#define RLW_OBSTACLE 7
#define RLW_UNUSED 8
#define RLW_STOP_SLOW 9
#define RLW_SIGNAL_SLOW 10
#define RLW_NO_DRIVER 11
#define RLW_FOLLOW 12

// C_railway::unknown_e4 (the names are ours, by AI's use)
#define RLWF_BLOCKED 2
#define RLWF_STOP 4

// The rail vehicles (PS2 C_railway, with doors, windows and seats; PS2's C_railway.cpp, whose order the PC's file at
// 0x4826c0-0x49908c keeps: the seats, the passengers, then the vehicle, its entries, doors and windows), 0x370 bytes
// (PS2's 0x390): the tram, or the elevated railway's train (metro)
// VTABLE: GAME 0x624a70
class C_railway : public C_actor
{
public:
    // Puts the model among the vehicle's passengers at the entry (PS2's parameters; false for none)
    bool AddPerson(I3D_model *model, int wagon, int entry);
    C_railway();
    virtual ~C_railway();
    // The vtable's other overrides (the slots' names are C_actor's); NetDirect is the linker's folded empty function
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual void ChangeState();
    virtual void GetActorSpeed(S_vector &speed);
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual int GetMaterialID(I3D_frame *frm);
    virtual bool IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll);
    virtual bool GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm);
    // The player's sounds of the vehicle, inside and outside, as he gets in and out (PS2 names,
    // C_human::Intern_UseRailway)
    void RemoveOuterEntries();
    void AddInnerEntries();
    void RemoveInnerEntries();
    void AddOuterEntries();
    // Counts lock_count down, not below 0 (PS2 name)
    void Unlock();
    // PS2 name; the parameters' names are ours
    void SetRainDummy(bool in, I3D_frame *frm);
    // The wagon's frame, from 1 (C_human::wagon), the vehicle's own for 0, NULL past the last (PS2 name)
    I3D_frame *GetVagon(uint wagon);
    // The entry nearest the actor: its position and direction, and the wagon it leads into, from 1, 0 for the
    // vehicle itself (PS2 name; the parameters' names are ours); false for none
    bool GetEntry(C_actor *act, S_vector *pos, S_vector *dir, uint *wagon);
    // Counts lock_count up (PS2 name), which Unlock counts down
    void Lock();
    // The path's nodes (PS2 name; PS2's takes a bool and has a copy of its own)
    inline C_web_node *GetCheckpoints();

    // The file's other functions (PS2 names and parameters, by value where the PC's frames show it; the parameters'
    // names are ours)
    void trafficAI(uint time);
    bool WantLeave();
    bool WantEnter();
    void CreateBackVector(S_vector pos);
    void CreateForwardVector(uint index, float dist);
    // Whether the path node is a stop: the linker folded the PC's copy with another class's identical one
    bool IsStopPoint(uint index);
    bool IsSemaPoint(uint index);
    // The distance to the next stop or semaphore ahead, -1 for none; flags gets what it is (8 or 0x800)
    float MustStop(uint *flags);
    S_vector FindBackPos(S_vector pos, float dist, int *index);
    void SpringProcess(uint time, int index, S_vector *out, S_vector *dir, S_vector p1, S_vector p2);
    // Sets visibility from the player's distance (PS2 name; inline on the PC, AI's)
    void CheckDistance(S_vector pos);
    // The curve of spheres of the stretch ahead (PS2 name and parameter; the gif::vector is a std::vector here)
    void FillCurve(std::vector<g_collision::gcol_CurveSphereStruct> *curve);
    // Whether the vehicle has to brake: a car or pedestrian meets its curve (PS2 name)
    bool PreduceMovement();
    void Place(int time);
    void Collide();
    // The speed the stretch ahead allows (PS2 name)
    float GetSafeSpeed(float &cos);
    void RemovePerson(S_rail_human *person);
    uint GetNearestVagon(S_vector *p);
    void Enumerate();
    void DestroyEntries();
    void GenEntries();
    bool GetOuterEntry(C_actor *act, S_vector *pos, S_vector *dir, uint *wagon);
    bool GetOuterEntry(uint entry, uint wagon, S_vector *pos, S_vector *dir);
    bool GetInnerEntry(C_actor *act, S_vector *pos, S_vector *dir, uint *wagon);
    bool GetOuterEntry(S_vector pos, S_vector *out_pos, S_vector *dir, uint *wagon);
    bool GetInnerEntry(S_vector pos, S_vector *out_pos, S_vector *dir, uint *wagon);
    void ExportEntries();
    bool isInStation();
    bool isInStop();
    bool IsOnSchedule(C_web_node *node);
    void DoPanic(uint time, uint how);
    void SetRLWState(uint state);
    void DoBrousicka();
    // Update's, between GetBoundPoints2D and SetRainDummy, no PS2 counterpart (the names are ours): the vehicle ahead
    // on the same rails, and the stop behind it closer than 40 m
    void FindVehicleAhead();
    void StopBehindVehicleAhead();
    // Copy count elements of size bytes between the saved game's record and memory, and move the record's pointer past
    // them (PS2 names): the linker folded the PC's copies with ai_follow_manager's
    void MemWrite(void **buf, void *src, uint count, uint size);
    void MemRead(void **buf, void *dst, uint count, uint size);

    // Set while the traffic is hidden (C_game::SetTrafficVisible's; PS2 name, Czech slang: "the tram in the
    // cinema"); the constructor and other functions of the file set it too
    static bool salina_v_kine;
    // The nodes the vehicles stopped at lately (IsOnSchedule's), the semaphores along the rails (GameInit's) and the
    // vehicles (PS2 names)
    static std::vector<S_shedule_item *> shedule_table;
    static std::vector<S_rail_semaphore *> semaphores;
    static std::vector<C_railway *> rlws;
    // The dummy the rain stops at while the player is inside (SetRainDummy's; PS2 name)
    static I3D_dummy *rain_dummy;

    S_vector unknown_70;                // 0x70, zero from SaveGameLoad
    // The path's nodes behind the vehicle, the one it is at [0] (CreateBackVector's; the name is ours)
    std::vector<uint> back_nodes;       // 0x7c
    // How far back they reach: the vehicle's and its wagons' length and 30 more (GameInit's; the name is ours)
    float back_dist;                    // 0x8c
    S_vector front_pos;                 // 0x90, the vehicle's front on the path (the name is ours)
    S_vector back_pos;                  // 0x9c, its back (the name is ours)
    // The path's nodes ahead (CreateForwardVector's; the name is ours)
    std::vector<uint> forward_nodes;    // 0xa8
    // Each wagon's and the vehicle's (the names are ours): a random angle (GameInit's, SpringProcess's), position and
    // direction
    float *wagon_angles;                // 0xb8
    S_vector *wagon_pos;                // 0xbc
    S_vector *wagon_dirs;               // 0xc0
    // The wagons' frames, the first wagon's [0]: C_human::wagon 1 and up indexes it from 1 (the name is ours)
    I3D_frame **wagon_frames;           // 0xc4
    S_vector *unknown_c8;               // 0xc8, num_wagons + 1, zero while the vehicle stands
    S_vector *unknown_cc;               // 0xcc, likewise
    float safe_speed;                   // 0xd0, GetSafeSpeed's, the speed AI drives to (the name is ours)
    float drive_speed;                  // 0xd4, the speed (the name is ours)
    uint state_time;                    // 0xd8, ms in rlw_state, 10000 from the constructor (the name is ours)
    uint rlw_state;                     // 0xdc, RLW_* (SetRLWState's; the name is ours)
    // The state to go back to after RLW_FOLLOW or RLW_NO_DRIVER (the name is ours)
    uint saved_rlw_state;               // 0xe0
    uint unknown_e4;                    // 0xe4, RLWF_*
    // The vehicle's collision, then one for each wagon (the name is ours)
    tDynamicCollObject *colls;          // 0xe8
    // The collisions of the model's RAIL_COLL dummies (Enumerate's; the name is ours)
    std::vector<tDynamicCollObject *> dummy_colls;  // 0xec
    // The dynamic collisions in the vehicle, the vehicle's own [0], then one a wagon, which C_human::Intern_UseRailway
    // puts the human's into (the name is ours)
    std::vector<tDynamicCollObject *> *dyn_colls;  // 0xfc
    // The passengers' collisions (AddPerson's; the name is ours)
    std::vector<tDynamicCollObject *> person_colls;  // 0x100
    // The inverse of each frame's world matrix, the vehicle's first (the name is ours)
    S_matrix *inv_matrices;             // 0x110
    uchar unknown_114[0x44];            // 0x114
    S_timer horn_timer;                 // 0x158, the horn's, 7000 ms in AI (the name is ours)
    // The vehicle ahead on the same rails, this one for none (FindVehicleAhead's; the name is ours)
    C_railway *ahead;                   // 0x160
    // The "DUMMY_COL" frame and its matrix without the scale, the vehicle's collision's when set (the names are ours)
    I3D_frame *coll_frame;              // 0x164
    S_matrix coll_matrix;               // 0x168
    // The wagons, 0 for the vehicle alone: GetVagon's last (DuplicateFrom copies it; the name is ours)
    int num_wagons;                     // 0x1a8
    // The vehicle's and its wagons' parameters (SaveData's and LoadData's, in that order but vehicle_length; the names
    // are ours)
    float vehicle_length;               // 0x1ac, 8 from the constructor: front_pos and back_pos are half of it away
    float wagon_length;                 // 0x1b0, 17 from the constructor
    float max_speed;                    // 0x1b4, 12.5 from the constructor (DuplicateFrom copies it)
    float acceleration;                 // 0x1b8, 3 from the constructor
    float vehicle_mass;                 // 0x1bc, 10000 from the constructor
    float inv_mass;                     // 0x1c0, 1 / vehicle_mass
    // How far ahead the vehicle looks: its braking distance (the name is ours)
    float brake_dist;                   // 0x1c4
    float inv_inertia;                  // 0x1c8, GameInit's (the name is a guess)
    S_vector move_velocity;             // 0x1cc (the name is a guess)
    // Set for the elevated railway's train ("metro.i3d"; the name is ours): its own using objects BOARD_TRAIN and
    // EXIT_TRAIN, no radar marks, C_human::Use_Actor walks the human in and out along the frame's direction
    // (animation 6 for 800 ms), else plays animation 0x140 or 0x141
    bool metro;                         // 0x1d8
    // Set by Collision for a hit through one of the vehicle's frames (the name is ours)
    bool collided;                      // 0x1d9
    // The semaphore phase of the crossing the vehicle faces: C_game's semaphore_phase_x or _z (the name is ours)
    int *semaphore_phase;               // 0x1dc
    rail_entries entries;               // 0x1e0 (the name is ours)
    rail_doors doors;                   // 0x1e8 (the name is ours)
    rail_glass glass;                   // 0x234 (the name is ours)
    S_RailSeats seats;                  // 0x248 (the name is ours)
    std::vector<S_rail_human *> persons;  // 0x250, the passengers (the name is ours)
    std::vector<uint> unknown_260;      // 0x260
    // The using objects of the entries, inside (the exits, at inner_pos) and outside (the boardings, at outer_pos),
    // (num_wagons + 1) * entries.num_entries of each (the names are ours): Add/Remove(Inner|Outer)Entries put them into
    // the game's C_using_object
    C_using_object::S_using_object *inner_entries;  // 0x270
    C_using_object::S_using_object *outer_entries;  // 0x274
    std::vector<S_rail_light> lights;   // 0x278 (the name is ours)
    // The passengers boarding or leaving, which Lock counts up and Unlock down (the name is ours; unsigned: Unlock's
    // test is jbe)
    uint lock_count;                    // 0x288
    // The engine's, the horn's and the pantograph shoe's sounds, and their sound table ids (the names are ours)
    I3D_sound *sounds[3];               // 0x28c
    I3D_frame *unknown_298;             // 0x298
    int sound_ids[3];                   // 0x29c
    // 0 shown, 2 to be shown, 1 to be hidden, others (3) hidden: CheckDistance's by the player's distance (the name is
    // ours)
    int visibility;                     // 0x2a8
    // GetMaterialID's, a materials.def index (materialList's; the name is ours)
    int material;                       // 0x2ac (PS2 0x330)
    S_rail_human driver;                // 0x2b0 (the name is ours)
    float center_z;                     // 0x344, the z the seats are split at (FindFreeSeat's dist; the name is ours)
    // The bounding volume, which the dying passengers stay in (the name is ours)
    I3D_bvolume bound;                  // 0x348
};

// SYNTHETIC: GAME 0x486740
// C_railway::`scalar deleting destructor'
