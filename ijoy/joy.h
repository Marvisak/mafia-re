#pragma once

#include "ijoy.h"

// The implementation of the interfaces in ijoy.h. Like those, every name here is a guess. Only the names of
// uninitialized data show in the bytes, through VC6's .bss order (SetCondition's conditions).

// From the DirectX 8.1 SDK: dinput.lib's data format and dxguid.lib's GUIDs. The axis GUIDs are only referenced by
// the data format's objects.
// GLOBAL: IJOY 0x1000d33c
// c_dfDIJoystick2
// GLOBAL: IJOY 0x1000d354
// GUID_Friction
// GLOBAL: IJOY 0x1000d364
// GUID_Inertia
// GLOBAL: IJOY 0x1000d374
// GUID_Damper
// GLOBAL: IJOY 0x1000d384
// GUID_Spring
// GLOBAL: IJOY 0x1000d394
// GUID_SawtoothDown
// GLOBAL: IJOY 0x1000d3a4
// GUID_SawtoothUp
// GLOBAL: IJOY 0x1000d3b4
// GUID_Triangle
// GLOBAL: IJOY 0x1000d3c4
// GUID_Sine
// GLOBAL: IJOY 0x1000d3d4
// GUID_Square
// GLOBAL: IJOY 0x1000d3e4
// GUID_RampForce
// GLOBAL: IJOY 0x1000d3f4
// GUID_ConstantForce
// GLOBAL: IJOY 0x1000d404
// GUID_POV
// GLOBAL: IJOY 0x1000d414
// GUID_Button
// GLOBAL: IJOY 0x1000d424
// GUID_Slider
// GLOBAL: IJOY 0x1000d434
// GUID_RzAxis
// GLOBAL: IJOY 0x1000d444
// GUID_RyAxis
// GLOBAL: IJOY 0x1000d454
// GUID_RxAxis
// GLOBAL: IJOY 0x1000d464
// GUID_ZAxis
// GLOBAL: IJOY 0x1000d474
// GUID_YAxis
// GLOBAL: IJOY 0x1000d484
// GUID_XAxis
// GLOBAL: IJOY 0x1000d494
// IID_IDirectInputDevice7A
// GLOBAL: IJOY 0x1000d4a4
// IID_IDirectInput7A

// An object (axis, button, POV...) of a device, from EnumObjects
struct S_object {
    DIDEVICEOBJECTINSTANCEA *instance;
    uint unused_1;
    uint unused_2;
    S_object *next;

    S_object()
    {
        memset(this, 0, sizeof(S_object));
    }
};

// An attached joystick, from EnumDevices
struct S_device {
    DIDEVICEINSTANCEA *instance;
    IDirectInputDevice7A *device;   // only while the objects are enumerated
    DIDEVCAPS caps;
    S_object *objects;
    S_device *next;
    bool ok;                        // set by EnumObjectsCallback
};

// What EnumDevices passes to its callback
struct S_enum_context {
    IDirectInput7A *direct_input;
    HWND hwnd;
    S_device *devices;
    bool ok;
};

class C_joy;
class C_joy_effect;

// The joysticks a JoyBase created, one per device
struct S_joy_node {
    int device;
    C_joy *joy;
    S_joy_node *next;
};

// VTABLE: IJOY 0x1000d100
class C_joy_base : public IJoyBase {
public:
    uint ref;
    S_enum_context enum_context;
    HINSTANCE hinstance;
    IDirectInput7A *direct_input;
    HWND hwnd;
    int num_devices;
    S_joy_node *joys;

    C_joy_base(HINSTANCE hinstance, HWND hwnd, IDirectInput7A *direct_input, bool *ok);
    ~C_joy_base();
    void Close();
    bool EnumDevices();
    void RemoveJoy(C_joy *joy);

    uint AddRef();
    uint Release();
    const char *GetAxisName(int device, int axis);
    bool GetAxisInfo(int device, int axis, S_joy_axis_info *info);
    const char *GetButtonName(int device, int button);
    int NumDevices();
    IDirectInput7A *GetDirectInput();
    bool GetDeviceInfo(S_joy_device_info *info, int device);
    const char *GetPOVName(int device, int pov);
    bool RunControlPanel();
    bool CreateJoy(IJoy **joy, int device, uint flags);
};

// SYNTHETIC: IJOY 0x10001130
// C_joy_base::`scalar deleting destructor'

// SYNTHETIC: IJOY 0x10001780
// IJoyBase::`scalar deleting destructor'

// A DirectInput effect type, from EnumEffects
struct S_effect_info {
    DIEFFECTINFOA info;
    bool supported;
};

// VTABLE: IJOY 0x1000d134
class C_joy : public IJoy {
public:
    uint ref;
    C_joy_base *joy_base;
    S_device *device_info;
    S_enum_context *enum_context;
    IDirectInput7A *direct_input;
    IDirectInputDevice7A *device;
    HWND hwnd;
    DIJOYSTATE2 device_state;
    S_joy_state state;
    S_joy_state last_state;         // before JOY_AXIS_RELATIVE
    bool acquired;
    bool force_feedback;
    S_effect_info effects[JOY_NUM_EFFECT_TYPES];
    int num_effects;
    uint buttons_changed[4];
    bool pov_changed[4];
    float dead_zone[JOY_NUM_AXES];
    float saturation[JOY_NUM_AXES];
    uint axis_mode[JOY_NUM_AXES];
    float unused[JOY_NUM_AXES];     // only initialized
    float half_range[JOY_NUM_AXES]; // half the device's range of an axis that can't be set to -10000..10000
    float range_offset[JOY_NUM_AXES];

    C_joy(C_joy_base *base, S_device *info, S_enum_context *context, uint flags, bool *ok);
    ~C_joy();
    bool Acquire(bool acquire);

    uint AddRef();
    uint Release();
    float GetDeadZone(int axis);
    bool SetDeadZone(int axis, float dead_zone);
    float GetDIDeadZone(int axis);
    bool SetDIDeadZone(int axis, float dead_zone);
    float GetSaturation(int axis);
    bool SetSaturation(int axis, float saturation);
    float GetDISaturation(int axis);
    bool SetDISaturation(int axis, float saturation);
    uint GetAxisMode(int axis);
    bool SetAxisMode(int axis, uint mode);
    float GetAxis(int axis);
    bool IsButtonDown(int button);
    bool IsButtonPressed(int button);
    uint GetButtons(int group);
    uint GetButtonsPressed(int group);
    bool GetPOV(float *angle, int pov);
    bool GetPOVChanged(float *angle, int pov);
    bool GetState(S_joy_state *state);
    bool GetStateChanges(S_joy_state *state);
    bool Update(uint flags);
    const char *GetAxisName(int axis);
    bool GetAxisInfo(int axis, S_joy_axis_info *info);
    const char *GetButtonName(int button);
    bool GetInfo(S_joy_device_info *info);
    const char *GetPOVName(int pov);
    bool RunControlPanel();
    bool SetAutoCenter(bool on);
    float GetGain();
    bool SetGain(float gain);
    bool GetFFState(uint *state);
    bool SendFFCommand(int command);
    bool CreateEffect(IJoyEffect **effect);
};

// SYNTHETIC: IJOY 0x100023b0
// IJoy::`scalar deleting destructor'

// SYNTHETIC: IJOY 0x100023d0
// C_joy::`scalar deleting destructor'

// An effect of a .ffe file
struct S_file_effect {
    IDirectInputEffect *effect;
    S_file_effect *next;
};

// What EnumEffectsInFile passes to its callback
struct S_file_context {
    S_file_effect *effects;
    IDirectInputDevice7A *device;
    int num_effects;
};

// VTABLE: IJOY 0x1000d278
class C_joy_effect : public IJoyEffect {
public:
    uint ref;
    C_joy *joy;
    int type;                       // JOY_EFFECT_*
    IDirectInputDevice7A *device;
    IDirectInputEffect *effect;
    S_file_effect *file_effects;
    int num_effects;
    S_effect_info *effect_info;     // C_joy::effects
    int num_effect_info;
    DIEFFECT parameters;
    DIENVELOPE envelope;
    DICONSTANTFORCE constant_force;
    DIRAMPFORCE ramp_force;
    DIPERIODIC periodic;
    DICONDITION condition[2];
    uint axes[JOY_NUM_AXES];        // set by SetAxes, for the next SetType
    DWORD effect_axes[JOY_NUM_AXES]; // parameters.rgdwAxes
    uint num_axes;
    uint num_effect_axes;
    LONG direction[JOY_NUM_AXES];

    C_joy_effect(C_joy *owner, IDirectInputDevice7A *input_device, S_effect_info *info, int num_info, bool *ok);
    ~C_joy_effect();

    uint AddRef();
    uint Release();
    bool Start(uint iterations, bool solo);
    bool Stop(bool unload);
    bool SetType(int type, float direction, uint duration, float gain, const char *file_name, const void *data);
    bool SetAxes(int num_axes, const uchar *axes);
    bool SetGain(float gain, bool update);
    bool ApplyGain();
    bool SetDirection(float direction, bool update);
    bool ApplyDirection();
    bool SetDuration(uint duration, bool update);
    bool ApplyDuration();
    bool SetEnvelope(float attack_level, uint attack_time, float fade_level, uint fade_time, bool update);
    bool ApplyEnvelope();
    bool SetConstantForce(float magnitude, bool update);
    bool SetRampForce(float start, float end, bool update);
    bool SetPeriodic(float magnitude, float offset, float invert, uint period, bool update);
    bool SetCondition(const S_joy_condition *condition_0, const S_joy_condition *condition_1, bool update);
    bool ApplyTypeSpecific();
    bool GetEffectCaps(S_joy_effect_caps *caps, int type);
    // Inline: SetType's [offset + pointer] address shows it, written out it gives [pointer + offset]
    bool IsSupported(int type) { return effect_info[type].supported; }
    int GetType();
    bool GetStatus(uint *status);
};

// SYNTHETIC: IJOY 0x100045b0
// IJoyEffect::`scalar deleting destructor'

// SYNTHETIC: IJOY 0x100045d0
// C_joy_effect::`scalar deleting destructor'

// SetCondition's static conditions. This compiler puts a function's static into the PDB as a module-level symbol
// with its plain name, where reccmp doesn't look for one, so a by-name marker matches it.
// GLOBAL: IJOY 0x1000e920
// conditions

// The classes C_leak_detector counts
enum {
    LEAK_JOY_BASE,
    LEAK_JOY,
    LEAK_EFFECT,
    LEAK_NUM_CLASSES,
};

// Counts the objects and references of each class and reports those not released when the DLL is unloaded
class C_leak_detector {
public:
    int refs[LEAK_NUM_CLASSES];
    int objects[LEAK_NUM_CLASSES];
    bool enabled;                   // CreateJoyBase's JOY_BASE_LEAK_REPORT

    C_leak_detector();
    ~C_leak_detector();
};

// SYNTHETIC: IJOY 0x10001020
// $E3

// SYNTHETIC: IJOY 0x10001030
// $E2
