#pragma once

#include <types.h>

// The engine's radix sort (RadixSort.cpp), which the sort lists (I3D.h's C_sort_list) sort with. Kept apart from the
// engine header: RadixSort.cpp doesn't include it.

// A key and the data it sorts (PS2 name; the members are H&D Deluxe's S_sort_entry's, whose RadixSort32 this is an
// earlier relative of)
struct sort_entry_32 {
    uint sort_value;
    uint data;
};

// Sorts the entries by their keys, ascending, as unsigned (PS2 name)
void RadixSort32(sort_entry_32 *data, int num);
