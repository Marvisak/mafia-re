#pragma once

#include "Main.h"
#include "tools.h"

#include <vector>

class C_actor;
class C_car;
class C_human;

// The police (PS2 C_shvestky, C_shvestky.cpp), the game's 0x2da0, 0xb0 bytes. PS2's has the same members, its offsets in
// the comments. Only the constructor, the destructor, GameInit and GameDone are written so far, and the accessors the
// script's commands use.
class C_shvestky
{
public:
    // A policeman's record of the people he is after (PS2 name; PS2's DelNasrany is its): only its destructor is known,
    // which frees a vector at 0x70
    struct S_shvestky
    {
        ~S_shvestky();
    };
    // The road blocks' cars, policemen and enemies (PS2's names), 8 bytes each: the actor and whether GameDone leaves it
    // in the game's lists (the members' names are ours)
    struct S_road_block_car
    {
        C_car *car;                     // 0x0
        bool unknown_4;                 // 0x4
    };
    struct S_road_block_iris
    {
        C_actor *actor;                 // 0x0
        bool unknown_4;                 // 0x4
    };
    struct S_road_block_enemy
    {
        C_actor *actor;                 // 0x0
        bool unknown_4;                 // 0x4
    };

    bool unknown_0;                     // 0x0, SetActive's; PM_SHOWSYMBOL and its group do nothing while it is set
    uchar unknown_1[3];
    S_timer unknown_4;                  // 0x4, PS2's Tick runs TickDoRiciActors at each of its periods
    // Two speeds, POLICEMANAGER_SETSPEED's (km/h made m/s)
    float unknown_c;                    // 0xc, 17 from GameInit (PS2's 27)
    float unknown_10;                   // 0x10, 28 from GameInit (PS2's 1000)
    S_timer unknown_14;                 // 0x14
    S_timer unknown_1c;                 // 0x1c
    S_timer unknown_24;                 // 0x24
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30
    uchar unknown_34[8];                // 0x34
    std::vector<S_shvestky *> unknown_3c;  // 0x3c (PS2 0x40)
    std::vector<S_road_block_car> road_block_cars;  // 0x4c
    std::vector<S_road_block_iris> road_block_irises;  // 0x5c
    std::vector<S_road_block_enemy> road_block_enemies;  // 0x6c
    std::vector<C_actor *> unknown_7c;  // 0x7c (PS2 0x70)
    std::vector<C_car *> unknown_8c;    // 0x8c (PS2 0x7c), the police's temporary cars, on the radar
    std::vector<ushort> unknown_9c;     // 0x9c (PS2 0x88)
    bool unknown_ac;                    // 0xac, which C_game::Init sets
    uchar unknown_ad[3];

    C_shvestky();
    ~C_shvestky();

    // PS2 names: C_game's Init and Done call them
    void GameInit();
    void GameDone();
    // PS2 name (Czech, roughly "tick the actors in the face")
    void TickDoRiciActors(bool);

    // The police manager on or off (PS2 name; the parameters' names are ours): POLICEMANAGER_ON's
    void SetActive(bool active, int unknown, bool unknown2);
    // The police's people and their cars (PS2 names): POLICEMANAGER_ADD's and DEL's
    void AddPerson(C_human *human);
    void DelPerson(C_human *human);
    void DriveCar(C_human *human, C_car *car);
    // PS2 name: POLICEMANAGER_FORCEARREST's (the parameters' names are ours)
    void ForceArrest(C_human *human, bool a, bool b);
    // What the police know of a human (PS2 names), the GET_PM_ commands': its status bits, its record's state, the
    // times of its crash and of its fire, the number of people angry at it ("pocet nasranych") and of those alive
    // ("zivych"); JakPicusNaTomJest is Czech, rudely, for "how is the guy doing", 0 without a record
    uint GetHumanStatus(C_human *human);
    int JakPicusNaTomJest(C_human *human);
    uint GetCrashTime(C_human *human);
    uint GetFireTime(C_human *human);
    int PocetNasranych(C_human *human);
    int PocetNasranychZivych(C_human *human);
    // A flag of the human's record, while the byte at 0x38 is clear (PS2 name, Czech, roughly "doesn't know which cop
    // he ran into")
    bool NeviDoKterePiceVlezl(C_human *human);
    // The actors TickDoRiciActors ticks (PS2 names): ACTOR_ADDDORICI's and DELDORICI's
    void AddDoRiciActor(C_actor *act);
    void DelDoRiciActor(C_actor *act);
    // More policemen for the human (PS2 name, Czech for "more guys to the party"): POLICE_SUPPORT's (the parameters'
    // names are ours)
    void DalsiChalpiDoPartie(C_human *human, uint unknown, float unknown2);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b4e0
    bool GetUnknown0() const
    {
        return unknown_0;
    }
    // PS2 writes the members directly
    // FUNCTION: GAME 0x47b4f0
    void SetSpeed(float speed1, float speed2)
    {
        unknown_c = speed1;
        unknown_10 = speed2;
    }
};
