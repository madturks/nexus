/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace mad {

/**
 * @brief Create a non-existing file path in system temporary file directory
 *
 * @param [in] pattern File name pattern, where {} represents the randomly
 * generated string placeholder.
 *
 * @return Non-null on success, std::nullopt otherwise.
 */
[[nodiscard]]
std::optional<std::filesystem::path>
make_temp_file_path(std::string_view pattern = "tmp-{}-file") noexcept;

/**
 * @brief Create a temporary file in system temporary file directory
 *
 * @param pattern File name pattern, where {} represents the randomly generated
 * string placeholder.
 *
 * @return Non-null on success, std::nullopt otherwise.
 */
[[nodiscard]]
std::optional<std::pair<std::filesystem::path, std::ofstream>>
make_temp_file(std::string_view pattern = "tmp-{}-file") noexcept;
} // namespace mad
