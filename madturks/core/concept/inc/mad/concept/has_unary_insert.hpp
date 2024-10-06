#pragma once

#include <mad/concept/has_value_type.hpp>

namespace mad {
/**
 * Checks whether type T contains a member function `insert`, and
 * if so checks whether the `insert` function accepts a unary `value_type`
 * argument or not.
 *
 * @tparam T Type to check the constraint against
 */
template <typename T, typename ValueType = typename T::value_type>
concept has_unary_insert = has_value_type<T> && requires(T t) {
    { t.insert(ValueType{}) };
};

} // namespace mad
