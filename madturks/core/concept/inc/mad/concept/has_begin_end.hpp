/**
 * Copyright 2024 madturks
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace mad {

template <typename T>
concept has_begin_end = requires(T a) {
    { a.begin() };
    { a.end() };
};

template <typename T>
concept has_const_begin_end = requires(T a) {
    { a.cbegin() };
    { a.cend() };
};

} // namespace mad
