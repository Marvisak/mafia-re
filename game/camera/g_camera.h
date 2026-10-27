#pragma once

#include "Main.h"

#include <vector>

class I3D_frame;
class I3D_camera;

class C_human;
class C_car;
struct S_vector;

// A camera state pushed and popped (PS2 G_CameraPushPop), 0x10 bytes: the mode and the I3D camera's field of view and
// clipping planes (PS2's PushMode; the names are ours, the camera's)
struct G_CameraPushPop
{
    uint mode;                          // 0x0, CAMERA_*
    float fov;                          // 0x4
    float ncp, fcp;                     // 0x8
};

// A race camera (PS2 G_RaceCamera, C_racecam's), 0x18 bytes: CAMERA_C_RACING shows the race from the camera whose
// range of gates holds the player's car
struct G_RaceCamera
{
    I3D_frame *frame;                   // 0x0, the actor's frame
    uchar unknown_4[0xc];               // 0x4
    // The camera shows the gates after gate_from up to gate_to (the names are ours)
    int gate_from;                      // 0x10
    int gate_to;                        // 0x14
};

// The cameras' numbers (S_camera_params::id, G_Camera::mode). The names are ours, after the entries' own names where
// there is one ("P" on foot, "C" in a car or another vehicle), with the number where two entries share a name; the
// modes without an entry after what Tick does in them.
#define CAMERA_P_BEHIND 1                   // "P Behind"
#define CAMERA_P_MOUSE 2                    // P Behind turned by the mouse
#define CAMERA_P_IN 3                       // "P In", from the player's eyes
#define CAMERA_CP_SNIPER 5                  // "CP Sniper"
#define CAMERA_DEATH 6                      // the player's death (the entry is called "P Behind")
#define CAMERA_C_BEHIND_HELLBOY 7           // "C Beh. Hellboy"
#define CAMERA_C_BEHIND_EMETH 8             // "C Behind Emeth"
#define CAMERA_C_BEHIND3 9                  // "C Behind3"
#define CAMERA_C_AROUND 10                  // behind the car, turned round it by LookAround
#define CAMERA_C_BEHIND_HELLBOY_11 11       // "C Beh. Hellboy", turned towards the velocity
#define CAMERA_C_CHASE 12                   // "C Chase"
#define CAMERA_C_IN 13                      // "C In", at the driver's head
#define CAMERA_C_OVER 14                    // "C Over"
#define CAMERA_C_ABOVE 15                   // above the car, looking down
#define CAMERA_C_RAILWAY_IN 17              // "C RailwayIn"
#define CAMERA_ROUND 18                     // round the player
#define CAMERA_C_DIALOG 19                  // "C Dialog"
#define CAMERA_C_RACING 20                  // "C Racing"
#define CAMERA_C_LOOK_TO 21                 // "C LookTo"
#define CAMERA_C_LOCK 22                    // "C Lock"
#define CAMERA_P_BEHIND_23 23               // "P Behind", as C RailwayIn lower
#define CAMERA_UNKNOWN_24 24                // Tick does nothing in it
// SetMode's: the camera's next one, G_Camera::unknown_14 (the name is ours)
#define CAMERA_NEXT ((uint)-1)
// The ranges of cameras the profile keeps, which SetMode stores there: the on-foot ones, the profile's camera, and the
// car ones, S_profile_settings::unknown_34 (the names are ours)
#define CAMERA_P_FIRST CAMERA_P_BEHIND
#define CAMERA_P_END 4
#define CAMERA_C_FIRST CAMERA_C_BEHIND_HELLBOY
#define CAMERA_C_END 16

// S_camera_params' flags (the names are ours, by value where the use isn't known): the code copies 1 into
// G_Camera::unknown_22
#define CAMPF_UNKNOWN_1 1
#define CAMPF_UNKNOWN_2 2
// Tick moves the camera into the player's eyes while it comes close to his head
#define CAMPF_EYES 4
#define CAMPF_UNKNOWN_8 8

// The indices of a camera's values (S_camera_params::values, G_Camera::params). The names are ours, after their use in
// Tick: the car cameras' behind the car (CAMERA_C_BEHIND_HELLBOY to CAMERA_C_BEHIND_HELLBOY_11)
#define CAMV_C_HEIGHT 0                     // the point the camera looks at, above the car's: times 10
#define CAMV_C_DOWN 1                       // how far down the camera looks, times 10, against CAMV_C_ALONG
#define CAMV_C_ALONG 2                      // times 20
#define CAMV_C_HEADING 3                    // the weight of the car's direction in the camera's, times 10
#define CAMV_C_VELOCITY 4                   // the weight of the velocity, times 10
#define CAMV_C_SIDE 6                       // how far LookAround moves the camera aside, times 10
#define CAMV_C_DISTANCE 7                   // the camera's distance from the point it looks at, times 10
// P Behind's and the mouse camera's
#define CAMV_P_AHEAD 0                      // the point looked at, ahead of the player: times 10, less 5
#define CAMV_P_HEIGHT 1                     // its height above him, times 5
#define CAMV_P_BEHIND 2                     // the camera behind that point, times 5 (with the tilt's cosine)
#define CAMV_P_ABOVE 3                      // above it, times 5 (with the tilt's sine)
#define CAMV_P_ALONG 4                      // then along the look: times 10, less 5
// both kinds'
#define CAMV_POS_FILTER 8                   // how fast the position follows, times the frame's time in 1/40 s
#define CAMV_DIR_FILTER 9                   // the direction's (the car cameras' also times their speed squared)

// The cameras' entries (the name is ours)
#define NUM_CAMERAS 17

// One of the cameras (an entry of PS2's cameraParams; the struct's and the members' names are ours), 0x44 bytes. PS2's
// entries are 0x38 bytes, without the name.
struct S_camera_params
{
    int id;                             // 0x0, CAMERA_*: G_Camera::mode is looked up by it
    int next;                           // 0x4, the CAMERA_* the player switches to (0: none)
    float values[10];                   // 0x8
    char name[16];                      // 0x30, "P Behind", "C Chase": P on foot, C in a car
    uint flags;                         // 0x40, CAMPF_*
};

// The game's camera (PS2 G_Camera, g_camera.cpp), the game's 0x4c, 0x98 bytes: the game's player follows it, as PS2's
// does. PS2's has the same members in the same order, at other offsets (in the comments). The names are ours, after the
// setters that set them (PS2's SetCamera, SetPlayer, SetCar, Link, SetMode, SetSwing, LookAround); the others are named
// by their offset.
class G_Camera
{
public:
    I3D_frame *link;                    // 0x0, the frame Link gives it (CAMERA_C_RAILWAY_IN with one)
    I3D_camera *camera;                 // 0x4
    C_human *player;                    // 0x8
    C_car *car;                         // 0xc, the car the player is in, NULL on foot
    // CAMERA_* (S_camera_params::id), the profile's after Reset (PS2's Reset sets CAMERA_P_BEHIND)
    uint mode;                          // 0x10
    uint unknown_14;                    // 0x14, Reset sets it to mode, the camera's code to the entry's next
    uint flags;                         // 0x18, the camera's CAMPF_*
    // The camera's values, the first camera's after Reset (PS2 0xd0)
    float *params;                      // 0x1c
    bool unknown_20;                    // 0x20 (PS2 0x1c)
    bool unknown_21;                    // 0x21
    bool unknown_22;                    // 0x22, the flags' 1
    float unknown_24;                   // 0x24, 5 after Reset (PS2 0x20)
    // The on-foot cameras' tilt in half turns, 0.5 (level) after Reset: their direction is the player's times sin(tilt
    // * pi), its y cos(tilt * pi) (the name is ours)
    float tilt;                         // 0x28
    // The angle in half turns LookAround turns to (0, +-0.5 or +-1) and the one Tick turns towards it (the names are
    // ours)
    float look_around_want;             // 0x2c
    float look_around;                  // 0x30
    uchar unknown_34[0x18];             // 0x34
    // 0x4c, the up vector the camera is turned with, an S_vector (the name is ours): the header doesn't include the
    // engine's math, which would move the code of the files that include it through game.h (their /YX precompiled
    // headers)
    float up[3];
    // SetSwing's: the swing's on, its amount and the frame swung along with the scene's backdrop (the names are ours)
    bool swing;                         // 0x58 (PS2 0x70)
    float swing_amount;                 // 0x5c
    I3D_frame *swing_frame;             // 0x60 (PS2 0x78)
    // Set when the mode changes, cleared at the end of Tick: the camera goes there at once (the name is ours)
    bool mode_changed;                  // 0x64
    std::vector<G_CameraPushPop> push_pop;  // 0x68 (PS2 0x80)
    // 0x78 (PS2 0x8c), PS2's AddRaceCamera's (MSL's vector of pointers is a vector<unsigned int> underneath; the name is
    // ours)
    std::vector<G_RaceCamera *> race_cameras;
    I3D_frame *look_to;                 // 0x88, the frame CAMERA_C_LOOK_TO looks at (PS2 SetLookTo's; the name is ours)
    float unknown_8c;                   // 0x8c (PS2 0x9c)
    float unknown_90;                   // 0x90, 0.2 after Reset
    bool free_look;                     // 0x94 (PS2 0xa4), the car cameras' free look (the name is ours)
    uchar unknown_95[3];                // 0x95

    G_Camera();
    ~G_Camera();

    // Sets the camera's defaults; the constructor calls it
    void Reset();
    // A frame of the camera
    void Tick(uint time);
    // Moves the camera towards a position looking at a point (PS2 names; the car's without PS2's time)
    void WantPosCar(const S_vector &target, const S_vector &look);
    void WantPos(const S_vector &target, const S_vector &pos, const S_vector &look, float time);
    // The mouse's free look (PS2 names)
    void FreeLook(float x, float y);
    void FreeLookBase(float x, float y, S_vector &pos, S_vector &dir, S_vector &up);
    // PS2 names, of what C_game's Init and Done call: each of the setters ends the camera, sets the member and begins it
    // again; Link's frame (NULL for none) gives CAMERA_C_RAILWAY_IN, without it the profile's camera
    void SetCamera(I3D_camera *cam);
    void SetPlayer(C_human *human);
    void Link(I3D_frame *frm);
    // Pops the pushed states
    void GameDone();
    // Changes the camera to a CAMERA_* or, with CAMERA_NEXT, to the camera's next one, which the profile then keeps; with
    // begin, the old camera ends and the new one begins (PS2 names)
    void SetMode(uint m, bool begin);
    // Sets the I3D camera up for the mode, from its cameraParams entry: its values and flags, its parent and position
    // (PS2 name)
    void Begin();
    // Undoes Begin's: the sniper's HUD and the frames the camera was linked to, and shows the car again (PS2 name)
    void End();
    // The last pushed state back, the mode through SetMode and the I3D camera's view; clear clears mode_changed (PS2
    // names)
    void PopMode(bool clear);
    void SetSniperMode(bool on);
    // C_game::Done turns it off after a cutscene (PC only, the name is a guess)
    void SetCutscene(bool on);
    // The name is ours: its copy is in game.cpp's range
    void SetModeChanged()
    {
        mode_changed = true;
    }
};
