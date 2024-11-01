/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

namespace mad {
/**
 * Checks whether type T contains a typedef named `key_type`.
 *
 * @tparam T Type to check the constraint against
 */
template <typename T>
concept has_key_type = requires { typename T::key_type; };

} // namespace mad
