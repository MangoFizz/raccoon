// SPDX-License-Identifier: GPL-3.0-only

#include <balltze/plugin.hpp>
#include "resources.hpp"

namespace Raccoon {
    namespace Resources {
        std::filesystem::path get_resources_map_path() noexcept {
            return Balltze::get_plugin_path() / "raccoon.map";
        }
    }
}
