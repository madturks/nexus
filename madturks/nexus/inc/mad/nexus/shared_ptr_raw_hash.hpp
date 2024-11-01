/******************************************************
 * A hasher that uses the underlying raw pointer for
 * the shared_ptr's hashing.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <memory>

namespace mad::nexus {
/******************************************************
 * shared_ptr raw hasher to enable heterogeneous lookup
 * by raw ptr.
 ******************************************************/
struct shared_ptr_raw_hash {
    using is_transparent = void;

    /******************************************************
     * raw_ptr hash.
     * @param ptr raw_ptr
     * @return the hash
     ******************************************************/
    [[nodiscard]] size_t operator()(const void * ptr) const noexcept {
        return std::hash<const void *>{}(ptr);
    }

    /******************************************************
     * shared_ptr hash.
     * @param sp shared_ptr
     * @return the hash
     ******************************************************/
    [[nodiscard]] size_t
    operator()(const std::shared_ptr<void> & sp) const noexcept {
        return std::hash<const void *>{}(sp.get());
    }
};

} // namespace mad::nexus
