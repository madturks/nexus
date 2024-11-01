/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <initializer_list>

namespace mad {
template <typename T, typename U>
concept has_assign_range = requires(T a, U first, U last) {
    { a.assign(first, last) };
};

template <typename T, typename U>
concept has_assign_value = requires(
    T a, U count, const typename T::value_type & value) {
    { a.assign(count, value) };
};

template <typename T>
concept has_assign_initializer_list = requires(
    T a, std::initializer_list<typename T::value_type> ilist) {
    { a.assign(ilist) };
};

// General concept that checks if a type has any of the assign functions.
template <typename T>
concept has_assign = has_assign_value<T, typename T::size_type> ||
                     has_assign_range<T, typename T::iterator> ||
                     has_assign_initializer_list<T>;
} // namespace mad
