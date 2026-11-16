#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"
#include "tools.h"

class C_car;
class C_human;
class C_railway;
class C_traffic_generator;
class C_animations_machine_de_luxe;
struct rentry;

// A pedestrian of the generators (PS2 C_traffic_element; its file is the generator's), 0x200 bytes (PS2's 0x250, its
// vectors 16 bytes and first). The names but PS2's are ours, PS2's offsets where known. CreateModel and SetModel give a
// model's members, Activate and ActivateFree a walk's.
class C_traffic_element
{
public:
    // PS2's: clears the collision and the usable object as their constructors do, the members after them last
    C_traffic_element();
    // Empty: the generator's array new and delete name it (the linker folds it with the other empty functions)
    ~C_traffic_element() {}

    // PS2 names and parameters, in PS2's order
    // Steps around an actor in the way, the player or a human (Czech: "walk around that crap"): two points to the side
    // the node's line is farther from, then back to the node; false for another actor (frm isn't read)
    bool ObejdiTuPicovinu(C_actor *act, I3D_frame *frm);
    // The next point of the walk; true
    bool SetNewNode();
    // Waves at the player's taxi and gets in as its passenger (Czech: "waves at the taxi"), and ends the waving
    void MavaNaTaxi(uint time);
    void EndMavaniNaTaxi();
    // The pedestrian by the time passed, which TickElements calls for the active ones
    void Tick(uint time);
    // Tick's end of an animation with none after it, which it has twice: alive, the walk or the duck goes on, dead,
    // it stays lying (the name is ours; inline, as the original's one copy of the code shows)
    void EndOfAnim();
    // The model placed at pos along dir, inv_matrix and the collision updated
    void UpdateModel();
    // A model given (AddFreePerson's), or one from the mission's buffer or the file for the generator (GameInit's):
    // false where there is none
    bool SetModel(I3D_model *model);
    bool CreateModel(char *name, C_traffic_generator *gen);
    // The "base" object's transparency
    void SetAlpha(float alpha);
    // The model given back to the mission's buffer (a generator's) or released
    void DestroyModel();
    // Sets out from where the model stands (a free pedestrian's), or from a node near the camera (the generator's AI,
    // which passes the near and far limits of the camera's distance); false where no place is found
    bool ActivateFree(bool b1, bool b2);
    bool Activate(bool b, float f1, float f2);
    // Takes the pedestrian out of the game: a free one deletes itself
    void Deactivate();
    // Hit along dir (PS2's type an E_hit_type, HIT_*), and its three reactions, which PS2's Hit calls: inline on the
    // PC, which has no copy of them
    void Hit(int type, const S_vector &dir, uint u, C_actor *act, float f);
    inline void HitFall(const S_vector &dir, uint u);
    inline void HitGun(const S_vector &dir, uint u, bool b);
    inline void HitStroke(const S_vector &dir, uint u);
    // The animation anim_next started
    void SetAnim();
    // A panic at last_panic, and a pedestrian ducking in one of three postures (SetMegaPanic's i, 1 to 3)
    void SetPanic();
    void SetMegaPanic(int i);
    // The player talks to it (Czech: "the player talks to me")
    void PlayerNaMneMluvi();
    // Greets the player (Czech: "hey, Karel!")
    void Karle_hoj();
    void DropUmbrella();
    // A mafioso made of a pedestrian hit (CreateMafianos)
    void CreateMafianos(int type, const S_vector &hit_pos, uint u, C_actor *act, float f);

    // The name is a guess. TryGoToAssHole's comparison of the navigation points reads the position through it: the two
    // inline calls still to come starve its first nested Dot, which the original calls out of line.
    const S_vector &GetPos() const
    {
        return pos;
    }

    // 0x0 (PS2 0x180), set once the energy is gone (PS2's Tick), with the collision and the shadows taken
    bool dead;
    uchar unknown_1[3];
    // 0x4 (PS2 0x184), the generator it belongs to (CreateModel's), NULL for a free pedestrian (AddFreePerson's),
    // which Deactivate deletes
    C_traffic_generator *generator;
    C_animations_machine_de_luxe *machine;  // 0x8 (PS2 0x188), its animations (AddElement's), which Deactivate stops
    bool active;                        // 0xc (PS2 0x18c)
    uchar unknown_d[3];
    S_vector pos;                       // 0x10 (PS2 0x0)
    S_vector dir;                       // 0x1c (PS2 0x10), the direction it walks in
    S_vector velocity;                  // 0x28, cleared by every Tick
    S_vector anim_pos;                  // 0x34, pos where SetAnim started the animation
    I3D_model *frame;                   // 0x40 (PS2 0x190)
    // 0x44, a dog's head (Activate's "hlavapsa.i3d", 1 in 5000) in place of the model's: no train for it
    I3D_model *dog_head;
    // 0x48, the distance from the camera beyond which Tick takes it out (SetModel's 90, the generator's max_dist)
    float max_dist;
    I3D_model *umbrella;                // 0x4c, Activate's in rain ("9dest.i3d"), dropped by DropUmbrella
    bool visible;                       // 0x50, the model was rendered: Tick doesn't take it out within 20 m then
    uchar unknown_51[3];
    float dist;                         // 0x54 (PS2 0x1a4), the camera's squared distance in the horizontal plane
    int keep_alive;                     // 0x58, 2 from the activation, counted down by Tick (the name is a guess)
    int invisible_time;                 // 0x5c, the time the model hasn't been rendered, in ms
    int node;                           // 0x60, the web's node it walks to
    S_vector target;                    // 0x64 (PS2 0x40), the point it walks to
    float node_width;                   // 0x70, node's width, in m: the target is a random place across it
    float fall_speed;                   // 0x74, -1 while it stands on the ground
    int no_ground_count;                // 0x78, Tick's ground tests that found none: a fall at 4
    int prev_node;                      // 0x7c, the node it came from, the way back (-1 from ActivateFree)
    // 0x80 (PS2 0x50), the points of a walk off the web (to a car, around an actor, after a car): the car's placement
    // (C_car::GetPlacementFromTCK1) and two of its navigation points, each put on the ground, way[2] the walk's end
    S_vector way[3];
    S_vector way_dir;                   // 0xa4 (PS2 0x80), the placement's direction
    int num_way;                        // 0xb0 (PS2 0x1c4), way's points left, -1 for none
    // 0xb4 (PS2 0x1c8), the walk off the web goes back to node after its points (ObejdiTuPicovinu's)
    bool detour;
    uchar unknown_b5[3];
    I3D_object *base;                   // 0xb8, the model's "base" object, a single mesh or morph (SetAlpha's)
    float alpha;                        // 0xbc, the model's transparency, 1 for a passenger (SetAlpha's argument)
    int alpha_dir;                      // 0xc0, 1 fading in, -1 fading out, 0 neither
    // 0xc4, its collision, a DYNCOLL_UNKNOWN_40 (CreateModel's, the frame's world matrix and inv_matrix, the game's
    // don_ocas its owner), in the moving objects' grid while in_coll is set
    tDynamicCollObject coll;
    S_matrix inv_matrix;                // 0x10c, the frame's world matrix inverted (coll's)
    int coll_wait;                      // 0x14c, a fall's count of SetAnim's calls, which puts coll in again at 1
    bool in_coll;                       // 0x150, coll is in the moving objects' grid
    uchar unknown_151[3];
    // 0x154, the model's scale: the number after the third letter of its "back1" frame's six-letter property, in
    // hundredths
    float scale;
    // 0x158, the first three letters of the "back1" frame's property, 'A' 0: model_type[0] 1 for a woman, model_type[2]
    // the kind (1 and 3 the police's, 4 a mafioso's)
    uchar model_type[3];
    uchar unknown_15b;
    float energy;                       // 0x15c, 60 to 80 from the activation, dead at 0
    int anim;                           // 0x160, the animation SetAnim started (an _anim_info_ index)
    int anim_next;                      // 0x164, the animation SetAnim starts
    int anim_after;                     // 0x168, the animation after anim_next (a fall's rise, a duck's stay)
    bool anim_once;                     // 0x16c, the animation is played once, for anim_time
    bool anim_after_once;               // 0x16d, anim_after's anim_once
    uchar unknown_16e[2];
    int anim_time;                      // 0x170, the animation's length less 250 ms (SetAnim's), counted down
    S_vector anim_end;                  // 0x174, the animation's end point, turned by the model's rotation (SetAnim's)
    int ground_timer;                   // 0x180, Tick's time to its next ground test
    I3D_pose *neck;                     // 0x184, a pose CreateModel gives the model's "neck"
    I3D_frame *look_at;                 // 0x188, the "neck" frame the neck turns to (the player's, a pedestrian's)
    bool neck_reset;                    // 0x18c, the neck turning back
    uchar unknown_18d[3];
    int wait_time;                      // 0x190, the time it waits
    bool line_walk;                     // 0x194, it walks along the line between the nodes
    uchar unknown_195[3];
    int unknown_198;                    // 0x198
    bool running;                       // 0x19c, it runs (anim 5) rather than walks (anim 6): SetPanic's
    bool turning;                       // 0x19d, it turns in place at a crossing
    // 0x19e (PS2 0x21e), set once for a panic, with the place it runs to (PS2's SetPanic), cleared by Tick with the
    // panic
    bool fleeing;
    uchar unknown_19f;
    int panic_time;                     // 0x1a0, the panic's time left, 20 to 25 s from SetPanic
    uchar mega_panic;                   // 0x1a4, SetMegaPanic's posture, 1 to 3, 0 for none
    // 0x1a5 (PS2 0x225), made a human (a policeman by FizlosNamakatDoAkce, a mafioso by CreateMafianos): the generator's
    // AI leaves it, C_human::GameDone clears it
    bool made_human;
    uchar unknown_1a6[2];
    C_car *car;                         // 0x1a8 (PS2 0x228), the parked car it walks to
    C_car *hit_car;                     // 0x1ac (PS2 0x22c), the car that ran into it (Hit's)
    bool hit_wait;                      // 0x1b0, it lies after a hit
    uchar unknown_1b1[3];
    int walk;                           // 0x1b4, the man's walk, 0 to 4 (SetAnim's)
    C_human *target_human;              // 0x1b8, the human it walks to (SetNewNode's DalsiChalpiDoPartie at the end)
    // 0x1bc, the player's "Talk to the pedestrian" (CreateModel's, USING_ACTION_TALK, the game's don_ocas its actor)
    C_using_object::S_using_object using_object;
    // 0x1f4, the web connections' flags it walks along (C_web_connection::flags), GetNearNode's: 1 for a generator's, 3
    // for a free pedestrian
    ushort web_flags;
    uchar voice;                        // 0x1f6, its voice, ElementNewScream's first argument
    uchar unknown_1f7;
    int greet_timer;                    // 0x1f8, Tick's time to its next Karle_hoj
    bool may_greet;                     // 0x1fc, Karle_hoj greets the player
    bool greeted;                       // 0x1fd, Karle_hoj has: PlayerNaMneMluvi answers with the other voices
    bool waving;                        // 0x1fe (PS2 0x246), waving at a taxi
    bool on_sidewalk;                   // 0x1ff, it stands on MAT_SIDEWALK, where it may wave at a taxi
};

// The generator's saved game (the names are ours): the version, the generator's flags and its pedestrians out, then a
// record of each pedestrian out
#define TRAFFIC_SAVEGAME_VERSION 1

#pragma pack(push, 1)
struct S_traffic_generator_savegame
{
    uchar version;                      // 0x0
    bool active;                        // 0x1
    int num_active;                     // 0x2, the records that follow
    uint spawn_timer;                   // 0x6
};

// A pedestrian out, 0xbc bytes: its index in the generator's array, then its members
struct S_traffic_element_savegame
{
    ushort index;                       // 0x0
    bool dead;                          // 0x2
    S_vector pos;                       // 0x3
    S_vector dir;                       // 0xf
    S_vector velocity;                  // 0x1b
    S_vector anim_pos;                  // 0x27
    uchar visible;                      // 0x33
    float dist;                         // 0x34
    int keep_alive;                     // 0x38
    int node;                           // 0x3c
    S_vector target;                    // 0x40
    float node_width;                   // 0x4c
    int prev_node;                      // 0x50
    S_vector way[4];                    // 0x54, the pedestrian's way and way_dir
    int num_way;                        // 0x84
    float alpha;                        // 0x88
    int alpha_dir;                      // 0x8c
    float energy;                       // 0x90
    int anim;                           // 0x94, the pedestrian's anim: SaveGameLoad starts it again as anim_next
    int anim_after;                     // 0x98
    bool anim_once;                     // 0x9c
    bool anim_after_once;               // 0x9d
    int anim_time;                      // 0x9e
    int ground_timer;                   // 0xa2
    int wait_time;                      // 0xa6
    bool line_walk;                     // 0xaa
    int unknown_198;                    // 0xab
    bool running;                       // 0xaf
    bool turning;                       // 0xb0
    bool fleeing;                       // 0xb1
    int panic_time;                     // 0xb2
    uchar mega_panic;                   // 0xb6
    bool made_human;                    // 0xb7
    int car;                            // 0xb8, the car's number, -1 for none
};
#pragma pack(pop)

// The pedestrians' generator (PS2 C_traffic_generator, C_person_traffic.cpp), 0xb8 bytes (PS2's 0xd0), which
// C_mission::CreateActor makes for ACTOR_TRAFFIC_GENERATOR
// VTABLE: GAME 0x623fa0
class C_traffic_generator : public C_actor
{
public:
    C_traffic_generator();
    virtual ~C_traffic_generator();
    // The vtable's other overrides (the slots' names are C_actor's), but the linker's folded constant functions
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

    // PS2 names and parameters, in PS2's order (PS2's DelTaxiPassGlobAct, InvalidateActor, ChangeSceneInit,
    // ChangeSceneDone and DetachMesh have no copy on the PC)
    // The player talks to the pedestrian of the frame (Czech: "the player spoke to that dude")
    static void PlayerPromluvilNaTehoChuja(I3D_frame *frm);
    // The player has a weapon out (Czech, "the player has it drawn"), which C_shvestky::Tick calls for a human on foot
    // with a weapon in the hand: the pedestrians near the player look at him
    static void PlayerHoMaVytaseneho();
    // The pedestrians shown or hidden
    static void SetVisible(bool visible);
    // The pedestrian the pedestrian would run into, NULL for none (Tick asks)
    static C_traffic_element *IsPossibleCollElem(C_traffic_element *e);
    // A pedestrian into the active ones, with its animations (NULL where it is in already), and out
    static C_animations_machine_de_luxe *AddElement(C_traffic_element *e);
    static void DelElement(C_traffic_element *e);
    // The pedestrians by the time passed, which C_game::Tick calls
    static void TickElements(uint time);
    // Whether a pedestrian walks to the car
    static bool IsCarHasOcas(C_car *car);
    // TickElements' every 2123 ms (try_do_rici): a pedestrian sent to a parked car ("go to the asshole")
    static void TryGoToAssHole();
    // The taxi driver's state: whether the player's car is a taxi, a pedestrian waving at it while it is one, the
    // passengers on or off (off ends the waving), and the waving ended
    static void PlayerInTaxi(C_car *car);
    static void GeneratePassengerForTaxi();
    static void EnableTaxiDriver(bool enable);
    static void ZrusVsechnyMavajici();
    // The pedestrians forgotten and their animations freed
    static void ClearElements();
    // A pedestrian within range made a policeman for the police (Czech: "cop, get into action"), which
    // C_shvestky::VasekSelChlastatNaOslavu calls; in_front narrows the range to a cone ahead of the human
    static void FizlosNamakatDoAkce(C_human *human, uint u, std::vector<C_actor *> &actors, bool b1, float range,
        bool in_front);
    // The pedestrians within range of an explosion at pos hit, which C_game::NewExplosion calls
    static void Explosion(S_vector &pos, float range, float power);
    // The pedestrians within range of a fire at pos hurt, which C_fire::Tick calls with its range's square and damage
    static void Fire(S_vector &pos, float range_2, float damage);
    // The pedestrian whose model the frame is, NULL for none
    static C_traffic_element *GetElemFromFrame(I3D_frame *frm);
    // The active pedestrians taken out of the game
    static void FreeDynCollElements();
    // The squared distance of the pedestrian nearest pos (Activate asks)
    static float GetNearElementDist2(S_vector &pos);
    // The pedestrian just ahead of the pedestrian (half a step ahead, within half a metre), NULL for none (Tick asks)
    static C_traffic_element *IsNearElementBefore(C_traffic_element *e);
    // The pedestrians' panic at a shot or an explosion at pos, which C_game's NewShoot, NewExplosion and NewFire call,
    // and at a pedestrian of the frame (C_railway::Hit's)
    static void NewPanic(S_vector &pos);
    static void NewPanic(I3D_frame *frm);
    // The pedestrians near a rail vehicle's entries at a station board it (C_railway's)
    static void RailwayInStation(C_railway *rail, std::vector<rentry> &entries);
    // Gives a pedestrian of that model to the generators (a script's command), and one getting out of a car
    // (C_traffic_car's); the actor isn't read
    static void AddFreePerson(I3D_model *model, C_actor *act, bool b);
    static void AddFreePersonFromCar(I3D_model *model, C_actor *act, S_vector &pos, S_vector &dir);
    // A pedestrian's scream: the voice's sound (gameVoices' entry i2 - 2) at pos, heard within f2 of the camera
    // (PS2's parameters; the names are ours)
    static void ElementNewScream(int i1, int i2, S_vector &pos, float f1, float f2, float f3, float f4);
    // The pedestrians walking to the car sent away (Czech: "blow off from that machine")
    static void VyfunOdTeMasiny(C_car *car);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours): NPC_SHUTUP's
    // FUNCTION: GAME 0x47b780
    static void SetGlobalValidScream(bool b)
    {
        global_valid_scream = b;
    }

    // 0x70 (PS2 0x80), a generator without a range, 1 by the constructor (SaveData's first): C_game::SetActiveTrafficP
    // takes the nearest of them where the camera is in no other's range (the name is ours)
    bool no_range;
    uchar unknown_71[3];
    float range;                        // 0x74 (PS2 0x84), 100 (the name is ours)
    // 0x78, the one C_game::SetActiveTrafficP chose (the name is ours): its AI activates the pedestrians
    bool active;
    uchar unknown_79[3];
    C_traffic_element *elements;        // 0x7c (PS2 0x8c), its pedestrians, num_elements of them (GameInit's)
    int num_active;                     // 0x80, its pedestrians out: ActivateFree counts them up, Deactivate down
    int num_elements;                   // 0x84, the sum of model_counts
    float max_dist;                     // 0x88 (PS2 0x98), 100, its pedestrians' max_dist
    // 0x8c (PS2 0x9c) and 0x90 (PS2 0xa0), 50 and 90, the near and far limits of the camera's distance the AI passes
    // to Activate
    float activate_min;
    float activate_max;
    float unknown_94;                   // 0x94 (PS2 0xa4), 50
    char **models;                      // 0x98 (PS2 0xa8), the models' names, 0x11 bytes each (SaveData's)
    int num_models;                     // 0x9c (PS2 0xac)
    int *model_counts;                  // 0xa0 (PS2 0xb0), the pedestrians of each model
    uint spawn_timer;                   // 0xa4, AI's time since its last activation: it tries the next at 113 ms
    // 0xa8 (PS2 0xb8), the most pedestrians out, 10: GameInit scales it by ratio_all, GameDone gives it back
    int max_active;
    int max_active_orig;                // 0xac
    int next_try;                       // 0xb0, the pedestrian AI tries to activate next
    // 0xb4, the last policeman of the pedestrians ratio_police keeps, -1 for none: AI leaves those after it
    int police_last;

    // Whether the pedestrians may scream (PS2's ElementNewScream tests it): C_game::Init sets it, NPC_SHUTUP clears
    // it with a nonzero parameter
    static bool global_valid_scream;
    // PS2 names: the pedestrians, whether they are shown, whether the taxi driver has passengers, whether the player
    // drives a taxi, and the pedestrians' animations
    static std::vector<C_traffic_element *> array;
    static bool all_visible;
    static bool enable_taxi_driver;
    static bool player_in_taxi;
    static std::vector<C_animations_machine_de_luxe *> anims;
    // PS2 names, TickElements' times, in ms: counted down to 0, counted down to TryGoToAssHole (try_do_rici, the same
    // name in Czech), and counted down to GeneratePassengerForTaxi while the player drives a taxi; and the screams'
    // (ElementNewScream's)
    static int t_horn;
    static int try_do_rici;
    static int taxi_timer;
    static int t_scream;
    // PS2 names: the last panic's place (NewPanic's) and the first AI's (GameInit sets it, AI activates every
    // pedestrian that fits and clears it)
    static S_vector last_panic;
    static bool first_frame;
    // PS2 names, its GameInit's (PS2's FreerideSetupFromScript sets the police's): C_game::DoneIntern sets both to 1
    static float ratio_all;
    static float ratio_police;
};

// SYNTHETIC: GAME 0x44d480
// C_traffic_generator::`scalar deleting destructor'

// The don's tail (PS2 C_don_ocas, "ocas" Czech for a tail), ACTOR_DON_OCAS, 0x70 bytes: C_game::Init creates one for
// the game (its don_ocas), which every pedestrian's frame and collision name as their actor, so that its Collision and
// Hit reach the pedestrian of the frame (GetElemFromFrame)
// VTABLE: GAME 0x623ef8
class C_don_ocas : public C_actor
{
public:
    C_don_ocas();
    virtual ~C_don_ocas();
    // The vtable's other overrides (the slots' names are C_actor's)
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
