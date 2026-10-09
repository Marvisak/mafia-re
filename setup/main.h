#pragma once

#include <windows.h>
#include <cstring>
#include <types.h>
#include <settings.h>

#define SETTINGS_MAGIC 0x14

#define BIT_DEPTH_16 0x1
#define BIT_DEPTH_32 0x2
#define MULTISAMPLE_2X_WINDOWED 0x4
#define MULTISAMPLE_4X_WINDOWED 0x8
#define MULTISAMPLE_2X_FULLSCREEN 0x10
#define MULTISAMPLE_4X_FULLSCREEN 0x20
#define MULTISAMPLE_WINDOWED (MULTISAMPLE_2X_WINDOWED | MULTISAMPLE_4X_WINDOWED)
#define MULTISAMPLE_FULLSCREEN (MULTISAMPLE_2X_FULLSCREEN | MULTISAMPLE_4X_FULLSCREEN)
#define MULTISAMPLE_MASK (MULTISAMPLE_WINDOWED | MULTISAMPLE_FULLSCREEN)

struct S_state
{
	HINSTANCE hinst;
	struct
	{
		HWND main;
		HWND display;
		HWND resolution;
		HWND bit_depth;
		HWND antialiasing;
		HWND driver_version;
		HWND vram_size;
		HWND refresh_rate;
	} windows;
};

extern S_state app_state;
extern S_settings settings;
extern int vram_size;
extern bool supports_w_buffer;
