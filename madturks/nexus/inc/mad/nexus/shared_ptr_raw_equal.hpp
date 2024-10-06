#pragma once

#include <memory>

namespace mad::nexus {
struct shared_ptr_raw_equal {
    using is_transparent = void;

    bool operator()(const std::shared_ptr<void> & lhs,
                    const std::shared_ptr<void> & rhs) const {
        return lhs.get() == rhs.get();
    }

    bool operator()(const std::shared_ptr<void> & lhs, const void * rhs) const {
        return lhs.get() == rhs;
    }

    bool operator()(const void * lhs, const std::shared_ptr<void> & rhs) const {
        return lhs == rhs.get();
    }
};
} // namespace mad::nexus
