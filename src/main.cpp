// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/logger.hpp>
#include <balltze/plugin.hpp>

Balltze::Logger logger("Halo world");

BALLTZE_PLUGIN_API Balltze::PluginMetadata plugin_metadata() {
    return {
        "Halo world",
        "Plugin's author",
        { 1, 0, 0, 0 },
        { 1, 0, 0, 0 },
        true
    };
}

BALLTZE_PLUGIN_API bool plugin_init() noexcept {
    logger.info("Hello world!");
    return true;
}

BALLTZE_PLUGIN_API void plugin_load() noexcept {
    logger.info("Hello world again!");
}

WINAPI BOOL DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    return TRUE;
}
