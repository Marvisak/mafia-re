#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"
#include "tools.h"

// C_door::door_flags (the names are ours)
#define DOORF_IN_GAMEINIT 0x1           // GameInit runs, the linked door's GameInit too: the linked door finds it set
#define DOORF_NO_COLL 0x2               // the door has no collision in gifCollision
#define DOORF_NO_USE 0x4                // the door has no using object (a passive door's and a linked one's)
#define DOORF_SILENT 0x8                // the door plays no sounds (a linked one, whose master plays them)
#define DOORF_REVERSE 0x10              // reverse at GameInit, which GameDone gives back
#define DOORF_LOCKED 0x20               // locked at GameInit, which GameDone gives back
#define DOORF_USING 0x40                // the using object is in the game's C_using_object

// The version of the door's settings SaveData writes (the name is ours)
#define DOOR_DATA_VERSION 8

// A door's saved game, after the actor's (the name is ours), 0xc bytes: SaveGameSave writes the members in the order
// of PS2's, whose version is an int and state the fourth member, 0x10 bytes
#pragma pack(push, 1)
struct S_door_savegame
{
    uchar version;                      // 0x0, DOOR_SAVEGAME_VERSION
    uchar state;                        // 0x1, C_door::E_state
    float angle;                        // 0x2
    uint door_flags;                    // 0x6
    bool reverse;                       // 0xa
    bool locked;                        // 0xb
};
#pragma pack(pop)

// The version of the door's saved game (the name is ours)
#define DOOR_SAVEGAME_VERSION 1

// The doors (PS2 C_door, C_door.cpp), 0x1b8 bytes (PS2's 0x200): a frame turning about its vertical axis, opened and
// closed by the humans (C_human::Use_Actor) and the script (DOOR_OPEN, DOOR_LOCK, DOOR_ENABLEUS, DOOR_SETOPENPOSE). A
// door whose frame is a child of another door's frame opens and closes with it.
// VTABLE: GAME 0x623b20
class C_door : public C_actor
{
public:
    // The door's state (PS2's type; the names are ours, from the script's DOOR_OPEN, which opens a closed door and
    // closes an open one)
    enum E_state
    {
        STATE_OPEN,
        STATE_CLOSED,
        STATE_OPENING,
        STATE_CLOSING
    };

    C_door();
    virtual ~C_door();
    // The vtable's other overrides (the slots' names are C_actor's)
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
    // PS2's, which returns false as C_actor's: the linker folded the two (0x4aa450)
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm)
    {
        return false;
    }
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual bool GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm);

    // PS2 names, which the script calls; the parameters' names are ours
    // Opens or closes the door for the actor: with to_master the topmost door whose linked door it is (the callers
    // pass true; unknown1 is only passed on)
    void SetState(E_state state, C_actor *actor, bool unknown1, bool to_master);
    void EnableUsingObject(bool enable);
    // How far the door is open, 0 to 1
    void SetOpenAngle(float open);

    // The flags and the linked doors cleared, before GameInit finds them again (the name is ours: C_game::Init's, its
    // stores in this order)
    void ResetLinks()
    {
        linked = NULL;
        master = NULL;
        door_flags = 0;
    }

    E_state state;                      // 0x70 (PS2 0xf0, the name is ours)
    float angle;                        // 0x74, how far the door is open, 0 to max_angle (the name is ours)
    tDynamicCollObject coll;            // 0x78, the door's collision, a box (the name is ours)
    S_matrix inv_matrix;                // 0xc0, the inverse of the frame's matrix, coll's (the name is ours)
    S_quat rot;                         // 0x100, the frame's rotation GameInit found, the closed door's (the name is ours)
    uint door_flags;                    // 0x110, DOORF_* (the name is ours)
    // 0x114, the door whose frame is a child of the door's frame, opened and closed with it (the name is ours)
    C_door *linked;
    C_door *master;                     // 0x118, the door whose linked door this one is (the name is ours)

    // The mission's settings, in SaveData's order (the names are ours)
    bool passive;                       // 0x11c, GameInit gives the door neither collision nor using object
    bool open_away;                     // 0x11d, the door opens away from the actor who opens it
    bool reverse;                       // 0x11e, the door turns the other way
    float max_angle;                    // 0x120, how far the open door is turned, PI / 2 at first
    bool open;                          // 0x124, GameInit opens the door
    bool locked;                        // 0x125 (PS2 0x175), DOOR_LOCK's
    float open_speed;                   // 0x128, radians per ms
    float close_speed;                  // 0x12c
    char open_sound[16];                // 0x130, "d_woodop.wav" at first
    char close_sound[16];               // 0x140, "d_woodcl.wav"
    char lock_sound[16];                // 0x150, "d_woodlo.wav", played when a locked door is to open
    uint lock_sound_time;               // 0x160, the game's time of the last lock sound, 0 from GameInit
    // 0x164, the frame is in the scene's omni sector, not in the primary sector, and SetState doesn't move it between
    // sectors
    bool in_omni_sector;
    uchar unknown_165[3];               // 0x165
    S_vector normal;                    // 0x168, the door's horizontal axis across its leaf, in world space
    // 0x174, the point 0.4 beyond the end of the leaf, in the frame's space (GameInit's, GetBoundPoints2D's)
    S_vector leaf_end;
    C_using_object::S_using_object using_object;    // 0x180

    // The frame turned to the door's angle about its axis, and the collision moved with it (the name is ours: the
    // code AI, SetOpenAngle, SaveGameLoad and GameInit share, whose calls the inline calls after it in each caller
    // leave out of line in part, as the original's). Open: the original copies the frame's matrix into a temporary of
    // its own for the inverse (the matrix to it, it to inv_matrix, inv_matrix back to it), where ours reuses m.
    void TurnFrame()
    {
        S_vector axis(0.0f, 1.0f, 0.0f);
        if (reverse)
            axis *= -1.0f;
        S_matrix m = frame->GetMatrix();
        m(3, 0) = m(3, 1) = m(3, 2) = 0.0f;
        axis *= m;
        S_quat q;
        q.Make(axis, angle);
        q = rot * q;
        frame->SetRot(q);
        frame->Update();
        if (!(door_flags & DOORF_NO_COLL))
        {
            m = frame->GetMatrix();
            inv_matrix = m;
            inv_matrix.Inverse(S_matrix(inv_matrix));
            I3D_bvolume bvol;
            frame->GetHRBoundVolume(bvol);
            ((I3D_bsphere &)coll.bsphere).pos = bvol.bsphere.pos * frame->GetMatrix();
            gifCollision.DynUpdate(&coll);
        }
    }
    // The door's using object (the name is ours)
    C_using_object::S_using_object *GetUsingObject()
    {
        return &using_object;
    }

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b790
    E_state GetState() const
    {
        return state;
    }
};

// SYNTHETIC: GAME 0x439370
// C_door::`scalar deleting destructor'
