// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/logger.hpp>
#include <balltze/command.hpp>
#include <balltze/features/tags_handling.hpp>
#include <balltze/events/map_load.hpp>
#include <balltze/plugin.hpp>
#include "postprocess/postprocess.hpp"
#include "medals/medals.hpp"

namespace Raccoon {
    Balltze::Logger logger("Raccoon");

    void set_up_tags_loader() {
        Balltze::Event::MapLoadEvent::subscribe([](auto &event) {
            logger.info("Importing tags...");
            if(event.time == Balltze::Event::EVENT_TIME_BEFORE) {
                auto path = Balltze::get_plugin_path() / "raccoon.map";
                Balltze::Features::import_tag_from_map(path, "raccoon\\raccoon", Balltze::Engine::TAG_CLASS_TAG_COLLECTION);
            }
        });
    }
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
    Raccoon::logger.mute_ingame(true);
    Raccoon::set_up_tags_loader();
    Raccoon::Medals::set_up_medals();
    return true;
}

BALLTZE_PLUGIN_API void plugin_load() noexcept {
    Balltze::load_commands_settings();
    Raccoon::logger.info("Loaded");
}

WINAPI BOOL DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    return TRUE;
}
