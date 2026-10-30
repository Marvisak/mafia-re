#pragma once

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
    C_detector();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual void SaveData(C_chunk *chunk);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    // C_program's: the detector's commands first, which Process handles (a stub); the original's other three only
    // call C_program's
    virtual bool DecodeParams(char *params, S_instruction &ins);
    virtual int DecodeInstruction(char *line, int &len);
    virtual void DecompileCommand(int i, char *s);
    virtual void Process(uint time);
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
    // The script's (PS2 name)
    // FUNCTION: GAME 0x434020
    virtual std::vector<S_instruction> *GetProgram()
    {
        return &instructions;
    }

    // Takes the collisions back (PS2 name)
    void EraseColls();

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b750
    bool GetSignal() const
    {
        return signal;
    }
    // The signal set, which clears the actor's unknown_65 first (PS2 inlines it the other way round)
    // FUNCTION: GAME 0x47b760
    void SetSignal(bool on)
    {
        unknown_65 = false;
        signal = on;
    }

    int unknown_d8;                     // 0xd8, 0 from Init, then LoadData's (PS2 0x108)
    // GameInit clears them (PS2 0x10c to 0x10f)
    bool signal;                        // 0xdc, DETECTOR_SETSIGNAL's (the name is ours)
    bool unknown_dd;                    // 0xdd
    bool unknown_de;                    // 0xde
    bool unknown_df;                    // 0xdf
    // What the player can use, which GameDone takes from the game's objects (PS2's at 0x110; the name is ours): its
    // item is cleared through a register of its own after the other members' zero stores, S_GameItem's memset inlined
    C_using_object::S_using_object using_object;  // 0xe0
    // The collisions PS2's CreateColls makes and EraseColls frees (PS2 0x100, 0xe0, 0xe4 and 0x104; the names are ours):
    // the objects, most likely their matrices and the matrices' inverses, as C_game's inter-city boxes, and the count
    tDynamicCollObject **colls;         // 0x118
    S_matrix *coll_matrices;            // 0x11c
    S_matrix *coll_inv_matrices;        // 0x120
    int num_colls;                      // 0x124
};

// SYNTHETIC: GAME 0x432910
// C_detector::`scalar deleting destructor'
