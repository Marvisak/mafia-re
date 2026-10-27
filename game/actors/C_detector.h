#pragma once

#include <string.h>

#include "actors/C_actor.h"
#include "mission/C_program.h"
#include "tools.h"

struct tDynamicCollObject;

// The script objects (PS2 C_detector, C_detector.cpp), an actor and its script: C_program is the second base (at 0x70,
// PS2's 0x80, whose this adjustors PS2 names @128@), 0x128 bytes (PS2's 0x150). Only the constructor, the destructor,
// Init and LoadData so far, which C_mission::CreateActor and OpenBin call for ACTOR_DETECTOR, GameInit and GameDone,
// which C_game::Init and Done call, and the vtable's other functions from DuplicateFrom to SaveGameLoad as stubs. PS2's
// save functions override C_program's too (its @128@ thunks), which doesn't declare them yet.
// VTABLE: GAME 0x6239b0 C_actor
// VTABLE: GAME 0x623a48 C_program
class C_detector : public C_actor, public C_program
{
public:
    // Cleared through a register of its own after the other members' zero stores (PS2's memset of its 0x140): an inlined
    // memset
    struct S_unknown_108
    {
        int unknown_0[4];

        S_unknown_108() { memset(this, 0, sizeof(*this)); }
    };

    C_detector();
    virtual ~C_detector();
    // The script runs for the actor itself
    virtual bool Init(I3D_frame *frm);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    // Takes the collisions back (PS2 name)
    void EraseColls();

    int unknown_d8;                     // 0xd8, 0 from Init, then LoadData's (PS2 0x108)
    // GameInit clears them (PS2 0x10c to 0x10f)
    bool unknown_dc;                    // 0xdc
    bool unknown_dd;                    // 0xdd
    bool unknown_de;                    // 0xde
    bool unknown_df;                    // 0xdf
    // What the player can use, which GameDone takes from the game's objects (PS2's at 0x110; the name is ours)
    C_using_object::S_using_object using_object;  // 0xe0
    S_unknown_108 unknown_108;          // 0x108
    // The collisions PS2's CreateColls makes and EraseColls frees (PS2 0x100, 0xe0, 0xe4 and 0x104; the names are ours):
    // the objects, most likely their matrices and the matrices' inverses, as C_game's inter-city boxes, and the count
    tDynamicCollObject **colls;         // 0x118
    S_matrix *coll_matrices;            // 0x11c
    S_matrix *coll_inv_matrices;        // 0x120
    int num_colls;                      // 0x124
};
