// The import library of RAD Game Tools' binkw32.dll, which nobody has: LINK makes it from these stubs, one for each of
// the DLL's 85 exports. An import's hint is the position of its name among the DLL's sorted exports, so all of them are
// here. The names are __stdcall's (_BinkOpen@8), which a .def file can't give VC6's LIB.

#define P1 int a1
#define P2 P1, int a2
#define P3 P2, int a3
#define P4 P3, int a4
#define P5 P4, int a5
#define P6 P5, int a6
#define P7 P6, int a7
#define P8 P7, int a8
#define P9 P8, int a9
#define P10 P9, int a10
#define P11 P10, int a11
#define P12 P11, int a12
#define P13 P12, int a13
#define P14 P13, int a14
#define P15 P14, int a15

#define STUB0(name) __declspec(dllexport) void __stdcall name(void) {}
#define STUB1(name) __declspec(dllexport) void __stdcall name(P1) {}
#define STUB2(name) __declspec(dllexport) void __stdcall name(P2) {}
#define STUB3(name) __declspec(dllexport) void __stdcall name(P3) {}
#define STUB4(name) __declspec(dllexport) void __stdcall name(P4) {}
#define STUB5(name) __declspec(dllexport) void __stdcall name(P5) {}
#define STUB6(name) __declspec(dllexport) void __stdcall name(P6) {}
#define STUB7(name) __declspec(dllexport) void __stdcall name(P7) {}
#define STUB8(name) __declspec(dllexport) void __stdcall name(P8) {}
#define STUB9(name) __declspec(dllexport) void __stdcall name(P9) {}
#define STUB10(name) __declspec(dllexport) void __stdcall name(P10) {}
#define STUB11(name) __declspec(dllexport) void __stdcall name(P11) {}
#define STUB12(name) __declspec(dllexport) void __stdcall name(P12) {}
#define STUB13(name) __declspec(dllexport) void __stdcall name(P13) {}
#define STUB14(name) __declspec(dllexport) void __stdcall name(P14) {}
#define STUB15(name) __declspec(dllexport) void __stdcall name(P15) {}

STUB3(BinkBufferBlit)
STUB3(BinkBufferCheckWinPos)
STUB2(BinkBufferClear)
STUB1(BinkBufferClose)
STUB1(BinkBufferGetDescription)
STUB0(BinkBufferGetError)
STUB1(BinkBufferLock)
STUB4(BinkBufferOpen)
STUB2(BinkBufferSetDirectDraw)
STUB2(BinkBufferSetHWND)
STUB3(BinkBufferSetOffset)
STUB3(BinkBufferSetResolution)
STUB3(BinkBufferSetScale)
STUB1(BinkBufferUnlock)
STUB5(BinkCheckCursor)
STUB1(BinkClose)
STUB1(BinkCloseTrack)
STUB7(BinkCopyToBuffer)
STUB11(BinkCopyToBufferRect)
STUB1(BinkDDSurfaceType)
STUB1(BinkDX8SurfaceType)
STUB1(BinkDoFrame)
STUB0(BinkGetError)
STUB3(BinkGetKeyFrame)
STUB3(BinkGetRealtime)
STUB2(BinkGetRects)
STUB2(BinkGetSummary)
STUB2(BinkGetTrackData)
STUB2(BinkGetTrackID)
STUB2(BinkGetTrackMaxSize)
STUB2(BinkGetTrackType)
STUB3(BinkGoto)
STUB2(BinkIsSoftwareCursor)
STUB0(BinkLogoAddress)
STUB1(BinkNextFrame)
STUB2(BinkOpen)
STUB1(BinkOpenDirectSound)
STUB1(BinkOpenMiles)
STUB2(BinkOpenTrack)
STUB1(BinkOpenWaveOut)
STUB2(BinkPause)
STUB1(BinkRestoreCursor)
STUB1(BinkService)
STUB1(BinkSetError)
STUB2(BinkSetFrameRate)
STUB1(BinkSetIO)
STUB1(BinkSetIOSize)
STUB5(BinkSetMixBinVolumes)
STUB4(BinkSetMixBins)
STUB3(BinkSetPan)
STUB1(BinkSetSimulate)
STUB2(BinkSetSoundOnOff)
STUB2(BinkSetSoundSystem)
STUB2(BinkSetSoundTrack)
STUB2(BinkSetVideoOnOff)
STUB3(BinkSetVolume)
STUB1(BinkWait)
STUB2(RADSetMemory)
STUB0(RADTimerRead)
STUB13(YUV_blit_16a1bpp)
STUB15(YUV_blit_16a1bpp_mask)
STUB13(YUV_blit_16a4bpp)
STUB15(YUV_blit_16a4bpp_mask)
STUB12(YUV_blit_16bpp)
STUB14(YUV_blit_16bpp_mask)
STUB12(YUV_blit_24bpp)
STUB14(YUV_blit_24bpp_mask)
STUB12(YUV_blit_24rbpp)
STUB14(YUV_blit_24rbpp_mask)
STUB13(YUV_blit_32abpp)
STUB15(YUV_blit_32abpp_mask)
STUB12(YUV_blit_32bpp)
STUB14(YUV_blit_32bpp_mask)
STUB13(YUV_blit_32rabpp)
STUB15(YUV_blit_32rabpp_mask)
STUB12(YUV_blit_32rbpp)
STUB14(YUV_blit_32rbpp_mask)
STUB12(YUV_blit_UYVY)
STUB14(YUV_blit_UYVY_mask)
STUB12(YUV_blit_YUY2)
STUB14(YUV_blit_YUY2_mask)
STUB13(YUV_blit_YV12)
STUB1(YUV_init)
STUB1(radfree)
STUB1(radmalloc)
