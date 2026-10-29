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

    uchar unknown_0[0xc];
    bool active;                        // 0xc (PS2 0x18c)
    uchar unknown_d[0x33];
    I3D_frame *frame;                   // 0x40 (PS2 0x190)
    uchar unknown_44[0x1ba];
    bool waving;                        // 0x1fe (PS2 0x246), waving at a taxi
};

// The pedestrians' generator (PS2 C_traffic_generator, C_person_traffic.cpp), 0xb8 bytes (PS2's 0xd0). Only the
// constructor and the static functions C_game's Init and Done call so far; the constructor, which
// C_mission::CreateActor calls for ACTOR_TRAFFIC_GENERATOR, is a stub.
class C_traffic_generator : public C_actor
{
public:
    C_traffic_generator();

    // PS2 names, of the generators' shared state: the taxi driver's passengers on or off (off ends the waving), whether
    // the player's car is a taxi, the pedestrians shown or hidden, the pedestrians and their animations freed, and the
    // pedestrians taken out of the game
    static void EnableTaxiDriver(bool enable);
    static void ZrusVsechnyMavajici();
    static void PlayerInTaxi(C_car *car);
    static void SetVisible(bool visible);
    static void ClearElements();
    static void FreeDynCollElements();
    // Gives a pedestrian of that model, or the actor, to the generators (PS2 name; the bool's name is ours)
    static void AddFreePerson(I3D_model *model, C_actor *act, bool unknown);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours): NPC_SHUTUP's
    // FUNCTION: GAME 0x47b780
    static void SetUnknown63d728(bool b)
    {
        unknown_63d728 = b;
    }

    int unknown_70;                     // 0x70, without it the camera has to be in the range
    float range;                        // 0x74 (the name is ours)
    int active;                         // 0x78, the one C_game::SetActiveTrafficP chose (the name is ours)
    uchar unknown_7c[0x3c];             // 0x7c

    // C_game::Init sets it (the name is ours; PS2's GameInit sets first_frame), NPC_SHUTUP clears it with a nonzero
    // parameter: most likely whether the pedestrians talk
    static bool unknown_63d728;
    // PS2 names: the pedestrians, whether they are shown, whether the taxi driver has passengers, whether the player
    // drives a taxi, and the pedestrians' animations
    static std::vector<C_traffic_element *> array;
    static bool all_visible;
    static bool enable_taxi_driver;
    static bool player_in_taxi;
    static std::vector<C_animations_machine_de_luxe *> anims;
    // C_game::DoneIntern sets both to 1 (the names are ours)
    static float unknown_632638;
    static float unknown_63263c;
};

// The don's tail (PS2 C_don_ocas, "ocas" Czech for a tail), ACTOR_DON_OCAS, 0x70 bytes: C_game::Init creates one for
// the game. Only the constructor so far.
// VTABLE: GAME 0x623ef8
class C_don_ocas : public C_actor
{
public:
    C_don_ocas();
};
