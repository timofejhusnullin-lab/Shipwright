// Atmosphere mod
// F3 opens a small window where you can:
//  - force any of the game's sky texture sets (works in every outdoor area with a normal sky)
//  - change the time of day with a slider
//  - freeze time
// The sky textures themselves can be replaced with your own PNGs via an .o2r pack in the mods folder.

#include "Atmosphere.h"

#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#include <ship/window/MouseStateManager.h>

#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
}

#define CVAR_FREEZE_TIME CVAR_CHEAT("FreezeTime")
#define CVAR_FREEZE_PREV_TIME CVAR_GENERAL("PrevTime")

struct SkyOption {
    const char* label;
    const char* textureFolder; // where the textures live inside the .o2r (useful for making mods)
};

// Order matches sSkyboxTable in src/code/z_vr_box.c
static const SkyOption sSkyOptions[] = {
    { "Sunrise (clear)", "vr_fine0_static" },     { "Day (clear)", "vr_fine1_static" },
    { "Sunset (clear)", "vr_fine2_static" },      { "Night (clear)", "vr_fine3_static" },
    { "Sunrise (overcast)", "vr_cloud0_static" }, { "Day (overcast)", "vr_cloud1_static" },
    { "Sunset (overcast)", "vr_cloud2_static" },  { "Night (overcast)", "vr_cloud3_static" },
    { "Holy / special", "vr_holy0_static" },
};
static const int sSkyOptionCount = sizeof(sSkyOptions) / sizeof(sSkyOptions[0]);

extern "C" int Atmosphere_GetSkyOverride(void) {
    if (!CVarGetInteger(CVAR_ATMOSPHERE("SkyOverride"), 0)) {
        return -1;
    }
    int idx = CVarGetInteger(CVAR_ATMOSPHERE("SkyIndex"), 1);
    if (idx < 0 || idx >= sSkyOptionCount) {
        return -1;
    }
    return idx;
}

static void SaveCVars() {
    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

static void SetDayTime(uint16_t time) {
    gSaveContext.dayTime = time;
    gSaveContext.skyboxTime = time;
    // The Freeze Time cheat keeps restoring a stored time, so update it too.
    if (CVarGetInteger(CVAR_FREEZE_TIME, 0)) {
        CVarSetInteger(CVAR_FREEZE_PREV_TIME, time);
    }
}

// ---------- Light follows sky ----------
// The game's lighting, fog and ambient colour come from the time of day. When a sky is forced,
// lock the time to one that matches it, so the ground is lit like the sky looks.
#define CVAR_MATCH_LIGHT CVAR_ATMOSPHERE("MatchLight")

static uint16_t SkyIndexToTime(int idx) {
    switch (idx % 4) {
        case 0:
            return 0x3AAA; // 05:30 sunrise
        case 1:
            return 0x8000; // 12:00 day
        case 2:
            return 0xBAAA; // 17:30 sunset
        default:
            return 0x0000; // 00:00 night (also used for the dark "Holy" sky)
    }
}

static bool LightFollowsSky() {
    return Atmosphere_GetSkyOverride() >= 0 && CVarGetInteger(CVAR_MATCH_LIGHT, 1) != 0;
}

static void OnGameFrameUpdateMatchLight() {
    if (!GameInteractor::IsSaveLoaded(true)) {
        return;
    }
    int sky = Atmosphere_GetSkyOverride();
    if (sky < 0) {
        return;
    }
    uint16_t target = SkyIndexToTime(sky == 8 ? 3 : sky);
    if (gSaveContext.dayTime != target) {
        SetDayTime(target);
    }
}

static void RegisterMatchLight() {
    COND_HOOK(OnGameFrameUpdate, LightFollowsSky(), OnGameFrameUpdateMatchLight);
}

static RegisterShipInitFunc sMatchLightInit(RegisterMatchLight, { CVAR_MATCH_LIGHT, CVAR_ATMOSPHERE("SkyOverride"),
                                                                  CVAR_ATMOSPHERE("SkyIndex") });

static void SetAtmosphereCVar(const char* cvar, int value) {
    CVarSetInteger(cvar, value);
    ShipInit::Init(cvar);
    SaveCVars();
}

void ModWindows_KeepCursorFree(int& lastFrame) {
    auto wnd = Ship::Context::GetRawInstance()->GetWindow();
    int frame = ImGui::GetFrameCount();
    bool justOpened = lastFrame != frame - 1;
    lastFrame = frame;
    if (wnd->IsMouseCaptured()) {
        wnd->SetMouseCapture(false); // also shows the cursor
    } else if (justOpened) {
        wnd->SetCursorVisibility(true); // only once: ShowCursor is counter-based on Windows
    }
    // Keep the idle timer from hiding the cursor while the window is open
    wnd->GetMouseStateManager()->ResetCursorVisibilityTimer();
}

void ModWindows_RestoreMouseCapture() {
    Ship::Context::GetRawInstance()->GetWindow()->GetMouseStateManager()->UpdateMouseCapture();
}

static int sAtmosphereLastFrame = -1;

void AtmosphereWindow::DrawElement() {
    ModWindows_KeepCursorFree(sAtmosphereLastFrame);
    // ---------- Sky ----------
    ImGui::SeparatorText("Sky");

    bool overrideOn = CVarGetInteger(CVAR_ATMOSPHERE("SkyOverride"), 0) != 0;
    if (ImGui::Checkbox("Override sky", &overrideOn)) {
        SetAtmosphereCVar(CVAR_ATMOSPHERE("SkyOverride"), overrideOn ? 1 : 0);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Works in outdoor areas that have a normal sky (Hyrule Field, Lake Hylia, etc).\n"
                          "Indoor areas and dungeons have no sky to replace.");
    }

    int current = CVarGetInteger(CVAR_ATMOSPHERE("SkyIndex"), 1);
    if (current < 0 || current >= sSkyOptionCount) {
        current = 1;
    }

    ImGui::BeginDisabled(!overrideOn);
    for (int i = 0; i < sSkyOptionCount; i++) {
        if (ImGui::RadioButton(sSkyOptions[i].label, current == i)) {
            SetAtmosphereCVar(CVAR_ATMOSPHERE("SkyIndex"), i);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Textures: textures/%s/  (replace them with a pack in the mods folder)",
                              sSkyOptions[i].textureFolder);
        }
    }
    bool matchLight = CVarGetInteger(CVAR_MATCH_LIGHT, 1) != 0;
    if (ImGui::Checkbox("Light follows sky", &matchLight)) {
        SetAtmosphereCVar(CVAR_MATCH_LIGHT, matchLight ? 1 : 0);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Locks the time of day to the chosen sky, so lighting, fog and shadows match it\n"
                          "(night sky = night light, sunset sky = sunset light).");
    }
    ImGui::EndDisabled();

    // ---------- Time ----------
    ImGui::SeparatorText("Time of day");

    if (!GameInteractor::IsSaveLoaded(true)) {
        ImGui::TextDisabled("Load a save file to change the time.");
        return;
    }

    bool timeLocked = LightFollowsSky();
    if (timeLocked) {
        ImGui::TextDisabled("Time follows the chosen sky.");
    }
    ImGui::BeginDisabled(timeLocked);
    float hours = gSaveContext.dayTime * 24.0f / 65536.0f;
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::SliderFloat("##hours", &hours, 0.0f, 23.99f, "%.2f h")) {
        SetDayTime((uint16_t)(hours / 24.0f * 65536.0f));
    }
    ImGui::SameLine();
    int h = (int)hours;
    int m = (int)((hours - h) * 60.0f);
    ImGui::Text("%02d:%02d", h, m);

    struct Preset {
        const char* label;
        float hour;
    };
    static const Preset presets[] = { { "Dawn", 5.5f }, { "Noon", 12.0f }, { "Sunset", 17.5f }, { "Night", 0.0f } };
    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            ImGui::SameLine();
        }
        if (ImGui::Button(presets[i].label)) {
            SetDayTime((uint16_t)(presets[i].hour / 24.0f * 65536.0f));
        }
    }

    ImGui::EndDisabled();

    bool freeze = CVarGetInteger(CVAR_FREEZE_TIME, 0) != 0;
    if (ImGui::Checkbox("Freeze time", &freeze)) {
        CVarSetInteger(CVAR_FREEZE_TIME, freeze ? 1 : 0);
        ShipInit::Init(CVAR_FREEZE_TIME); // re-registers the Freeze Time hook
        SaveCVars();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("F3 - show / hide this window");
}
