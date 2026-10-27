#pragma once

#include "Main.h"

#include <vector>

class C_chunk;
class I3D_frame;

// The commands' numbers, S_instruction::command: DecodeInstruction's table names them (the names are its, but for the
// blocks' { and } and the bracketed debugging commands, [BREAK], [DEBUG_ME], [DEBUG_STOP] and [DAN_DEBUG])
#define CMD_LABEL                       0
#define CMD_GOTO                        1
#define CMD_GOSUB                       2
#define CMD_RETURN                      3
#define CMD_WAIT                        4
#define CMD_BEEP                        5
#define CMD_END                         6
#define CMD_DIM_FLT                     7
#define CMD_LET                         8
#define CMD_IF                          9
#define CMD_RND                         10
#define CMD_DIM_FRM                     11
#define CMD_DIM_ACT                     12
#define CMD_FINDFRAME                   13
#define CMD_FRM_SETON                   14
#define CMD_FRM_SETON1                  15
#define CMD_FRM_ISON                    16
#define CMD_FRM_LINKTO                  17
#define CMD_FINDACTOR                   18
#define CMD_CALL                        19
#define CMD_IFFLTINRANGE                20
#define CMD_PLAYSOUND                   21
#define CMD_PLAYSOUNDSTOP               22
#define CMD_PLAYSOUNDEX                 23
#define CMD_STOPSOUND                   24
#define CMD_GETSOUNDTIME                25
#define CMD_ACT_SETSTATE                26
#define CMD_DETECTOR_ISSIGNAL           27
#define CMD_DETECTOR_SETSIGNAL          28
#define CMD_DETECTOR_INRANGE            29
#define CMD_SETCOMPASS                  30
#define CMD_SETLMLEVEL                  31
#define CMD_DETECTOR_WAITFORUSE         32
#define CMD_DETECTOR_WAITFORHIT         33
#define CMD_COMMANDBLOCK                34
#define CMD_CALLSUBROUTINE              35
#define CMD_SOUNDFADE                   36
#define CMD_ENDOFMISSION                37
#define CMD_PROGRAM_STORAGE             38
#define CMD_DOOR_LOCK                   39
#define CMD_DOOR_OPEN                   40
#define CMD_DOOR_ENABLEUS               41
#define CMD_RECCLEAR                    42
#define CMD_RECADDACTOR                 43
#define CMD_RECLOAD                     44
#define CMD_RECLOADFULL                 45
#define CMD_RECWAITFOREND               46
#define CMD_RECUNLOAD                   47
#define CMD_LOADCOLLTREE                48
#define CMD_EXPLOSION                   49
#define CMD_TIMERON                     50
#define CMD_TIMEROFF                    51
#define CMD_CHANGE_MISSION              52
#define CMD_CAMERA_SETSWING             53
#define CMD_AIRPLANE_START              54
#define CMD_AIRPLANE_START2             55
#define CMD_DEBUG_TEXT                  56
#define CMD_GETACTORSDIST               57
#define CMD_GETCARDAMAGE                58
#define CMD_HUMAN_ADDWEAPON             59
#define CMD_HUMAN_CANADDWEAPON          60
#define CMD_HUMAN_DELWEAPON             61
#define CMD_HUMAN_ISWEAPON              62
#define CMD_HUMAN_ACTIVATEWEAPON        63
#define CMD_HUMAN_ANYWEAPONINHAND       64
#define CMD_HUMAN_ANYWEAPONININVENTORY  65
#define CMD_HUMAN_GETITEMINRHAND        66
#define CMD_PRELOADMODEL                67
#define CMD_LOADDIFFERENCES             68
#define CMD_CLEARDIFFERENCES            69
#define CMD_BLOCK_BEGIN                 70
#define CMD_BLOCK_END                   71
#define CMD_SETAIPRIORITY               72
#define CMD_EVENT                       73
#define CMD_SETEVENT                    74
#define CMD_EVENT_USE_CB                75
#define CMD_ENEMY_CHANGEANIM            76
#define CMD_HUMAN_CHANGEANIM            77
#define CMD_DIALOG_BEGIN                78
#define CMD_DIALOG_END                  79
#define CMD_DIALOG_CAMSWITCH            80
#define CMD_GAME_NIGHTMISSION           81
#define CMD_HUMAN_LOOKTOACTOR           82
#define CMD_HUMAN_LOOKTOFRAME           83
#define CMD_GETMISSIONNUMBER            84
#define CMD_SETMISSIONNUMBER            85
#define CMD_HUMAN_LINKTOHAND            86
#define CMD_HUMAN_UNLINKFROMHAND        87
#define CMD_CREATEWEAPONFROMFRAME       88
#define CMD_GARAGE_ADDCAR               89
#define CMD_GARAGE_ADDCARIDX            90
#define CMD_GARAGE_DELCAR               91
#define CMD_GARAGE_GENERATECARS         92
#define CMD_GARAGE_RELEASECARS          93
#define CMD_GARAGE_CARMANAGER           94
#define CMD_PLAYER_LOCKCONTROLS         95
#define CMD_PERSON_PLAYANIM             96
#define CMD_PERSON_STOPANIM             97
#define CMD_GUNSHOP_MENU                98
#define CMD_HUMAN_SET8SLOT              99
#define CMD_ENEMY_FORCESCRIPT           100
#define CMD_COMPAREOWNERWITHEX          101
#define CMD_HUMAN_SETPROPERTY           102
#define CMD_HUMAN_GETPROPERTY           103
#define CMD_DISABLECOLLS                104
#define CMD_HUMAN_GETOWNER              105
#define CMD_HUMAN_FORCE_SETTOCAR        106
#define CMD_CAR_SETDESTROYMOTOR         107
#define CMD_ACTOR_SETDIR                108
#define CMD_ACTOR_SETPOS                109
#define CMD_ACTOR_SETPLACEMENT          110
#define CMD_GARAGE_ENABLESTEAL          111
#define CMD_SETTIMEOUTEVENT             112
#define CMD_EMITPARTICLE                113
#define CMD_STOPPARTICLE                114
#define CMD_SETPLAYERFIREEVENT          115
#define CMD_FINDNEARACTOR               116
#define CMD_SETPLAYERHORNEVENT          117
#define CMD_HUMAN_HOLSTER               118
#define CMD_CAMERA_LOCK                 119
#define CMD_CAMERA_UNLOCK               120
#define CMD_SETMISSIONNAMEID            121
#define CMD_WINGMAN_SETINDICATOR        122
#define CMD_WINGMAN_DELINDICATOR        123
#define CMD_MODEL_PLAYANIM              124
#define CMD_MODEL_STOPANIM              125
#define CMD_MODEL_CREATE                126
#define CMD_MODEL_DESTROY               127
#define CMD_HUMAN_CHANGEMODEL           128
#define CMD_AUTOSAVEGAME                129
#define CMD_AUTOSAVEGAMEFULL            130
#define CMD_HUMAN_DEATH                 131
#define CMD_HUMAN_TALK                  132
#define CMD_HUMAN_ERASEAB               133
#define CMD_HUMAN_CREATEAB              134
#define CMD_POLICEMANAGER_ON            135
#define CMD_POLICEMANAGER_ADD           136
#define CMD_POLICEMANAGER_DEL           137
#define CMD_POLICEMANAGER_SETSPEED      138
#define CMD_POLICEMANAGER_FORCEARREST   139
#define CMD_CAR_ENABLEUS                140
#define CMD_CAR_SETDOOROPEN             141
#define CMD_HUMAN_SETFIRETARGET         142
#define CMD_ISCARUSABLE                 143
#define CMD_DOOR_SETOPENPOSE            144
#define CMD_AIRPLANE_ISDESTROYED        145
#define CMD_HUMAN_RETURNFROMPANIC       146
#define CMD_CAR_GETSPEED                147
#define CMD_CAR_SETSPEED                148
#define CMD_CAR_FORCESTOP               149
#define CMD_CAR_BREAKMOTOR              150
#define CMD_INVENTORY_PUSH              151
#define CMD_INVENTORY_POP               152
#define CMD_FUCKINGBOX_ADD              153
#define CMD_FUCKINGBOX_RECOMPILE        154
#define CMD_FUCKINGBOX_ADD_DEST         155
#define CMD_HUMAN_THROWITEM             156
#define CMD_FUCKINGBOX_GETNUMBOX        157
#define CMD_FUCKINGBOX_GETNUMDEST       158
#define CMD_FUCKINGBOX_MOVE             159
#define CMD_WAGON_SETEVENT              160
#define CMD_WAGON_GETLASTNODE           161
#define CMD_CAR_SETSPECCOLL             162
#define CMD_CAR_GETSPECCOLL             163
#define CMD_MISSION_OBJECTIVES          164
#define CMD_MISSION_OBJECTIVESREMOVE    165
#define CMD_MISSION_OBJECTIVESCLEAR     166
#define CMD_RACING_MISSION6_INIT        167
#define CMD_RACING_MISSION6_START       168
#define CMD_CAR_EXPLOSION               169
#define CMD_HUMAN_FROMCAR               170
#define CMD_BREAK                       171
#define CMD_CAR_REMOVE_DRIVER           172
#define CMD_SETMODELTOCAR               173
#define CMD_GETENEMYAISTATE             174
#define CMD_GET_PM_STATE                175
#define CMD_GET_PM_CRASHTIME            176
#define CMD_GET_PM_FIRETIME             177
#define CMD_GET_PM_NUMPREDATORS         178
#define CMD_GET_PM_HUMANSTATE           179
#define CMD_USE_LIGHTCACHE              180
#define CMD_STREAM_CREATE               181
#define CMD_STREAM_CONNECT              182
#define CMD_STREAM_PLAY                 183
#define CMD_STREAM_PAUSE                184
#define CMD_STREAM_STOP                 185
#define CMD_STREAM_DESTROY              186
#define CMD_STREAM_FADEVOL              187
#define CMD_STREAM_SETLOOP              188
#define CMD_STREAM_SETPOS               189
#define CMD_STREAM_GETPOS               190
#define CMD_SETTANKHITCOUNT             191
#define CMD_SETNOANIMHIT                192
#define CMD_CARLIGHT_MAIN               193
#define CMD_CARLIGHT_LIGHT              194
#define CMD_CARLIGHT_INDIC_L            195
#define CMD_CARLIGHT_INDIC_R            196
#define CMD_CARLIGHT_INDIC_OFF          197
#define CMD_CAR_MUSTSTEAL               198
#define CMD_CAR_SETPROJECTOR            199
#define CMD_GARAGE_CANSTEAL             200
#define CMD_GARAGE_ADDLASTSTOLEN        201
#define CMD_GARAGE_SHOW                 202
#define CMD_CAR_INWATER                 203
#define CMD_HUMAN_FORCEFALL             204
#define CMD_CREATE_PHYSICALOBJECT       205
#define CMD_DESTROY_PHYSICALOBJECT      206
#define CMD_WEATHER_RESET               207
#define CMD_WEATHER_SETPARAM            208
#define CMD_GETTICKTIME                 209
#define CMD_GETGAMETIME                 210
#define CMD_GETACTIVEPLAYER             211
#define CMD_GETANGLEACTORTOACTOR        212
#define CMD_SETCITYTRAFFICVISIBLE       213
#define CMD_SHOWCARDAMAGE               214
#define CMD_CHARACTER_PUSH              215
#define CMD_CHARACTER_POP               216
#define CMD_CONSOLE_ADDTEXT             217
#define CMD_CAMERA_SETRANGE             218
#define CMD_ACTOR_DUPLICATE             219
#define CMD_ACTOR_DELETE                220
#define CMD_HUMAN_RESET                 221
#define CMD_HUMAN_HAVEBOX               222
#define CMD_HUMAN_HAVEBODY              223
#define CMD_HUMAN_STOPTALK              224
#define CMD_ENEMY_BLOCK                 225
#define CMD_ENEMY_UNBLOCK               226
#define CMD_GET_REMOTE_FLOAT            227
#define CMD_GET_REMOTE_FRAME            228
#define CMD_GET_REMOTE_ACTOR            229
#define CMD_SET_REMOTE_FLOAT            230
#define CMD_SET_REMOTE_FRAME            231
#define CMD_SET_REMOTE_ACTOR            232
#define CMD_GETACTORFRAME               233
#define CMD_ZATMYSE                     234
#define CMD_CARDAMAGEVISIBLE            235
#define CMD_DEBUG_ME                    236
#define CMD_DEBUG_STOP                  237
#define CMD_IFPLAYERSTEALCAR            238
#define CMD_INTRO_SUBTITLE_ADD          239
#define CMD_SUBTITLE_ADD                240
#define CMD_MATH_COS                    241
#define CMD_MATH_SIN                    242
#define CMD_MATH_ACOS                   243
#define CMD_MATH_ASIN                   244
#define CMD_MATH_ATAN                   245
#define CMD_MATH_ABS                    246
#define CMD_FRM_GETDIR                  247
#define CMD_FRM_SETDIR                  248
#define CMD_FRM_GETPOS                  249
#define CMD_FRM_SETPOS                  250
#define CMD_FRM_GETSCALE                251
#define CMD_FRM_SETSCALE                252
#define CMD_FRM_GETROT                  253
#define CMD_FRM_SETROT                  254
#define CMD_FRM_GETWORLDDIR             255
#define CMD_FRM_GETWORLDPOS             256
#define CMD_FRM_GETWORLDSCALE           257
#define CMD_FRM_GETWORLDROT             258
#define CMD_FRM_GETLOCALMATRIX          259
#define CMD_FRM_GETWORLDMATRIX          260
#define CMD_VECT_SET                    261
#define CMD_VECT_COPY                   262
#define CMD_VECT_MUL_SCL                263
#define CMD_VECT_MUL_VECT               264
#define CMD_VECT_MUL_QUAT               265
#define CMD_VECT_MUL_MATRIX             266
#define CMD_VECT_ADD_VECT               267
#define CMD_VECT_SUB_VECT               268
#define CMD_VECT_CROSS                  269
#define CMD_VECT_NORMALIZE              270
#define CMD_VECT_MAGNITUDE              271
#define CMD_VECT_INVERSE                272
#define CMD_VECT_ANGLETO                273
#define CMD_VECT_ANGLETO_2D             274
#define CMD_VECT_ROTATE_2D              275
#define CMD_MATRIX_ZERO                 276
#define CMD_MATRIX_IDENTITY             277
#define CMD_MATRIX_COPY                 278
#define CMD_MATRIX_INVERSE              279
#define CMD_MATRIX_MUL                  280
#define CMD_QUAT_COPY                   281
#define CMD_QUAT_MAKE                   282
#define CMD_QUAT_SETDIR                 283
#define CMD_QUAT_EXTRACT                284
#define CMD_QUAT_INVERSE                285
#define CMD_QUAT_GETROTMATRIX           286
#define CMD_QUAT_ADD                    287
#define CMD_QUAT_SUB                    288
#define CMD_QUAT_MUL_SCL                289
#define CMD_QUAT_MUL_QUAT               290
#define CMD_QUAT_DOT                    291
#define CMD_QUAT_NORMALIZE              292
#define CMD_QUAT_ROTBYMATRIX            293
#define CMD_QUAT_SLERP                  294
#define CMD_COLL_TESTLINE               295
#define CMD_VLVP                        296
#define CMD_CTRL_READ                   297
#define CMD_CTRL_READEX                 298
#define CMD_GETFRAMEFROMACTOR           299
#define CMD_ACTORUPDATEPLACEMENT        300
#define CMD_RACING_AUTOINVISIBLE        301
#define CMD_SETPLAYERWFALLEVENT         302
#define CMD_CAR_INVISIBLE               303
#define CMD_CAR_SWITCHSHOWENERGY        304
#define CMD_DEBUG_GETFRAMEINFO          305
#define CMD_POLICEITCHFORPLAYER         306
#define CMD_NOANIMPRELOAD               307
#define CMD_COMPAREACTORS               308
#define CMD_SETNULLACTOR                309
#define CMD_COMPAREFRAMES               310
#define CMD_SETNULLFRAME                311
#define CMD_SETFREERIDE                 312
#define CMD_ENABLEMAP                   313
#define CMD_FRAMEISTELEPHONE            314
#define CMD_CAR_CALM                    315
#define CMD_BRIDGE_SHUTDOWN             316
#define CMD_SETTRAFFSECTORSND           317
#define CMD_CITYMUSIC_OFF               318
#define CMD_CITYMUSIC_ON                319
#define CMD_DAN_DEBUG                   320
#define CMD_FRM_GETPARENT               321
#define CMD_ISPOINTINSECTOR             322
#define CMD_ISPOINTINAREA               323
#define CMD_GETACTIVECAMERA             324
#define CMD_SOUND_GETVOLUME             325
#define CMD_SOUND_SETVOLUME             326
#define CMD_RACING_CHANGE_MODEL         327
#define CMD_CITYCACHING_TICK            328
#define CMD_ACTORLIGHTNESSON            329
#define CMD_CAR_LOCK                    330
#define CMD_CAR_LOCK_ALL                331
#define CMD_PM_SHOWSYMBOL               332
#define CMD_PM_SHOWPROGRESS             333
#define CMD_PM_SETPROGRESS              334
#define CMD_SETFILMMUSIC                335
#define CMD_GETFILMMUSIC                336
#define CMD_HUMAN_GETSEATIDX            337
#define CMD_HUMAN_ENTERTOTRUCK          338
#define CMD_FLOATREG_PUSH               339
#define CMD_FLOATREG_POP                340
#define CMD_AIRPLANESHOWDAMAGE          341
#define CMD_SETNPCKILLEVENT             342
#define CMD_CAR_GETSEATCOUNT            343
#define CMD_FRM_SETALPHA                344
#define CMD_HUMAN_THROWGRENADE          345
#define CMD_DETECTOR_CREATEDYNCOLL      346
#define CMD_DETECTOR_ERASEDYNCOLL       347
#define CMD_CAR_UNBREAKABLE             348
#define CMD_PHOBJ_IMPULS                349
#define CMD_WEATHER_PREPAREBUFFER       350
#define CMD_SUBTITLE                    351
#define CMD_HUMAN_CANDIE                352
#define CMD_VERSION_IS_GERMANY          353
#define CMD_VERSION_IS_EDITOR           354
#define CMD_GARAGE_NEZAHAZUJ            355
#define CMD_POLICE_SUPPORT              356
#define CMD_PLAY_AVI_INTRO              357
#define CMD_CAMERA_SETFOV               358
#define CMD_CAMERA_GETFOV               359
#define CMD_DOOR_GETSTATE               360
#define CMD_CAR_GETMAXLEVELS            361
#define CMD_CAR_GETACTLEVEL             362
#define CMD_CAR_SETACTLEVEL             363
#define CMD_FREERIDE_SCOREON            364
#define CMD_FREERIDE_SCORESET           365
#define CMD_FREERIDE_SCOREGET           366
#define CMD_FREERIDE_SCOREADD           367
#define CMD_TAXIDRIVER_ENABLE           368
#define CMD_HUMAN_RETURNTOTRAFF         369
#define CMD_TIMER_GETINTERVAL           370
#define CMD_TIMER_SETINTERVAL           371
#define CMD_FRM_GETNUMCHILDREN          372
#define CMD_FRM_GETCHILD                373
#define CMD_HUMAN_WAITTOREADY           374
#define CMD_ACTOR_ADDDORICI             375
#define CMD_CAR_REPAIR                  376
#define CMD_POLICE_SPEED_FACTOR         377
#define CMD_CARTRIDGE_INVALIDATE        378
#define CMD_NPC_SHUTUP                  379
#define CMD_FREERIDE_ENABLECAR          380
#define CMD_GETCARLINENUMFROMTABLE      381
#define CMD_ENEMY_GETHOSTILESTATE       382
#define CMD_HUMAN_SHUTUP                383
#define CMD_PUMPER_CANWORK              384
#define CMD_GROUP_DISBAND               385
#define CMD_INVENTORY_CLEAR             386
#define CMD_CAR_DISABLE_UO              387
#define CMD_GETLASTSAVENUM              388
#define CMD_FREERIDE_TRAFFSETUP         389
#define CMD_POCZKURVENYCHBEDENCAR       390
#define CMD_HUMAN_GETACTANIMID          391
#define CMD_CAR_SETAXIS                 392
#define CMD_M7_ZASTAVZKURVENEPOLISE     393
#define CMD_ISVALIDTAXIPASSENGER        394
#define CMD_GRABER_SETFOCUS             395
#define CMD_DAN_INTERNAL_1              396
#define CMD_DAN_INTERNAL_2              397
// How many there are
#define NUM_COMMANDS 398

// A script (PS2 C_program, C_program.cpp), 0x68 bytes. PS2's has the same members at offsets 4 lower (its vtable is
// last), and 4 more lower after each vector (PS2's are 12 bytes). Only what InitSystem, CloseSystem, C_mission::OpenBin
// and the mission's and the game's Init and Done use so far.
// VTABLE: GAME 0x624858
class C_program
{
public:
    // A compiled command (PS2 C_program::S_instruction, 8 bytes; the members' names are ours): its number, which
    // DecodeInstruction finds by its name, and its parameters, which DecodeParams allocates for the command
    struct S_instruction
    {
        int command;                    // 0x0, CMD_*
        void *params;                   // 0x4
    };

    // The script's own commands while it runs another actor's script's (the names are ours): one of Process's commands
    // saves them here with the command to go on at, and copies the other script's commands, whose parameters stay that
    // script's. 0x18 bytes, PS2's 0x14.
    struct S_saved_code
    {
        std::vector<S_instruction> instructions;  // 0x0
        int return_command;             // 0x10
        S_saved_code *next;             // 0x14, the code saved before it
    };

    // What the derived classes' CallBack hears (PS2's type; its values aren't known yet)
    enum E_cb_action
    {
    };

    // A script replaced by the patch (PS2 C_program::S_prog_change, 16 bytes; the members' names are ours, from PS2's
    // TryChangeProgramData, which compares the first three with the mission's name, the script's and its frame's, and
    // gives the script the source of the first match)
    struct S_prog_change
    {
        char *mission;                  // 0x0
        char *program;                  // 0x4
        char *frame;                    // 0x8
        char *source;                   // 0xc
    };

    // What the patch's files replace, which LoadPatchFile fills (PS2 name; nothing fills PS2's)
    static std::vector<S_prog_change> change_progs;

    C_program();
    // The vtable's ten functions in PS2's order (0x624858): C_mission::Close deletes its scripts through the destructor
    // (PS2's too), C_detector and the AI's humans override the save functions, the compiler's two and Process
    virtual ~C_program();
    // Writes the script into its chunk of the mission's scene2.bin (PS2 name)
    virtual bool SaveSourceCode(C_chunk *chunk);
    // A saved game's part (PS2 names; the same as C_actor's, which C_detector's override both)
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    // Reads the parameters of a command (PS2 name): false if they don't parse
    virtual bool DecodeParams(char *params, S_instruction &ins);
    // The command a line starts with (PS2 name), CMD_*, and the length of its name; -1 for none
    virtual int DecodeInstruction(char *line, int &len);
    // Runs the current command for the time passed (PS2 name), the interpreter
    virtual void Process(uint time);
    // Empty, as PS2's (PS2 name)
    virtual void DecompileCommand(int, char *);
    // Empty, as PS2's (PS2 name): a folded copy of the in-class kind
    virtual void CallBack(E_cb_action) {}

    // Reads the script from its chunk of the mission's scene2.bin (PS2 name)
    bool LoadSourceCode(C_chunk *chunk);
    // Gives the script the source of the last of change_progs that names its mission, its name and its actor's frame
    // (PS2 name): a change without a name or a frame fits only a script without one
    void TryChangeProgramData();
    // Replaces the source with a copy of src, NULL for none (PS2 name)
    void SetSourceCode(char *src);
    // Compiles the source anew, at a mission's start (PS2 name)
    void Init();
    // Frees the commands and the registers, at the mission's end (PS2 name)
    void Done();
    // Compiles the lines of src into the commands (PS2 name), one only if one_line; returns the number of lines that
    // didn't compile, the last one's offset and length in err_pos and err_len (if both are given)
    int Compile(char *src, bool one_line, int *err_pos, int *err_len);
    // Runs the script from its start, one command after another, until one stops it (PS2 name)
    void CallSubroutine();
    // Runs the current command, and while run_through stays set the next ones (PS2 name)
    void CallProcess(uint time);

    char *name;                         // 0x4, what S_prog_change::program names (the names are ours)
    char *source;                       // 0x8
    std::vector<S_instruction> instructions;  // 0xc
    int cur_command;                    // 0x1c, the index of the command that runs
    int prev_command;                   // 0x20, -1 at first
    uint command_time;                  // 0x24, how long the command has run (Process adds the time)
    // SetEvent's, which jumps to an event's commands and back: the time the command had run and the game's time then
    uint event_command_time;            // 0x28
    uint event_time;                    // 0x2c
    std::vector<ushort> gosub_stack;    // 0x30 (PS2's vector<unsigned short>), the commands to return to
    S_saved_code *saved_code;           // 0x40
    // Whose frame S_prog_change::frame names (the name is ours, from TryChangeProgramData, which reads the actor's frame)
    C_actor *actor;                     // 0x44
    // The registers, each an array and its size, as PS2's GetFloatReg, GetFrameReg and GetActorReg read them (the
    // names are ours)
    float *float_regs;                  // 0x48
    int num_float_regs;                 // 0x4c
    I3D_frame **frame_regs;             // 0x50
    int num_frame_regs;                 // 0x54
    C_actor **actor_regs;               // 0x58
    int num_actor_regs;                 // 0x5c
    float unknown_float_60;             // 0x60, 0.5 at first, 0 for a script without commands
    // Set while CallProcess runs the commands one after another (CallSubroutine sets it, CMD_END clears it)
    bool run_through;                   // 0x64
    // Set where RETURN ends CallProcess's run too (Process's)
    bool stop_at_return;                // 0x65
    // Set by a command block's {, whose parameter gives the mode, cleared by its } (Process's, LeaveCompactBlock's)
    bool in_block;                      // 0x66
    uchar block_mode;                   // 0x67
};

// Loads one of the patch's script files, patch\a1.bin to a4.bin, into C_program::change_progs (PC only, the name is
// ours)
void LoadPatchFile(const char *name);
// Frees what LoadPatchFile loaded, at CloseSystem (PC only, the name is ours)
void UnloadPatchFiles();

// The changes' push_back calls insert out of line, and insert its helpers; the linker folded _Ucopy, _Construct and
// _Ufill with those of every other 16-byte type, whose earlier files' copies the original kept
// TEMPLATE: GAME 0x47be20
// std::vector<C_program::S_prog_change,std::allocator<C_program::S_prog_change> >::size

// TEMPLATE: GAME 0x47be40
// std::vector<C_program::S_prog_change,std::allocator<C_program::S_prog_change> >::insert

// TEMPLATE: GAME 0x498f60
// std::vector<C_program::S_prog_change,std::allocator<C_program::S_prog_change> >::_Ucopy

// TEMPLATE: GAME 0x5bca00
// ?_Construct@std@@YIXPAUS_prog_change@C_program@@ABU23@@Z

// TEMPLATE: GAME 0x5f4af0
// std::vector<C_program::S_prog_change,std::allocator<C_program::S_prog_change> >::_Ufill
