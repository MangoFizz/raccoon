// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__RESOURCES__RESOURCES_HPP
#define RACCOON__RESOURCES__RESOURCES_HPP

#include <filesystem>

namespace Raccoon::Medals {
    std::filesystem::path get_resources_map_path() noexcept;
}

#endif
