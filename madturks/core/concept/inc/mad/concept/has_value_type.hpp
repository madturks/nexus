#pragma once

namespace mad {
    /**
     * Checks whether type T contains a typedef named `value_type`.
     *
     * @tparam T Type to check the constraint against
     */
    template <typename T>
    concept has_value_type = requires { typename T::value_type; };

} // namespace mad
