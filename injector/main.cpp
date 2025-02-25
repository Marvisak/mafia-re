#undef UNICODE

#include <iostream>
#include <windows.h>
#include <TlHelp32.h>

DWORD GetProcId(const char *procName)
{
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry))
        {
            do
            {
                if (!_stricmp(procEntry.szExeFile, procName))
                {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Missing DLL path argument" << std::endl;
        return 1;
    }
    const char *procName = "Game.exe";
    char fullFileName[MAX_PATH];
    GetFullPathName(argv[1], MAX_PATH, fullFileName, nullptr);
    DWORD attrib = GetFileAttributes(fullFileName);
    if (attrib == INVALID_FILE_ATTRIBUTES || attrib & FILE_ATTRIBUTE_DIRECTORY) {
        std::cerr << "DLL doesn't exist" << std::endl;
        return 1;
    }


    DWORD procId = 0;
    while (!procId)
    {
        procId = GetProcId(procName);
        Sleep(1);
    }
    std::cout << "Injecting..." << std::endl;
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);

    if (hProc && hProc != INVALID_HANDLE_VALUE)
    {
        void *loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        WriteProcessMemory(hProc, loc, fullFileName, strlen(fullFileName) + 1, 0);

        HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);

        if (hThread)
        {
            CloseHandle(hThread);
        }
    }

    if (hProc)
    {
        CloseHandle(hProc);
    }
}
