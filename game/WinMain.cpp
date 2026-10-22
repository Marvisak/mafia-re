// The program's entry point (0x566160-0x5661eb), a file of its own in the original: nothing else lies between the end
// of the file before it and its own std::ctype<unsigned short>::id initializer. The name is ours.

#include "Main.h"

#include <direct.h>
#include <string.h>

// FUNCTION: GAME 0x566160
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // The frame is 0x3ec bytes: 1001 to 1004 give that, GetModuleFileName gets 1000
    char filename[1001];
    GetModuleFileName(hInstance, filename, 1000);
    // The game's files are relative to the executable's folder, wherever it was started from
    char *cp = strrchr(filename, '\\');
    if (cp)
        *cp = 0;
    _chdir(filename);
    main(hInstance);
    return 0;
}
