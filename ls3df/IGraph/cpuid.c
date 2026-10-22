// The CPU identification (PC only, 0x1006ad40-0x1006b430 in the original): the original's only C object. Its first
// three functions are Microsoft's CPUID sample from the VC6 Processor Pack (an early version: no Pentium 4, no OS
// support test), so they have the sample's names; the file's own name isn't known. GetCPUSpeed is the engine's.
#include <windows.h>
#include <mmsystem.h>
#include <string.h>

// What _cpuid returns: the features found
#define CPU_FEATURE_MMX 0x0001
#define CPU_FEATURE_SSE 0x0002
#define CPU_FEATURE_SSE2 0x0004
#define CPU_FEATURE_3DNOW 0x0008

// The sample's _p_info, which IGraph.h declares again for the C++ files
typedef struct {
    char v_name[13];                    // "GenuineIntel", "AuthenticAMD"
    char model_name[30];
    int family;
    int model;
    int stepping;
    int feature;                        // CPU_FEATURE_*
    int checks;                         // the features tested for
} _p_info;

// Whether the CPU has the cpuid instruction
// FUNCTION: LS3DF 0x1006ad40
// FUNCTION: LS3DF11 0x10067dd0
BOOL IsCPUID()
{
    __try {
        __asm {
            xor eax, eax
            cpuid
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
    return TRUE;
}

// The model's name: "Unknown" for a model of a known family, nothing for an unknown family
// FUNCTION: LS3DF 0x1006adc0
// FUNCTION: LS3DF11 0x10067e50
void map_mname(int family, int model, const char *v_name, char *m_name)
{
    if (!strncmp("AuthenticAMD", v_name, 12)) {
        switch (family) {
        case 4:
            strcpy(m_name, "AMD Am486");
            break;
        case 5:
            switch (model) {
            case 0:
            case 1:
            case 2:
            case 3:
                strcpy(m_name, "AMD K5");
                break;
            case 4:
            case 5:
                break;
            case 6:
            case 7:
                strcpy(m_name, "AMD K6");
                break;
            case 8:
                strcpy(m_name, "AMD K6-2");
                break;
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
                strcpy(m_name, "AMD K6-3");
                break;
            default:
                strcpy(m_name, "Unknown");
            }
            break;
        case 6:
            strcpy(m_name, "AMD ATHLON");
            break;
        }
    } else if (!strncmp("GenuineIntel", v_name, 12)) {
        switch (family) {
        case 4:
            switch (model) {
            case 0:
            case 1:
                strcpy(m_name, "INTEL 486DX");
                break;
            case 2:
                strcpy(m_name, "INTEL 486SX");
                break;
            case 3:
                strcpy(m_name, "INTEL 486DX2");
                break;
            case 4:
                strcpy(m_name, "INTEL 486SL");
                break;
            case 5:
                strcpy(m_name, "INTEL 486SX2");
                break;
            case 7:
                strcpy(m_name, "INTEL 486DX2E");
                break;
            case 8:
                strcpy(m_name, "INTEL 486DX4");
                break;
            default:
                strcpy(m_name, "Unknown");
            }
            break;
        case 5:
            switch (model) {
            case 1:
            case 2:
            case 3:
                strcpy(m_name, "INTEL Pentium");
                break;
            case 4:
                strcpy(m_name, "INTEL Pentium-MMX");
                break;
            default:
                strcpy(m_name, "Unknown");
            }
            break;
        case 6:
            switch (model) {
            case 1:
                strcpy(m_name, "INTEL Pentium-Pro");
                break;
            case 3:
                strcpy(m_name, "INTEL Pentium-II");
                break;
            case 5:
                strcpy(m_name, "INTEL Pentium-II");
                break;
            case 6:
                strcpy(m_name, "INTEL Celeron");
                break;
            case 7:
                strcpy(m_name, "INTEL Pentium-III");
                break;
            default:
                strcpy(m_name, "Unknown");
            }
            break;
        }
    } else if (!strncmp("CyrixInstead", v_name, 12)) {
        strcpy(m_name, "Unknown");
    } else if (!strncmp("CentaurHauls", v_name, 12)) {
        strcpy(m_name, "Unknown");
    } else {
        strcpy(m_name, "Unknown");
    }
}

// Returns the features and, with pinfo, fills it in
// FUNCTION: LS3DF 0x1006b2a0
// FUNCTION: LS3DF11 0x10068330
int _cpuid(_p_info *pinfo)
{
    DWORD dwStandard = 0;
    DWORD dwFeature = 0;
    DWORD dwMax;
    DWORD dwExt = 0;
    int feature = 0;
    union {
        char cBuf[12 + 1];
        struct {
            DWORD dw0;
            DWORD dw1;
            DWORD dw2;
        } s;
    } Ident;

    if (!IsCPUID())
        return 0;

    __asm {
        push ebx
        push ecx
        push edx

        // the vendor
        xor eax, eax
        cpuid
        mov dwMax, eax
        mov Ident.s.dw0, ebx
        mov Ident.s.dw1, edx
        mov Ident.s.dw2, ecx

        // the standard features
        mov eax, 1
        cpuid
        mov dwStandard, eax
        mov dwFeature, edx

        // AMD's extended features
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000000
        jb no_extended
        mov eax, 0x80000001
        cpuid
        mov dwExt, edx

no_extended:
        // in the sample's order, which swaps ebx and ecx
        pop ecx
        pop ebx
        pop edx
    }

    if (dwFeature & 0x00800000)
        feature |= CPU_FEATURE_MMX;
    if (dwExt & 0x80000000)
        feature |= CPU_FEATURE_3DNOW;
    if (dwFeature & 0x02000000)
        feature |= CPU_FEATURE_SSE;
    if (dwFeature & 0x04000000)
        feature |= CPU_FEATURE_SSE2;

    if (pinfo) {
        memset(pinfo, 0, sizeof(_p_info));
        pinfo->feature = feature;
        pinfo->family = (dwStandard >> 8) & 0xf;
        pinfo->model = (dwStandard >> 4) & 0xf;
        pinfo->stepping = dwStandard & 0xf;
        Ident.cBuf[12] = 0;
        strcpy(pinfo->v_name, Ident.cBuf);
        map_mname(pinfo->family, pinfo->model, pinfo->v_name, pinfo->model_name);
        pinfo->checks = CPU_FEATURE_MMX | CPU_FEATURE_SSE | CPU_FEATURE_SSE2 | CPU_FEATURE_3DNOW;
    }
    return feature;
}

// The CPU's clock in MHz: the time stamp counter's count over a second, read after cpuid has serialized the
// instructions before it (PC only, names guessed)
// FUNCTION: LS3DF 0x1006b3b0
// FUNCTION: LS3DF11 0x10068440
int GetCPUSpeed()
{
    DWORD start = 0, end = 0;
    int t0 = timeGetTime();
    int t1;
    while ((t1 = timeGetTime()) - t0 <= 1)
        ;
    __asm {
        xor eax, eax
        xor ebx, ebx
        xor ecx, ecx
        xor edx, edx
        cpuid
        rdtsc
        mov start, eax
    }
    while ((int)timeGetTime() - t1 <= 1000)
        ;
    __asm {
        xor eax, eax
        xor ebx, ebx
        xor ecx, ecx
        xor edx, edx
        cpuid
        rdtsc
        mov end, eax
    }
    return (end - start) / 1000000;
}
