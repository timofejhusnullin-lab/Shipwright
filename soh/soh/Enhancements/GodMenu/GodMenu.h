#pragma once

// God Menu mod: Minecraft-style cheat window (F4).

#include "soh/cvar_prefixes.h"

#define CVAR_GOD_MENU(var) CVAR_ENHANCEMENT("GodMenu." var)
#define GOD_MENU_WINDOW_NAME "God Menu"

#ifdef __cplusplus
#include <ship/window/gui/GuiWindow.h>

class GodMenuWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

  protected:
    void InitElement() override {};
    void DrawElement() override;
    void UpdateElement() override {};
};
#endif
