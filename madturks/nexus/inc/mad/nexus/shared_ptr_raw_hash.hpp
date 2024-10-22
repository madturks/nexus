#pragma once

#include <memory>

namespace mad::nexus {
struct shared_ptr_raw_hash {
    using is_transparent = void;

    [[nodiscard]] size_t operator()(const void * ptr) const noexcept {
        return std::hash<const void *>{}(ptr);
    }

    [[nodiscard]] size_t
    operator()(const std::shared_ptr<void> & sp) const noexcept {
        return std::hash<const void *>{}(sp.get());
    }
};

} // namespace mad::nexus
