#pragma once

#include "actors/C_actor.h"

class C_railway;
class C_web_node;
class C_web_path;
struct S_rail_human;

// A track the generator runs a vehicle on (the name is ours, after PS2's rail_tracks), 0x1c bytes: a closed way of the
// path's rail nodes, which GameInit finds
struct S_rail_track
{
    std::vector<C_web_node *> nodes;    // 0x0, the track's nodes in order along the rail
    C_railway *vehicle;                 // 0x10, the vehicle on the track, NULL for none (GetFreeTram's)
    bool metro;                         // 0x14, the elevated railway's (a node of id 999 is on it)
    uint time;                          // 0x18, ms the vehicle has been on the track: AI counts it

    S_rail_track() : vehicle(NULL), metro(false), time(0) {}
};

// The rail vehicles' generator (PS2 C_rail_generator, C_rail_generator.cpp), 0x84 bytes (PS2's 0xa0): it creates the
// trams and the elevated railway's trains ("Salina%u", "metro%u") and their passengers, finds the tracks and the
// semaphores along them, and sends a free vehicle onto a free track out of the camera's sight from time to time.
// VTABLE: GAME 0x6259e8
class C_rail_generator : public C_actor
{
public:
    C_rail_generator();
    virtual ~C_rail_generator();
    // The vtable's other overrides (the slots' names are C_actor's), the linker's folded constant functions among them
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void GameInit();
    virtual void GameDone();

    // The path (PS2 name; PS2's takes a bool more, for its second path)
    inline C_web_path *GetCheckpoints();
    // The file's other functions (PS2 names; the parameters by the PC's `ret N`, their names ours): PS2's
    // GetNextTrackNode and GetPrevTrackNode take a bool more
    int GetNearFreeTrackIDandNodeInRange(C_web_node **node);
    // The node a node's last connection leads to, along the rail or against it (this isn't read)
    C_web_node *GetNextTrackNode(C_web_node *node);
    C_web_node *GetPrevTrackNode(C_web_node *node);
    // The first node along the track from node, forward or back, farther than range from the camera
    C_web_node *GetNodeOutOfRange(bool forward, C_web_node *node);
    // A vehicle no track has
    C_railway *GetFreeTram();

    // The generator's statics (PS2 names): the passengers, the tracks, the vehicles (the trams and the trains both;
    // actor_metro stays empty)
    static std::vector<S_rail_human *> actor_persons;
    static std::vector<S_rail_track *> rail_tracks;
    static std::vector<C_railway *> actor_tram;
    static std::vector<C_railway *> actor_metro;
    // The passengers made (PS2 name), 10 from the constructor
    static int max_person_count;

    // The parameters (SaveData's and LoadData's; the names are ours)
    uint num_vehicles;                  // 0x70, the trams and the trains made, of each, 1 from the constructor
    uint ai_period;                     // 0x74, ms between AI's tries to send a vehicle, 5000 from the constructor
    float range;                        // 0x78, how far from the camera a vehicle starts, 130 from the constructor
    int unknown_7c;                     // 0x7c, 1 from the constructor
    uint ai_timer;                      // 0x80, ms since AI's last try (the name is ours)
};

// SYNTHETIC: GAME 0x596660
// C_rail_generator::`scalar deleting destructor'
