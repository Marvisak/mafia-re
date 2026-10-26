// The loading screen's callback (PS2 cbLoadScene, in PS2's Main.cpp). The PC's file is the one at 0x5bcc10-0x5bcdfc,
// between game.cpp's and Main.cpp's (docs/game-files.md), which has nothing else; its name is a guess, PS2's file of
// the loading screen's other helper (_show_progress_, which the PC's C_game::Init has inlined). Written: all of it.

#include "ui/Load_progress.h"
#include "ui/g_m_func.h"
#include "ui/g_loadsave.h"

#include "IGraph/IGraph.h"

// Zero-initialized, so it follows the hashed static (last_bit), as the original's
// GLOBAL: GAME 0x647b20
GM_LoadProgress *menuProgress = NULL;

// PS2's also returns at once for CBM_ERROR, which the PC's scene callback (Mission.cpp's __SceneErrFnc) filters out
// FUNCTION: GAME 0x5bcc10
void __stdcall cbLoadScene(LS3D_CALLBACK_MESSAGE msg, uint prm1, uint prm2, void *context)
{
    static float last_bit;
    float bit;
    if (msg != CBM_PROGRESS && msg != CBM_MISSION_PROGRESS)
        return;
    if (msg == CBM_PROGRESS)
    {
        // The scene's end isn't the bar's
        if (prm2 == LOAD_PROGRESS_END)
            prm2 = LOAD_PROGRESS_STEP;
        bit = *(float *)&prm1 * .5f;
    }
    else
        bit = (*(float *)&prm1 + 1.0f) * .5f;
    switch (prm2)
    {
    case LOAD_PROGRESS_BEGIN:
        // PS2's InitProgressScreen, which PS2's C_mission::Open calls: mafiaLoadSave's loading screen, or one of its own.
        // Written here, not a helper: VC6 inlines no function whose new needs an unwind state for its constructor, not
        // even __forceinline (probed; with a bare operator new the helper inlines), and the original has that state.
        if (!mafiaLoadSave.progress)
        {
            menuProgress = new GM_LoadProgress(0, 1.0f, NULL);
            menuProgress->Create();
            menuProgress->Present();
        }
        else
        {
            menuProgress = NULL;
            mafiaLoadSave.progress->Create();
            mafiaLoadSave.progress->Present();
        }
        last_bit = 0.0f;
        break;
    case LOAD_PROGRESS_STEP:
        if (bit - last_bit < .02f)
            break;
        igraph->SetState(DXS_ALPHA, ALPHASTATE_BLEND);
        if (mafiaLoadSave.progress)
        {
            mafiaLoadSave.progress->SetProgressMissionOpen(bit);
            mafiaLoadSave.progress->Present();
        }
        else if (menuProgress)
        {
            menuProgress->SetProgressMissionOpen(bit);
            menuProgress->Present();
        }
        last_bit = bit;
        break;
    case LOAD_PROGRESS_END:
        if (menuProgress)
        {
            menuProgress->Destroy();
            menuProgress = NULL;
        }
        break;
    }
}
