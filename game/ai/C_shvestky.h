#pragma once

#include "Main.h"
#include "tools.h"

#include <vector>

class C_actor;
class C_car;
class C_human;

// The police (PS2 C_shvestky, C_shvestky.cpp), the game's 0x2da0, 0xb0 bytes. PS2's has the same members, its offsets in
// the comments. Only the constructor, the destructor, GameInit, GameDone, Tick and TickDoRiciActors are written so far,
// and the accessors the script's commands use.
// The records' flags (S_shvestky::flags; the names are ours), the most set by the PS2 function their comment names
#define SHVF_WEAPON 0x1                 // an item of 5 to 15 in the right hand
#define SHVF_FIRE 0x2                   // Firing's, cleared once fire_time runs out
#define SHVF_SPEEDING 0x4               // the car faster than C_shvestky::speed_limit
// CarCollision's three bits, cleared once collision_time runs out: 8 for anything, 0x10 for a car, 0x20 for a human
#define SHVF_CRASH 0x38
#define SHVF_ILLEGAL_PATH 0x40          // CarIllegalPath's: counts illegal_path_time up while the car is faster than 11
#define SHVF_BOXING 0x200               // HumanBoxing's, cleared once boxing_time runs out
#define SHVF_RED_LIGHT 0x400            // TestOverRed's, cleared once signal_time runs out
#define SHVF_CAR_STEAL 0x800            // CarSteal's, cleared once car_steal_time runs out
#define SHVF_SIGNAL_MARK 0x1000         // DestroySignalMark's (a traffic sign destroyed), cleared with SHVF_RED_LIGHT
// DalJsemPochuzkariDoDrzky's (Czech, rudely, "I punched the patrolman in the face"): SHVSTATE_KILL
#define SHVF_HIT_POLICEMAN 0x2000
#define SHVF_FORCE_ARREST 0x4000        // ForceArrest's: SHVSTATE_ARREST
// StrcilJsemHoPoldumDoVyfuku's (Czech, rudely, "I shoved it up the cops' exhaust"): a police car rammed
#define SHVF_RAM_POLICE_CAR 0x8000
// S_shvestky::flags2's (the name is ours): CarCollision's bits of SHVF_CRASH, cleared once crash_time runs out
#define SHVF2_CRASH 0x38
// The records' states, S_shvestky::state, JakPicusNaTomJest's (the names are ours, after the police's calls for them)
#define SHVSTATE_NONE 0
#define SHVSTATE_1 1
#define SHVSTATE_ARREST 2               // the police told to arrest him (PanoveZmenaZatkneteHoZmrda)
#define SHVSTATE_KILL 3                 // to kill him (PanoveZmenaZabijteHoZmrda)
// Why a record's state went up, S_shvestky::reason (the names are ours, after the flags that give them)
#define SHVREASON_WEAPON 0
#define SHVREASON_SPEEDING 2
#define SHVREASON_ILLEGAL_PATH 4
#define SHVREASON_RED_LIGHT 6
#define SHVREASON_CAR_STEAL 7
#define SHVREASON_HIT_POLICEMAN 9
#define SHVREASON_RAM_POLICE_CAR 0xa
// The items in a human's right hand the police mind, gameItemTable's ids (the names are ours): any of the first range
// on foot (SHVF_WEAPON), in a car the second, and in its seat 0 only up to SHVITEM_DRIVER_LAST
#define SHVITEM_FIRST 5
#define SHVITEM_LAST 15
#define SHVITEM_CAR_FIRST 6
#define SHVITEM_CAR_LAST 14
#define SHVITEM_DRIVER_LAST 9

class C_shvestky
{
public:
    // A human the police watch (PS2 name; PS2's DelNasrany is its), 0x80 bytes, the size AddPerson allocates (its
    // constructor, inline there, clears most of it). The names are ours.
    struct S_shvestky
    {
        C_human *human;                 // 0x0
        C_car *car;                     // 0x4, the human's car where it is ACTOR_CAR
        ushort flags;                   // 0x8, SHVF_*
        ushort flags2;                  // 0xa, SHVF2_*
        int car_steal_time;             // 0xc, SHVF_CAR_STEAL's: 5000 from CarSteal, or 0
        int fire_time;                  // 0x10, GetFireTime's, SHVF_FIRE's
        short illegal_path_time;        // 0x14, SHVF_ILLEGAL_PATH's, at most 5000: SHVREASON_ILLEGAL_PATH past 5800
        int collision_time;             // 0x18, SHVF_CRASH's: CarCollision's strongest hit, at most 4000
        int crash_time;                 // 0x1c, GetCrashTime's, SHVF2_CRASH's: CarCollision's hits added, at most 20000
        int boxing_time;                // 0x20, SHVF_BOXING's: 5000 more for each of HumanBoxing's
        // 0x24, SHVF_RED_LIGHT's and SHVF_SIGNAL_MARK's: 4000 more for each red light (at most 10000) and each destroyed
        // sign (DestroySignalMark's, at most 20000)
        int signal_time;
        // 0x28, PocetPrestupku's (Czech, "the number of offenses"): 3 make SHVSTATE_1 an arrest
        int offenses;
        // 0x2c, SerSeMuDoCesty's (Czech, rudely, "get in his way"): the police action's at SHVSTATE_KILL once alerted;
        // Tick's road blocks come up only for it
        bool block_way;
        uchar unknown_2d[7];            // 0x2d
        S_vector2 pos;                  // 0x34, the car's last position on the ground (x and z), TestOverRed's
        ushort state;                   // 0x3c, SHVSTATE_*
        ushort new_state;               // 0x3e, what CreateShvestkyTest makes state, cleared after
        // 0x40 and 0x4c: the humans the car hit (CarCollision's, below SHVSTATE_KILL) within 5000 ms of the first, which
        // Tick forgets once the time has run out (-1 from the constructor)
        int hits_time;
        // 0x44, the time left until the police let the human go (DobreMiPokuril_MuzeJitDom) while nobody is angry at
        // him: 20000, 40000 or 60000 by the state (NabonzovalJsemHo's), the HUD's bar of the player's
        int wanted_time;
        int wanted_time_start;          // 0x48, wanted_time's start
        int hits;                       // 0x4c
        // 0x50, NabonzovalJsemHo's (Czech, "I snitched on him") with its message "All Cops have been alerted!", and
        // ForceArrest's: ChceHoOpichatKazdyPoldaNaOkresku returns it ("every cop in the district wants him")
        bool alerted;
        bool nevi;                      // 0x51, NeviDoKterePiceVlezl's
        uchar unknown_52[2];            // 0x52
        // 0x54 and 0x58, ZalozSpisATahniDoRiti_Haleluja's (Czech, rudely, "open a file and get lost, hallelujah"), the
        // police actions' once they end: CreateShvestkyTest makes nevi the human in another car than known_car, or on
        // foot without known_on_foot (the names are guesses)
        C_car *known_car;
        bool known_on_foot;
        uchar unknown_59[3];            // 0x59
        // 0x5c, the more policemen the player's states 5 and 6 brought (1 or 2), DalsiChalpiDoPartie's
        int support_level;
        int rammed_car;                 // 0x60, StrcilJsemHoPoldumDoVyfuku's police car (a C_car *)
        int reason;                     // 0x64, SHVREASON_*
        // 0x68, NextNaSrackySoudruh's count: the humans his car ran over (PS2's C_human::Collision calls it for the
        // car's driver), 6 make CarCollision's hit of a human SHVSTATE_KILL
        int run_over;
        bool hard_ram;                  // 0x6c, StrcilJsemHoPoldumDoVyfuku's ram above 9.7: SHVSTATE_ARREST, else 1
        uchar unknown_6d[3];            // 0x6d
        // 0x70, the actors angry at the human (PS2's AddNasrany's and DelNasrany's), the dead ones (C_actor::dead) taken
        // out by Tick
        std::vector<C_actor *> nasrany;

        ~S_shvestky();
    };
    // The road blocks' cars, policemen and enemies (PS2's names), 8 bytes each: the actor and whether it is free, as
    // PS2's FreeTempCar and FreeTempEnemy set it and its GetTempCar takes a free car (GameDone takes the others out of
    // the game's lists; the members' names are ours)
    struct S_road_block_car
    {
        C_car *car;                     // 0x0
        bool free;                      // 0x4
    };
    struct S_road_block_iris
    {
        C_actor *actor;                 // 0x0
        bool free;                      // 0x4
    };
    struct S_road_block_enemy
    {
        C_actor *actor;                 // 0x0
        bool free;                      // 0x4
    };

    // The names are ours
    bool active;                        // 0x0, SetActive's; PM_SHOWSYMBOL and its group do nothing while it is set
    uchar unknown_1[3];
    S_timer records_timer;              // 0x4, Tick's period of the records
    // Two speeds, POLICEMANAGER_SETSPEED's (km/h made m/s): faster than the first is SHVF_SPEEDING (SHVSTATE_1),
    // faster than the second while speeding SHVSTATE_ARREST
    float speed_limit;                  // 0xc, 17 from GameInit (PS2's 27)
    float speed_limit_arrest;           // 0x10, 28 from GameInit (PS2's 1000)
    S_timer do_rici_timer;              // 0x14, Tick runs TickDoRiciActors at each of its periods
    S_timer red_light_timer;            // 0x1c, Tick runs TestOverRed at each of its periods
    S_timer unknown_24;                 // 0x24
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30
    // 0x34, the text of the failure's menu Tick runs with the player in the cooler: SetActive's, POLICEMANAGER_ON's
    // (980000 by default, "You've been arrested")
    uint arrest_text;
    bool unknown_38;                    // 0x38
    uchar unknown_39[3];                // 0x39
    std::vector<S_shvestky *> records;  // 0x3c (PS2 0x40)
    std::vector<S_road_block_car> road_block_cars;  // 0x4c
    std::vector<S_road_block_iris> road_block_irises;  // 0x5c
    std::vector<S_road_block_enemy> road_block_enemies;  // 0x6c
    std::vector<C_actor *> do_rici_actors;  // 0x7c (PS2 0x70), AddDoRiciActor's, TickDoRiciActors'
    // 0x8c (PS2 0x7c), the police's temporary cars (PS2's GetTempCar and FreeTempCar), on the radar
    std::vector<C_car *> temp_cars;
    // 0x9c (PS2 0x88), SetActive's: the nodes nearest to the mission's bridges, whose red lights TestOverRed leaves out
    std::vector<ushort> bridge_nodes;
    bool unknown_ac;                    // 0xac, which C_game::Init sets
    uchar unknown_ad[3];

    C_shvestky();
    ~C_shvestky();

    // PS2 names: C_game's Init and Done call them
    void GameInit();
    void GameDone();
    // The police by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // PS2 names, which Tick calls: inline on the PC, PS2 has them out of line. The records' new states, and the police
    // cars' drivers who run a red light.
    inline void CreateShvestkyTest();
    inline void TestOverRed();
    // PS2 name (Czech, rudely, roughly "tick the actors gone to hell", "do riti"; the parameter's name is ours): those
    // of do_rici_actors that went far enough from the camera, or all of them, taken out of it and out of the game
    void TickDoRiciActors(bool all);
    // PS2 names, which Tick calls (the parameters' names are ours): Czech, roughly "he smoked me well, he can go home"
    // (the human let go) and "Vasek went boozing to the party"
    void DobreMiPokuril_MuzeJitDom(C_human *human, bool unknown);
    void VasekSelChlastatNaOslavu(C_human *human, const S_vector &pos, float range, bool unknown, bool unknown2);
    // A road block of the game's set up for the human (PC only, the name is ours), which Tick calls once a police car
    // comes near it
    void CreateRoadBlock(int index, C_human *human);

    // The police manager on or off (PS2 name; the parameters' names are ours): POLICEMANAGER_ON's, with arrest_text
    void SetActive(bool on, int text, bool unknown2);
    // The police's people and their cars (PS2 names): POLICEMANAGER_ADD's and DEL's
    void AddPerson(C_human *human);
    void DelPerson(C_human *human);
    void DriveCar(C_human *human, C_car *car);
    // The human's car ran over another human (PS2 name, 0x49d220): C_human::Collision calls it with the car's
    // driver, which counts S_shvestky::run_over
    void NextNaSrackySoudruh(C_human *human);
    // PS2 name: POLICEMANAGER_FORCEARREST's (the parameters' names are ours)
    void ForceArrest(C_human *human, bool a, bool b);
    // The car the human gets out of stolen (PS2 name; the bools' meaning isn't known): C_human::Hit calls it with
    // false, false for the player hit while he steals a car
    void CarSteal(C_human *human, bool a, bool b);
    // The human boxing (PS2 name): SHVF_BOXING set, boxing_time 5000 more
    void HumanBoxing(C_human *human);
    // What the police know of a human (PS2 names), the GET_PM_ commands': its status bits, its record's state, the
    // times of its crash and of its fire, the number of people angry at it ("pocet nasranych") and of those alive
    // ("zivych"); JakPicusNaTomJest is Czech, rudely, for "how is the guy doing", 0 without a record
    uint GetHumanStatus(C_human *human);
    int JakPicusNaTomJest(C_human *human);
    // The human's offences (PS2 name; Czech, "the number of offences")
    int PocetPrestupku(C_human *human);
    uint GetCrashTime(C_human *human);
    uint GetFireTime(C_human *human);
    int PocetNasranych(C_human *human);
    int PocetNasranychZivych(C_human *human);
    // A flag of the human's record, while unknown_38 is clear (PS2 name, Czech, roughly "doesn't know which cop he ran
    // into")
    bool NeviDoKterePiceVlezl(C_human *human);
    // The actors TickDoRiciActors ticks (PS2 names): ACTOR_ADDDORICI's and DELDORICI's
    void AddDoRiciActor(C_actor *act);
    void DelDoRiciActor(C_actor *act);
    // PS2 names, TickDoRiciActors': the actor taken out of the records' angry ones, and a road block's car, enemy or
    // policeman freed (PS2's FreeTempCar, FreeTempEnemy and FreeTempIris by its type): false for any other actor
    void GlobalDelNasrany(C_actor *act);
    bool FreeMyTempActor(C_actor *act);
    // More policemen for the human (PS2 name, Czech for "more guys to the party"): POLICE_SUPPORT's (the parameters'
    // names are ours)
    void DalsiChalpiDoPartie(C_human *human, uint unknown, float unknown2);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b4e0
    bool GetActive() const
    {
        return active;
    }
    // PS2 writes the members directly
    // FUNCTION: GAME 0x47b4f0
    void SetSpeed(float speed1, float speed2)
    {
        speed_limit = speed1;
        speed_limit_arrest = speed2;
    }
};

// The copies of the engine's math the original has right after Tick (0x49b7a0-0x49b853, I3d_math.h's): S_vector2's
// operator*(float) (whose marker is g_indicators.h's), operator+= and Normalize, and S_vector::IsNull
// LIBRARY: GAME 0x49b7c0
// ??YS_vector2@@QAEAAU0@ABU0@@Z

// LIBRARY: GAME 0x49b7e0
// ?Normalize@S_vector2@@QAEXXZ

// LIBRARY: GAME 0x49b820
// ?IsNull@S_vector@@QBE_NXZ

// The size of bridge_nodes, which TestOverRed calls out of line: the copy the linker kept is among the accessors at the
// end of C_program.cpp's file
// TEMPLATE: GAME 0x47bc00
// ?size@?$vector@GV?$allocator@G@std@@@std@@QBEIXZ
