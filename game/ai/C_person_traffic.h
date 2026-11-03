#pragma once

#include "actors/C_actor.h"

class C_car;
class C_animations_machine_de_luxe;

// A pedestrian of the generators (PS2 C_traffic_element; its file is the generator's on PC). Only what the generator's
// static functions use so far; the names are ours, PS2's offsets in the comments.
class C_traffic_element
{
public:
    // PS2 names: ends its waving at a taxi, and takes it out of the game
    void EndMavaniNaTaxi();
    void Deactivate();
    // The pedestrian by the time passed, which TickElements calls for the active ones (PS2 name)
    void Tick(uint time);
    // PS2 name; TryGoToAssHole calls it once it has set way
    void SetNewNode();
    // PS2 name, which C_human::GameDone calls for the pedestrian a human was made of (the parameters' names are
    // ours)
    void ActivateFree(bool b1, bool b2);

    // The name is a guess. TryGoToAssHole's comparison of the navigation points reads the position through it: the two
    // inline calls still to come starve its first nested Dot, which the original calls out of line.
    const S_vector &GetPos() const
    {
        return pos;
    }

    // 0x0 (PS2 0x180), 0x4 (PS2 0x184) and 0x19e (PS2 0x21e): TryGoToAssHole sends only a pedestrian with the first and
    // the last clear and the second set to a car. The first is set by PS2's Tick once the health is gone, taking its
    // collision and shadows, the second is its C_traffic_generator (PS2 CreateModel's, which SetModel clears), the
    // last PS2's SetPanic sets once for a panic, with the place it runs to, and Tick clears with the panic (the name is
    // a guess).
    bool dead;
    uchar unknown_1[3];
    void *generator;
    uchar unknown_8[4];
    bool active;                        // 0xc (PS2 0x18c)
    uchar unknown_d[3];
    S_vector pos;                       // 0x10 (PS2 0x0)
    S_vector dir;                       // 0x1c (PS2 0x10), the direction it walks in
    uchar unknown_28[0x18];
    I3D_model *frame;                   // 0x40 (PS2 0x190)
    uchar unknown_44[0x10];
    float dist;                         // 0x54 (PS2 0x1a4), the camera's squared distance in the horizontal plane
    uchar unknown_58[0xc];
    S_vector target;                    // 0x64 (PS2 0x40), the point it walks to
    uchar unknown_70[0x10];
    // 0x80 (PS2 0x50), the points of a walk to a car: the car's placement (C_car::GetPlacementFromTCK1) and two of its
    // navigation points, each put on the ground
    S_vector way[3];
    S_vector way_dir;                   // 0xa4 (PS2 0x80), the placement's direction
    int num_way;                        // 0xb0 (PS2 0x1c4), way's points
    uchar way_index;                    // 0xb4 (PS2 0x1c8)
    uchar unknown_b5[0xe9];
    bool fleeing;
    uchar unknown_19f[6];
    bool unknown_1a5;                   // 0x1a5 (PS2 0x225), C_human::GameDone clears it
    uchar unknown_1a6[2];
    C_car *car;                         // 0x1a8 (PS2 0x228), the parked car it walks to
    uchar unknown_1ac[0x52];
    bool waving;                        // 0x1fe (PS2 0x246), waving at a taxi
};

// The pedestrians' generator (PS2 C_traffic_generator, C_person_traffic.cpp), 0xb8 bytes (PS2's 0xd0). Only the
// constructor and the static functions C_game's Init, Done and Tick call, and what TickElements calls, so far; the
// constructor, which C_mission::CreateActor calls for ACTOR_TRAFFIC_GENERATOR, is a stub.
// VTABLE: GAME 0x623fa0
class C_traffic_generator : public C_actor
{
public:
    C_traffic_generator();
    virtual ~C_traffic_generator();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    // PS2 names, of the generators' shared state: the taxi driver's passengers on or off (off ends the waving), whether
    // the player's car is a taxi, the pedestrians shown or hidden, the pedestrians and their animations freed, and the
    // pedestrians taken out of the game
    static void EnableTaxiDriver(bool enable);
    static void ZrusVsechnyMavajici();
    static void PlayerInTaxi(C_car *car);
    static void SetVisible(bool visible);
    static void ClearElements();
    static void FreeDynCollElements();
    // The pedestrians by the time passed, which C_game::Tick calls (PS2 name)
    static void TickElements(uint time);
    // PS2 names, which TickElements calls: every 2123 ms (try_do_rici) a pedestrian sent to a parked car, and a
    // passenger made for the player's taxi
    static void TryGoToAssHole();
    static void GeneratePassengerForTaxi();
    // Whether a pedestrian walks to the car (PS2 name)
    static bool IsCarHasOcas(C_car *car);
    // The player has a weapon out (PS2 name, Czech, "the player has it drawn"), which C_shvestky::Tick calls for a
    // human on foot with a weapon in the hand
    static void PlayerHoMaVytaseneho();
    // Gives a pedestrian of that model, or the actor, to the generators (PS2 name; the bool's name is ours)
    static void AddFreePerson(I3D_model *model, C_actor *act, bool unknown);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours): NPC_SHUTUP's
    // FUNCTION: GAME 0x47b780
    static void SetGlobalValidScream(bool b)
    {
        global_valid_scream = b;
    }

    // 0x70, a generator without a range: C_game::SetActiveTrafficP takes the nearest of them where the camera is in no
    // other's range (the name is ours)
    bool no_range;
    uchar unknown_71[3];                // 0x71
    float range;                        // 0x74 (the name is ours)
    bool active;                        // 0x78, the one C_game::SetActiveTrafficP chose (the name is ours)
    uchar unknown_79[3];                // 0x79
    uchar unknown_7c[0x3c];             // 0x7c

    // Whether the pedestrians may scream (PS2 name; PS2's ElementNewScream tests it): C_game::Init sets it, NPC_SHUTUP
    // clears it with a nonzero parameter
    static bool global_valid_scream;
    // PS2 names: the pedestrians, whether they are shown, whether the taxi driver has passengers, whether the player
    // drives a taxi, and the pedestrians' animations
    static std::vector<C_traffic_element *> array;
    static bool all_visible;
    static bool enable_taxi_driver;
    static bool player_in_taxi;
    static std::vector<C_animations_machine_de_luxe *> anims;
    // PS2 names, TickElements' times, in ms: counted down to 0, counted down to TryGoToAssHole (try_do_rici, the same
    // name in Czech), and counted down to GeneratePassengerForTaxi while the player drives a taxi
    static int t_horn;
    static int try_do_rici;
    static int taxi_timer;
    // PS2 names, its GameInit's (PS2's FreerideSetupFromScript sets the police's): C_game::DoneIntern sets both to 1
    static float ratio_all;
    static float ratio_police;
};

// SYNTHETIC: GAME 0x44d480
// C_traffic_generator::`scalar deleting destructor'

// The don's tail (PS2 C_don_ocas, "ocas" Czech for a tail), ACTOR_DON_OCAS, 0x70 bytes: C_game::Init creates one for
// the game. Only the constructor so far.
// VTABLE: GAME 0x623ef8
class C_don_ocas : public C_actor
{
public:
    C_don_ocas();
    virtual ~C_don_ocas();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void GameInit();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual bool IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll);
    virtual bool GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm);
};

// SYNTHETIC: GAME 0x44c660
// C_don_ocas::`scalar deleting destructor'
