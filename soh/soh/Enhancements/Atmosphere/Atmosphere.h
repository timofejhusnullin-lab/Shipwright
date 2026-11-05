#pragma once

#include "soh/cvar_prefixes.h"

// Atmosphere mod: in-game window (F3) to pick the sky texture set and control time of day.

#define CVAR_ATMOSPHERE(var) CVAR_ENHANCEMENT("Atmosphere." var)
#define ATMOSPHERE_WINDOW_NAME "Atmosphere"

#ifdef __cplusplus
#include <ship/window/gui/GuiWindow.h>

class AtmosphereWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

  protected:
    void InitElement() override {};
    void DrawElement() override;
    void UpdateElement() override {};
};

// Shared by the mod windows (Atmosphere, God Menu): while a window is open, keep the mouse
// released and the cursor visible so the window can be clicked.
// lastFrame: per-window storage (start at -1), used to detect the frame the window was opened.
void ModWindows_KeepCursorFree(int& lastFrame);
// Call after a mod window was closed with its hotkey: hands mouse capture back to the game settings.
void ModWindows_RestoreMouseCapture();

extern "C" {
#endif

// Returns the forced sky texture set index (0..8), or -1 when the override is off.
int Atmosphere_GetSkyOverride(void);

#ifdef __cplusplus
}
#endif
