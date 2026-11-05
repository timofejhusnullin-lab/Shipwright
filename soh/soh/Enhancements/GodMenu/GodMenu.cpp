// God Menu mod (F4): one window with the classic cheats.
// Most toggles reuse Ship of Harkinian's own cheats (same settings as Esc -> Enhancements -> Cheats).
// God mode is new: Link takes no damage at all and does not flicker.

#include "GodMenu.h"

#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>

#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/SwitchAge.h"
#include "soh/Enhancements/Atmosphere/Atmosphere.h"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
}

#define CVAR_GOD_MODE_NAME CVAR_GOD_MENU("GodMode")
#define CVAR_GOD_MODE_VALUE CVarGetInteger(CVAR_GOD_MODE_NAME, 0)

// ---------- God mode ----------
static void OnPlayerUpdateGodMode() {
    if (gPlayState == nullptr) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return;
    }
    // A negative timer is the game's own "invulnerable" state (as during a roll): no damage, no flicker.
    player->invincibilityTimer = -2;
    gSaveContext.health = gSaveContext.healthCapacity;
}

static void RegisterGodMode() {
    COND_HOOK(OnPlayerUpdate, CVAR_GOD_MODE_VALUE, OnPlayerUpdateGodMode);
}

static RegisterShipInitFunc initFunc(RegisterGodMode, { CVAR_GOD_MODE_NAME });

// ---------- UI ----------
static void SaveCVars() {
    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

// Checkbox bound to a cheat setting; re-runs the cheat's registration so it applies immediately.
static void CheatToggle(const char* label, const char* cvar, const char* tooltip) {
    bool value = CVarGetInteger(cvar, 0) != 0;
    if (ImGui::Checkbox(label, &value)) {
        CVarSetInteger(cvar, value ? 1 : 0);
        ShipInit::Init(cvar);
        SaveCVars();
    }
    if (tooltip != nullptr && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
}

static int sGodMenuLastFrame = -1;

void GodMenuWindow::DrawElement() {
    ModWindows_KeepCursorFree(sGodMenuLastFrame);

    ImGui::SeparatorText("Survival");
    CheatToggle("God mode", CVAR_GOD_MODE_NAME, "No damage, no knockback flicker, hearts always full.");
    CheatToggle("Infinite magic", CVAR_CHEAT("InfiniteMagic"), nullptr);
    CheatToggle("Infinite ammo", CVAR_CHEAT("InfiniteAmmo"), "Arrows, seeds, bombs, bombchus, sticks, nuts.");
    CheatToggle("Infinite rupees", CVAR_CHEAT("InfiniteMoney"), nullptr);

    ImGui::SeparatorText("Movement");
    CheatToggle("Fly (hold E / L)", CVAR_CHEAT("MoonJumpOnL"), "Hold L (E on keyboard) to rise, release to fall.");
    CheatToggle("No clip", CVAR_CHEAT("NoClip"), "Walk through walls.");
    CheatToggle("Climb everything", CVAR_CHEAT("ClimbEverything"), "Every wall is climbable.");
    CheatToggle("Hookshot everything", CVAR_CHEAT("HookshotEverything"), "Hookshot sticks to any surface.");

    ImGui::SeparatorText("Actions");
    bool saveLoaded = GameInteractor::IsSaveLoaded(true);
    ImGui::BeginDisabled(!saveLoaded);
    if (ImGui::Button("Heal")) {
        gSaveContext.health = gSaveContext.healthCapacity;
        gSaveContext.magic = gSaveContext.magicLevel * MAGIC_NORMAL_METER;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Full hearts and magic.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Switch age")) {
        SwitchAge();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Child <-> adult. Reloads the current area.");
    }
    ImGui::EndDisabled();
    if (!saveLoaded) {
        ImGui::TextDisabled("Load a save file to use actions.");
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Teleport: E + R + Z (needs Debug Mode in Esc -> Dev Tools)");
    ImGui::TextDisabled("F4 - show / hide this window");
}
