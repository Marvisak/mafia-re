#pragma once

#include "Main.h"

// The saved games, profiles and the game's progress (PS2 G_LoadSaveClass, g_loadsave.cpp), at least 0x5bc bytes (one of
// its methods clears 0x520 to 0x5bb). Only what main and the camera use so far.
class G_LoadSaveClass
{
public:
    uchar unknown_0[0x4f4];             // 0x0
    // A profile's value, 1 to 3 (the profile's load at 0x60e0fe resets others to 1): the camera G_Camera::Reset takes,
    // the number of an entry of the camera's cameraParams (where PS2's Reset takes 1)
    uint unknown_4f4;                   // 0x4f4
    // The same for 7 to 15, 7 otherwise
    uint unknown_4f8;                   // 0x4f8
    uchar unknown_4fc[0x5bc - 0x4fc];   // 0x4fc


    // Sets up the profiles and the saved games at the start; false is fatal
    bool Init();
    // Saves the profile and frees the game's tables, at CloseSystem
    void Close();
    // The player chose another profile; true to restart the menu with it
    bool ProfileChange();
    void SaveGameNewGame();
    // Loads the saved game the player chose and opens its mission
    void SaveGameLoad();
    // Plays tables\credits.def
    void ExecuteCredits();
    void FreerideSetup();
    // Finds Tommy and his car in the free ride's mission
    void FreerideSetupAfter();
    // The game's language, two lowercase letters as a multicharacter constant ('en')
    void LanguageSelect(uint language);
};

extern G_LoadSaveClass mafiaLoadSave;
