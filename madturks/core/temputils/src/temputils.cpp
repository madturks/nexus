/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#include <mad/random_string.hpp>
#include <mad/temputils.hpp>

#include <fmt/format.h>

#include <fstream>
#include <string>

namespace mad {

// Make temporary file name
std::optional<std::filesystem::path>
make_temp_file_path(std::string_view pattern) noexcept {
    try {

        auto temp_dir = std::filesystem::temp_directory_path();
        auto random_str = mad::random::generate<std::string>();
        auto file_name = fmt::format(fmt::runtime(pattern), random_str);

        std::uint32_t remaining_attempts = 5;

        do {
            auto result = temp_dir.append(file_name);
            if (false == std::filesystem::exists(result))
                return result;
        } while (remaining_attempts-- > 0);
    }
    catch (...) {
    }

    return std::nullopt;
}

// Make temporary file
std::optional<std::pair<std::filesystem::path, std::ofstream>>
make_temp_file(std::string_view pattern) noexcept {
    try {
        if (auto path = make_temp_file_path(pattern)) {
            if (std::ofstream ofs(
                    path.value(), std::ios::out | std::ios::trunc);
                ofs.is_open()) {
                return std::make_pair(std::move(path.value()), std::move(ofs));
            }
        }
    }
    catch (...) {
    }
    return std::nullopt;
}

} // namespace mad
