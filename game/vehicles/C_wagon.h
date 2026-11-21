#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"
#include "mission/Mission.h"
#include "ai/pathweb.h"

class C_program;

// A wagon's saved game, after the actor's (the name is ours), 0x5f bytes: SaveGameSave writes the members in this
// order, then the nodes of forward_nodes and of back_nodes (4 bytes each). PS2's has the same fields, 0x64 bytes.
#pragma pack(push, 1)
struct S_wagon_savegame
{
    uchar version;                      // 0x0, WAGON_SAVEGAME_VERSION
    S_vector pos;                       // 0x1
    S_vector dir;                       // 0xd
    S_vector front_pos;                 // 0x19
    S_vector back_pos;                  // 0x25
    float safe_speed;                   // 0x31
    float drive_speed;                  // 0x35
    bool stopped;                       // 0x39
    uint stop_time;                     // 0x3a
    bool found_next;                    // 0x3e
    int deactivate_count;               // 0x3f
    int blocker_number;                 // 0x43, the number of blocker's actor, -1 for none
    int event_actor_number;             // 0x47, the number of the actor of event_program's script, -1 for none
    int event_id;                       // 0x4b
    ushort num_forward_nodes;           // 0x4f
    ushort num_back_nodes;              // 0x51
    S_vector node_dir;                  // 0x53
};
#pragma pack(pop)

// The version of the wagon's saved game, the only one SaveGameLoad takes (the name is ours)
#define WAGON_SAVEGAME_VERSION 1

// The wagons (PS2 C_wagon, C_wagon.cpp), 0x1a8 bytes (PS2's 0x1e0): the elevated railway's scripted train ("nadzemka"),
// a rail vehicle of its own that drives along its path as C_railway does, with the same helpers (CreateBackVector,
// CreateForwardVector, MustStop, FindBackPos, SpringProcess) and many of the same members, named as C_railway's. Its
// model's copies are the carriages behind it.
// VTABLE: GAME 0x624e78
class C_wagon : public C_actor
{
public:
    C_wagon();
    virtual ~C_wagon();
    // The vtable's other overrides (the slots' names are C_actor's), the linker's folded constant functions and those
    // that only call the base's among them
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk)
    {
        return true;
    }
    virtual bool LoadData(C_chunk *chunk)
    {
        return true;
    }
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void DuplicateFrom(C_actor *act)
    {
        C_actor::DuplicateFrom(act);
    }
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours, after the
    // scripts' WAGON_SETEVENT and WAGON_GETLASTNODE)
    // FUNCTION: GAME 0x47b6f0
    void SetEvent(C_program *program, int id)
    {
        event_program = program;
        event_id = id;
    }
    // The id of the path node the wagon is at, 0 without one. The size is tested around the read: with an early
    // return for 0 VC6 reads the vector's array again. Either way it stores the size to a slot nothing reads, as the
    // original's copy does.
    // FUNCTION: GAME 0x47b710
    int GetLastNode()
    {
        if (back_nodes.size())
            return act_mission->web_path->nodes[back_nodes[0]].id;
        return 0;
    }

    // The path's nodes (PS2 name, as C_railway's)
    C_web_node *GetCheckpoints()
    {
        return act_mission->web_path->nodes;
    }
    // The file's other functions (PS2 names and parameters, by value where the PC's `ret N` shows it; the parameters'
    // names are ours)
    void CreateBackVector(S_vector pos);
    void CreateForwardVector(uint index, float dist);
    // Whether the path node is a stop: the linker folded the PC's copy with C_railway's identical one
    bool IsStopPoint(uint index);
    // The distance to the next stop ahead, -1 for none
    float MustStop();
    S_vector FindBackPos(S_vector pos, float dist, int *index);
    void SpringProcess(uint time, int index, S_vector *out, S_vector *dir, S_vector p1, S_vector p2);
    // The path ahead of every wagon made again, after a turnout switched (C_turnout::AI's)
    static void RecompileNewPath();
    // AI's sweep along the move d (C_railway::Collide's twin; an inline helper of ours, which PS2 doesn't have)
    inline void Collide(S_vector &d);

    // The direction to back_nodes[0] when the wagon took that node: past the node once the direction to it turns
    // away from this one (the name is ours)
    S_vector node_dir;                  // 0x70
    // The path's nodes behind the wagon, the one it goes to [0] (CreateBackVector's; the name is C_railway's)
    std::vector<uint> back_nodes;       // 0x7c (PS2 0x100)
    float back_dist;                    // 0x8c, how far back they reach (the name is C_railway's)
    S_vector front_pos;                 // 0x90, the wagon's front on the path (the name is C_railway's)
    S_vector back_pos;                  // 0x9c, its back (the name is C_railway's)
    // The path's nodes ahead (CreateForwardVector's; the name is C_railway's)
    std::vector<uint> forward_nodes;    // 0xa8 (PS2 0x110)
    // The wagon's and its carriages' spring angles (num_wagons + 1), then the carriages' positions, directions and
    // frames (num_wagons; the names are C_railway's)
    float *wagon_angles;                // 0xb8
    S_vector *wagon_pos;                // 0xbc
    S_vector *wagon_dirs;               // 0xc0
    I3D_frame **wagon_frames;           // 0xc4
    // How far the wagon's and each carriage's position and direction moved in the last AI (num_wagons + 1; the names
    // are ours)
    S_vector *pos_deltas;               // 0xc8
    S_vector *dir_deltas;               // 0xcc
    float safe_speed;                   // 0xd0, the speed AI drives to (the name is C_railway's)
    float drive_speed;                  // 0xd4, the speed (the name is C_railway's)
    // Waiting at a stop, and for how long in ms: 10 s, then the wagon drives on (the names are ours)
    bool stopped;                       // 0xd8
    uint stop_time;                     // 0xdc
    tDynamicCollObject coll;            // 0xe0, the wagon's collision (the name is ours)
    S_matrix coll_matrix;               // 0x128, its matrix's inverse (the name is C_railway's)
    // Set while CreateForwardVector finds the next node ahead, which AI extends the path through (the name is ours)
    bool found_next;                    // 0x168
    // The wheels, the model's frames named "kolo" (S_hlp::cbEnumFrame's), which Update turns (the name is ours)
    std::vector<I3D_frame *> wheel_frames;  // 0x16c (PS2 0x1a4)
    float inv_wheel_radius;             // 0x17c, from the first wheel's height (the name is a guess)
    int num_wagons;                     // 0x180, the carriages, 0 from the constructor (the name is C_railway's)
    // The vehicle's and its carriages' parameters (the names are C_railway's)
    float vehicle_length;               // 0x184, 5.2 from the constructor
    float wagon_length;                 // 0x188, 9.4 from the constructor
    float max_speed;                    // 0x18c, 5 from the constructor
    float acceleration;                 // 0x190, 0.5 from the constructor
    // "nadzemka.wav", which follows the wagon at a volume of drive_speed / max_speed (the name is ours)
    I3D_sound *sound;                   // 0x194
    // The script and the event WAGON_SETEVENT gave the wagon (PS2 0x1cc and 0x1d0, the names are ours)
    C_program *event_program;           // 0x198
    int event_id;                       // 0x19c
    // Updates left before the wagon deactivates itself, 2 from GameInit (the name is a guess)
    int deactivate_count;               // 0x1a0
    // The wagon in the way that stopped this one, NULL for none (the name is ours)
    C_actor *blocker;                   // 0x1a4
};

// SYNTHETIC: GAME 0x4f1c00
// C_wagon::`scalar deleting destructor'
