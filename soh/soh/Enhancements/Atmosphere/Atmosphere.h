#pragma once

#include "soh/cvar_prefixes.h"

// Atmosphere mod: in-game window (F2) to pick the sky texture set and control time of day.

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

extern "C" {
#endif

// Returns the forced sky texture set index (0..8), or -1 when the override is off.
int Atmosphere_GetSkyOverride(void);

#ifdef __cplusplus
}
#endif
