#pragma once

#include <windows.h>
#include <types.h>
#include <settings.h>

#define SETTINGS_MAGIC 0x14

#define BIT_DEPTH_16 0x1
#define BIT_DEPTH_32 0x2
#define MULTISAMPLE_2X_WINDOWED 0x4
#define MULTISAMPLE_4X_WINDOWED 0x8
#define MULTISAMPLE_2X_FULLSCREEN 0x10
#define MULTISAMPLE_4X_FULLSCREEN 0x20

// I am not entirely sure if the rest of the values after windows were in the struct
// but it makes the whole thing much more simple so let's just keep it that way
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
	S_settings settings;
	bool settings_loaded;
	uint vram_size;
	bool supports_w_buffer;
};

extern S_state app_state;