#pragma once

#include "actors/C_actor.h"

struct g_collision_header;
struct tDynamicCollObject;

// C_bridge::state (the names are ours): the bridge down, its lights blinking and its signal ringing while the cars
// leave it, the start's sound, its leaves going up, up while a steamship needs it (bridgeNeedUp), and going down.
// C_human::TozNaselJsemTenNejvyssiMost takes a bridge from BRIDGE_RAISING to BRIDGE_LOWERING as one a car can't cross.
#define BRIDGE_DOWN 0
#define BRIDGE_SIGNAL 1
#define BRIDGE_START 2
#define BRIDGE_RAISING 3
#define BRIDGE_UP 4
#define BRIDGE_LOWERING 5

// The routes of the steamships (uzly, uzlyDist, S_parnik::route) and the nodes of each (the names are ours): a
// steamship sails a route from its first node to its last, PARNIK_NODES - 1 stretches
#define PARNIK_ROUTES 4
#define PARNIK_NODES 7
// The steamships, two (parnik; the name is ours)
#define PARNIKS 2

// A node of a steamship's route (the names are ours), 0x18 bytes, tables\parnik.def's (C_bridge::LoadData)
struct S_parnik_node
{
    S_vector pos;                       // 0x0
    S_vector dir;                       // 0xc
};

// A steamship (the names are ours), 0x24 bytes: the second bridge's GameInit creates them, its Update sails them
struct S_parnik
{
    // 0x0, the route it sails (PARNIK_ROUTES), 0xffff from C_bridge::Init till GameInit gives it one: the first ship
    // sails the routes 0 and 1, the second 2 and 3
    int route;
    int node;                           // 0x4, the stretch from this node to the next it is on
    int last_node;                      // 0x8, node before the tick
    float pos;                          // 0xc, its place on the stretch, 0 to 1
    I3D_model *model;                   // 0x10, lodriver.i3d, named most_parnik
    float speed;                        // 0x14, up to 5, down to 0 while it waits at a bridge (Update)
    // Leonardo (Tommy.i3d) and Kate (czena06.i3d) on the bow, C_bridge::SetTitanic's, NULL without them
    I3D_model *leonardo;                // 0x18
    I3D_model *kate;                    // 0x1c
    I3D_sound *sound;                   // 0x20, linked to the model
};

// The bridges (PS2 C_bridge, C_bridge.cpp), 0x194 bytes (PS2's 0x1a0): a drawbridge of two leaves, the actor's frame
// and the other (other_frame), which AI raises for the steamships and lowers again, its lights blinking and the
// cars' collisions (nadparnik_coll) in the way while it isn't down. A mission has two (index), the second sails the
// two steamships too (S_parnik). C_mission::CreateActor creates them for ACTOR_BRIDGE.
// VTABLE: GAME 0x625588
class C_bridge : public C_actor
{
public:
    // A collision volume of a leaf (PS2 C_bridge::S_BridgeCol; the members' names are ours), 0x84 bytes: an oriented
    // box of the level's (VOLTYPE_XTOBB or VOLTYPE_OBB) and its matrices in the leaf's space, which GameInit takes
    // and Update moves the volume's by
    struct S_BridgeCol
    {
        g_collision_header *vol;        // 0x0
        S_matrix matrix;                // 0x4
        S_matrix inv_matrix;            // 0x44
    };

    C_bridge();
    virtual ~C_bridge();
    // The vtable's other overrides (the slots' names are C_actor's), but the linker's folded constant functions
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk)
    {
        return true;
    }
    // Reads the steamships' routes (uzly), whatever the chunk
    virtual bool LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act) {}
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf)
    {
        return true;
    }
    virtual void GameInit();
    virtual void GameDone();
    // A player touching the bridge falls dead (HIT_LANDING)
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);

    // The bridge shut down or not (PS2 name): its cycle hurries to its end, and the cars' collision goes
    void ShutDown(bool shut);
    // Leonardo (Tommy.i3d) and Kate (czena06.i3d) put on the steamships' bows, or taken off (PC only, the name is
    // ours): the cheat PARNIK's (GAMEF_TITANIC)
    void SetTitanic(bool on);

    std::vector<S_BridgeCol> colls;     // 0x70, the frame's volumes
    std::vector<S_BridgeCol> other_colls; // 0x80, other_frame's
    // 0x90, the other leaf, the frame named as the actor's frame with its last character one up (Init)
    I3D_frame *other_frame;
    I3D_sound *sound;                   // 0x94, the leaves' sounds (sound_start to sound_down)
    I3D_sound *signal_sound;            // 0x98, the signal's (sound_signal)
    // 0x9c, the semaphore's red lights (watercity.RED5, RED4, RED2, RED3, RED1 and RED6 for the second bridge, RED11,
    // RED10, RED8, RED9, RED7 and RED12 for the first), which blink in turns while the bridge isn't down
    I3D_frame *lights[6];
    // 0xb4, the collision of the frame over the steamships' way (watercity.nadparnik2, nadparnik1 for the first
    // bridge), in the cars' way while the bridge isn't down, and its matrices
    tDynamicCollObject *nadparnik_coll;
    S_matrix nadparnik_matrix;          // 0xb8
    S_matrix nadparnik_inv_matrix;      // 0xf8
    // 0x138, a semaphore phase (0 to 9, phase_colors' index): the chases' path nodes before the bridge wait at it as
    // at C_game's (C_sledovacka::RecompilePath); 1 while the bridge isn't down, 7 while it is (the name is a guess)
    int phase;
    bool shut;                          // 0x13c, ShutDown's
    // 0x140, the bridge's entry in the bridges' tables (bridgeIsUp, bridgeNeedUp): 0 for the bridge whose frame's
    // name starts with L, 1 for the other, which sails the steamships (the name is ours)
    int index;
    int state;                          // 0x144, BRIDGE_*
    float angle;                        // 0x148, the leaves', 0 to max_angle
    float last_angle;                   // 0x14c, the leaves' frames turned to it, -1 from GameInit
    int timer;                          // 0x150, ms left of the state
    int blink_timer;                    // 0x154, ms to the lights' next turn, 0 while they are off
    bool blink;                         // 0x158, which lights are on
    uint wait_time;                     // 0x15c, ms AI waits before it goes on (0 from GameInit)
    // The states' times in ms: down (unused), the signal's, raising, up and lowering
    int down_time;                      // 0x160
    int signal_time;                    // 0x164
    int raise_time;                     // 0x168
    int up_time;                        // 0x16c
    int lower_time;                     // 0x170
    bool reverse;                       // 0x174, the leaves turn the other way
    float max_angle;                    // 0x178
    // The sounds' ids (C_sound_table::ConstructSound): the leaves going up, up, going down, down, the start, the signal
    uint sound_raise;                   // 0x17c
    uint sound_up;                      // 0x180
    uint sound_lower;                   // 0x184
    uint sound_down;                    // 0x188
    uint sound_start;                   // 0x18c
    uint sound_signal;                  // 0x190
};

// SYNTHETIC: GAME 0x567e00
// C_bridge::`scalar deleting destructor'

// The volumes' vectors' templates, which end the file: push_back's insert(P, n, x) and clear's copy (GameInit's)
// TEMPLATE: GAME 0x56aa10
// ?insert@?$vector@US_BridgeCol@C_bridge@@V?$allocator@US_BridgeCol@C_bridge@@@std@@@std@@QAEXPAUS_BridgeCol@C_bridge@@IABU34@@Z

// TEMPLATE: GAME 0x56ad40
// ?copy@std@@YIPAUS_BridgeCol@C_bridge@@PAU23@00@Z
