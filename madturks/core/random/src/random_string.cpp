/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#include <mad/random_string.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <numeric>
#include <span>

namespace mad::random {

static constexpr auto numeric_chars = [] {
    std::array<char, 10> chars = {};
    std::iota(chars.begin(), chars.end(), '0'); // Fills with '0' to '9'
    return chars;
}();

static constexpr auto lowercase_chars = [] {
    std::array<char, 26> chars = {};
    std::iota(chars.begin(), chars.end(), 'a'); // Fills with 'a' to 'z'
    return chars;
}();

static constexpr auto uppercase_chars = [] {
    std::array<char, 26> chars = {};
    std::iota(chars.begin(), chars.end(), 'A'); // Fills with 'A' to 'Z'
    return chars;
}();

static constinit std::array<char, 62> alphanumeric_chars = [] {
    std::array<char, 62> chars = {};
    std::copy_n(numeric_chars.begin(), 10, chars.begin()); // Copy numeric chars
    std::copy_n(lowercase_chars.begin(), 26,
                chars.begin() + 10); // Copy lowercase chars
    std::copy_n(uppercase_chars.begin(), 26,
                chars.begin() + 36); // Copy uppercase chars
    return chars;
}();

auto ascii_alphanumeric_charset() -> const std::span<const char> & {
    constinit static std::span<const char> charset{ alphanumeric_chars };
    return charset;
}

auto ascii_number_charset() -> const std::span<const char> & {
    constinit static std::span<const char> charset{ numeric_chars };
    return charset;
}

auto ascii_lowercase_charset() -> const std::span<const char> & {
    constinit static std::span<const char> charset{ lowercase_chars };
    return charset;
}

auto ascii_uppercase_charset() -> const std::span<const char> & {
    constinit static std::span<const char> charset{ uppercase_chars };
    return charset;
}

} // namespace mad::random
