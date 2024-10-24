/******************************************************
 * An equality comparator for shared_ptr that allows
 * the comparison to be made with the underlying raw
 * pointer.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <memory>

namespace mad::nexus {

/******************************************************
 * This equality comparator for shared_ptr enables the
 * shared_ptr type to be comparable with the raw pointer
 * and vice-versa. This allows heterogeneous lookups with
 * the raw pointer.
 *
 * All comparisons are made using the underlying raw pointer.
 ******************************************************/
struct shared_ptr_raw_equal {
    /******************************************************
     * Enable comparison without having to constructing the
     * key type.
     ******************************************************/
    using is_transparent = void;

    /******************************************************
     * shared_ptr - shared_ptr comparison
     *
     * @param lhs Left-hand side value of the comparison
     * @param rhs Right-hand side value of the comparison
     * @return Equality result
     ******************************************************/
    bool operator()(const std::shared_ptr<void> & lhs,
                    const std::shared_ptr<void> & rhs) const noexcept {
        return lhs.get() == rhs.get();
    }

    /******************************************************
     * shared_ptr - raw comparison
     *
     * @param lhs Left-hand side value of the comparison
     * @param rhs Right-hand side value of the comparison
     * @return Equality result
     ******************************************************/
    bool operator()(const std::shared_ptr<void> & lhs,
                    const void * rhs) const noexcept {
        return lhs.get() == rhs;
    }

    /******************************************************
     *  raw - shared_ptr comparison
     *
     * @param lhs Left-hand side value of the comparison
     * @param rhs Right-hand side value of the comparison
     * @return Equality result
     ******************************************************/
    bool operator()(const void * lhs,
                    const std::shared_ptr<void> & rhs) const noexcept {
        return lhs == rhs.get();
    }
};
} // namespace mad::nexus
