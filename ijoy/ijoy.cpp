// The whole DLL is a single translation unit (the Rich header has one object from the compiler), its functions are at
// 0x10001000-0x100060e7 in the order of this file. The C++ exception handling funclets of the constructors and
// destructors are at the end of .text, after the CRT.
#include "joy.h"

#include <math.h>

#define PI 3.1415927f

// GLOBAL: IJOY 0x1000e900
C_leak_detector leak_detector;

BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCEA instance, LPVOID context);
BOOL CALLBACK EnumObjectsCallback(LPCDIDEVICEOBJECTINSTANCEA instance, LPVOID context);
BOOL CALLBACK EnumEffectsCallback(LPCDIEFFECTINFOA info, LPVOID context);
BOOL CALLBACK EnumEffectsInFileCallback(LPCDIFILEEFFECT file_effect, LPVOID context);

// The DIJOYSTATE2 offset of an axis. Its callers check the axis first, there is no default.
static inline uint AxisOffset(int axis)
{
    uint offset;
    switch (axis) {
    case 0:
        offset = DIJOFS_X;
        break;
    case 1:
        offset = DIJOFS_Y;
        break;
    case 2:
        offset = DIJOFS_Z;
        break;
    case 3:
        offset = DIJOFS_RX;
        break;
    case 4:
        offset = DIJOFS_RY;
        break;
    case 5:
        offset = DIJOFS_RZ;
        break;
    case 6:
        offset = DIJOFS_SLIDER(0);
        break;
    case 7:
        offset = DIJOFS_SLIDER(1);
        break;
    }
    return offset;
}

static inline uint POVOffset(int pov)
{
    uint offset;
    switch (pov) {
    case 0:
        offset = DIJOFS_POV(0);
        break;
    case 1:
        offset = DIJOFS_POV(1);
        break;
    case 2:
        offset = DIJOFS_POV(2);
        break;
    case 3:
        offset = DIJOFS_POV(3);
        break;
    }
    return offset;
}

// FUNCTION: IJOY 0x10001040
bool EXPORT CreateJoyBase(IJoyBase **joy_base, HINSTANCE hinstance, HWND hwnd, IDirectInput7A *direct_input,
    uint flags)
{
    bool ok = false;
    *joy_base = NULL;
    if (hinstance && hwnd) {
        if (flags & 1)
            leak_detector.enabled = true;
        C_joy_base *base = new C_joy_base(hinstance, hwnd, direct_input, &ok);
        *joy_base = base;
        if (base) {
            if (ok)
                return true;
            delete base;
        }
        *joy_base = NULL;
    }
    return false;
}

// FUNCTION: IJOY 0x100010d0
C_joy_base::~C_joy_base()
{
    Close();
    leak_detector.objects[LEAK_JOY_BASE]--;
}

// FUNCTION: IJOY 0x10001150
void C_joy_base::Close()
{
    while (enum_context.devices) {
        S_device *device = enum_context.devices;
        enum_context.devices = device->next;
        while (device->objects) {
            S_object *object = device->objects;
            device->objects = object->next;
            delete object->instance;
            delete object;
        }
        if (device->device)
            device->device->Release();
        delete device->instance;
        delete device;
    }
    while (joys) {
        S_joy_node *node = joys;
        joys = node->next;
        delete node;
    }
    if (direct_input) {
        direct_input->Release();
        direct_input = NULL;
    }
}

// FUNCTION: IJOY 0x100011f0
C_joy_base::C_joy_base(HINSTANCE hinstance, HWND hwnd, IDirectInput7A *direct_input, bool *ok)
{
    leak_detector.objects[LEAK_JOY_BASE]++;
    ref = 0;
    *ok = false;
    this->direct_input = NULL;
    this->hinstance = hinstance;
    this->hwnd = hwnd;
    num_devices = 0;
    joys = NULL;
    memset(&enum_context, 0, sizeof(enum_context));
    if (direct_input) {
        this->direct_input = direct_input;
        direct_input->AddRef();
    } else if (FAILED(DirectInputCreateEx(this->hinstance, DIRECTINPUT_VERSION, IID_IDirectInput7A,
                   (void **)&this->direct_input, NULL)))
        return;
    if (!EnumDevices())
        Close();
    else {
        AddRef();
        *ok = true;
    }
}

// FUNCTION: IJOY 0x100012c0
uint C_joy_base::Release()
{
    uint result = --ref;
    leak_detector.refs[LEAK_JOY_BASE]--;
    if (ref < 1)
        delete this;
    return result;
}

// FUNCTION: IJOY 0x100012f0
uint C_joy_base::AddRef()
{
    leak_detector.refs[LEAK_JOY_BASE]++;
    return ++ref;
}

// FUNCTION: IJOY 0x10001310
bool C_joy_base::EnumDevices()
{
    enum_context.direct_input = direct_input;
    enum_context.hwnd = hwnd;
    enum_context.ok = false;
    enum_context.devices = NULL;
    direct_input->EnumDevices(DIDEVTYPE_JOYSTICK, EnumDevicesCallback, &enum_context, DIEDFL_ATTACHEDONLY);
    if (!enum_context.ok)
        return false;
    for (S_device *device = enum_context.devices; device; device = device->next) {
        device->device->EnumObjects(EnumObjectsCallback, device, DIDFT_ALL);
        if (!enum_context.ok)
            return false;
        memset(&device->caps, 0, sizeof(device->caps));
        device->caps.dwSize = sizeof(device->caps);
        device->device->GetCapabilities(&device->caps);
        device->device->Release();
        device->device = NULL;
        num_devices++;
    }
    return true;
}

// FUNCTION: IJOY 0x100013b0
bool C_joy_base::GetDeviceInfo(S_joy_device_info *info, int device)
{
    if (device < 0 || device >= num_devices || !info)
        return false;
    S_device *device_info = enum_context.devices;
    for (int i = device; i > 0; i--)
        device_info = device_info->next;
    info->name = device_info->instance->tszInstanceName;
    info->product = device_info->instance->tszProductName;
    info->num_axes = device_info->caps.dwAxes;
    info->num_buttons = device_info->caps.dwButtons;
    info->num_povs = device_info->caps.dwPOVs;
    info->force_feedback = (device_info->caps.dwFlags & DIDC_FORCEFEEDBACK) != 0;
    info->type = 0;
    switch (GET_DIDEVICE_SUBTYPE(device_info->instance->dwDevType)) {
    case DIDEVTYPEJOYSTICK_UNKNOWN:
        info->type = 1;
        break;
    case DIDEVTYPEJOYSTICK_TRADITIONAL:
        info->type = 2;
        break;
    case DIDEVTYPEJOYSTICK_FLIGHTSTICK:
        info->type = 3;
        break;
    case DIDEVTYPEJOYSTICK_GAMEPAD:
        info->type = 4;
        break;
    case DIDEVTYPEJOYSTICK_RUDDER:
        info->type = 5;
        break;
    case DIDEVTYPEJOYSTICK_WHEEL:
        info->type = 6;
        break;
    case DIDEVTYPEJOYSTICK_HEADTRACKER:
        info->type = 7;
        break;
    }
    return true;
}

// FUNCTION: IJOY 0x100014b0
int C_joy_base::NumDevices()
{
    return num_devices;
}

// FUNCTION: IJOY 0x100014c0
const char *C_joy_base::GetButtonName(int device, int button)
{
    if (!num_devices || device < 0 || device >= num_devices)
        return NULL;
    S_device *device_info = enum_context.devices;
    for (int i = device; i > 0; i--)
        device_info = device_info->next;
    if (button < 0 || button >= (int)device_info->caps.dwButtons)
        return NULL;
    S_object *object = device_info->objects;
    WORD offset = DIJOFS_BUTTON(button);
    do {
        if (IsEqualGUID(object->instance->guidType, GUID_Button) && object->instance->dwOfs == offset)
            return object->instance->tszName;
        object = object->next;
    } while (object);
    return NULL;
}

// FUNCTION: IJOY 0x10001540
const char *C_joy_base::GetAxisName(int device, int axis)
{
    if (!num_devices || device < 0 || device >= num_devices || axis < 0 || axis > 7)
        return NULL;
    S_device *device_info = enum_context.devices;
    for (int i = device; i > 0; i--)
        device_info = device_info->next;
    S_object *object = device_info->objects;
    uint offset = AxisOffset(axis);
    do {
        if (object->instance->dwOfs == offset)
            return object->instance->tszName;
        object = object->next;
    } while (object);
    return NULL;
}

// FUNCTION: IJOY 0x10001600
bool C_joy_base::GetAxisInfo(int device, int axis, S_joy_axis_info *info)
{
    memset(info, 0, sizeof(*info));
    if (!num_devices || device < 0 || device >= num_devices || !info || axis < 0 || axis > 7)
        return false;
    S_device *device_info = enum_context.devices;
    for (int i = device; i > 0; i--)
        device_info = device_info->next;
    S_object *object = device_info->objects;
    uint offset = AxisOffset(axis);
    do {
        if (object->instance->dwOfs == offset) {
            info->name = object->instance->tszName;
            if (object->instance->dwFlags & DIDOI_FFACTUATOR)
                info->ff_actuator = true;
            else
                info->ff_actuator = false;
            info->ff_force_resolution = object->instance->dwFFForceResolution;
            info->ff_max_force = object->instance->dwFFMaxForce;
            info->aspect = object->instance->dwFlags & DIDOI_ASPECTMASK;
            if (info->aspect == DIDOI_ASPECTACCEL)
                info->aspect = JOY_ASPECT_ACCELERATION;
            if (info->aspect == DIDOI_ASPECTFORCE)
                info->aspect = JOY_ASPECT_FORCE;
            if (info->aspect == DIDOI_ASPECTPOSITION)
                info->aspect = JOY_ASPECT_POSITION;
            if (info->aspect == DIDOI_ASPECTVELOCITY)
                info->aspect = JOY_ASPECT_VELOCITY;
            return true;
        }
        object = object->next;
    } while (object);
    memset(info, 0, sizeof(*info));
    return false;
}

// The original defines the interface's destructor and constructor here, not in the class: VC6 emits the destructor
// and its scalar deleting destructor here. A constructor defined in the class would make C_joy_base's constructor
// reference the interface's vtable, which puts the scalar deleting destructor after it.
// FUNCTION: IJOY 0x10001770
inline IJoyBase::~IJoyBase()
{
}

inline IJoyBase::IJoyBase()
{
}

// FUNCTION: IJOY 0x100017a0
const char *C_joy_base::GetPOVName(int device, int pov)
{
    if (!num_devices || device < 0 || device >= num_devices || pov < 0 || pov > 3)
        return NULL;
    S_device *device_info = enum_context.devices;
    for (int i = device; i > 0; i--)
        device_info = device_info->next;
    S_object *object = device_info->objects;
    uint offset = POVOffset(pov);
    do {
        if (IsEqualGUID(object->instance->guidType, GUID_POV) && object->instance->dwOfs == offset)
            return object->instance->tszName;
        object = object->next;
    } while (object);
    return NULL;
}

// FUNCTION: IJOY 0x10001850
IDirectInput7A *C_joy_base::GetDirectInput()
{
    return direct_input;
}

// FUNCTION: IJOY 0x10001860
bool C_joy_base::RunControlPanel()
{
    return SUCCEEDED(direct_input->RunControlPanel(enum_context.hwnd, 0)) ? true : false;
}

// FUNCTION: IJOY 0x10001880
bool C_joy_base::CreateJoy(IJoy **joy, int device, uint flags)
{
    *joy = NULL;
    if (!num_devices || device < 0 || device >= num_devices)
        return false;
    S_device *device_info = enum_context.devices;
    for (int i = device; i > 0; i--)
        device_info = device_info->next;
    bool ok = false;
    if (!joys) {
        joys = new S_joy_node;
        if (!joys)
            return false;
        joys->next = NULL;
        joys->device = device;
        joys->joy = new C_joy(this, device_info, &enum_context, flags, &ok);
        if (joys->joy) {
            if (ok) {
                *joy = joys->joy;
                return true;
            }
            delete joys->joy;
        }
        delete joys;
        joys = NULL;
        return false;
    } else {
        S_joy_node *node = joys;
        do {
            if (node->device == device)
                break;
            node = node->next;
        } while (node);
        if (!node) {
            node = new S_joy_node;
            if (!node)
                return false;
            node->device = device;
            node->joy = new C_joy(this, device_info, &enum_context, flags, &ok);
            if (node->joy) {
                if (ok) {
                    node->next = joys;
                    joys = node;
                    *joy = node->joy;
                    return true;
                }
                delete node->joy;
            }
            delete node;
        } else {
            *joy = node->joy;
            node->joy->AddRef();
            return true;
        }
    }
    return false;
}

// FUNCTION: IJOY 0x10001a40
void C_joy_base::RemoveJoy(C_joy *joy)
{
    if (joys->joy == joy) {
        S_joy_node *node = joys;
        joys = node->next;
        delete node;
        return;
    }
    for (S_joy_node *node = joys; node; node = node->next) {
        if (node->next && node->next->joy == joy) {
            S_joy_node *next = node->next;
            node->next = next->next;
            delete next;
            return;
        }
    }
}

// FUNCTION: IJOY 0x10001a90
C_joy::C_joy(C_joy_base *base, S_device *info, S_enum_context *context, uint flags, bool *ok)
{
    leak_detector.objects[LEAK_JOY]++;
    ref = 0;
    *ok = false;
    joy_base = base;
    device_info = info;
    enum_context = context;
    hwnd = context->hwnd;
    direct_input = context->direct_input;
    buttons_changed[0] = 0xffffffff;
    buttons_changed[1] = 0xffffffff;
    buttons_changed[2] = 0xffffffff;
    buttons_changed[3] = 0xffffffff;
    device = NULL;
    acquired = true;
    force_feedback = false;
    memset(&state, 0, sizeof(state));
    memset(&last_state, 0, sizeof(last_state));
    // One dword store, not memset: VC6 inlines a memset through a register
    *(uint *)pov_changed = 0x01010101;
    for (int i = 0; i < 8; i++) {
        dead_zone[i] = 0;
        axis_mode[i] = 0;
        saturation[i] = 1.0f;
        unused[i] = 0;
        half_range[i] = 10000.0f;
        range_offset[i] = 0;
    }

    if (FAILED(direct_input->CreateDeviceEx(device_info->instance->guidInstance, IID_IDirectInputDevice7A,
            (void **)&device, NULL)))
        return;
    HRESULT result;
    if (flags & 1)
        result = device->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    else
        result = device->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    if (FAILED(result)) {
        device->Release();
        device = NULL;
        return;
    }
    if (FAILED(device->SetDataFormat(&c_dfDIJoystick2))) {
        device->Release();
        device = NULL;
        return;
    }

    DIPROPRANGE range;
    memset(&range, 0, sizeof(range));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = DIJOFS_X;
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -10000;
    range.lMax = 10000;
    if (FAILED(device->SetProperty(DIPROP_RANGE, &range.diph))) {
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = DIJOFS_X;
        range.diph.dwHow = DIPH_BYOFFSET;
        device->GetProperty(DIPROP_RANGE, &range.diph);
        if (range.lMin >= 0) {
            half_range[0] = (range.lMax - range.lMin) * 0.5f;
            range_offset[0] = -(range.lMax + half_range[0]);
        } else
            half_range[0] = (float)range.lMax;
    }
    memset(&range, 0, sizeof(range));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = DIJOFS_Y;
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -10000;
    range.lMax = 10000;
    if (FAILED(device->SetProperty(DIPROP_RANGE, &range.diph))) {
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = DIJOFS_Y;
        range.diph.dwHow = DIPH_BYOFFSET;
        device->GetProperty(DIPROP_RANGE, &range.diph);
        if (range.lMin >= 0) {
            half_range[1] = (range.lMax - range.lMin) * 0.5f;
            range_offset[1] = -(range.lMax + half_range[1]);
        } else
            half_range[1] = (float)range.lMax;
    }
    memset(&range, 0, sizeof(range));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = DIJOFS_Z;
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -10000;
    range.lMax = 10000;
    if (FAILED(device->SetProperty(DIPROP_RANGE, &range.diph))) {
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = DIJOFS_Z;
        range.diph.dwHow = DIPH_BYOFFSET;
        device->GetProperty(DIPROP_RANGE, &range.diph);
        if (range.lMin >= 0) {
            half_range[2] = (range.lMax - range.lMin) * 0.5f;
            range_offset[2] = -(range.lMax + half_range[2]);
        } else
            half_range[2] = (float)range.lMax;
    }
    memset(&range, 0, sizeof(range));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = DIJOFS_RX;
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -10000;
    range.lMax = 10000;
    if (FAILED(device->SetProperty(DIPROP_RANGE, &range.diph))) {
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = DIJOFS_RX;
        range.diph.dwHow = DIPH_BYOFFSET;
        device->GetProperty(DIPROP_RANGE, &range.diph);
        if (range.lMin >= 0) {
            half_range[3] = (range.lMax - range.lMin) * 0.5f;
            range_offset[3] = -(range.lMax + half_range[3]);
        } else
            half_range[3] = (float)range.lMax;
    }
    memset(&range, 0, sizeof(range));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = DIJOFS_RY;
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -10000;
    range.lMax = 10000;
    if (FAILED(device->SetProperty(DIPROP_RANGE, &range.diph))) {
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = DIJOFS_RY;
        range.diph.dwHow = DIPH_BYOFFSET;
        device->GetProperty(DIPROP_RANGE, &range.diph);
        if (range.lMin >= 0) {
            half_range[4] = (range.lMax - range.lMin) * 0.5f;
            range_offset[4] = -(range.lMax + half_range[4]);
        } else
            half_range[4] = (float)range.lMax;
    }
    memset(&range, 0, sizeof(range));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = DIJOFS_RZ;
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -10000;
    range.lMax = 10000;
    if (FAILED(device->SetProperty(DIPROP_RANGE, &range.diph))) {
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = DIJOFS_RZ;
        range.diph.dwHow = DIPH_BYOFFSET;
        device->GetProperty(DIPROP_RANGE, &range.diph);
        if (range.lMin >= 0) {
            half_range[5] = (range.lMax - range.lMin) * 0.5f;
            range_offset[5] = -(range.lMax + half_range[5]);
        } else
            half_range[5] = (float)range.lMax;
    }
    memset(&range, 0, sizeof(range));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = DIJOFS_SLIDER(0);
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -10000;
    range.lMax = 10000;
    if (FAILED(device->SetProperty(DIPROP_RANGE, &range.diph))) {
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = DIJOFS_SLIDER(0);
        range.diph.dwHow = DIPH_BYOFFSET;
        device->GetProperty(DIPROP_RANGE, &range.diph);
        if (range.lMin >= 0) {
            half_range[6] = (range.lMax - range.lMin) * 0.5f;
            range_offset[6] = -(range.lMax + half_range[6]);
        } else
            half_range[6] = (float)range.lMax;
    }
    memset(&range, 0, sizeof(range));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = DIJOFS_SLIDER(1);
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = -10000;
    range.lMax = 10000;
    if (FAILED(device->SetProperty(DIPROP_RANGE, &range.diph))) {
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = DIJOFS_SLIDER(1);
        range.diph.dwHow = DIPH_BYOFFSET;
        device->GetProperty(DIPROP_RANGE, &range.diph);
        if (range.lMin >= 0) {
            half_range[7] = (range.lMax - range.lMin) * 0.5f;
            range_offset[7] = -(range.lMax + half_range[7]);
        } else
            half_range[7] = (float)range.lMax;
    }

    DIPROPDWORD property;
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = 0;
    property.diph.dwHow = DIPH_DEVICE;
    property.dwData = DIPROPAXISMODE_ABS;
    device->SetProperty(DIPROP_AXISMODE, &property.diph);
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = 0;
    property.diph.dwHow = DIPH_DEVICE;
    property.dwData = 0;
    device->SetProperty(DIPROP_DEADZONE, &property.diph);
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = 0;
    property.diph.dwHow = DIPH_DEVICE;
    property.dwData = 10000;
    device->SetProperty(DIPROP_SATURATION, &property.diph);

    num_effects = JOY_NUM_EFFECT_TYPES;
    for (int j = 0; j < num_effects; j++)
        effects[j].supported = false;
    if (device_info->caps.dwFlags & DIDC_FORCEFEEDBACK) {
        Acquire(true);
        force_feedback = true;
        property.diph.dwSize = sizeof(DIPROPDWORD);
        property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        property.diph.dwObj = 0;
        property.diph.dwHow = DIPH_DEVICE;
        property.dwData = 10000;
        if (FAILED(device->SetProperty(DIPROP_FFGAIN, &property.diph)))
            Sleep(0);
        Sleep(0);
        Acquire(true);
        device->EnumEffects(EnumEffectsCallback, effects, DIEFT_ALL);
        Sleep(0);
        Acquire(true);
        DIPROPDWORD auto_center;
        auto_center.diph.dwSize = sizeof(DIPROPDWORD);
        auto_center.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        auto_center.diph.dwObj = 0;
        auto_center.diph.dwHow = DIPH_DEVICE;
        auto_center.dwData = DIPROPAUTOCENTER_ON;
        if (FAILED(device->SetProperty(DIPROP_AUTOCENTER, &auto_center.diph)))
            Sleep(0);
        Sleep(0);
    }
    joy_base->AddRef();
    Acquire(true);
    AddRef();
    *ok = true;
}

// FUNCTION: IJOY 0x100023f0
C_joy::~C_joy()
{
    if (device) {
        device->Unacquire();
        device->Release();
        device = NULL;
    }
    leak_detector.objects[LEAK_JOY]--;
    joy_base->RemoveJoy(this);
    joy_base->Release();
}

// FUNCTION: IJOY 0x10002470
bool C_joy::RunControlPanel()
{
    return SUCCEEDED(device->RunControlPanel(hwnd, 0)) ? true : false;
}

// FUNCTION: IJOY 0x10002490
bool C_joy::SetDeadZone(int axis, float dead_zone)
{
    if (axis < 0 || axis > 7)
        return false;
    if (dead_zone < 0.0 || dead_zone > 1.0)
        return false;
    this->dead_zone[axis] = dead_zone;
    return true;
}

// FUNCTION: IJOY 0x100024e0
float C_joy::GetDeadZone(int axis)
{
    if (axis < 0 || axis > 7)
        return 0.0f;
    return dead_zone[axis];
}

// FUNCTION: IJOY 0x10002500
bool C_joy::SetDIDeadZone(int axis, float dead_zone)
{
    if (axis < 0 || axis > 7)
        return false;
    if (dead_zone < 0.0 || dead_zone > 1.0)
        return false;
    uint offset = AxisOffset(axis);
    DIPROPDWORD property;
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = offset;
    property.diph.dwHow = DIPH_BYOFFSET;
    property.dwData = (uint)(dead_zone * 10000.0f);
    device->SetProperty(DIPROP_DEADZONE, &property.diph);
    return true;
}

// FUNCTION: IJOY 0x10002600
float C_joy::GetDIDeadZone(int axis)
{
    if (axis < 0 || axis > 7)
        return 0.0f;
    uint offset = AxisOffset(axis);
    DIPROPDWORD property;
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = offset;
    property.diph.dwHow = DIPH_BYOFFSET;
    property.dwData = 0;
    if (FAILED(device->GetProperty(DIPROP_DEADZONE, &property.diph)))
        return 0.0f;
    return property.dwData * 0.0001f;
}

// FUNCTION: IJOY 0x100026e0
bool C_joy::SetSaturation(int axis, float saturation)
{
    if (axis < 0 || axis > 7)
        return false;
    if (saturation < 0.0 || saturation > 1.0)
        return false;
    this->saturation[axis] = saturation;
    return true;
}

// FUNCTION: IJOY 0x10002730
float C_joy::GetSaturation(int axis)
{
    if (axis < 0 || axis > 7)
        return 0.0f;
    return saturation[axis];
}

// FUNCTION: IJOY 0x10002750
bool C_joy::SetDISaturation(int axis, float saturation)
{
    if (axis < 0 || axis > 7)
        return false;
    if (saturation < 0.0 || saturation > 1.0)
        return false;
    uint offset = AxisOffset(axis);
    DIPROPDWORD property;
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = offset;
    property.diph.dwHow = DIPH_BYOFFSET;
    property.dwData = (uint)(saturation * 10000.0f);
    device->SetProperty(DIPROP_SATURATION, &property.diph);
    return true;
}

// FUNCTION: IJOY 0x10002850
float C_joy::GetDISaturation(int axis)
{
    if (axis < 0 || axis > 7)
        return 0.0f;
    uint offset = AxisOffset(axis);
    DIPROPDWORD property;
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = offset;
    property.diph.dwHow = DIPH_BYOFFSET;
    property.dwData = 0;
    if (FAILED(device->GetProperty(DIPROP_SATURATION, &property.diph)))
        return 0.0f;
    return property.dwData * 0.0001f;
}

// FUNCTION: IJOY 0x10002930
bool C_joy::SetAutoCenter(bool on)
{
    Acquire(true);
    DIPROPDWORD property;
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = 0;
    property.diph.dwHow = DIPH_DEVICE;
    property.dwData = on ? DIPROPAUTOCENTER_ON : DIPROPAUTOCENTER_OFF;
    return SUCCEEDED(device->SetProperty(DIPROP_AUTOCENTER, &property.diph)) ? true : false;
}

// FUNCTION: IJOY 0x10002990
bool C_joy::SetGain(float gain)
{
    if (gain < 0.0f || gain > 1.0f || !(device_info->caps.dwFlags & DIDC_FORCEFEEDBACK))
        return false;
    DIPROPDWORD property;
    property.diph.dwSize = sizeof(DIPROPDWORD);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = 0;
    property.diph.dwHow = DIPH_DEVICE;
    property.dwData = (uint)(gain * 10000.0f);
    return SUCCEEDED(device->SetProperty(DIPROP_FFGAIN, &property.diph)) ? true : false;
}

// FUNCTION: IJOY 0x10002a20
float C_joy::GetGain()
{
    if (device_info->caps.dwFlags & DIDC_FORCEFEEDBACK) {
        DIPROPDWORD property;
        property.diph.dwSize = sizeof(DIPROPDWORD);
        property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        property.diph.dwObj = 0;
        property.diph.dwHow = DIPH_DEVICE;
        property.dwData = 0;
        if (SUCCEEDED(device->GetProperty(DIPROP_FFGAIN, &property.diph)))
            return property.dwData * 0.0001f;
    }
    return 0.0f;
}

// FUNCTION: IJOY 0x10002a90
bool C_joy::SetAxisMode(int axis, uint mode)
{
    if (mode > 15 || axis < 0 || axis > 7)
        return false;
    uint raw = mode & JOY_AXIS_RAW;
    if (raw != (axis_mode[axis] & JOY_AXIS_RAW)) {
        uint offset = AxisOffset(axis);
        DIPROPDWORD property;
        property.diph.dwSize = sizeof(DIPROPDWORD);
        property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        property.diph.dwObj = offset;
        property.diph.dwHow = DIPH_BYOFFSET;
        property.dwData = raw ? DIPROPCALIBRATIONMODE_RAW : DIPROPCALIBRATIONMODE_COOKED;
        if (FAILED(device->SetProperty(DIPROP_CALIBRATIONMODE, &property.diph)))
            return false;
        if (!raw) {
            DIPROPRANGE range;
            memset(&range, 0, sizeof(range));
            range.diph.dwSize = sizeof(DIPROPRANGE);
            range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            range.diph.dwObj = offset;
            range.diph.dwHow = DIPH_BYOFFSET;
            range.lMin = -10000;
            range.lMax = 10000;
            device->SetProperty(DIPROP_RANGE, &range.diph);
            memset(&range, 0, sizeof(range));
            range.diph.dwSize = sizeof(DIPROPRANGE);
            range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            range.diph.dwObj = offset;
            range.diph.dwHow = DIPH_BYOFFSET;
            device->GetProperty(DIPROP_RANGE, &range.diph);
            if (range.lMin >= 0) {
                half_range[axis] = (range.lMax - range.lMin) * 0.5f;
                range_offset[axis] = -(range.lMax + half_range[axis]);
            } else
                half_range[axis] = (float)range.lMax;
        }
    }
    axis_mode[axis] = (axis_mode[axis] & 0xf0) | mode;
    return true;
}

// FUNCTION: IJOY 0x10002c70
uint C_joy::GetAxisMode(int axis)
{
    if (axis < 0 || axis > 7)
        return 0;
    return axis_mode[axis] & 0xf;
}

// FUNCTION: IJOY 0x10002c90
bool C_joy::Update(uint flags)
{
    bool ok = true;
    if (FAILED(device->Poll()) && (!acquired || !Acquire(true))) {
        if (flags & JOY_UPDATE_RESET)
            ok = false;
        else
            return false;
    }
    if (ok) {
        memset(&device_state, 0, sizeof(device_state));
        if (FAILED(device->GetDeviceState(sizeof(device_state), &device_state)) && (!acquired || !Acquire(true)))
            return false;

        if (axis_mode[0] & JOY_AXIS_RAW)
            state.x = (float)device_state.lX;
        else if (half_range[0] != 0.0f) {
            if (axis_mode[0] & JOY_AXIS_POSITIVE)
                state.x = (device_state.lX + range_offset[0] + half_range[0]) / (half_range[0] + half_range[0]);
            else
                state.x = (device_state.lX + range_offset[0]) / half_range[0];
            if (axis_mode[0] & JOY_AXIS_INVERT)
                state.x = -state.x;
        } else
            state.x = 0;
        if (axis_mode[1] & JOY_AXIS_RAW)
            state.y = (float)device_state.lY;
        else if (half_range[1] != 0.0f) {
            if (axis_mode[1] & JOY_AXIS_POSITIVE)
                state.y = (device_state.lY + range_offset[1] + half_range[1]) / (half_range[1] + half_range[1]);
            else
                state.y = (device_state.lY + range_offset[1]) / half_range[1];
            if (axis_mode[1] & JOY_AXIS_INVERT)
                state.y = -state.y;
        } else
            state.y = 0;
        if (axis_mode[2] & JOY_AXIS_RAW)
            state.z = (float)device_state.lZ;
        else if (half_range[2] != 0.0f) {
            if (axis_mode[2] & JOY_AXIS_POSITIVE)
                state.z = (device_state.lZ + range_offset[2] + half_range[2]) / (half_range[2] + half_range[2]);
            else
                state.z = (device_state.lZ + range_offset[2]) / half_range[2];
            if (axis_mode[2] & JOY_AXIS_INVERT)
                state.z = -state.z;
        } else
            state.z = 0;
        if (axis_mode[3] & JOY_AXIS_RAW)
            state.rx = (float)device_state.lRx;
        else if (half_range[3] != 0.0f) {
            if (axis_mode[3] & JOY_AXIS_POSITIVE)
                state.rx = (device_state.lRx + range_offset[3] + half_range[3]) / (half_range[3] + half_range[3]);
            else
                state.rx = (device_state.lRx + range_offset[3]) / half_range[3];
            if (axis_mode[3] & JOY_AXIS_INVERT)
                state.rx = -state.rx;
        } else
            state.rx = 0;
        if (axis_mode[4] & JOY_AXIS_RAW)
            state.ry = (float)device_state.lRy;
        else if (half_range[4] != 0.0f) {
            if (axis_mode[4] & JOY_AXIS_POSITIVE)
                state.ry = (device_state.lRy + range_offset[4] + half_range[4]) / (half_range[4] + half_range[4]);
            else
                state.ry = (device_state.lRy + range_offset[4]) / half_range[4];
            if (axis_mode[4] & JOY_AXIS_INVERT)
                state.ry = -state.ry;
        } else
            state.ry = 0;
        if (axis_mode[5] & JOY_AXIS_RAW)
            state.rz = (float)device_state.lRz;
        else if (half_range[5] != 0.0f) {
            if (axis_mode[5] & JOY_AXIS_POSITIVE)
                state.rz = (device_state.lRz + range_offset[5] + half_range[5]) / (half_range[5] + half_range[5]);
            else
                state.rz = (device_state.lRz + range_offset[5]) / half_range[5];
            if (axis_mode[5] & JOY_AXIS_INVERT)
                state.rz = -state.rz;
        } else
            state.rz = 0;
        if (axis_mode[6] & JOY_AXIS_RAW)
            state.slider[0] = (float)device_state.rglSlider[0];
        else if (half_range[6] != 0.0f) {
            if (axis_mode[6] & JOY_AXIS_POSITIVE)
                state.slider[0] = (device_state.rglSlider[0] + range_offset[6] + half_range[6])
                    / (half_range[6] + half_range[6]);
            else
                state.slider[0] = (device_state.rglSlider[0] + range_offset[6]) / half_range[6];
            if (axis_mode[6] & JOY_AXIS_INVERT)
                state.slider[0] = -state.slider[0];
        } else
            state.slider[0] = 0;
        if (axis_mode[7] & JOY_AXIS_RAW)
            state.slider[1] = (float)device_state.rglSlider[1];
        else if (half_range[7] != 0.0f) {
            if (axis_mode[7] & JOY_AXIS_POSITIVE)
                state.slider[1] = (device_state.rglSlider[1] + range_offset[7] + half_range[7])
                    / (half_range[7] + half_range[7]);
            else
                state.slider[1] = (device_state.rglSlider[1] + range_offset[7]) / half_range[7];
            if (axis_mode[7] & JOY_AXIS_INVERT)
                state.slider[1] = -state.slider[1];
        } else
            state.slider[1] = 0;
    } else {
        if (!(axis_mode[0] & JOY_AXIS_RAW))
            state.x = 0;
        if (!(axis_mode[1] & JOY_AXIS_RAW))
            state.y = 0;
        if (!(axis_mode[2] & JOY_AXIS_RAW))
            state.z = 0;
        if (!(axis_mode[3] & JOY_AXIS_RAW))
            state.rx = 0;
        if (!(axis_mode[4] & JOY_AXIS_RAW))
            state.ry = 0;
        if (!(axis_mode[5] & JOY_AXIS_RAW))
            state.rz = 0;
        if (!(axis_mode[6] & JOY_AXIS_RAW))
            state.slider[0] = 0;
        if (!(axis_mode[7] & JOY_AXIS_RAW))
            state.slider[1] = 0;
    }

    // Dead zones
    if (!(axis_mode[0] & JOY_AXIS_RAW) && fabs(state.x) < dead_zone[0])
        state.x = 0;
    if (!(axis_mode[1] & JOY_AXIS_RAW) && fabs(state.y) < dead_zone[1])
        state.y = 0;
    if (!(axis_mode[2] & JOY_AXIS_RAW) && fabs(state.z) < dead_zone[2])
        state.z = 0;
    if (!(axis_mode[3] & JOY_AXIS_RAW) && fabs(state.rx) < dead_zone[3])
        state.rx = 0;
    if (!(axis_mode[4] & JOY_AXIS_RAW) && fabs(state.ry) < dead_zone[4])
        state.ry = 0;
    if (!(axis_mode[5] & JOY_AXIS_RAW) && fabs(state.rz) < dead_zone[5])
        state.rz = 0;
    if (!(axis_mode[6] & JOY_AXIS_RAW) && fabs(state.slider[0]) < dead_zone[6])
        state.slider[0] = 0;
    if (!(axis_mode[7] & JOY_AXIS_RAW) && fabs(state.slider[1]) < dead_zone[7])
        state.slider[1] = 0;

    // Saturation
    if (!(axis_mode[0] & JOY_AXIS_RAW) && fabs(state.x) > saturation[0]) {
        if (state.x > 0)
            state.x = saturation[0];
        else
            state.x = -saturation[0];
    }
    if (!(axis_mode[1] & JOY_AXIS_RAW) && fabs(state.y) > saturation[1]) {
        if (state.y > 0)
            state.y = saturation[1];
        else
            state.y = -saturation[1];
    }
    if (!(axis_mode[2] & JOY_AXIS_RAW) && fabs(state.z) > saturation[2]) {
        if (state.z > 0)
            state.z = saturation[2];
        else
            state.z = -saturation[2];
    }
    if (!(axis_mode[3] & JOY_AXIS_RAW) && fabs(state.rx) > saturation[3]) {
        if (state.rx > 0)
            state.rx = saturation[3];
        else
            state.rx = -saturation[3];
    }
    if (!(axis_mode[4] & JOY_AXIS_RAW) && fabs(state.ry) > saturation[4]) {
        if (state.ry > 0)
            state.ry = saturation[4];
        else
            state.ry = -saturation[4];
    }
    if (!(axis_mode[5] & JOY_AXIS_RAW) && fabs(state.rz) > saturation[5]) {
        if (state.rz > 0)
            state.rz = saturation[5];
        else
            state.rz = -saturation[5];
    }
    if (!(axis_mode[6] & JOY_AXIS_RAW) && fabs(state.slider[0]) > saturation[6]) {
        if (state.slider[0] > 0)
            state.slider[0] = saturation[6];
        else
            state.slider[0] = -saturation[6];
    }
    // The original sets slider 0 here
    if (!(axis_mode[7] & JOY_AXIS_RAW) && fabs(state.slider[1]) > saturation[7]) {
        if (state.slider[1] > 0)
            state.slider[0] = saturation[7];
        else
            state.slider[1] = -saturation[7];
    }

    // Relative axes
    S_joy_state absolute = state;
    if (!(axis_mode[0] & JOY_AXIS_RAW) && (axis_mode[0] & JOY_AXIS_RELATIVE))
        state.x -= last_state.x;
    if (!(axis_mode[1] & JOY_AXIS_RAW) && (axis_mode[1] & JOY_AXIS_RELATIVE))
        state.y -= last_state.y;
    if (!(axis_mode[2] & JOY_AXIS_RAW) && (axis_mode[2] & JOY_AXIS_RELATIVE))
        state.z -= last_state.z;
    if (!(axis_mode[3] & JOY_AXIS_RAW) && (axis_mode[3] & JOY_AXIS_RELATIVE))
        state.rx -= last_state.rx;
    if (!(axis_mode[4] & JOY_AXIS_RAW) && (axis_mode[4] & JOY_AXIS_RELATIVE))
        state.ry -= last_state.ry;
    if (!(axis_mode[5] & JOY_AXIS_RAW) && (axis_mode[5] & JOY_AXIS_RELATIVE))
        state.rz -= last_state.rz;
    if (!(axis_mode[6] & JOY_AXIS_RAW) && (axis_mode[6] & JOY_AXIS_RELATIVE))
        state.slider[0] -= last_state.slider[0];
    if (!(axis_mode[7] & JOY_AXIS_RAW) && (axis_mode[7] & JOY_AXIS_RELATIVE))
        state.slider[1] -= last_state.slider[1];
    last_state = absolute;

    // POVs
    if (ok) {
        if (pov_changed[0] && !state.pov_centered[0])
            pov_changed[0] = false;
        float last = state.pov[0];
        state.pov_centered[0] = LOWORD(device_state.rgdwPOV[0]) == 0xffff;
        if (!state.pov_centered[0])
            state.pov[0] = (device_state.rgdwPOV[0] * PI) * (1.0f / 18000.0f);
        else
            pov_changed[0] = true;
        if (last != state.pov[0] && !state.pov_centered[0])
            pov_changed[0] = true;

        if (pov_changed[1] && !state.pov_centered[1])
            pov_changed[1] = false;
        last = state.pov[1];
        state.pov_centered[1] = LOWORD(device_state.rgdwPOV[1]) == 0xffff;
        if (!state.pov_centered[1])
            state.pov[1] = (device_state.rgdwPOV[1] * PI) * (1.0f / 18000.0f);
        else
            pov_changed[1] = true;
        if (last != state.pov[1] || !state.pov_centered[1])
            pov_changed[1] = true;

        if (pov_changed[2] && !state.pov_centered[2])
            pov_changed[2] = false;
        last = state.pov[2];
        state.pov_centered[2] = LOWORD(device_state.rgdwPOV[2]) == 0xffff;
        if (!state.pov_centered[2])
            state.pov[2] = (device_state.rgdwPOV[2] * PI) * (1.0f / 18000.0f);
        else
            pov_changed[2] = true;
        if (last != state.pov[2] || !state.pov_centered[2])
            pov_changed[2] = true;

        if (pov_changed[3] && !state.pov_centered[3])
            pov_changed[3] = false;
        last = state.pov[3];
        state.pov_centered[3] = LOWORD(device_state.rgdwPOV[3]) == 0xffff;
        if (!state.pov_centered[3])
            state.pov[3] = (device_state.rgdwPOV[3] * PI) * (1.0f / 18000.0f);
        else
            pov_changed[3] = true;
        if (last != state.pov[3] || !state.pov_centered[3])
            pov_changed[3] = true;
    } else {
        // Eight of each, the original writes past both arrays into buttons[0] and dead_zone[0]
        state.pov_centered[0] = true;
        pov_changed[0] = false;
        state.pov_centered[1] = true;
        pov_changed[1] = false;
        state.pov_centered[2] = true;
        pov_changed[2] = false;
        state.pov_centered[3] = true;
        pov_changed[3] = false;
        state.pov_centered[4] = true;
        pov_changed[4] = false;
        state.pov_centered[5] = true;
        pov_changed[5] = false;
        state.pov_centered[6] = true;
        pov_changed[6] = false;
        state.pov_centered[7] = true;
        pov_changed[7] = false;
    }

    // Buttons
    int i;
    buttons_changed[0] &= ~state.buttons[0];
    if (!ok)
        memset(device_state.rgbButtons, 0, sizeof(device_state.rgbButtons));
    state.buttons[0] = 0;
    for (i = 0; i < 32; i++) {
        if (device_state.rgbButtons[i])
            state.buttons[0] |= 1 << i;
    }
    buttons_changed[0] = ~(state.buttons[0] & buttons_changed[0]);
    buttons_changed[0] = state.buttons[0] ^ buttons_changed[0];
    buttons_changed[1] &= ~state.buttons[1];
    state.buttons[1] = 0;
    for (i = 0; i < 32; i++) {
        if (device_state.rgbButtons[32 + i])
            state.buttons[1] |= 1 << i;
    }
    buttons_changed[1] = ~(state.buttons[1] & buttons_changed[1]);
    buttons_changed[1] = state.buttons[1] ^ buttons_changed[1];
    buttons_changed[2] &= ~state.buttons[2];
    state.buttons[2] = 0;
    for (i = 0; i < 32; i++) {
        if (device_state.rgbButtons[64 + i])
            state.buttons[2] |= 1 << i;
    }
    buttons_changed[2] = ~(state.buttons[2] & buttons_changed[2]);
    buttons_changed[2] = state.buttons[2] ^ buttons_changed[2];
    buttons_changed[3] &= ~state.buttons[3];
    state.buttons[3] = 0;
    for (i = 0; i < 32; i++) {
        if (device_state.rgbButtons[96 + i])
            state.buttons[3] |= 1 << i;
    }
    buttons_changed[3] = ~(state.buttons[3] & buttons_changed[3]);
    buttons_changed[3] = state.buttons[3] ^ buttons_changed[3];
    return true;
}

// FUNCTION: IJOY 0x10003a60
bool C_joy::Acquire(bool acquire)
{
    acquired = acquire;
    if (acquire) {
        if (FAILED(device->Acquire()))
            return false;
    } else
        device->Unacquire();
    return true;
}

// FUNCTION: IJOY 0x10003a90
bool C_joy::GetState(S_joy_state *state)
{
    if (state) {
        *state = this->state;
        return true;
    }
    return false;
}

// FUNCTION: IJOY 0x10003ac0
bool C_joy::GetStateChanges(S_joy_state *state)
{
    if (state) {
        *state = this->state;
        state->buttons[0] = buttons_changed[0];
        state->buttons[0] &= this->state.buttons[0];
        state->buttons[1] = buttons_changed[1];
        state->buttons[1] &= this->state.buttons[1];
        state->buttons[2] = buttons_changed[2];
        state->buttons[2] &= this->state.buttons[2];
        state->buttons[3] = buttons_changed[3];
        state->buttons[3] &= this->state.buttons[3];
        for (int i = 0; i < 4; i++) {
            if (pov_changed[i] && !this->state.pov_centered[i])
                state->pov_centered[i] = false;
            else
                state->pov_centered[i] = true;
        }
        return true;
    }
    return false;
}

// FUNCTION: IJOY 0x10003b70
bool C_joy::GetPOV(float *angle, int pov)
{
    if (pov < 0 || pov > 3 || !angle)
        return false;
    *angle = state.pov[pov];
    return state.pov_centered[pov] ? false : true;
}

// FUNCTION: IJOY 0x10003bb0
bool C_joy::GetPOVChanged(float *angle, int pov)
{
    if (pov < 0 || pov > 3 || !angle)
        return false;
    *angle = state.pov[pov];
    if (pov_changed[pov] && !state.pov_centered[pov])
        return true;
    return false;
}

// FUNCTION: IJOY 0x10003bf0
uint C_joy::GetButtons(int group)
{
    switch (group) {
    case 0:
        return state.buttons[0];
    case 1:
        return state.buttons[1];
    case 2:
        return state.buttons[2];
    case 3:
        return state.buttons[3];
    }
    return 0;
}

// FUNCTION: IJOY 0x10003c40
uint C_joy::GetButtonsPressed(int group)
{
    switch (group) {
    case 0:
        return buttons_changed[0] & state.buttons[0];
    case 1:
        return buttons_changed[1] & state.buttons[1];
    case 2:
        return buttons_changed[2] & state.buttons[2];
    case 3:
        return buttons_changed[3] & state.buttons[3];
    }
    return 0;
}

// FUNCTION: IJOY 0x10003cb0
float C_joy::GetAxis(int axis)
{
    switch (axis) {
    case 0:
        return state.x;
    case 1:
        return state.y;
    case 2:
        return state.z;
    case 3:
        return state.rx;
    case 4:
        return state.ry;
    case 5:
        return state.rz;
    case 6:
        return state.slider[0];
    case 7:
        return state.slider[1];
    }
    return 0.0f;
}

// FUNCTION: IJOY 0x10003d40
bool C_joy::IsButtonDown(int button)
{
    if (button < 0 || button > 127)
        return false;
    // The cases fall through
    switch (button / 32) {
    case 0:
        if (state.buttons[0] & (1 << button))
            return true;
    case 1:
        if (state.buttons[1] & (1 << (button - 32)))
            return true;
    case 2:
        if (state.buttons[2] & (1 << (button - 64)))
            return true;
    case 3:
        if (state.buttons[3] & (1 << (button - 96)))
            return true;
    }
    return false;
}

// FUNCTION: IJOY 0x10003df0
bool C_joy::IsButtonPressed(int button)
{
    if (button < 0 || button > 127)
        return false;
    // The cases fall through
    switch (button / 32) {
    case 0:
        if ((state.buttons[0] & (1 << button)) && (buttons_changed[0] & (1 << button)))
            return true;
    case 1:
        if ((state.buttons[1] & (1 << (button - 32))) && (buttons_changed[1] & (1 << (button - 32))))
            return true;
    case 2:
        if ((state.buttons[2] & (1 << (button - 64))) && (buttons_changed[2] & (1 << (button - 64))))
            return true;
    case 3:
        if ((state.buttons[3] & (1 << (button - 96))) && (buttons_changed[3] & (1 << (button - 96))))
            return true;
    }
    return false;
}

// FUNCTION: IJOY 0x10003ec0
bool C_joy::GetInfo(S_joy_device_info *info)
{
    if (!info)
        return false;
    info->name = device_info->instance->tszInstanceName;
    info->product = device_info->instance->tszProductName;
    info->num_axes = device_info->caps.dwAxes;
    info->num_buttons = device_info->caps.dwButtons;
    info->num_povs = device_info->caps.dwPOVs;
    info->force_feedback = force_feedback;
    info->type = 0;
    switch (GET_DIDEVICE_SUBTYPE(device_info->instance->dwDevType)) {
    case DIDEVTYPEJOYSTICK_UNKNOWN:
        info->type = 1;
        break;
    case DIDEVTYPEJOYSTICK_TRADITIONAL:
        info->type = 2;
        break;
    case DIDEVTYPEJOYSTICK_FLIGHTSTICK:
        info->type = 3;
        break;
    case DIDEVTYPEJOYSTICK_GAMEPAD:
        info->type = 4;
        break;
    case DIDEVTYPEJOYSTICK_RUDDER:
        info->type = 5;
        break;
    case DIDEVTYPEJOYSTICK_WHEEL:
        info->type = 6;
        break;
    case DIDEVTYPEJOYSTICK_HEADTRACKER:
        info->type = 7;
        break;
    }
    return true;
}

// FUNCTION: IJOY 0x10003fa0
const char *C_joy::GetButtonName(int button)
{
    if (button < 0 || button >= (int)device_info->caps.dwButtons)
        return NULL;
    S_object *object = device_info->objects;
    WORD offset = DIJOFS_BUTTON(button);
    do {
        if (IsEqualGUID(object->instance->guidType, GUID_Button) && object->instance->dwOfs == offset)
            return object->instance->tszName;
        object = object->next;
    } while (object);
    return NULL;
}

// FUNCTION: IJOY 0x10004000
const char *C_joy::GetAxisName(int axis)
{
    if (axis < 0 || axis > 7)
        return NULL;
    S_object *object = device_info->objects;
    uint offset = AxisOffset(axis);
    do {
        if (object->instance->dwOfs == offset)
            return object->instance->tszName;
        object = object->next;
    } while (object);
    return NULL;
}

// FUNCTION: IJOY 0x10004090
bool C_joy::GetAxisInfo(int axis, S_joy_axis_info *info)
{
    if (axis < 0 || axis > 7 || !info) {
        memset(info, 0, sizeof(*info));
        return false;
    }
    S_object *object = device_info->objects;
    uint offset = AxisOffset(axis);
    do {
        if (object->instance->dwOfs == offset) {
            info->name = object->instance->tszName;
            if (object->instance->dwFlags & DIDOI_FFACTUATOR)
                info->ff_actuator = true;
            else
                info->ff_actuator = false;
            info->ff_force_resolution = object->instance->dwFFForceResolution;
            info->ff_max_force = object->instance->dwFFMaxForce;
            info->aspect = object->instance->dwFlags & DIDOI_ASPECTMASK;
            if (info->aspect == DIDOI_ASPECTACCEL)
                info->aspect = JOY_ASPECT_ACCELERATION;
            if (info->aspect == DIDOI_ASPECTFORCE)
                info->aspect = JOY_ASPECT_FORCE;
            if (info->aspect == DIDOI_ASPECTPOSITION)
                info->aspect = JOY_ASPECT_POSITION;
            if (info->aspect == DIDOI_ASPECTVELOCITY)
                info->aspect = JOY_ASPECT_VELOCITY;
            return true;
        }
        object = object->next;
    } while (object);
    memset(info, 0, sizeof(*info));
    return false;
}

// FUNCTION: IJOY 0x100041e0
const char *C_joy::GetPOVName(int pov)
{
    if (pov < 0 || pov > 3)
        return NULL;
    S_object *object = device_info->objects;
    uint offset = POVOffset(pov);
    do {
        if (IsEqualGUID(object->instance->guidType, GUID_POV) && object->instance->dwOfs == offset)
            return object->instance->tszName;
        object = object->next;
    } while (object);
    return NULL;
}

// FUNCTION: IJOY 0x10004270
uint C_joy::Release()
{
    uint result = --ref;
    leak_detector.refs[LEAK_JOY]--;
    if (ref < 1)
        delete this;
    return result;
}

// FUNCTION: IJOY 0x100042a0
uint C_joy::AddRef()
{
    leak_detector.refs[LEAK_JOY]++;
    return ++ref;
}

// FUNCTION: IJOY 0x100042c0
bool C_joy::GetFFState(uint *state)
{
    if (!state)
        return false;
    *state = 0;
    DWORD ff_state = 0;
    if (FAILED(device->GetForceFeedbackState(&ff_state)))
        return false;
    if (ff_state & DIGFFS_ACTUATORSOFF)
        *state |= JOY_FF_ACTUATORS_OFF;
    if (ff_state & DIGFFS_ACTUATORSON)
        *state |= JOY_FF_ACTUATORS_ON;
    if (ff_state & DIGFFS_DEVICELOST)
        *state |= JOY_FF_DEVICE_LOST;
    if (ff_state & DIGFFS_EMPTY)
        *state |= JOY_FF_EMPTY;
    if (ff_state & DIGFFS_PAUSED)
        *state |= JOY_FF_PAUSED;
    if (ff_state & DIGFFS_POWEROFF)
        *state |= JOY_FF_POWER_OFF;
    if (ff_state & DIGFFS_POWERON)
        *state |= JOY_FF_POWER_ON;
    if (ff_state & DIGFFS_SAFETYSWITCHOFF)
        *state |= JOY_FF_SAFETY_SWITCH_OFF;
    if (ff_state & DIGFFS_SAFETYSWITCHON)
        *state |= JOY_FF_SAFETY_SWITCH_ON;
    if (ff_state & DIGFFS_STOPPED)
        *state |= JOY_FF_STOPPED;
    if (ff_state & DIGFFS_USERFFSWITCHOFF)
        *state |= JOY_FF_USER_SWITCH_OFF;
    if (ff_state & DIGFFS_USERFFSWITCHON)
        *state |= JOY_FF_USER_SWITCH_ON;
    return true;
}

// FUNCTION: IJOY 0x10004370
bool C_joy::SendFFCommand(int command)
{
    uint flags;
    switch (command) {
    case JOY_FF_CONTINUE:
        flags = DISFFC_CONTINUE;
        break;
    case JOY_FF_PAUSE:
        flags = DISFFC_PAUSE;
        break;
    case JOY_FF_RESET:
        flags = DISFFC_RESET;
        break;
    case JOY_FF_SET_ACTUATORS_OFF:
        flags = DISFFC_SETACTUATORSOFF;
        break;
    case JOY_FF_SET_ACTUATORS_ON:
        flags = DISFFC_SETACTUATORSON;
        break;
    case JOY_FF_STOP_ALL:
        flags = DISFFC_STOPALL;
        break;
    default:
        return false;
    }
    return SUCCEEDED(device->SendForceFeedbackCommand(flags)) ? true : false;
}

// FUNCTION: IJOY 0x10004430
bool C_joy::CreateEffect(IJoyEffect **effect)
{
    bool ok = false;
    C_joy_effect *joy_effect = new C_joy_effect(this, device, effects, num_effects, &ok);
    *effect = joy_effect;
    if (joy_effect) {
        if (ok)
            return true;
        delete joy_effect;
    }
    *effect = NULL;
    return false;
}

// FUNCTION: IJOY 0x100044e0
C_joy_effect::C_joy_effect(C_joy *owner, IDirectInputDevice7A *input_device, S_effect_info *info, int num_info,
    bool *ok)
{
    leak_detector.objects[LEAK_EFFECT]++;
    ref = 0;
    *ok = false;
    joy = owner;
    device = input_device;
    effect_info = info;
    effect = NULL;
    file_effects = NULL;
    num_effect_info = num_info;
    axes[0] = DIJOFS_X;
    axes[1] = DIJOFS_Y;
    num_axes = 2;
    for (int i = 0; i < 8; i++)
        direction[i] = 0;
    type = JOY_EFFECT_NONE;
    joy->AddRef();
    AddRef();
    *ok = true;
}

// FUNCTION: IJOY 0x100045f0
C_joy_effect::~C_joy_effect()
{
    while (file_effects) {
        S_file_effect *node = file_effects;
        S_file_effect *next = node->next;
        if (node->effect)
            node->effect->Release();
        delete file_effects;
        file_effects = next;
    }
    if (effect) {
        effect->Release();
        effect = NULL;
    }
    leak_detector.objects[LEAK_EFFECT]--;
    joy->Release();
}

// FUNCTION: IJOY 0x10004690
uint C_joy_effect::Release()
{
    uint result = --ref;
    leak_detector.refs[LEAK_EFFECT]--;
    if (ref < 1)
        delete this;
    return result;
}

// FUNCTION: IJOY 0x100046c0
uint C_joy_effect::AddRef()
{
    leak_detector.refs[LEAK_EFFECT]++;
    return ++ref;
}

// FUNCTION: IJOY 0x100046e0
bool C_joy_effect::SetAxes(int num_axes, const uchar *axes)
{
    if (type == JOY_EFFECT_FILE || num_axes > 8 || num_axes < 0)
        return false;
    if (num_axes > 0) {
        for (int i = 0; i < num_axes; i++) {
            switch (axes[i]) {
            case 0:
                this->axes[i] = DIJOFS_X;
                break;
            case 1:
                this->axes[i] = DIJOFS_Y;
                break;
            case 2:
                this->axes[i] = DIJOFS_Z;
                break;
            case 3:
                this->axes[i] = DIJOFS_RX;
                break;
            case 4:
                this->axes[i] = DIJOFS_RY;
                break;
            case 5:
                this->axes[i] = DIJOFS_RZ;
                break;
            case 6:
                this->axes[i] = DIJOFS_SLIDER(0);
                break;
            case 7:
                this->axes[i] = DIJOFS_SLIDER(1);
                break;
            default:
                this->axes[0] = DIJOFS_X;
                this->axes[1] = DIJOFS_Y;
                return false;
            }
        }
    } else {
        this->axes[0] = DIJOFS_X;
        this->axes[1] = DIJOFS_Y;
        num_axes = 2;
    }
    this->num_axes = num_axes;
    return true;
}

// FUNCTION: IJOY 0x100047f0
bool C_joy_effect::SetType(int type, float direction, uint duration, float gain, const char *file_name,
    const void *data)
{
    this->type = JOY_EFFECT_NONE;
    if (effect) {
        effect->Release();
        effect = NULL;
    }
    while (file_effects) {
        S_file_effect *node = file_effects;
        S_file_effect *next = node->next;
        if (node->effect)
            node->effect->Release();
        delete file_effects;
        file_effects = next;
    }
    num_effects = 0;
    if (type == JOY_EFFECT_NONE)
        return true;

    if (type == JOY_EFFECT_FILE) {
        S_file_context context;
        context.device = device;
        context.effects = NULL;
        context.num_effects = 0;
        if (file_name) {
            if (FAILED(device->EnumEffectsInFile(file_name, EnumEffectsInFileCallback, &context,
                    DIFEF_MODIFYIFNEEDED))) {
                file_effects = context.effects;
                while (file_effects) {
                    S_file_effect *node = file_effects;
                    S_file_effect *next = node->next;
                    if (node->effect)
                        node->effect->Release();
                    delete file_effects;
                    file_effects = next;
                }
                return false;
            }
        } else if (!data)
            return false;
        file_effects = context.effects;
        num_effects = context.num_effects;
        this->type = JOY_EFFECT_FILE;
        return true;
    }

    if (type < num_effect_info && effect_info[type].supported) {
        for (uint i = 0; i < num_axes; i++)
            effect_axes[i] = axes[i];
        num_effect_axes = num_axes;
        axes[0] = DIJOFS_X;
        axes[1] = DIJOFS_Y;
        num_axes = 2;
        this->direction[0] = (LONG)((direction * 18000.0f) * (1.0f / PI));
        if (gain > 1.0f)
            gain = 1.0f;
        else if (gain < 0.0f)
            gain = 0.0f;

        condition[0].lOffset = 0;
        condition[0].lPositiveCoefficient = 0;
        condition[0].lNegativeCoefficient = 0;
        condition[0].dwPositiveSaturation = 10000;
        condition[0].dwNegativeSaturation = 10000;
        condition[0].lDeadBand = 0;
        condition[1].lOffset = 0;
        condition[1].lPositiveCoefficient = 0;
        condition[1].lNegativeCoefficient = 0;
        condition[1].dwPositiveSaturation = 10000;
        condition[1].dwNegativeSaturation = 10000;
        condition[1].lDeadBand = 0;
        ramp_force.lStart = 0;
        ramp_force.lEnd = 0;
        constant_force.lMagnitude = 0;
        periodic.dwMagnitude = 0;
        periodic.lOffset = 0;
        periodic.dwPhase = 0;
        periodic.dwPeriod = 500000;
        envelope.dwSize = sizeof(DIENVELOPE);
        envelope.dwAttackLevel = 0;
        envelope.dwAttackTime = 500;
        envelope.dwFadeLevel = 0;
        envelope.dwFadeTime = 500;

        parameters.dwSize = sizeof(DIEFFECT);
        parameters.dwFlags = DIEFF_OBJECTOFFSETS | DIEFF_POLAR;
        if (!duration)
            parameters.dwDuration = INFINITE;
        else
            parameters.dwDuration = duration;
        parameters.dwSamplePeriod = 0;
        parameters.dwGain = (uint)(gain * 10000.0f);
        parameters.dwTriggerButton = DIEB_NOTRIGGER;
        parameters.dwTriggerRepeatInterval = 0;
        parameters.cAxes = num_effect_axes;
        parameters.rgdwAxes = effect_axes;
        parameters.rglDirection = this->direction;
        switch (type) {
        case JOY_EFFECT_CONSTANT_FORCE:
            parameters.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
            parameters.lpvTypeSpecificParams = &constant_force;
            parameters.lpEnvelope = NULL;
            break;
        case JOY_EFFECT_RAMP_FORCE:
            parameters.lpEnvelope = NULL;
            if (!duration)
                goto fail;
            parameters.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
            parameters.lpvTypeSpecificParams = &ramp_force;
            break;
        case JOY_EFFECT_SQUARE:
        case JOY_EFFECT_SINE:
        case JOY_EFFECT_TRIANGLE:
        case JOY_EFFECT_SAWTOOTH_UP:
        case JOY_EFFECT_SAWTOOTH_DOWN:
            parameters.cbTypeSpecificParams = sizeof(DIPERIODIC);
            parameters.lpvTypeSpecificParams = &periodic;
            parameters.lpEnvelope = NULL;
            break;
        case JOY_EFFECT_SPRING:
        case JOY_EFFECT_DAMPER:
        case JOY_EFFECT_INERTIA:
        case JOY_EFFECT_FRICTION:
            parameters.cbTypeSpecificParams = sizeof(condition);
            parameters.lpvTypeSpecificParams = condition;
            parameters.lpEnvelope = NULL;
            break;
        }
        if (FAILED(device->CreateEffect(effect_info[type].info.guid, &parameters, &effect, NULL))) {
            this->type = JOY_EFFECT_NONE;
            if (effect) {
                effect->Release();
                effect = NULL;
            }
        } else {
            this->type = type;
            num_effects = 1;
            return true;
        }
    }
fail:
    axes[0] = DIJOFS_X;
    axes[1] = DIJOFS_Y;
    num_axes = 2;
    return false;
}

// FUNCTION: IJOY 0x10004b90
bool C_joy_effect::Start(uint iterations, bool solo)
{
    if (type == JOY_EFFECT_NONE)
        return false;
    if (type == JOY_EFFECT_FILE) {
        S_file_effect *file_effect;
        int count = 0;
        for (file_effect = file_effects; file_effect; file_effect = file_effect->next, count++) {
            if (FAILED(file_effect->effect->Download())) {
                for (file_effect = file_effects; count; count--) {
                    file_effect->effect->Unload();
                    file_effect = file_effect->next;
                }
                return false;
            }
        }
        count = 0;
        for (file_effect = file_effects; file_effect; file_effect = file_effect->next, count++) {
            if (FAILED(file_effect->effect->Start(1, 0))) {
                for (file_effect = file_effects; count; count--) {
                    file_effect->effect->Unload();
                    file_effect = file_effect->next;
                }
                return false;
            }
        }
    } else {
        if (!iterations)
            iterations = 1;
        if (FAILED(effect->Start(iterations, solo ? DIES_SOLO : 0)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x10004c70
bool C_joy_effect::Stop(bool unload)
{
    if (type == JOY_EFFECT_NONE)
        return false;
    if (type == JOY_EFFECT_FILE) {
        for (S_file_effect *file_effect = file_effects; file_effect; file_effect = file_effect->next) {
            if (FAILED(file_effect->effect->Stop()))
                return false;
            if (unload)
                file_effect->effect->Unload();
        }
        return true;
    }
    if (FAILED(effect->Stop()))
        return false;
    if (unload)
        effect->Unload();
    return true;
}

// FUNCTION: IJOY 0x10004cf0
bool C_joy_effect::SetDirection(float direction, bool update)
{
    if (type == JOY_EFFECT_FILE)
        return false;
    if (type == JOY_EFFECT_NONE)
        return false;
    this->direction[0] = (LONG)((direction * 18000.0f) * (1.0f / PI));
    parameters.dwSize = sizeof(DIEFFECT);
    parameters.dwFlags = DIEFF_OBJECTOFFSETS | DIEFF_POLAR;
    parameters.cAxes = num_effect_axes;
    parameters.rgdwAxes = effect_axes;
    parameters.rglDirection = this->direction;
    if (update) {
        if (!(effect_info[type].info.dwDynamicParams & DIEP_DIRECTION)
            || FAILED(effect->SetParameters(&parameters, DIEP_DIRECTION | DIEP_NORESTART)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x10004d90
bool C_joy_effect::ApplyDirection()
{
    if (type == JOY_EFFECT_FILE || type == JOY_EFFECT_NONE)
        return false;
    effect->Unload();
    if (effect_info[type].info.dwStaticParams & DIEP_DIRECTION)
        return SUCCEEDED(effect->SetParameters(&parameters, DIEP_DIRECTION | DIEP_NODOWNLOAD)) ? true : false;
    return false;
}

// FUNCTION: IJOY 0x10004de0
bool C_joy_effect::SetGain(float gain, bool update)
{
    if (type == JOY_EFFECT_FILE)
        return false;
    if (type == JOY_EFFECT_NONE)
        return false;
    if (gain > 1.0f)
        gain = 1.0f;
    else if (gain < 0.0f)
        gain = 0.0f;
    parameters.dwSize = sizeof(DIEFFECT);
    parameters.dwGain = (uint)(gain * 10000.0f);
    if (update) {
        if (!(effect_info[type].info.dwDynamicParams & DIEP_GAIN)
            || FAILED(effect->SetParameters(&parameters, DIEP_GAIN | DIEP_NORESTART)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x10004e90
bool C_joy_effect::ApplyGain()
{
    if (type == JOY_EFFECT_FILE || type == JOY_EFFECT_NONE)
        return false;
    effect->Unload();
    if (effect_info[type].info.dwStaticParams & DIEP_GAIN)
        return SUCCEEDED(effect->SetParameters(&parameters, DIEP_GAIN | DIEP_NODOWNLOAD)) ? true : false;
    return false;
}

// FUNCTION: IJOY 0x10004ee0
bool C_joy_effect::SetDuration(uint duration, bool update)
{
    if (type == JOY_EFFECT_FILE)
        return false;
    if (type == JOY_EFFECT_NONE)
        return false;
    parameters.dwSize = sizeof(DIEFFECT);
    if (!duration) {
        // A ramp needs a duration
        if (type != JOY_EFFECT_RAMP_FORCE)
            parameters.dwDuration = INFINITE;
        else
            return false;
    } else
        parameters.dwDuration = duration;
    if (update) {
        if (!(effect_info[type].info.dwDynamicParams & DIEP_DURATION)
            || FAILED(effect->SetParameters(&parameters, DIEP_DURATION | DIEP_NORESTART)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x10004f60
bool C_joy_effect::ApplyDuration()
{
    if (type == JOY_EFFECT_FILE || type == JOY_EFFECT_NONE)
        return false;
    effect->Unload();
    if (effect_info[type].info.dwStaticParams & DIEP_DURATION)
        return SUCCEEDED(effect->SetParameters(&parameters, DIEP_DURATION | DIEP_NODOWNLOAD)) ? true : false;
    return false;
}

// FUNCTION: IJOY 0x10004fb0
bool C_joy_effect::SetEnvelope(float attack_level, uint attack_time, float fade_level, uint fade_time, bool update)
{
    if (type == JOY_EFFECT_FILE)
        return false;
    if (type == JOY_EFFECT_NONE)
        return false;
    if (attack_level > 1.0f)
        attack_level = 1.0f;
    else if (attack_level < 0.0f)
        attack_level = 0.0f;
    if (fade_level > 1.0f)
        fade_level = 1.0f;
    else if (fade_level < 0.0f)
        fade_level = 0.0f;
    parameters.dwSize = sizeof(DIEFFECT);
    parameters.lpEnvelope = &envelope;
    envelope.dwSize = sizeof(DIENVELOPE);
    envelope.dwAttackLevel = (uint)(attack_level * 10000.0f);
    envelope.dwAttackTime = attack_time;
    envelope.dwFadeLevel = (uint)(fade_level * 10000.0f);
    envelope.dwFadeTime = fade_time;
    if (update) {
        if (!(effect_info[type].info.dwDynamicParams & DIEP_ENVELOPE)
            || FAILED(effect->SetParameters(&parameters, DIEP_ENVELOPE | DIEP_NORESTART)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x100050c0
bool C_joy_effect::ApplyEnvelope()
{
    if (type == JOY_EFFECT_FILE || type == JOY_EFFECT_NONE)
        return false;
    effect->Unload();
    if (effect_info[type].info.dwStaticParams & DIEP_ENVELOPE)
        return SUCCEEDED(effect->SetParameters(&parameters, DIEP_ENVELOPE | DIEP_NODOWNLOAD)) ? true : false;
    return false;
}

// FUNCTION: IJOY 0x10005110
bool C_joy_effect::SetConstantForce(float magnitude, bool update)
{
    if (type == JOY_EFFECT_FILE)
        return false;
    if (type == JOY_EFFECT_NONE)
        return false;
    if (type != JOY_EFFECT_CONSTANT_FORCE)
        return false;
    if (magnitude > 1.0f)
        magnitude = 1.0f;
    else if (magnitude < -1.0f)
        magnitude = -1.0f;
    constant_force.lMagnitude = (LONG)(magnitude * 10000.0f);
    parameters.lpvTypeSpecificParams = &constant_force;
    parameters.dwSize = sizeof(DIEFFECT);
    parameters.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    if (update) {
        if (!(effect_info[JOY_EFFECT_CONSTANT_FORCE].info.dwDynamicParams & DIEP_TYPESPECIFICPARAMS)
            || FAILED(effect->SetParameters(&parameters, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x100051c0
bool C_joy_effect::ApplyTypeSpecific()
{
    if (type == JOY_EFFECT_FILE || type == JOY_EFFECT_NONE)
        return false;
    effect->Unload();
    if (effect_info[type].info.dwStaticParams & DIEP_TYPESPECIFICPARAMS)
        return SUCCEEDED(effect->SetParameters(&parameters, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD)) ? true : false;
    return false;
}

// FUNCTION: IJOY 0x10005210
bool C_joy_effect::SetRampForce(float start, float end, bool update)
{
    if (type == JOY_EFFECT_FILE)
        return false;
    if (type == JOY_EFFECT_NONE)
        return false;
    if (type != JOY_EFFECT_RAMP_FORCE)
        return false;
    if (start > 1.0f)
        start = 1.0f;
    else if (start < -1.0f)
        start = -1.0f;
    if (end > 1.0f)
        end = 1.0f;
    else if (end < -1.0f)
        end = -1.0f;
    ramp_force.lStart = (LONG)(start * 10000.0f);
    ramp_force.lEnd = (LONG)(end * 10000.0f);
    parameters.lpvTypeSpecificParams = &ramp_force;
    parameters.dwSize = sizeof(DIEFFECT);
    parameters.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
    if (update) {
        if (!(effect_info[JOY_EFFECT_RAMP_FORCE].info.dwDynamicParams & DIEP_TYPESPECIFICPARAMS)
            || FAILED(effect->SetParameters(&parameters, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x10005310
bool C_joy_effect::SetPeriodic(float magnitude, float offset, float invert, uint period, bool update)
{
    if (type == JOY_EFFECT_FILE)
        return false;
    if (type == JOY_EFFECT_NONE)
        return false;
    if (type != JOY_EFFECT_SQUARE && type != JOY_EFFECT_SINE && type != JOY_EFFECT_TRIANGLE
        && type != JOY_EFFECT_SAWTOOTH_UP && type != JOY_EFFECT_SAWTOOTH_DOWN)
        return false;
    uint phase = (uint)invert;
    if (phase)
        phase = type == JOY_EFFECT_SINE ? 9000 : 18000;
    if (magnitude > 1.0f)
        magnitude = 1.0f;
    else if (magnitude < 0.0f)
        magnitude = 0.0f;
    if (offset > 1.0f)
        offset = 1.0f;
    else if (offset < -1.0f)
        offset = -1.0f;
    periodic.dwMagnitude = (uint)(magnitude * 10000.0f);
    periodic.lOffset = (LONG)(offset * 10000.0f);
    periodic.dwPhase = phase;
    periodic.dwPeriod = period;
    parameters.lpvTypeSpecificParams = &periodic;
    parameters.dwSize = sizeof(DIEFFECT);
    parameters.cbTypeSpecificParams = sizeof(DIPERIODIC);
    if (update) {
        if (!(effect_info[type].info.dwDynamicParams & DIEP_TYPESPECIFICPARAMS)
            || FAILED(effect->SetParameters(&parameters, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x10005470
bool C_joy_effect::SetCondition(const S_joy_condition *condition_0, const S_joy_condition *condition_1, bool update)
{
    // The conditions last given, clamped. A local static: VC6 orders uninitialized data by a hash of the name, which
    // for a local static is the decorated name, and only that puts it after leak_detector. Annotated in joy.h.
    static S_joy_condition conditions[2];
    if (type == JOY_EFFECT_FILE)
        return false;
    if (type == JOY_EFFECT_NONE)
        return false;
    if (type != JOY_EFFECT_SPRING && type != JOY_EFFECT_DAMPER && type != JOY_EFFECT_FRICTION
        && type != JOY_EFFECT_INERTIA)
        return false;
    conditions[0] = *condition_0;
    conditions[1] = *condition_1;
    if (conditions[0].offset > 1.0f)
        conditions[0].offset = 1.0f;
    else if (conditions[0].offset < -1.0f)
        conditions[0].offset = -1.0f;
    if (conditions[0].positive_coefficient > 1.0f)
        conditions[0].positive_coefficient = 1.0f;
    else if (conditions[0].positive_coefficient < -1.0f)
        conditions[0].positive_coefficient = -1.0f;
    if (conditions[0].negative_coefficient > 1.0f)
        conditions[0].negative_coefficient = 1.0f;
    else if (conditions[0].negative_coefficient < -1.0f)
        conditions[0].negative_coefficient = -1.0f;
    if (conditions[0].positive_saturation > 1.0f)
        conditions[0].positive_saturation = 1.0f;
    else if (conditions[0].positive_saturation < 0.0f)
        conditions[0].positive_saturation = 0.0f;
    if (conditions[0].negative_saturation > 1.0f)
        conditions[0].negative_saturation = 1.0f;
    else if (conditions[0].negative_saturation < 0.0f)
        conditions[0].negative_saturation = 0.0f;
    if (conditions[0].dead_band > 1.0f)
        conditions[0].dead_band = 1.0f;
    else if (conditions[0].dead_band < 0.0f)
        conditions[0].dead_band = 0.0f;
    if (conditions[1].offset > 1.0f)
        conditions[1].offset = 1.0f;
    else if (conditions[1].offset < -1.0f)
        conditions[1].offset = -1.0f;
    if (conditions[1].positive_coefficient > 1.0f)
        conditions[1].positive_coefficient = 1.0f;
    else if (conditions[1].positive_coefficient < -1.0f)
        conditions[1].positive_coefficient = -1.0f;
    if (conditions[1].negative_coefficient > 1.0f)
        conditions[1].negative_coefficient = 1.0f;
    else if (conditions[1].negative_coefficient < -1.0f)
        conditions[1].negative_coefficient = -1.0f;
    if (conditions[1].positive_saturation > 1.0f)
        conditions[1].positive_saturation = 1.0f;
    else if (conditions[1].positive_saturation < 0.0f)
        conditions[1].positive_saturation = 0.0f;
    if (conditions[1].negative_saturation > 1.0f)
        conditions[1].negative_saturation = 1.0f;
    else if (conditions[1].negative_saturation < 0.0f)
        conditions[1].negative_saturation = 0.0f;
    if (conditions[1].dead_band > 1.0f)
        conditions[1].dead_band = 1.0f;
    else if (conditions[1].dead_band < 0.0f)
        conditions[1].dead_band = 0.0f;
    condition[0].lOffset = (LONG)(conditions[0].offset * 10000.0f);
    condition[0].lPositiveCoefficient = (LONG)(conditions[0].positive_coefficient * 10000.0f);
    condition[0].lNegativeCoefficient = (LONG)(conditions[0].negative_coefficient * 10000.0f);
    condition[0].dwPositiveSaturation = (uint)(conditions[0].positive_saturation * 10000.0f);
    condition[0].dwNegativeSaturation = (uint)(conditions[0].negative_saturation * 10000.0f);
    condition[0].lDeadBand = (LONG)(conditions[0].dead_band * 10000.0f);
    condition[1].lOffset = (LONG)(conditions[1].offset * 10000.0f);
    condition[1].lPositiveCoefficient = (LONG)(conditions[1].positive_coefficient * 10000.0f);
    condition[1].lNegativeCoefficient = (LONG)(conditions[1].negative_coefficient * 10000.0f);
    condition[1].dwPositiveSaturation = (uint)(conditions[1].positive_saturation * 10000.0f);
    condition[1].dwNegativeSaturation = (uint)(conditions[1].negative_saturation * 10000.0f);
    condition[1].lDeadBand = (LONG)(conditions[1].dead_band * 10000.0f);
    parameters.cbTypeSpecificParams = sizeof(condition);
    parameters.dwSize = sizeof(DIEFFECT);
    parameters.lpvTypeSpecificParams = condition;
    if (update) {
        if (!(effect_info[type].info.dwDynamicParams & DIEP_TYPESPECIFICPARAMS)
            || FAILED(effect->SetParameters(&parameters, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART)))
            return false;
    }
    return true;
}

// FUNCTION: IJOY 0x10005920
bool C_joy_effect::GetEffectCaps(S_joy_effect_caps *caps, int type)
{
    if (!caps)
        return false;
    // The size of the pointer: only the first four members are cleared
    memset(caps, 0, sizeof(caps));
    if (this->type == JOY_EFFECT_FILE || type >= num_effect_info || type < 0)
        return false;
    caps->supported = effect_info[type].supported;
    if (!caps->supported)
        return false;
    caps->dead_band = (effect_info[type].info.dwEffType & DIEFT_DEADBAND) != 0;
    caps->saturation = (effect_info[type].info.dwEffType & DIEFT_SATURATION) != 0;
    caps->pos_neg_saturation = (effect_info[type].info.dwEffType & DIEFT_POSNEGSATURATION) != 0;
    caps->pos_neg_coefficients = (effect_info[type].info.dwEffType & DIEFT_POSNEGCOEFFICIENTS) != 0;
    caps->static_direction = (effect_info[type].info.dwStaticParams & DIEP_DIRECTION) != 0;
    caps->static_duration = (effect_info[type].info.dwStaticParams & DIEP_DURATION) != 0;
    caps->static_envelope = (effect_info[type].info.dwStaticParams & DIEP_ENVELOPE) != 0;
    caps->static_gain = (effect_info[type].info.dwStaticParams & DIEP_GAIN) != 0;
    caps->static_type_specific = (effect_info[type].info.dwStaticParams & DIEP_TYPESPECIFICPARAMS) != 0;
    caps->dynamic_direction = (effect_info[type].info.dwDynamicParams & DIEP_DIRECTION) != 0;
    caps->dynamic_duration = (effect_info[type].info.dwDynamicParams & DIEP_DURATION) != 0;
    caps->dynamic_envelope = (effect_info[type].info.dwDynamicParams & DIEP_ENVELOPE) != 0;
    caps->dynamic_gain = (effect_info[type].info.dwDynamicParams & DIEP_GAIN) != 0;
    caps->dynamic_type_specific = (effect_info[type].info.dwDynamicParams & DIEP_TYPESPECIFICPARAMS) != 0;
    return true;
}

// FUNCTION: IJOY 0x10005a60
bool C_joy_effect::GetStatus(uint *status)
{
    if (!status || !effect)
        return false;
    *status = 0;
    DWORD effect_status = 0;
    if (type == JOY_EFFECT_FILE) {
        bool emulated = false;
        bool playing = false;
        for (S_file_effect *file_effect = file_effects; file_effect; file_effect = file_effect->next) {
            if (FAILED(effect->GetEffectStatus(&effect_status)))
                return false;
            if (effect_status & DIEGES_PLAYING)
                playing = true;
            if (effect_status & DIEGES_EMULATED)
                emulated = true;
        }
        if (playing)
            *status |= JOY_EFFECT_PLAYING;
        if (emulated)
            *status |= JOY_EFFECT_EMULATED;
    } else {
        if (FAILED(effect->GetEffectStatus(&effect_status)))
            return false;
        if (effect_status & DIEGES_PLAYING)
            *status |= JOY_EFFECT_PLAYING;
        if (effect_status & DIEGES_EMULATED)
            *status |= JOY_EFFECT_EMULATED;
    }
    return true;
}

// FUNCTION: IJOY 0x10005b30
int C_joy_effect::GetType()
{
    return type;
}

// FUNCTION: IJOY 0x10005b40
BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCEA instance, LPVOID context)
{
    S_enum_context *enum_context = (S_enum_context *)context;
    enum_context->ok = false;
    S_device *device_info = new S_device;
    memset(device_info, 0, sizeof(S_device));
    device_info->instance = new DIDEVICEINSTANCEA;
    *device_info->instance = *instance;
    IDirectInputDevice7A *device;
    if (FAILED(enum_context->direct_input->CreateDeviceEx(instance->guidInstance, IID_IDirectInputDevice7A,
            (void **)&device, NULL))) {
        OutputDebugString("Failed to create device.\n");
        return DIENUM_STOP;
    }
    device->SetDataFormat(&c_dfDIJoystick2);
    device_info->device = device;
    if (!device)
        return DIENUM_STOP;
    device_info->next = enum_context->devices;
    if (device_info->next) {
        while (device_info->next->next)
            device_info->next = device_info->next->next;
        device_info->next->next = device_info;
    } else
        enum_context->devices = device_info;
    device_info->next = NULL;
    enum_context->ok = true;
    return DIENUM_CONTINUE;
}

// FUNCTION: IJOY 0x10005c20
BOOL CALLBACK EnumObjectsCallback(LPCDIDEVICEOBJECTINSTANCEA instance, LPVOID context)
{
    S_device *device_info = (S_device *)context;
    device_info->ok = false;
    S_object *object = new S_object;
    if (!object)
        return DIENUM_STOP;
    object->instance = new DIDEVICEOBJECTINSTANCEA;
    if (!object->instance)
        return DIENUM_STOP;
    *object->instance = *instance;
    object->unused_1 = 0;
    object->unused_2 = 0;
    object->next = device_info->objects;
    if (object->next) {
        while (object->next->next)
            object->next = object->next->next;
        object->next->next = object;
    } else
        device_info->objects = object;
    object->next = NULL;
    device_info->ok = true;
    return DIENUM_CONTINUE;
}

// FUNCTION: IJOY 0x10005cd0
BOOL CALLBACK EnumEffectsCallback(LPCDIEFFECTINFOA info, LPVOID context)
{
    S_effect_info *effects = (S_effect_info *)context;
    if (IsEqualGUID(info->guid, GUID_ConstantForce)) {
        effects[JOY_EFFECT_CONSTANT_FORCE].info = *info;
        effects[JOY_EFFECT_CONSTANT_FORCE].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_RampForce)) {
        effects[JOY_EFFECT_RAMP_FORCE].info = *info;
        effects[JOY_EFFECT_RAMP_FORCE].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_Square)) {
        effects[JOY_EFFECT_SQUARE].info = *info;
        effects[JOY_EFFECT_SQUARE].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_Sine)) {
        effects[JOY_EFFECT_SINE].info = *info;
        effects[JOY_EFFECT_SINE].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_Triangle)) {
        effects[JOY_EFFECT_TRIANGLE].info = *info;
        effects[JOY_EFFECT_TRIANGLE].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_SawtoothUp)) {
        effects[JOY_EFFECT_SAWTOOTH_UP].info = *info;
        effects[JOY_EFFECT_SAWTOOTH_UP].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_SawtoothDown)) {
        effects[JOY_EFFECT_SAWTOOTH_DOWN].info = *info;
        effects[JOY_EFFECT_SAWTOOTH_DOWN].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_Spring)) {
        effects[JOY_EFFECT_SPRING].info = *info;
        effects[JOY_EFFECT_SPRING].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_Damper)) {
        effects[JOY_EFFECT_DAMPER].info = *info;
        effects[JOY_EFFECT_DAMPER].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_Inertia)) {
        effects[JOY_EFFECT_INERTIA].info = *info;
        effects[JOY_EFFECT_INERTIA].supported = true;
    } else if (IsEqualGUID(info->guid, GUID_Friction)) {
        effects[JOY_EFFECT_FRICTION].info = *info;
        effects[JOY_EFFECT_FRICTION].supported = true;
    }
    return DIENUM_CONTINUE;
}

// FUNCTION: IJOY 0x10005f10
BOOL CALLBACK EnumEffectsInFileCallback(LPCDIFILEEFFECT file_effect, LPVOID context)
{
    S_file_context *file_context = (S_file_context *)context;
    S_file_effect *node = new S_file_effect;
    if (!node)
        return DIENUM_STOP;
    if (FAILED(file_context->device->CreateEffect(file_effect->GuidEffect, file_effect->lpDiEffect, &node->effect,
            NULL))) {
        delete node;
        return DIENUM_CONTINUE;
    }
    node->next = file_context->effects;
    file_context->effects = node;
    file_context->num_effects++;
    return DIENUM_CONTINUE;
}

// FUNCTION: IJOY 0x10005f80
C_leak_detector::C_leak_detector()
{
    for (int i = 0; i < 3; i++) {
        refs[i] = 0;
        objects[i] = 0;
    }
    enabled = false;
}

// FUNCTION: IJOY 0x10005fa0
C_leak_detector::~C_leak_detector()
{
    if (enabled) {
        char buffer[128];
        char message[512];
        message[0] = 0;
        if (refs[LEAK_JOY_BASE]) {
            wsprintf(buffer, "Not released:  %d  reference(s) of JoyBase.\n", refs[LEAK_JOY_BASE]);
            lstrcat(message, buffer);
        }
        if (objects[LEAK_JOY_BASE]) {
            wsprintf(buffer, "Not released:  %d  JoyBase(s).\n", objects[LEAK_JOY_BASE]);
            lstrcat(message, buffer);
        }
        if (refs[LEAK_JOY]) {
            wsprintf(buffer, "Not released:  %d  reference(s) of Joy.\n", refs[LEAK_JOY]);
            lstrcat(message, buffer);
        }
        if (objects[LEAK_JOY]) {
            wsprintf(buffer, "Not released:  %d  Joy(s).\n", objects[LEAK_JOY]);
            lstrcat(message, buffer);
        }
        if (refs[LEAK_EFFECT]) {
            wsprintf(buffer, "Not released:  %d  reference(s) of Effect.\n", refs[LEAK_EFFECT]);
            lstrcat(message, buffer);
        }
        if (objects[LEAK_EFFECT]) {
            wsprintf(buffer, "Not released:  %d  Effect(s).\n", objects[LEAK_EFFECT]);
            lstrcat(message, buffer);
        }
        if (lstrlen(message)) {
            MessageBeep(0xffffffff);
            MessageBox(NULL, message, "JoyBase error message", MB_OK);
        }
    }
}
