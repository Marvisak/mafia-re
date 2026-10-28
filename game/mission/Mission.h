#pragma once

#include "Main.h"

#include <vector>

class C_game;
class C_tshift;
class C_web_path;
class C_roads;
class C_effects;
class C_program;
class I3D_driver;
class I3D_frame;
class I3D_scene;
class I3D_model;
class I3D_object;
class I3D_sound;

// The game's video options (the struct's name is ours), the controls of the Audio & Video menu (tables\menu\
// optionsa.mnu), which give the members their names. main hands each the value setup.exe saved, through a setter that
// clamps it, and saves them back when the player quits. PS2's C_mission has them at 0xcbc, without methods.
struct S_mission_options
{
    float level_of_detail;              // 0x0, 0.4 to 2: the engine's RS_LOD_QUALITY
    float draw_distance;                // 0x4, 0.3 to 1: the part of far_range the camera sees
    float far_range;                    // 0x8, the scene's camera range, which Open stores
    int game_effects;                   // 0xc, "In Game Effects": Low, Medium or High (0 to 2)
    int game_sounds;                    // 0x10, "In Game Sounds": Low, Medium or High
    int shadows;                        // 0x14, 0, 1, 3, 6, 9 or 15 for the menu's Very Low to Highest
    int particles;                      // 0x18, Low, Medium or High

    // Inline: the mission's constructor has it inlined among its members; VC6 groups its constant stores by value
    S_mission_options() :
        level_of_detail(1.0f),
        draw_distance(1.0f),
        game_effects(2),
        game_sounds(2),
        shadows(3),
        particles(2)
    {
    }

    void SetLevelOfDetail(float level);
    void SetDrawDistance(float distance);
    void SetGameEffects(int level);
    void SetGameSounds(int level);
    // By the menu's choice, 0 to 5
    void SetShadows(int index);
    // The menu's choice; the game's code compares it too
    int GetShadows();
    // Inline: main copies the floats into the settings through the FPU (fld, fstp), where the members read directly
    // are copied with integer moves
    float GetLevelOfDetail() const { return level_of_detail; }
    float GetDrawDistance() const { return draw_distance; }
    void SetParticles(int level);
};

// What C_mission::OpenBin returns (the names are ours)
#define OPENBIN_OK 0
#define OPENBIN_CORRUPTED 1             // a chunk that isn't complete
#define OPENBIN_NOFILE 3                // no scene2.bin

// What C_mission::Open returns (the names are ours)
#define MISSION_OPEN_OK 0
#define MISSION_OPEN_FAILED 1           // OpenBin failed
#define MISSION_OPEN_NO_CD 4            // the player cancelled WaitForCD

// The loaded mission (PS2 Mission.cpp): its scene, actors and game, 0xdc bytes. PS2's has these members in the same
// order (its offsets in the comments) and a few more, its particle manager (0x64) among them. Members without a name
// are named by their offset.
class C_mission
{
public:
    // A model the mission's animations use (PS2 C_mission::S_anim_model, AddAnimModel's; the members' names are ours)
    struct S_anim_model
    {
        I3D_model *model;
        bool unknown_4;
    };
    // An object that fades out over time (PS2 C_mission::S_t_transparent, AddTimeTransparentObject's)
    struct S_t_transparent
    {
        I3D_object *object;             // 0x0
        short unknown_4;                // 0x4, 0 when added
        short unknown_6;                // 0x6, the time AddTimeTransparentObject takes
    };
    // An actor or model kept between missions (PS2 C_mission::S_frame_actor_buffer, the FrmActBuff functions'), 0x10
    // bytes. PS2's StoreActor stores the actor, its frame, the frame's name and its flag, StoreModel no actor, the
    // model, its name and false (the members' names are ours).
    struct S_frame_actor_buffer
    {
        C_actor *actor;                 // 0x0
        I3D_frame *frame;               // 0x4, the actor's frame or the model
        char *name;                     // 0x8, the file's name without its folders, deleted with the entry
        // 0xc, StoreActor's flag: FrmActBuff_ClearAll releases the actor's frame too (a model's always)
        bool release_frame;
    };
    // An actor's sound kept between missions (PS2 C_mission::S_actor_sound_buffer, FrmActBuff_StoreActorSound's), 0xc
    // bytes
    struct S_actor_sound_buffer
    {
        C_actor *actor;                 // 0x0
        int unknown_4;                  // 0x4, the sound's number
        I3D_sound *sound;               // 0x8
    };

    int unknown_0;                      // 0x0
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8, the actors C_actor::Init has numbered (C_actor::number)
    bool unknown_c;                     // 0xc, set while a mission is open: Close does nothing without it, and clears it
    I3D_scene *scene;                   // 0x10
    std::vector<S_anim_model> anim_models;  // 0x14
    C_game *game;                       // 0x24 (PS2 0x20)
    C_tshift *tshift;                   // 0x28 (PS2 0x24)
    C_web_path *web_path;               // 0x2c (PS2 0x28)
    C_roads *roads;                     // 0x30
    std::vector<C_actor *> actors;      // 0x34
    // The actors AddActor adds with its flag set (PS2 0x40)
    std::vector<C_actor *> unknown_44;  // 0x44
    // The scripts that run for the whole game (PS2 0x4c, the GlobalProgram functions')
    std::vector<C_program *> programs;  // 0x54
    int unknown_64;                     // 0x64 (PS2 0x58)
    // Open's copy of the mission's name, which UnInit deletes (the name is ours)
    char *mission_name;                 // 0x68 (PS2 0x5c)
    // PC only: PS2 has its particle manager (0x64) in the mission
    C_effects *effects;                 // 0x6c
    int unknown_70;                     // 0x70, cleared after each mission is opened (PS2 0xca0)
    int unknown_74;                     // 0x74 (PS2 0xca4)
    int unknown_78;                     // 0x78, cleared by Open (PS2 0xca8 the same)
    // Set by Open, which mutes the sound while it loads: the first Tick with unknown_78 below 1 sets the profile's
    // volume again and clears it (PC only, the name is ours)
    bool restore_volume;                // 0x7c
    uchar unknown_7d[3];
    std::vector<S_t_transparent> t_transparent;  // 0x80 (PS2 0xcb0)
    S_mission_options options;          // 0x90 (PS2 0xcbc)
    std::vector<S_frame_actor_buffer> frame_actor_buffer;  // 0xac (PS2 0xcd8)
    std::vector<C_actor *> active_actors;  // 0xbc (PS2 0xce4)
    std::vector<S_actor_sound_buffer> actor_sound_buffer;  // 0xcc (PS2 0xcf0)

    C_mission();
    ~C_mission();

    // Creates the scene and the objects that live as long as the game (the game among them), once
    void Init(I3D_driver *driver);
    // Undoes Init: CloseSystem calls it once, and the destructor
    void UnInit();
    // Opens the mission of that name, a folder in missions\: its scene (log adds I3DLOAD_LOG), webs, cache and data
    // (OpenBin, which takes flags), with mafiaLoadSave's loading screen, which progress creates if there is none.
    // MISSION_OPEN_*.
    int Open(const char *name, bool log, uint flags, bool progress);
    // A new actor of a type of the mission's scene2.bin, NULL for an unknown type (PS2 name)
    C_actor *CreateActor(int type);
    // Adds the actor to the mission's, and with the flag to unknown_44 too, unless it is there already (PS2 name)
    void AddActor(C_actor *actor, bool add_unknown_44);
    // Loads the mission's data from its folder (PS2 name), its scene2.bin: the scene's objects and camera, the actors,
    // the scripts, the webs. OPENBIN_*.
    int OpenBin(const char *dir, uint flags);
    // The name is ours
    // FUNCTION: GAME 0x547120
    C_tshift *GetTShift()
    {
        return tshift;
    }
    // Unloads the open mission: its game, actors, scripts and objects, the caches' contents and the scene's frames
    void Close();
    // PS2 names, of what C_game's Init and Done call
    void GlobalProgramGameInit();
    void GlobalProgramRun(const char *name);
    void GlobalProgramGameDone();
    bool FrmActBuff_IsActiveActor(C_actor *actor);
    // The kept actor of a frame's name and type, taken from the buffer (PS2 name), NULL for none
    C_actor *FrmActBuff_GetActor(const char *name, int type);
    // Keeps the actor of a model for the next mission (PS2 name and parameters)
    // Keeps the actor of a model frame for the next mission (PS2 name): false where it doesn't
    bool FrmActBuff_StoreActor(C_actor *actor, bool flag);
    // Takes the actor from actors and unknown_44 (the name is ours): whether it was one of actors
    bool RemoveActor(C_actor *actor);
    // Whether the actor is one of actors but not of unknown_44 (the name is ours)
    bool IsOwnActor(C_actor *actor);
    bool FrmActBuff_StoreActiveActor(C_actor *actor);
    void FrmActBuff_RemoveActiveActor(C_actor *actor);
    bool DelActor(C_actor *actor);
    void DelNotMeActors();
    void ClearGameAnimModels();
    void FinishTimeTransparentObjects();
    // The video options' effects on the scene, and the scene's own back (PS2 names)
    void UpdateSettings();
    void RestoreSettings();
    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    void SetUnknown78(int value)
    {
        unknown_78 = value;
    }
    // Its copy most likely the getter of 0x24 at 0x47ae50
    C_game *GetGame() const
    {
        return game;
    }
    const char *GetMissionName() const
    {
        return mission_name;
    }
    I3D_scene *GetScene() const
    {
        return scene;
    }
    // A frame of the mission: its scene, actors, scripts and game
    void Tick(int time);
    // PS2 names: Close's helpers. DelActors releases the actors and takes them from their frames, FrmActBuff_ClearAll
    // releases what is kept between missions.
    void DelActors();
    void FrmActBuff_ClearAll();
    // The name is ours. main clears both members through one load of act_mission, so through a method: written out,
    // VC6 reloads the pointer after the first store.
    void ResetUnknown70()
    {
        unknown_70 = 0;
        unknown_74 = 0;
    }
};

// The mission (PS2 main_mission, Main.cpp's), which act_mission points to
extern C_mission main_mission;
extern C_mission *act_mission;

// What Close's inlined clear of the scripts and of the animations' models calls: the scripts' erase (folded with every
// vector of pointers', the original kept C_effects.cpp's copy) and the models' copy (folded too, an earlier file's)
// TEMPLATE: GAME 0x44b270
// ?erase@?$vector@PAVC_program@@V?$allocator@PAVC_program@@@std@@@std@@QAEPAPAVC_program@@PAPAV3@0@Z

// TEMPLATE: GAME 0x538520
// ?copy@std@@YIPAUS_anim_model@C_mission@@PAU23@00@Z
