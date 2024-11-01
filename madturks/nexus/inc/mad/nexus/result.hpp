/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <expected>
#include <system_error>

namespace mad::nexus {

/******************************************************
 * The result type template for all API functions.
 *
 * @tparam ResultType
 ******************************************************/
template <typename ResultType = void>
using result [[clang::warn_unused_result]] =
    ::std::expected<ResultType, std::error_code>;
// Unfortunately, we can't use `nodiscard` here, but there's a paper:
// https://www9.open-std.org/JTC1/SC22/WG21/docs/papers/2024/p3245r1.html
// Between the three major compilers, only clang has a compiler-specific
// attribute that allows this.

} // namespace mad::nexus
