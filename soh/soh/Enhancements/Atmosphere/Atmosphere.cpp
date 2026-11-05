// Atmosphere mod
// F2 opens a small window where you can:
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

void AtmosphereWindow::DrawElement() {
    // ---------- Sky ----------
    ImGui::SeparatorText("Sky");

    bool overrideOn = CVarGetInteger(CVAR_ATMOSPHERE("SkyOverride"), 0) != 0;
    if (ImGui::Checkbox("Override sky", &overrideOn)) {
        CVarSetInteger(CVAR_ATMOSPHERE("SkyOverride"), overrideOn ? 1 : 0);
        SaveCVars();
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
            CVarSetInteger(CVAR_ATMOSPHERE("SkyIndex"), i);
            SaveCVars();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Textures: textures/%s/  (replace them with a pack in the mods folder)",
                              sSkyOptions[i].textureFolder);
        }
    }
    ImGui::EndDisabled();

    // ---------- Time ----------
    ImGui::SeparatorText("Time of day");

    if (!GameInteractor::IsSaveLoaded(true)) {
        ImGui::TextDisabled("Load a save file to change the time.");
        return;
    }

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

    bool freeze = CVarGetInteger(CVAR_FREEZE_TIME, 0) != 0;
    if (ImGui::Checkbox("Freeze time", &freeze)) {
        CVarSetInteger(CVAR_FREEZE_TIME, freeze ? 1 : 0);
        ShipInit::Init(CVAR_FREEZE_TIME); // re-registers the Freeze Time hook
        SaveCVars();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("F2 - show / hide this window");
}
