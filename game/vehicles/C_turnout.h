#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"
#include "tools.h"

// The turnout's states (the names are ours): connected to one rail, with the frame swung to its end, or on the way to
// one. Start sends a turnout at either end to the other, AI swings it and connects the path's nodes on arrival.
#define TURNOUT_RAIL1 0                 // at +max_angle, rail1_frame on, the node connected to rail1_node_id
#define TURNOUT_RAIL2 1                 // at -max_angle, rail2_frame on, the node connected to rail2_node_id
#define TURNOUT_TO_RAIL1 2
#define TURNOUT_TO_RAIL2 3

// The rails' turnouts (PS2 C_turnout, C_turnout.cpp), 0x1a0 bytes (PS2's 0x1d0): a frame the player switches
// (C_human::Use_Actor runs Start through the using object), which swings between two rails and connects the path's
// node of the turnout to one rail's node or the other's. The parameters (0x70-0xc8) are the mission's (SaveData's,
// LoadData's, in that order but the switch).
// VTABLE: GAME 0x624d58
class C_turnout : public C_actor
{
public:
    C_turnout();
    virtual ~C_turnout();
    // The vtable's other overrides (the slots' names are C_actor's), the linker's folded constant functions among them
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    // Switches the turnout (PS2 name), which C_human::Use_Actor calls
    void Start();
    // Puts the collision's sphere above the turnout (PS2 name)
    void SetCollPos();
    // The path's nodes of the three ids, false unless all exist and differ (PS2 name and parameters; the parameters'
    // names are ours)
    bool NodesOK(uint &node, uint &rail1_node, uint &rail2_node);

    // The names of the members are ours
    float max_angle;                    // 0x70, the swing's ends, 0.5 from the constructor
    float turn_speed;                   // 0x74, the swing per ms, 0.004 from the constructor
    int sound_id;                       // 0x78, the switch's sound, a sound table id (0x85 from the constructor)
    // The ids of the path's nodes (C_web_node::id, 0x7b, 0x7c and 0x7d from the constructor): the turnout's own, which
    // is connected to one of the other two
    uint node_id;                       // 0x7c
    uint rail1_node_id;                 // 0x80
    uint rail2_node_id;                 // 0x84
    // The two rails' frames' names, "rail1" and "rail2" from the constructor
    char rail1_name[0x20];              // 0x88
    char rail2_name[0x20];              // 0xa8
    // The turnout starts at TURNOUT_RAIL2 (the name is a guess)
    bool switched;                      // 0xc8
    S_matrix rot_matrix;                // 0xcc, the frame's rotation, SetCollPos's
    // The game's using object for the turnout, USING_ACTION_USE_SWITCH (GameInit adds it to C_game's, GameDone takes it
    // out)
    C_using_object::S_using_object using_obj;  // 0x10c
    float angle;                        // 0x144, the swing's angle, from -max_angle to max_angle
    tDynamicCollObject coll;            // 0x148, the turnout's collision
    int state;                          // 0x190, TURNOUT_*
    I3D_sound *sound;                   // 0x194, the switch's sound, played by Start
    I3D_frame *rail1_frame;             // 0x198, on at TURNOUT_RAIL1, the frame named rail1_name
    I3D_frame *rail2_frame;             // 0x19c, on at TURNOUT_RAIL2, the frame named rail2_name
};

// SYNTHETIC: GAME 0x4bc470
// C_turnout::`scalar deleting destructor'
