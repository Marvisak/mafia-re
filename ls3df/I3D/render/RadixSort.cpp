// The radix sort (PS2 has its functions at the start of render.cpp's code), at 0x100619b0-0x10061cd0 in the original:
// RadixSort32 with its passes inlined. A file of its own, without the engine header: the counts are the first .bss of
// an object before render.cpp's, in VC6's hash order of their PS2 names (count1, count2, count0, count3), the
// zero-initialized buffer after them, and the object has neither a zero vector nor an identity matrix. The name is
// H&D Deluxe's (its Insanity engine's base library has RadixSort.cpp, an earlier relative of this one) and sorts
// between its neighbours'.
// The loops are inline assembly, the Xbox build's out of line but for the indices' loop, which its compiler inlines
// as VC6 does: MakeCountIndicies is inline, the other helpers are plain functions VC6 inlines (/Ob2). Names are the
// PS2 build's but the helpers without a PS2 counterpart (H&D's).
#include "ls3df.h"
#include "I3D/render/RadixSort.h"
#include <algorithm>

// The counts of each byte value of the keys, by byte (PS2 names)
// GLOBAL: LS3DF 0x101c4430
// GLOBAL: LS3DF11 0x101c1540
static uint count0[256];

// GLOBAL: LS3DF 0x101c3c30
// GLOBAL: LS3DF11 0x101c0d40
static uint count1[256];

// GLOBAL: LS3DF 0x101c4030
// GLOBAL: LS3DF11 0x101c1140
static uint count2[256];

// GLOBAL: LS3DF 0x101c4830
// GLOBAL: LS3DF11 0x101c1940
static uint count3[256];

// The second buffer the passes sort into, kept for the next sort, and its size in bytes (PS2 names)
// GLOBAL: LS3DF 0x101c4c30
// GLOBAL: LS3DF11 0x101c1d40
sort_entry_32 *GlobalStorage = NULL;

// GLOBAL: LS3DF 0x101c4c34
// GLOBAL: LS3DF11 0x101c1d44
uint GlobalStorageSize = 0;

// Counts each byte value of the keys, from the last entry down
void MakeCountsOfBytes(const sort_entry_32 *se, int num)
{
    __asm {
        push ecx
        mov esi, se
        mov edi, num
        dec edi
        xor eax, eax
        xor ebx, ebx
    l:
        mov edx, [esi + edi * 8]
        mov ecx, edx
        mov al, dl
        shr ecx, 16
        inc dword ptr [count0 + eax * 4]
        mov bl, cl
        mov al, dh
        inc dword ptr [count2 + ebx * 4]
        shr ecx, 8
        inc dword ptr [count1 + eax * 4]
        inc dword ptr [count3 + ecx * 4]
        dec edi
        jns l
        pop ecx
    }
}

// Makes each value's first index from a byte's counts, clearing the counts
inline void MakeCountIndicies(uint *indx, uint *count)
{
    __asm {
        mov eax, indx
        mov edx, count
        xor edi, edi
        xor esi, esi
        xor ebx, ebx
    l:
        mov [eax + ebx * 4], edi
        add edi, [edx + ebx * 4]
        mov [edx + ebx * 4], esi
        inc bl
        jne l
    }
}

// The passes: src sorted into dst by one byte of the keys, at each value's next index
void sort_0_b(sort_entry_32 *dst, sort_entry_32 *src, uint *indx, uint num)
{
    __asm {
        push ecx
        mov edi, dst
        mov esi, src
        mov edx, indx
        mov ebx, num
        push ebp
        xor eax, eax
    l:
        mov al, [esi]
        mov ebp, [edx + eax * 4]
        inc dword ptr [edx + eax * 4]
        mov ecx, [esi]
        mov [edi + ebp * 8], ecx
        mov ecx, [esi + 4]
        mov [edi + ebp * 8 + 4], ecx
        add esi, 8
        dec ebx
        jne l
        pop ebp
        pop ecx
    }
}

void sort_1_b(sort_entry_32 *dst, sort_entry_32 *src, uint *indx, uint num)
{
    __asm {
        push ecx
        mov edi, dst
        mov esi, src
        mov edx, indx
        mov ebx, num
        push ebp
        xor eax, eax
    l:
        mov al, [esi + 1]
        mov ebp, [edx + eax * 4]
        inc dword ptr [edx + eax * 4]
        mov ecx, [esi]
        mov [edi + ebp * 8], ecx
        mov ecx, [esi + 4]
        mov [edi + ebp * 8 + 4], ecx
        add esi, 8
        dec ebx
        jne l
        pop ebp
        pop ecx
    }
}

void sort_2_b(sort_entry_32 *dst, sort_entry_32 *src, uint *indx, uint num)
{
    __asm {
        push ecx
        mov edi, dst
        mov esi, src
        mov edx, indx
        mov ebx, num
        push ebp
        xor eax, eax
    l:
        mov al, [esi + 2]
        mov ebp, [edx + eax * 4]
        inc dword ptr [edx + eax * 4]
        mov ecx, [esi]
        mov [edi + ebp * 8], ecx
        mov ecx, [esi + 4]
        mov [edi + ebp * 8 + 4], ecx
        add esi, 8
        dec ebx
        jne l
        pop ebp
        pop ecx
    }
}

void sort_3_b(sort_entry_32 *dst, sort_entry_32 *src, uint *indx, uint num)
{
    __asm {
        push ecx
        mov edi, dst
        mov esi, src
        mov edx, indx
        mov ebx, num
        push ebp
        xor eax, eax
    l:
        mov al, [esi + 3]
        mov ebp, [edx + eax * 4]
        inc dword ptr [edx + eax * 4]
        mov ecx, [esi]
        mov [edi + ebp * 8], ecx
        mov ecx, [esi + 4]
        mov [edi + ebp * 8 + 4], ecx
        add esi, 8
        dec ebx
        jne l
        pop ebp
        pop ecx
    }
}

// A byte whose values are all one is skipped. The pointers are swapped through std::swap's references, which keeps VC6
// from writing them into the passes' assembly in place of the parameters: it copies them into the parameters' own
// slots, as the original does. The last byte's test is the signed sort's (128 and 127), though its
// pass sorts it unsigned, as the others: PS2's has both, and so has H&D's, whose last pass flips the sign bit.
// FUNCTION: LS3DF 0x100619b0
// FUNCTION: LS3DF11 0x1005ec20
void RadixSort32(sort_entry_32 *data, int num)
{
    if (!num)
        return;
    uint index[256];
    MakeCountsOfBytes(data, num);
    uint size = num * sizeof(sort_entry_32);
    if (GlobalStorageSize < size) {
        delete[] GlobalStorage;
        GlobalStorage = new sort_entry_32[num];
        GlobalStorageSize = size;
    }
    sort_entry_32 *temp = GlobalStorage;
    sort_entry_32 *src = data, *dst = temp;

    if (count0[0] == num)
        count0[0] = 0;
    else if (count0[0xff] == num)
        count0[0xff] = 0;
    else {
        MakeCountIndicies(index, count0);
        sort_0_b(dst, src, index, num);
        std::swap(src, dst);
    }

    if (count1[0] == num)
        count1[0] = 0;
    else if (count1[0xff] == num)
        count1[0xff] = 0;
    else {
        MakeCountIndicies(index, count1);
        sort_1_b(dst, src, index, num);
        std::swap(src, dst);
    }

    if (count2[0] == num)
        count2[0] = 0;
    else if (count2[0xff] == num)
        count2[0xff] = 0;
    else {
        MakeCountIndicies(index, count2);
        sort_2_b(dst, src, index, num);
        std::swap(src, dst);
    }

    if (count3[0x80] == num)
        count3[0x80] = 0;
    else if (count3[0x7f] == num)
        count3[0x7f] = 0;
    else {
        MakeCountIndicies(index, count3);
        sort_3_b(dst, src, index, num);
        std::swap(src, dst);
    }

    if (src != data)
        memcpy(data, temp, size);
}
