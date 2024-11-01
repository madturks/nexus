/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <cassert>

#define MAD_ASSERT(what)  assert(what)
#define MAD_EXPECTS(what) MAD_ASSERT(what)
#define MAD_ENSURES(what) MAD_ASSERT(what)

// FIXME: MSVC?
#define MAD_ALWAYS_INLINE inline __attribute__((always_inline))

#define MAD_FLATTEN __attribute__((flatten))

// clang-format off
#if defined(__GNUC__) || defined(__clang__)
#define MAD_EXHAUSTIVE_SWITCH_BEGIN                                            \
    _Pragma("GCC diagnostic push")                                             \
        _Pragma("GCC diagnostic error \"-Wswitch\"")

#define MAD_EXHAUSTIVE_SWITCH_END _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define MAD_EXHAUSTIVE_SWITCH_BEGIN                                            \
    __pragma(warning(push)) __pragma(warning(error : 4062))
// clang-format on
#define MAD_EXHAUSTIVE_SWITCH_END __pragma(warning(pop))
#else
#define MAD_EXHAUSTIVE_SWITCH_BEGIN
#define MAD_EXHAUSTIVE_SWITCH_END
#endif
