#pragma once

#include <mad/concept/has_value_type.hpp>

namespace mad {
/**
 * Checks whether type T contains a member function `push_back`, and
 * if so checks whether the push_back function accepts a unary `value_type`
 * argument or not.
 *
 * @tparam T Type to check the constraint against
 */
template <typename T, typename ValueType = typename T::value_type>
concept has_push_back = has_value_type<T> && requires(T t) {
    { t.push_back(ValueType{}) };
};

} // namespace mad
