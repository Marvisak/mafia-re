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
    // Its copy is among those at the end of the file at 0x460a50 (the name is ours)
    // FUNCTION: GAME 0x47ae30
    int GetParticles() const
    {
        return particles;
    }
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
        // The game's model (a script's animation's, AddAnimModel's flag), which ClearGameAnimModels takes from the list
        // at the game's end; the scene's own have it clear (OpenBin's)
        bool game_model;
    };
    // An object that fades out over time (PS2 C_mission::S_t_transparent, AddTimeTransparentObject's): Tick sets its
    // transparency to the part of the fade gone by, and releases it at the end (the members' names are ours)
    struct S_t_transparent
    {
        I3D_object *object;             // 0x0
        short time;                     // 0x4, the time gone by, 0 when added
        short fade_time;                // 0x6, the time AddTimeTransparentObject takes
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
    // bytes (the members' names are ours)
    struct S_actor_sound_buffer
    {
        C_actor *actor;                 // 0x0
        int number;                     // 0x4, the sound's, which PS2's FrmActBuff_GetActorSound takes with the actor
        I3D_sound *sound;               // 0x8
    };

    int unknown_0;                      // 0x0
    int unknown_4;                      // 0x4
    // The last number C_actor::Init gave an actor (C_actor::number), cleared by Open, and whether a mission is open:
    // Close does nothing without it, and clears it (the names are ours)
    int last_actor_number;              // 0x8
    bool opened;                        // 0xc
    I3D_scene *scene;                   // 0x10
    std::vector<S_anim_model> anim_models;  // 0x14
    C_game *game;                       // 0x24 (PS2 0x20)
    C_tshift *tshift;                   // 0x28 (PS2 0x24)
    C_web_path *web_path;               // 0x2c (PS2 0x28)
    C_roads *roads;                     // 0x30
    std::vector<C_actor *> actors;      // 0x34
    // The actors AddActor adds with its flag set (PS2 0x40), which DelNotMeActors deletes at the game's end and
    // IsOwnActor tells from the mission's own (the name is ours, after PS2's DelNotMeActors)
    std::vector<C_actor *> not_me_actors;  // 0x44
    // The scripts that run for the whole game (PS2 0x4c, the GlobalProgram functions')
    std::vector<C_program *> programs;  // 0x54
    // The scene's m_RenderNumber, which C_game::Tick copies each frame (the name is ours)
    int render_number;                  // 0x64 (PS2 0x58)
    // Open's copy of the mission's name, which UnInit deletes (the name is ours)
    char *mission_name;                 // 0x68 (PS2 0x5c)
    // PC only: PS2 has its particle manager (0x64) in the mission
    C_effects *effects;                 // 0x6c
    // The mission's numbers (the names are ours): SETMISSIONNUMBER's for the next mission, which CHANGE_MISSION makes
    // the current one's (GETMISSIONNUMBER's) and clears; main clears both for each choice of the main menu
    int next_mission_number;            // 0x70 (PS2 0xca0)
    int mission_number;                 // 0x74 (PS2 0xca4)
    // 0x78, cleared by Open (PS2 0xca8 the same): the frames GameLoopSingle renders without presenting them, counting
    // it down (0x5bea3b): 10 after C_game::Init, at least 1 from RECLOADFULL, 2 from RECUNLOAD and a cutscene's Close
    // (the name is ours)
    int skip_frames;
    // Set by Open, which mutes the sound while it loads: the first Tick with skip_frames below 1 sets the profile's
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
    // Adds the actor to the mission's, and with the flag to not_me_actors too, unless it is there already (PS2 name)
    void AddActor(C_actor *actor, bool not_me);
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
    // What is kept between missions (PS2 names). StoreActor keeps the actor of a model frame, taken out of the game,
    // under the model's file name (false where it doesn't: no model, a new mission's actor, kept already), StoreModel a
    // model, StoreActiveActor an actor that stays in the game; GetActor and GetModel take one of that name back, NULL
    // for none, and IsActiveActor and RemoveActiveActor an active actor, false for none. StoreActorSound keeps an
    // actor's sound of a number (a sound kept for it already stays), GetActorSound takes it back.
    bool FrmActBuff_StoreActor(C_actor *actor, bool release_frame);
    void FrmActBuff_StoreModel(I3D_model *model);
    C_actor *FrmActBuff_GetActor(const char *name, int type);
    I3D_model *FrmActBuff_GetModel(const char *name);
    bool FrmActBuff_StoreActiveActor(C_actor *actor);
    bool FrmActBuff_RemoveActiveActor(C_actor *actor);
    bool FrmActBuff_IsActiveActor(C_actor *actor);
    void FrmActBuff_StoreActorSound(C_actor *actor, int number, I3D_sound *sound);
    I3D_sound *FrmActBuff_GetActorSound(C_actor *actor, int number);
    // Takes the actor from actors and not_me_actors (PS2 name): whether it was one of actors
    bool UnloadActor(C_actor *actor);
    // Takes the actor out of the mission and releases it (PS2 name): false for one that isn't the mission's
    bool DelActor(C_actor *actor);
    void DelNotMeActors();
    // Whether the actor is one of actors but not of not_me_actors (the name is ours)
    bool IsOwnActor(C_actor *actor);
    // The mission's actor of a frame's name (PS2 name), NULL for none
    C_actor *FindActorByName(const char *name);
    void ClearGameAnimModels();
    // An object whose transparency goes from 0 to 1 over fade_time (PS2 name and parameters), which Tick raises and
    // releases the object at 1; FinishTimeTransparentObjects ends every fade (PS2 name). Fades under 83 are left out.
    void AddTimeTransparentObject(I3D_object *object, short fade_time);
    void FinishTimeTransparentObjects();
    // The video options' effects on the scene, and the scene's own back (PS2 names)
    void UpdateSettings();
    void RestoreSettings();
    // A model the mission's animations use, and taken from them (PS2 names): the scripts add theirs as game_model
    void AddAnimModel(I3D_model *model, bool game_model);
    void DelAnimModel(I3D_model *model);
    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47ae40
    std::vector<C_actor *> &GetActors()
    {
        return actors;
    }
    // Its copy most likely the getter of 0x24 at 0x47ae50
    C_game *GetGame() const
    {
        return game;
    }
    // FUNCTION: GAME 0x47ae60
    C_effects *GetEffects() const
    {
        return effects;
    }
    // The mission's number (GETMISSIONNUMBER, SETMISSIONNUMBER, PS2's the same): the setter writes next_mission_number,
    // the getter reads mission_number, which KeepMissionNumber copies it to (CHANGE_MISSION's; the name is ours)
    // FUNCTION: GAME 0x47ae70
    void SetMissionNumber(int n)
    {
        next_mission_number = n;
    }
    // FUNCTION: GAME 0x47ae80
    int GetMissionNumber() const
    {
        return mission_number;
    }
    // FUNCTION: GAME 0x47ae90
    void KeepMissionNumber()
    {
        mission_number = next_mission_number;
        next_mission_number = 0;
    }
    // FUNCTION: GAME 0x47aea0
    void SetSkipFrames(int value)
    {
        skip_frames = value;
    }
    // FUNCTION: GAME 0x47aeb0
    int GetSkipFrames() const
    {
        return skip_frames;
    }
    // Its copy folded with I3D_frame::GetScale's (0x47aec0, annotated by name after the class)
    S_mission_options &GetOptions()
    {
        return options;
    }
    // Its copy folded with C_actor::GetFrame's: either is the one our linker keeps
    // FUNCTION: GAME 0x47afb0 FOLDED
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
    void ResetMissionNumbers()
    {
        next_mission_number = 0;
        mission_number = 0;
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
// ?copy@std@@YIPAUai_group_member@@PBU2@0PAU2@@Z

// The actors' operator[], which the scripts' FINDACTOR calls out of line: folded with every vector of pointers', the
// original kept the copy of C_sledovacka.cpp's range (named after the copy our linker keeps, foldfix)
// TEMPLATE: GAME 0x4a9930
// ??A?$vector@PAUS_ELEMENT@C_traffic_car@@V?$allocator@PAUS_ELEMENT@C_traffic_car@@@std@@@std@@QAEAAPAUS_ELEMENT@C_traffic_car@@I@Z

// The folded copy of I3D_frame::GetScale and C_mission::GetOptions, named after the copy our linker keeps (foldfix)
// SYNTHETIC: GAME 0x47aec0
// ?GetOptions@C_mission@@QAEAAUS_mission_options@@XZ

// What the animations' models' and the fades' inlined push_back call out of line: the 8-byte elements' _Construct,
// folded with every other 8-byte type's (the original kept the copy of m_cache.cpp's range)
// TEMPLATE: GAME 0x404830
// ?_Construct@std@@YIXPAUS_scream_area@C_game@@ABU23@@Z

// What the actor sounds' inlined push_back calls out of line, folded with the same functions of every other 12-byte
// type: the elements' _Ucopy and _Ufill (the original kept the copies of C_animations.cpp's range), size and _Construct
// (this file's copies)
// TEMPLATE: GAME 0x411000
// ?_Ucopy@?$vector@US_Telephone@C_game@@V?$allocator@US_Telephone@C_game@@@std@@@std@@IAEPAUS_Telephone@C_game@@PBU34@0PAU34@@Z

// TEMPLATE: GAME 0x411040
// ?_Ufill@?$vector@US_actor_sound_buffer@C_mission@@V?$allocator@US_actor_sound_buffer@C_mission@@@std@@@std@@IAEXPAUS_actor_sound_buffer@C_mission@@IABU34@@Z

// TEMPLATE: GAME 0x5470a0
// ?size@?$vector@US_vector@@V?$allocator@US_vector@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x5490b0
// ?_Construct@std@@YIXPAUS_Telephone@C_game@@ABU23@@Z

// What OpenBin calls out of line, its inline budget spent: the frames' map's destructor, operator[] and insert, its
// tree's functions and the pair's, and the frames' vector's push_back, this file's copies; the 8-byte elements'
// push_back and insert(P, x), folded with every other 8-byte type's (named after the copies our linker keeps)
// TEMPLATE: GAME 0x5456e0
// ??1?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@std@@QAE@XZ

// TEMPLATE: GAME 0x546d30
// ?push_back@?$vector@US_anim_model@C_mission@@V?$allocator@US_anim_model@C_mission@@@std@@@std@@QAEXABUS_anim_model@C_mission@@@Z

// TEMPLATE: GAME 0x546ef0
// ?push_back@?$vector@PAVC_program@@V?$allocator@PAVC_program@@@std@@@std@@QAEXABQAVC_program@@@Z

// TEMPLATE: GAME 0x547170
// ??A?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@std@@QAEAAPAVI3D_frame@@ABVC_str@@@Z

// TEMPLATE: GAME 0x548040
// ?insert@?$vector@US_sound_light_sector@@V?$allocator@US_sound_light_sector@@@std@@@std@@QAEPAUS_sound_light_sector@@PAU3@ABU3@@Z

// TEMPLATE: GAME 0x5481f0
// ?insert@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@std@@QAE?AU?$pair@Viterator@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3282db0e2

// TEMPLATE: GAME 0x5482d0
// ??0?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@QAE@ABU?$less@VC_str@@@1@_NABV?$allocator@PA3bd97113

// TEMPLATE: GAME 0x548300
// ?begin@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@QAE?AViterator@12@XZ

// TEMPLATE: GAME 0x548310
// ??0?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@QAE@ABVC_str@@ABQAVI3D_frame@@@Z

// TEMPLATE: GAME 0x548590
// ?insert@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@QAE?AU?$pair@Viterator@?$_Tree@VC_str@@af99994b

// TEMPLATE: GAME 0x5487f0
// ?erase@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@QAE?AViterator@12@V312@@Z

// TEMPLATE: GAME 0x548be0
// ?_Erase@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: GAME 0x548c30
// ?_Init@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@IAEXXZ

// TEMPLATE: GAME 0x548cb0
// ?_Insert@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@IAE?AViterator@12@PAU_Node@12@0ABU?$pa0c7b9762

// TEMPLATE: GAME 0x548f20
// ?_Lrotate@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: GAME 0x548f80
// ?_Rrotate@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@IAEXPAU_Node@12@@Z

// TEMPLATE: GAME 0x548fe0
// ?_Buynode@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@IAEPAU_Node@12@PAU312@W4_Redbl@12@@Z

// TEMPLATE: GAME 0x549000
// ?_Dec@const_iterator@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@QAEXXZ

// TEMPLATE: GAME 0x549060
// ?_Inc@const_iterator@?$_Tree@VC_str@@U?$pair@$$CBVC_str@@PAVI3D_frame@@@std@@U_Kfn@?$map@VC_str@@PAVI3D_frame@@U?$less@VC_str@@@std@@V?$allocator@PAVI3D_frame@@@4@@3@U?$less@VC_str@@@3@V?$allocator@PAVI3D_frame@@@3@@std@@QAEXXZ

// TEMPLATE: GAME 0x5490f0
// ?_Construct@std@@YIXPAU?$pair@$$CBVC_str@@PAVI3D_frame@@@1@ABU21@@Z

// The frames' map's tree's statics, in this file's .bss
// GLOBAL: GAME 0x646db4
// std::_Tree<C_str,std::pair<C_str const ,I3D_frame *>,std::map<C_str,I3D_frame *,std::less<C_str>,std::allocator<I3D_frame *> >::_Kfn,std::less<C_str>,std::allocator<I3D_frame *> >::_Nilrefs

// GLOBAL: GAME 0x646db8
// std::_Tree<C_str,std::pair<C_str const ,I3D_frame *>,std::map<C_str,I3D_frame *,std::less<C_str>,std::allocator<I3D_frame *> >::_Kfn,std::less<C_str>,std::allocator<I3D_frame *> >::_Nil

// Mission.cpp's static ClearCarSeats (PS2's, defined before FrmActBuff_StoreActor), which VC6 emits after its first
// caller, StoreActor, where its marker would be out of address order. It has no public symbol: the PDB's procedure
// name.
// FUNCTION: GAME 0x545fd0
// ClearCarSeats
