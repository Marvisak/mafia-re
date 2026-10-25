#pragma once

#define WIN32_LEAN_AND_MEAN
// The DirectInput 7 interfaces: the JoyBase constructor passes 0x700 to DirectInputCreateEx
#define DIRECTINPUT_VERSION 0x0700

#include <windows.h>
#include <dinput.h>
#include <types.h>

// No build has symbols for IJoy: the PS2 and Xbox builds don't use DirectInput. All names in this file are guesses,
// the interfaces' from the DLL's strings ("JoyBase", "Joy", "Effect") and the export, the methods' from what they do.
// The methods are declared in vtable order.

class IJoy;
class IJoyEffect;

// What GetDeviceInfo and IJoy::GetInfo return
struct S_joy_device_info {
    const char *name;           // DIDEVICEINSTANCE::tszInstanceName
    const char *product;        // DIDEVICEINSTANCE::tszProductName
    int num_axes;
    int num_buttons;
    int num_povs;
    bool force_feedback;
    int type;                   // GET_DIDEVICE_SUBTYPE (DIDEVTYPEJOYSTICK_*), 0 if unknown
};

// What GetAxisInfo returns
struct S_joy_axis_info {
    const char *name;
    bool ff_actuator;           // DIDOI_FFACTUATOR
    uint ff_force_resolution;
    uint ff_max_force;
    uint aspect;                // JOY_ASPECT_*, or the DIDOI_ASPECT* bits if none of them
};

enum {
    JOY_ASPECT_ACCELERATION = 1,
    JOY_ASPECT_FORCE = 2,
    JOY_ASPECT_POSITION = 3,
    JOY_ASPECT_VELOCITY = 4,
};

// The axes IJoy's methods take, in DIJOYSTATE2's order, JOY_SLIDER1 the last
enum {
    JOY_X,
    JOY_Y,
    JOY_Z,
    JOY_RX,
    JOY_RY,
    JOY_RZ,
    JOY_SLIDER0,
    JOY_SLIDER1,
    JOY_NUM_AXES,
};

// The state IJoy::Update computes. The axes are scaled to -1..1 (0..1 with JOY_AXIS_POSITIVE), the POVs are angles
// in radians.
struct S_joy_state {
    float x, y, z;
    float rx, ry, rz;
    float slider[2];
    float pov[4];
    bool pov_centered[4];
    uint buttons[4];            // one bit per button, 32 in each
};

// IJoy::SetAxisMode
enum {
    JOY_AXIS_RELATIVE = 1,      // the change since the last update
    JOY_AXIS_INVERT = 2,
    JOY_AXIS_POSITIVE = 4,      // 0..1 instead of -1..1
    JOY_AXIS_RAW = 8,           // DIPROPCALIBRATIONMODE_RAW, not scaled
    JOY_AXIS_MODES = 0xf,       // all of them: SetAxisMode refuses other bits, GetAxisMode returns these
};

// IJoy::Update
enum {
    JOY_UPDATE_RESET = 1,       // a device that can't be read reports nothing pressed
};

// IJoy::GetFFState
enum {
    JOY_FF_ACTUATORS_OFF = 0x1,
    JOY_FF_ACTUATORS_ON = 0x2,
    JOY_FF_DEVICE_LOST = 0x4,
    JOY_FF_EMPTY = 0x8,
    JOY_FF_PAUSED = 0x10,
    JOY_FF_POWER_OFF = 0x20,
    JOY_FF_POWER_ON = 0x40,
    JOY_FF_SAFETY_SWITCH_OFF = 0x80,
    JOY_FF_SAFETY_SWITCH_ON = 0x100,
    JOY_FF_STOPPED = 0x200,
    JOY_FF_USER_SWITCH_OFF = 0x400,
    JOY_FF_USER_SWITCH_ON = 0x800,
};

// IJoy::SendFFCommand, in alphabetical order like the DISFFC_* flags they stand for
enum {
    JOY_FF_CONTINUE = 1,
    JOY_FF_PAUSE = 2,
    JOY_FF_RESET = 3,
    JOY_FF_SET_ACTUATORS_OFF = 4,
    JOY_FF_SET_ACTUATORS_ON = 5,
    JOY_FF_STOP_ALL = 6,
};

// The effect types, in the order of the effect table IJoy fills from EnumEffects
enum {
    JOY_EFFECT_CONSTANT_FORCE,
    JOY_EFFECT_RAMP_FORCE,
    JOY_EFFECT_SQUARE,
    JOY_EFFECT_SINE,
    JOY_EFFECT_TRIANGLE,
    JOY_EFFECT_SAWTOOTH_UP,
    JOY_EFFECT_SAWTOOTH_DOWN,
    JOY_EFFECT_SPRING,
    JOY_EFFECT_DAMPER,
    JOY_EFFECT_INERTIA,
    JOY_EFFECT_FRICTION,
    JOY_EFFECT_FILE,            // the effects of a .ffe file
    JOY_EFFECT_NONE,
    JOY_NUM_EFFECT_TYPES = JOY_EFFECT_FILE,
};

// What IJoyEffect::GetEffectCaps returns
struct S_joy_effect_caps {
    bool supported;
    bool dead_band;             // DIEFT_DEADBAND
    bool saturation;            // DIEFT_SATURATION
    bool pos_neg_saturation;    // DIEFT_POSNEGSATURATION
    bool pos_neg_coefficients;  // DIEFT_POSNEGCOEFFICIENTS
    bool static_direction;      // DIEFFECTINFO::dwStaticParams
    bool static_duration;
    bool static_envelope;
    bool static_gain;
    bool static_type_specific;
    bool dynamic_direction;     // DIEFFECTINFO::dwDynamicParams
    bool dynamic_duration;
    bool dynamic_envelope;
    bool dynamic_gain;
    bool dynamic_type_specific;
};

// IJoyEffect::SetCondition, the fields of DICONDITION scaled to -1..1 (the saturations and the dead band to 0..1)
struct S_joy_condition {
    float offset;
    float positive_coefficient;
    float negative_coefficient;
    float positive_saturation;
    float negative_saturation;
    float dead_band;
};

// IJoyEffect::GetStatus
enum {
    JOY_EFFECT_PLAYING = 1,
    JOY_EFFECT_EMULATED = 2,
};

// IJoyBase::CreateJoy's flags
enum {
    JOY_BACKGROUND = 1,         // DISCL_BACKGROUND: the device is read while the window is inactive too
};

// VTABLE: IJOY 0x1000d0d0
class IJoyBase {
public:
    IJoyBase();
    virtual ~IJoyBase();
    virtual uint AddRef() = 0;
    virtual uint Release() = 0;
    virtual const char *GetAxisName(int device, int axis) = 0;
    virtual bool GetAxisInfo(int device, int axis, S_joy_axis_info *info) = 0;
    virtual const char *GetButtonName(int device, int button) = 0;
    virtual int NumDevices() = 0;
    virtual IDirectInput7A *GetDirectInput() = 0;
    virtual bool GetDeviceInfo(S_joy_device_info *info, int device) = 0;
    virtual const char *GetPOVName(int device, int pov) = 0;
    virtual bool RunControlPanel() = 0;
    virtual bool CreateJoy(IJoy **joy, int device, uint flags) = 0;
};

// VTABLE: IJOY 0x1000d1c0
class IJoy {
public:
    // FUNCTION: IJOY 0x100023a0
    virtual ~IJoy() {}
    virtual uint AddRef() = 0;
    virtual uint Release() = 0;
    virtual float GetDeadZone(int axis) = 0;
    virtual bool SetDeadZone(int axis, float dead_zone) = 0;
    virtual float GetDIDeadZone(int axis) = 0;
    virtual bool SetDIDeadZone(int axis, float dead_zone) = 0;
    virtual float GetSaturation(int axis) = 0;
    virtual bool SetSaturation(int axis, float saturation) = 0;
    virtual float GetDISaturation(int axis) = 0;
    virtual bool SetDISaturation(int axis, float saturation) = 0;
    virtual uint GetAxisMode(int axis) = 0;
    virtual bool SetAxisMode(int axis, uint mode) = 0;
    virtual float GetAxis(int axis) = 0;
    virtual bool IsButtonDown(int button) = 0;
    virtual bool IsButtonPressed(int button) = 0;
    virtual uint GetButtons(int group) = 0;
    virtual uint GetButtonsPressed(int group) = 0;
    virtual bool GetPOV(float *angle, int pov) = 0;
    virtual bool GetPOVChanged(float *angle, int pov) = 0;
    virtual bool GetState(S_joy_state *state) = 0;
    virtual bool GetStateChanges(S_joy_state *state) = 0;
    virtual bool Update(uint flags) = 0;
    virtual const char *GetAxisName(int axis) = 0;
    virtual bool GetAxisInfo(int axis, S_joy_axis_info *info) = 0;
    virtual const char *GetButtonName(int button) = 0;
    virtual bool GetInfo(S_joy_device_info *info) = 0;
    virtual const char *GetPOVName(int pov) = 0;
    virtual bool RunControlPanel() = 0;
    virtual bool SetAutoCenter(bool on) = 0;
    virtual float GetGain() = 0;
    virtual bool SetGain(float gain) = 0;
    virtual bool GetFFState(uint *state) = 0;
    virtual bool SendFFCommand(int command) = 0;
    virtual bool CreateEffect(IJoyEffect **effect) = 0;
};

// VTABLE: IJOY 0x1000d2d4
class IJoyEffect {
public:
    // FUNCTION: IJOY 0x100045a0
    virtual ~IJoyEffect() {}
    virtual uint AddRef() = 0;
    virtual uint Release() = 0;
    virtual bool Start(uint iterations, bool solo) = 0;
    virtual bool Stop(bool unload) = 0;
    virtual bool SetType(int type, float direction, uint duration, float gain, const char *file_name,
        const void *data) = 0;
    virtual bool SetAxes(int num_axes, const uchar *axes) = 0;
    virtual bool SetGain(float gain, bool update) = 0;
    virtual bool ApplyGain() = 0;
    virtual bool SetDirection(float direction, bool update) = 0;
    virtual bool ApplyDirection() = 0;
    virtual bool SetDuration(uint duration, bool update) = 0;
    virtual bool ApplyDuration() = 0;
    virtual bool SetEnvelope(float attack_level, uint attack_time, float fade_level, uint fade_time,
        bool update) = 0;
    virtual bool ApplyEnvelope() = 0;
    virtual bool SetConstantForce(float magnitude, bool update) = 0;
    virtual bool SetRampForce(float start, float end, bool update) = 0;
    virtual bool SetPeriodic(float magnitude, float offset, float invert, uint period, bool update) = 0;
    virtual bool SetCondition(const S_joy_condition *condition_0, const S_joy_condition *condition_1,
        bool update) = 0;
    virtual bool ApplyTypeSpecific() = 0;
    virtual bool GetEffectCaps(S_joy_effect_caps *caps, int type) = 0;
    virtual int GetType() = 0;
    virtual bool GetStatus(uint *status) = 0;
};

// CreateJoyBase's flags
enum {
    JOY_BASE_LEAK_REPORT = 1,   // report the objects not released when the DLL is unloaded
};

// hinstance and hwnd must not be NULL. direct_input is a DirectInput object to share (it gets an AddRef), or NULL to
// have the JoyBase create its own. flags are JOY_BASE_*.
extern "C" bool EXPORT CreateJoyBase(IJoyBase **joy_base, HINSTANCE hinstance, HWND hwnd, IDirectInput7A *direct_input,
    uint flags);
