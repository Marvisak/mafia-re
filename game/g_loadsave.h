#pragma once

#include "Main.h"

// The saved games, profiles and the game's progress (PS2 G_LoadSaveClass, g_loadsave.cpp). Only what main uses so far.
class G_LoadSaveClass
{
public:
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
