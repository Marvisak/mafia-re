#pragma once

#include <windows.h>
#include <cstring>
#include <types.h>
#include <settings.h>

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
	// In megabytes, from GetTotalVRAM
	int vram_size;

	// 1.2 zeroes dialog_state, vram_size included, in one dynamic initializer
	S_state()
	{
		hinst = NULL;
		windows.main = NULL;
		windows.display = NULL;
		windows.resolution = NULL;
		windows.bit_depth = NULL;
		windows.antialiasing = NULL;
		windows.driver_version = NULL;
		windows.vram_size = NULL;
		windows.refresh_rate = NULL;
		vram_size = 0;
	}
};

extern S_state dialog_state;
extern S_settings settings;
#ifndef SETUP_1_0
// 1.2 added the W-buffer test: 1.0 neither clears the setting nor disables its checkbox
extern bool supports_w_buffer;
#endif

// Error is defined before SetText, but 1.2 has it after DialogFunc, as 1.0 does. Its marker matches by name, here in a
// header, so the linter doesn't check it against main.cpp's order.
// FUNCTION: SETUP 0x402550
// FUNCTION: SETUP10 0x402180
// Error
