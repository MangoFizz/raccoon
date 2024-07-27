// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/logger.hpp>
#include <balltze/plugin.hpp>
#include "postprocess/postprocess.hpp"

namespace Raccoon {
    Balltze::Logger logger("Raccoon");
}

BALLTZE_PLUGIN_API Balltze::PluginMetadata plugin_metadata() {
    return {
        "Raccoon",
        "MangoFizz",
        { 1, 0, 0 },
        { 1, 0, 0 },
        true
    };
}

BALLTZE_PLUGIN_API bool plugin_init() noexcept {
    Raccoon::PostProcess::set_up_postprocess_effects();
    return true;
}

BALLTZE_PLUGIN_API void plugin_load() noexcept {
    Raccoon::logger.info("Loaded");
}

WINAPI BOOL DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    return TRUE;
}
