#pragma once

#include "Main.h"

#include <vector>

#include "input/controls.h"

// The game's controls, each an entry of the controls' tables (C_controls::SetControl's index; the name is ours)
#define NUM_CONTROLS 60

// The controls' indices (the names are ours, after the controls menus' texts, optionsc1 and optionsc2). Those with two
// columns there have a secondary binding at the next index, CTRL_SECONDARY.
#define CTRL_FORWARD 0x00
#define CTRL_BACKWARD 0x02
#define CTRL_LEFT 0x04
#define CTRL_RIGHT 0x06
#define CTRL_WALK 0x08                  // "Walk / Run"
#define CTRL_ACTION 0x0a
#define CTRL_FIRE 0x0c
#define CTRL_CROUCH 0x0e
#define CTRL_JUMP 0x10
#define CTRL_AIM 0x12
#define CTRL_CYCLE_UP 0x14
#define CTRL_CYCLE_DOWN 0x16
#define CTRL_INVENTORY 0x18
#define CTRL_HIDE_WEAPON 0x19
#define CTRL_DROP_WEAPON 0x1a
#define CTRL_TOGGLE_RUN 0x1b            // "Toggle Run / Walk"
#define CTRL_RELOAD 0x1c
#define CTRL_SNIPER 0x1d                // "Sniper Mode"
#define CTRL_OBJECTIVES 0x1e
#define CTRL_MAP 0x1f                   // "City Map"
// 0x20 is in neither menu
#define CTRL_ACCELERATE 0x21
#define CTRL_BRAKE 0x23                 // "Brake / Reverse"
#define CTRL_STEER_LEFT 0x25
#define CTRL_STEER_RIGHT 0x27
#define CTRL_HANDBRAKE 0x29
#define CTRL_SPEED_LIMITER 0x2b
#define CTRL_GEAR_UP 0x2d
#define CTRL_GEAR_DOWN 0x2f
#define CTRL_HORN 0x31
#define CTRL_LOOK_LEFT 0x33
#define CTRL_LOOK_RIGHT 0x35
#define CTRL_CLUTCH 0x37
#define CTRL_GEARING 0x39               // "Manual/Auto Gearing"
#define CTRL_CAMERA 0x3a
#define CTRL_REPLACE_CAR 0x3b           // "Replace car (racing)"
#define CTRL_SECONDARY 1
// The player's controls come first, the car's from CTRL_ACCELERATE (the name is ours)
#define NUM_PLAYER_CONTROLS CTRL_ACCELERATE

// G_LoadSaveClass::Refresh's flags, which options it applies (the names are ours)
#define REFRESH_SOUND 0x1               // the sound driver's volume and the cars', which depends on it
#define REFRESH_MUSIC 0x2               // the videos' and the music's
#define REFRESH_GAMMA 0x4
#define REFRESH_FORCE_FEEDBACK 0x8
#define REFRESH_CARS 0x20               // the cars' volume
#define REFRESH_ALL 0xffffffff

// The flags of G_LoadSaveClass::DefaultControls and DefaultSettings (the names are ours, after the controls menu's
// pages, whose Reset to Defaults passes the page's flag to both): 0x80 clears the block first, the other bits choose
// groups of defaults, DefaultControls' DEFAULTS_PLAYER and DEFAULTS_CAR, DefaultSettings' DEFAULTS_OTHER and
// DEFAULTS_AUDIO_VIDEO (the Audio & Video menu's)
#define DEFAULTS_PLAYER 0x1
#define DEFAULTS_CAR 0x2
#define DEFAULTS_OTHER 0x4
#define DEFAULTS_AUDIO_VIDEO 0x8
#define DEFAULTS_CLEAR 0x80
#define DEFAULTS_ALL 0xffff

// The profiles' files that ProfileEnumFiles looks for, savegame\mafia000.sav to mafia031.sav (the name is ours)
#define MAX_PROFILES 32

// The longest name ProfileCreate gives a profile (the head has room for more; the name is ours)
#define PROFILE_NAME_LENGTH 0x1f

// The saved games' files of a profile that ProfileDelete removes with it, savegame\mafia<profile>.000 to .599 (the name
// is ours)
#define PROFILE_SAVE_FILES 600

// The garage's slots (the name is ours)
#define MAX_GARAGE_CARS 25

// S_profile_settings::cursor, the texts of the Cursor setting's choices (the names are ours)
#define CURSOR_CROSSHAIR 0
#define CURSOR_CENTER_POINT 1
#define CURSOR_BOTH 2                   // "Crosshair + Center Point"

class C_car;
class GM_LoadProgress;

// G_Profile_FileHead::magic (the name is ours)
#define PROFILE_MAGIC 'Prof'

// A profile's head (the name is ours, after PS2's G_SaveGame_FileHead), 0x54 bytes: the first record of its file,
// savegame\mafia<number>.sav, which ProfileSave writes and ProfileEnumFiles reads from each file into the list
struct G_Profile_FileHead
{
    uint magic;                         // 0x0, PROFILE_MAGIC
    uint version;                       // 0x4, 1
    uint number;                        // 0x8, the file's
    uint unknown_c;                     // 0xc, 0 when saved
    uchar name[0x34];                   // 0x10, the player's, which the profiles' menu shows
    uint freeride_enabled;              // 0x44, G_LoadSaveClass::freeride_enabled
    uint unknown_48;                    // 0x48, G_LoadSaveClass::unknown_9c
    uint unknown_4c;                    // 0x4c, 0 when saved
    uint race_cars;                     // 0x50, G_LoadSaveClass::race_cars
};

// A saved game's head (PS2's name), 0x18 bytes: the first record of its file, savegame\mafia<profile>.<number>
struct G_SaveGame_FileHead
{
    uint unknown_0;
    uint version;                       // 0x4, 0x10
    uint number;                        // 0x8, the file's
    uchar unknown_c[0xc];
};

// The profile's options (the name is ours), 0x5c bytes, which ProfileSave writes after the controls; DefaultSettings
// sets their defaults. The names are ours, after the texts of the menus that set them: the controls menu's Other page
// (optionsc3) and the Audio & Video menu (optionsa).
struct S_profile_settings
{
    float steering_linearity;           // 0x0, "Nonlinear / Linear"
    float blood_level;                  // 0x4, "Level of Blood"
    // The mouse camera's filter (G_Camera::Tick's CAMERA_P_MOUSE: each older movement weighs unknown_c to the power of
    // unknown_8 times its age in hundredths of a second); nothing else reads them
    float unknown_8;                    // 0x8
    float unknown_c;                    // 0xc
    uchar unknown_10[4];                // 0x10
    float aiming_sensitivity_x;         // 0x14
    float aiming_sensitivity_y;         // 0x18
    float aiming_speed_x;               // 0x1c, "Max Aiming Speed X"
    float sound_volume;                 // 0x20, "Sounds"
    float music_volume;                 // 0x24
    float car_volume;                   // 0x28, "Cars"
    float speech_volume;                // 0x2c
    // 1 to 3 (Refresh resets others to 1): the camera G_Camera::Reset takes, the number of an
    // entry of the camera's cameraParams (where PS2's Reset takes 1)
    uint camera;                        // 0x30
    // The same for 7 to 15, 7 otherwise
    uint unknown_34;                    // 0x34
    float gamma;                        // 0x38
    // Force feedback
    float ff_volume;                    // 0x3c, "Global Stick Volume"
    float ff_suspension;                // 0x40
    float ff_collisions;                // 0x44
    float ff_revs;                      // 0x48
    float ff_tire_grip;                 // 0x4c
    float ff_player_movement;           // 0x50
    uint cursor;                        // 0x54, CURSOR_*
    bool speedometer_mph;               // 0x58, "Miles per Hour" or "Kilometers per Hour"
    bool side_roll;                     // 0x59
    bool mouse_inverted;                // 0x5a, "Mouse control": "Inverted" or "Normal"
    bool subtitles;                     // 0x5b
};

// A car in the garage (the names are ours), which the slot's car descriptor and the car's actor occupy
struct S_garage_car
{
    uint car;                           // the car database's index and flags, 0 for a free slot
    C_car *actor;                       // 0x4
};

// The free ride's state (PS2's name, the same 0x1c bytes; the members' names are ours), which PS2's FreerideStoreA
// and FreerideStoreB set
struct S_freeride
{
    uint unknown_0;
    uint unknown_4;
    uint state;                         // 0x8, FreerideStoreB's, which PS2's FreerideGetState returns
    float unknown_c[3];                 // 0xc, FreerideStoreA's floats
    bool unknown_18;                    // 0x18, cleared by SaveGameNewGame
};

// G_LoadSaveClass::freeride_enabled, how much of the free ride is open (the names are ours, after the texts of the
// locations it opens): the small city only, then the city by day and night, then the countryside too; above that also
// the main menu's Free Ride Extreme
#define FREERIDE_SMALL_CITY 0
#define FREERIDE_CITY 1
#define FREERIDE_COUNTRYSIDE 2

// The keys G_Stream's encryption starts with (the names are ours): two dates, 23.10.1976 and 10.7.2002
#define STREAM_KEY1 0x23101976
#define STREAM_KEY2 0x10072002

// The save files' stream (PS2 G_Stream, in g_loadsave.cpp as on PS2; the members' names are ours), 0x3c bytes: a file
// read and written through the C library, the data encrypted with two running keys when crypt is set
class G_Stream
{
public:
    bool crypt;                         // 0x0, SetParams's
    bool unknown_1;                     // 0x1, SetParams's
    uint key1;                          // 0x4
    uint key2;                          // 0x8
    uint magic;                         // 0xc, SetParams's, the file's kind (PROFILE_MAGIC)
    // The file's first bytes, which OpenRead reads and OpenWrite writes: the magic, 0, 1, then zeros
    uint head[6];                       // 0x10
    FILE *file;                         // 0x28
    bool unknown_2c;                    // 0x2c, Close returns whether this or unknown_2d is set
    bool unknown_2d;                    // 0x2d
    uint size;                          // 0x30, WantWrite's (0x60b1a0)
    uint unknown_34;                    // 0x34
    void *buffer;                       // 0x38, WantWrite's

    G_Stream();
    ~G_Stream();
    void SetParams(uint magic, bool crypt, bool unknown);
    // Opens the file for reading and reads its head, with the keys reset
    bool OpenRead(char *name);
    // Creates the file and writes its head, with the keys reset
    bool OpenWrite(char *name);
    bool Close();
    // Encrypts size bytes (whole dwords) in place, the keys running on, as Decrypt reverses it (PS2's is empty)
    void Crypt(void *data, uint size);
    void Decrypt(void *data, uint size);
    // Writes size bytes from adr, through an encrypted copy if crypt is set; returns what fwrite wrote
    uint WriteAdr(void *adr, uint size);
    // Reads size bytes into adr, decrypted if crypt is set; returns what fread read
    uint ReadAdr(void *adr, uint size);
};

// The saved games, profiles and the game's progress (PS2 G_LoadSaveClass, g_loadsave.cpp), at least 0x5d8 bytes (the
// constructor clears 0x5d4). Only what main, the camera and the start use so far.
class G_LoadSaveClass
{
public:
    std::vector<G_Profile_FileHead> profiles;   // 0x0, what ProfileEnumFiles found
    std::vector<G_SaveGame_FileHead> savegames; // 0x10, the profile's saved games
    G_Profile_FileHead profile;         // 0x20, the profile's head
    uchar unknown_74[8];
    uint mission;                       // 0x7c, the saved game's mission (the name is ours)
    uchar unknown_80[0xc];
    uint unknown_8c;                    // 0x8c
    bool unknown_90;                    // 0x90, set by Init (PS2's 0x2b0 the same)
    bool unknown_91;                    // 0x91, set while ProfileLoad reads the profile: a failed load leaves the defaults
    uint race_cars;                     // 0x94, RaceCarsEnable's bits
    // 0x98, cleared by SaveGameNewGame: the cars EnableSteal lets steal, a bit for each letter, which CanSteal tests
    // against the car's
    uint unknown_98;
    uint unknown_9c;                    // 0x9c, FreerideEnableCar's bits, a bit for each letter
    S_garage_car garage[MAX_GARAGE_CARS];   // 0xa0, the cars 0x60b9f0 adds
    uint unknown_168[MAX_GARAGE_CARS];  // 0x168, cleared with them
    // 0x1cc, cleared by GarageGameInit, set by GarageShowResults (PS2's G_GarageClass 0x7c)
    bool unknown_1cc;
    bool unknown_1cd;                   // 0x1cd, the same, set by GarageAddLastStolen (PS2's 0x7d)
    uchar unknown_1ce[2];
    uint mission_name_id;               // 0x1d0, SetMissionNameID's, of the text database (the name is ours)
    bool unknown_1d4;                   // 0x1d4, cleared by ProfileLoad
    std::vector<uint> objectives;       // 0x1d8, ObjectivesClear and ObjectivesRemove's
    // 0x1e8, cleared by SaveGameNewGame: a car GarageGameDone adds to the garage with unknown_1cc or unknown_1cd set
    C_car *unknown_1e8;
    C_car *cant_get_out;                // 0x1ec, CarCantGetOut's car (the name is ours)
    // 0x1f0, how much of the free ride is open (FREERIDE_*; the name is ours): PS2's FreerideEnable sets it (the PC's
    // 0x60e910), the profile keeps it
    uint freeride_enabled;
    S_control bindings[NUM_CONTROLS];   // 0x1f4, the profile's controls, which ApplyControls hands to the controls
    S_profile_settings settings;        // 0x4c4
    uchar unknown_520[0x9c];            // 0x520, which ProfileSave writes last (PS2's 0x2fc, as AllDefault clears it)
    uchar unknown_5bc[0x18];
    // The loading screen C_mission::Open creates when asked to, which the load callback (cbLoadScene) fills and the
    // game's Init then (PS2's 0x3d0; the name is ours)
    GM_LoadProgress *progress;          // 0x5d4

    G_LoadSaveClass();
    ~G_LoadSaveClass();

    // Sets up the profiles at the start, the player choosing one in the profiles' menu; false is fatal, the player
    // quitting there
    bool Init();
    // Saves the profile and frees the game's tables, at CloseSystem
    void Close();
    // The player chose another profile; true to restart the menu with it
    bool ProfileChange();
    // The defaults: the car database, the controls, the options, the race cars and a new game
    void AllDefault();
    // Reads the head of each profile file into profiles, which it returns (PS2's returns its list)
    std::vector<G_Profile_FileHead> *ProfileEnumFiles();
    // Forgets them
    void ProfileEnumRelease();
    // The list ProfileEnumFiles read (PS2 name)
    std::vector<G_Profile_FileHead> *ProfileGetList();
    // The list's profile of that index becomes the profile, which ProfileLoad then reads (PS2 name)
    void ProfileSelect(uint index);
    // A new profile of that name, the list's last and the profile: its file has only the head. False where a profile
    // has the name already, whatever the case (PS2 name).
    bool ProfileCreate(uchar *name);
    // Deletes the list's profile of that index, its file and its saved games' (PS2 name)
    void ProfileDelete(uint index);
    // Reads the profile's file, which profile.number names, or takes the defaults
    void ProfileLoad();
    // Writes the profile's file
    void ProfileSave();
    // A new game's progress: no saved position, an empty garage
    void SaveGameNewGame();
    // Loads the saved game the player chose and opens its mission
    void SaveGameLoad();
    // Saves the game in the slot from a mission's script (PS2 name; the parameters' names are guesses): the profile
    // too, and with message the console says so
    void SaveGameFromScript(uint slot, bool full, bool message);
    // The game's language, two lowercase letters as a multicharacter constant ('en')
    void LanguageSelect(uint language);
    // One control's binding (PC only, the name is ours), which DefaultControls passes
    void SetBinding(int index, ushort device, ushort code, int joy, int half);
    // Another control bound to the same key, button or axis (PC only, the name is ours), any half with any_half: among
    // the player's controls for one of the player's, among the car's for the car's or when other is one of them; skips
    // index and other. -1 for none, and for an unbound control.
    int FindBinding(int index, S_control *control, bool any_half, int other);
    // The controls' defaults (PC only, the name is ours): DEFAULTS_CLEAR unbinds them first, DEFAULTS_PLAYER and
    // DEFAULTS_CAR choose the groups
    void DefaultControls(uint flags);
    // Hands the profile's controls to the controls' object (PC only, the name is ours)
    void ApplyControls();
    // The options' defaults: DEFAULTS_CLEAR clears them first, DEFAULTS_OTHER and DEFAULTS_AUDIO_VIDEO choose groups
    void DefaultSettings(uint flags);
    void RaceCarsEnable(uint cars);
    void RaceCarsDisable(uint cars);
    // Empties the garage (PC only, the name is ours; PS2's SaveGameNewGame clears its G_GarageClass instead)
    void GarageClear();
    // PS2's G_GarageClass's (the PC has the garage here): IsCarIn, GameInit and GameDone
    bool IsCarIn(C_car *car);
    void GarageGameInit();
    // The garage at the mission's end: the cars the mission leaves in it, the actors taken away (but those of cars,
    // which the next mission keeps), the menu to choose which go where there are more than room
    void GarageGameDone(std::vector<C_car *> *cars);
    // PS2's G_GarageClass's: the garage's results' screen at a mission's end
    void GarageShowResultsProc();
    // PS2's G_GarageClass's too (the Garage prefix ours, where the name is the class's). Into the garage: a car of the
    // database, with two values it passes on to the database in the other order (PS2's takes one), or a car actor; out
    // of it the car of that model (the level is passed, not read). GenerateCars and ReleaseCars are empty on the PC
    // (the calls go to a folded empty function, 0x4aabe0). CarManager is the garage's manager; GarageShowResults and
    // GarageAddLastStolen set the flags GarageGameDone reads at the mission's end. Then the car the player can't get
    // out of, and the cars that can be stolen, a letter each (A to Z, either case), and whether a model's car can.
    void AddCarByIdx(uint car, uint unknown1, uint unknown2);
    void AddCarByActor(C_car *car);
    void RemoveCar(char *model, int level);
    void GenerateCars();
    void ReleaseCars();
    void CarManager();
    void GarageShowResults();
    void GarageAddLastStolen();
    void CarCantGetOut(C_car *car);
    void EnableSteal(uchar c);
    bool CanSteal(uchar *model);
    // The mission's objectives, texts of the text database (PS2 names)
    void ObjectivesClear();
    void ObjectivesAdd(uint id);
    void ObjectivesRemove(uint id);
    // The saved game's mission (PS2 name, a real function on PS2): the copy is in the file's range
    // FUNCTION: GAME 0x605e80
    uint SaveGameGetSelectedNumber() const
    {
        return mission;
    }
    // Applies the options REFRESH_* chooses, and puts the cameras back in their ranges
    void Refresh(uint flags);
    // Plays tables\credits.def
    void ExecuteCredits();
    // The free ride's cars, a bit for each letter (A to Z, either case), and how much of it is open, FREERIDE_* (PS2
    // names)
    void FreerideEnableCar(uchar c);
    void FreerideEnable(uint level);
    // The free ride's state (PS2's names)
    void FreerideStoreA(uint a, uint b, float c, float d, float e);
    void FreerideStoreB(uint state);
    // The free ride set up from its mission's script (PS2 name)
    void FreerideSetupFromScript();
    void FreerideSetup();
    // Finds Tommy and his car in the free ride's mission
    void FreerideSetupAfter();

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b0d0
    void SetMissionNameID(uint id)
    {
        mission_name_id = id;
    }
};

// A car of tables\carindex.def (the name is ours), 0xa8 bytes
struct S_car_index
{
    char id[0x20];                      // 0x0, "fordttud"
    char model[0x20];                   // 0x20, "fordTrun.I3D"
    char shadow_model[0x20];            // 0x40, "fordTrunS.i3d"
    char name[0x20];                    // 0x60, "Bolt Ace Tudor"
    uint unknown_80;
    uint unknown_84;                    // 0x84, PS2's AllDefault enables these race cars
    uint unknown_88;                    // 0x88
    uchar unknown_8c[0x1c];
};

// A player's model of tables\playerindex.def (the name is ours), 0x24 bytes
struct S_player_index
{
    char model[0x20];                   // "civil02.i3d"
    uint flags;
};

// The cars and the players' models (PS2 G_CarDatabase, in g_loadsave.cpp as on PC), 0xc4 bytes
class G_CarDatabase
{
public:
    uint num_cars;                      // 0x0
    S_car_index *cars;                  // 0x4, tables\carindex.def
    uint num_players;                   // 0x8
    S_player_index *players;            // 0xc, tables\playerindex.def
    S_car_index none;                   // 0x10, what PS2's GetCar returns for an index out of range
    uint unknown_b8;                    // 0xb8
    uint unknown_bc;                    // 0xbc
    uint unknown_c0;                    // 0xc0

    G_CarDatabase();
    ~G_CarDatabase() { Close(); }

    // Reads both tables
    void Init();
    // Frees them
    void Close();
    // The model's file of a car descriptor (PS2 name): the car's id and the variant's two digits, the variant 10 higher
    // with second; false for an index out of range
    bool GetCarModel(uint car, char *name, bool second);
    // A car descriptor's parts (PS2 name), each pointer may be NULL: the index, the level (0 to 2) and the variant (0 to
    // 4); false, all 0, for an index out of range
    bool GetCarParams(uint car, uint *index, uint *level, uint *variant);
    // A car descriptor's car (PS2 name): the first car for an index out of range, none without the table
    S_car_index *GetCar(uint car);
    // A player's model of tables\playerindex.def, "-" for an index out of range or without the table (PS2 name)
    char *GetPlayerModel(uint id);
    // The index of the first car (from 1) whose id is the model, whatever the case, 0 for none (PS2 name; PS2's takes
    // a uchar *)
    uint GetIdxByModel(const char *model);
    // The index of a model's file, "<id><variant>.i3d" with a path, and its variant (PS2 name); 0 for none
    uint GetIdxByModelLong(const char *model, uint &variant);
    // A car actor's descriptor, from its model's file and level (PS2 name), the level cleared as GetCarParams reads it
    uint GetIDByActor(C_car *car);
};

// Cars GarageGameDone doesn't take into the garage (the names are ours, after tables\carindex.def's ids): the trucks,
// and the racing Bugatti as the car of unknown_1e8
#define CAR_RACING_BUGATTI 0x2b         // bugatti, "Carrozella C-Otto 4WD"
#define CAR_TRUCK_FLATBED 0x39          // trucka
#define CAR_TRUCK_COVERED 0x3a          // truckb
#define CAR_TRUCK 0x3d                  // TruckBx

// The cars the garage keeps at a mission's end, past which GarageGameDone runs its menu (the name is ours)
#define GARAGE_ROOM 16

// A car GarageGameDone keeps (the names are ours), 0xc bytes
struct S_garage_item
{
    uint car;                           // the descriptor, its level cleared; 0 for an item to drop
    C_car *actor;                       // 0x4, NULL for a garage car without one
    int slot;                           // 0x8, its garage slot, -1 for a car new to the garage
};

// A garage car's descriptor (G_LoadSaveClass::garage, S_garage_car::car; the names are ours): the car database's index,
// the level and the variant
#define CAR_INDEX_MASK 0xffff
#define CAR_LEVEL_SHIFT 16
#define CAR_VARIANT_SHIFT 24

// The profiles' vector's helpers, which ProfileEnumFiles' inlined push_back calls
// TEMPLATE: GAME 0x60efa0
// ?size@?$vector@UG_Profile_FileHead@@V?$allocator@UG_Profile_FileHead@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x60f520
// ?_Ucopy@?$vector@UG_Profile_FileHead@@V?$allocator@UG_Profile_FileHead@@@std@@@std@@IAEPAUG_Profile_FileHead@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x60f560
// ?_Ufill@?$vector@UG_Profile_FileHead@@V?$allocator@UG_Profile_FileHead@@@std@@@std@@IAEXPAUG_Profile_FileHead@@IABU3@@Z

// ProfileCreate's push_back calls it; the original's copy is a folded one in the file at 0x4f6a60
// TEMPLATE: GAME 0x4f7cf0
// ?insert@?$vector@UG_Profile_FileHead@@V?$allocator@UG_Profile_FileHead@@@std@@@std@@QAEXPAUG_Profile_FileHead@@IABU3@@Z

// Set while the profile mustn't be saved: from the start until Init has loaded one, and when the player quit in the
// profiles' menu (PS2's name)
extern bool profileIgnoreSave;
extern G_LoadSaveClass mafiaLoadSave;
extern G_CarDatabase mafiaCarDatabase;
extern S_freeride freeride;
