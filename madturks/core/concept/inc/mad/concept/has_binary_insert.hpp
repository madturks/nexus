/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/concept/has_key_type.hpp>
#include <mad/concept/has_value_type.hpp>

namespace mad {
/**
 * Checks whether type T contains a member function `insert`, and
 * if so checks whether the `insert` function accepts a binary `key_type` and
 * `value_type` argument or not.
 *
 * @tparam T Type to check the constraint against
 */
template <typename T, typename KeyType = typename T::key_type,
          typename ValueType = typename T::value_type>
concept has_binary_insert = has_key_type<T> && has_value_type<T> &&
                            requires(T t) {
                                { t.insert(KeyType{}, ValueType{}) };
                            };

} // namespace mad
