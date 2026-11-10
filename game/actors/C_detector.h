#pragma once

#include "actors/C_actor.h"
#include "mission/C_program.h"
#include "tools.h"

struct tDynamicCollObject;

// A detector's saved game, after the actor's and the script's (the name is ours), 0xc bytes: SaveGameSave writes the
// members in this order
#pragma pack(push, 1)
struct S_detector_savegame
{
    uchar version;                      // 0x0, DETECTOR_SAVEGAME_VERSION
    int human_types;                    // 0x1
    bool signal;                        // 0x5
    bool wait_for_use;                  // 0x6
    bool wait_for_hit;                  // 0x7
    int action;                         // 0x8, the using object's special action, which SaveGameLoad's CreateUO takes
};
#pragma pack(pop)

// The version of the detector's saved game, the only one SaveGameLoad takes (the name is ours)
#define DETECTOR_SAVEGAME_VERSION 1

// The script objects (PS2 C_detector, C_detector.cpp), an actor and its script: C_program is the second base (at 0x70,
// PS2's 0x80, whose this adjustors PS2 names @128@), 0x128 bytes (PS2's 0x150): C_mission::CreateActor and OpenBin
// make them for ACTOR_DETECTOR, Process runs the detector's script commands.
// VTABLE: GAME 0x6239b0 C_actor
// VTABLE: GAME 0x623a48 C_program
class C_detector : public C_actor, public C_program
{
public:
    C_detector();
    // The vtable's other overrides (the slots' names are C_actor's)
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    // C_program's: the detector's commands first, which Process handles; the original's other three only call
    // C_program's
    virtual bool DecodeParams(char *params, S_instruction &ins);
    virtual int DecodeInstruction(char *line, int &len);
    virtual void DecompileCommand(int i, char *s);
    virtual void Process(uint time);
    virtual ~C_detector();
    // The script runs for the actor itself
    virtual bool Init(I3D_frame *frm);
    virtual bool LoadData(C_chunk *chunk);
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

    // PS2 names: makes the collisions, which follow the frame if dynamic (DETECTOR_CREATEDYNCOLL's) or don't move
    // (DETECTOR_WAITFORHIT's); takes them back
    void CreateColls(bool dynamic);
    void EraseColls();
    // CreateColls' callbacks (PS2's S_hlp, the name of its local struct)
    struct S_hlp;
    // What the player can use, DETECTOR_WAITFORUSE's (PS2 name; the parameters' names are ours): a special action
    // (USING_ACTION_*) at the frame, in the frame's sector where in_sector is set (the command's two integers, the
    // second as whether it is 0)
    void CreateUO(int action, bool in_sector);
    // The player uses the detector (PS2 name): its using object taken away, the script's waiting for it ended
    void UseDetector();

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

    // 0xd8, 0 from Init, then LoadData's (PS2 0x108): the humans DETECTOR_INRANGE counts, bits of C_human::human_type
    // (the name is ours)
    int human_types;
    // GameInit clears them (PS2 0x10c to 0x10f)
    bool signal;                        // 0xdc, DETECTOR_SETSIGNAL's (the name is ours)
    // 0xdd, set by CreateUO, which waits for the player to use the detector, until UseDetector (the name is ours)
    bool wait_for_use;
    // 0xde, set by DETECTOR_WAITFORHIT, which waits for a hit with collisions that don't move; DETECTOR_CREATEDYNCOLL
    // and ERASEDYNCOLL do nothing then (the name is ours, after the command)
    bool wait_for_hit;
    // 0xdf, DETECTOR_CREATEDYNCOLL's collisions, which follow the frame (Update), until DETECTOR_ERASEDYNCOLL (the name
    // is ours)
    bool dyn_colls;
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

// The adjustor thunks of the C_program vtable (this - 0x70), to the destructor and the save functions
// SYNTHETIC: GAME 0x433fe0
// ??_EC_detector@@WHA@AEPAXI@Z

// SYNTHETIC: GAME 0x433ff0
// ?SaveGameGetSize@C_detector@@WHA@AEHXZ

// SYNTHETIC: GAME 0x434000
// ?SaveGameSave@C_detector@@WHA@AE_NPAPAX@Z

// SYNTHETIC: GAME 0x434010
// ?SaveGameLoad@C_detector@@WHA@AE_NPAPAX@Z
