#pragma once

#include "ls3df.h"

// The graphics interface (PS2 IGraph.cpp), as far as the files written so far use it. Its state is in globals (PS2
// m_MainHWnd, m_pD3D8, ...): the object itself is 4 bytes. Placeholders keep the known members at their offsets and
// will make way for the real declarations.

class IGraph {
    uint unknown_00;

public:
    IGraph();
    ~IGraph();
};

// How bad an error is (PS2 type; the value names are guesses). ErrorHandler exits from 3 on.
enum E_SEVERITY {
    ERR_INFO,
    ERR_WARNING,
    ERR_ERROR,
    ERR_FATAL,
};

// What IGraph::Init takes (PS2 name, 0x38 bytes)
struct IGRAPH_INIT_DESC {
    uchar unknown_00[0x34];
    void (__cdecl *error_handler)(LS3D_RESULT result, E_SEVERITY severity, const char *message);   // 0x34
};

// PS2 name
extern IGRAPH_INIT_DESC m_InitDesc;

// Globals.cpp
void ErrorHandler(LS3D_RESULT result, E_SEVERITY severity, const char *message);
