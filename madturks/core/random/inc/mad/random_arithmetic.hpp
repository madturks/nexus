#pragma once

#include <mad/random.hpp>
#include <mad/concepts>

#include <type_traits>
#include <functional>
#include <span>
#include <cstdint>
#include <numeric>
#include <string>
#include <concepts>

namespace mad::random {

    /**
     * Generate a single random arithmetic value with type T.
     *
     * @tparam T Type of the value, which will be randomly generated
     *
     * @param [in] lower_bound Lower bound of generated value. Default: `std::numeric_limits<T>::min()`
     * @param [in] upper_bound Upper bound of generated value. Default: `std::numeric_limits<T>::max()`
     *
     * @return Random value of type t in boundary of [lower_bound, upper_bound]
     */
    template <typename T>
    requires(std::is_arithmetic_v<T>)
    [[nodiscard]] inline auto generate(T lower_bound = std::numeric_limits<T>::min(), T upper_bound = std::numeric_limits<T>::max()) -> T {
        T v{};
        std::span<T> result_span = {&v, 1};
        fill_span(result_span, arithmetic_boundary<T>{.lower = lower_bound, .upper = upper_bound});
        return v;
    }

    /**
     * Fill between [begin, end] with random elements of type T.
     *
     * (pointer specialization)
     *
     * @tparam Iter Auto-deduced iterator type
     * @tparam ValueType Auto-deduced iterator value type
     *
     */
    template <typename Iter, typename ValueType = std::remove_pointer_t<Iter>>
    requires(std::is_pointer_v<Iter> && std::is_arithmetic_v<ValueType>)
    inline auto fill(Iter begin, Iter end, ValueType lower_bound = std::numeric_limits<ValueType>::min(),
                     ValueType upper_bound = std::numeric_limits<ValueType>::max()) {
        while (begin != end) {
            *begin++ = generate<ValueType>(lower_bound, upper_bound);
        }
    }

    /**
     * Fill between [begin, end] with random elements of type T.
     *
     * (iterator specialization)
     *
     * @tparam Iter Auto-deduced iterator type
     * @tparam ValueType Auto-deduced iterator value type
     */
    template <typename Iter>
    requires(std::is_arithmetic_v<typename Iter::value_type>)
    inline auto fill(Iter begin, Iter end, typename Iter::value_type lower_bound = std::numeric_limits<typename Iter::value_type>::min(),
                     typename Iter::value_type upper_bound = std::numeric_limits<typename Iter::value_type>::max()) {
        while (begin != end) {
            *begin++ = generate<typename Iter::value_type>(lower_bound, upper_bound);
        }
    }

    /**
     * Make a container filled with random arithmetic type of
     * elements.
     *
     * (.push_back() overload)
     *
     * @tparam Container Container type
     * @tparam ValueType (auto-deduced) Value type
     * @tparam BoundType (auto-deduced) Bound type
     *
     * @param [in] count Number of elements generated
     * @param [in] lower_bound Number of elements generated
     * @param [in] upper_bound Number of elements generated
     *
     * @return Container filled with `count` number of elements randomly generated
     * in boundary of [lower_bound, upper_bound]
     */
    template <typename Container, typename ValueType = typename Container::value_type, typename BoundType>
    requires(std::is_arithmetic_v<ValueType> && has_push_back<Container> && std::is_default_constructible_v<Container>)
    [[nodiscard]] inline auto make(std::size_t count, BoundType lower_bound = std::numeric_limits<ValueType>::min(),
                                   BoundType upper_bound = std::numeric_limits<ValueType>::max()) {
        Container container;

        while (count--) {
            container.push_back(generate<ValueType>(lower_bound, upper_bound));
        }

        return container;
    }

    /**
     * Make a container filled with random arithmetic type of
     * elements.
     *
     * (.insert() overload)
     *
     * @tparam Container Container type
     * @tparam ValueType (auto-deduced) Value type
     * @tparam BoundType (auto-deduced) Bound type
     *
     * @param [in] count Number of elements generated
     * @param [in] lower_bound Number of elements generated
     * @param [in] upper_bound Number of elements generated
     *
     * @return Container filled with `count` number of elements randomly generated
     * in boundary of [lower_bound, upper_bound]
     */
    template <typename Container, typename ValueType = typename Container::value_type, typename BoundType>
    requires(std::is_arithmetic_v<ValueType> && has_unary_insert<Container> && std::is_default_constructible_v<Container>)
    [[nodiscard]] inline auto make(std::size_t count, BoundType lower_bound = std::numeric_limits<ValueType>::min(),
                                   BoundType upper_bound = std::numeric_limits<ValueType>::max()) {
        Container container;
        while (count) {
            while (container.insert(generate<ValueType>(lower_bound, upper_bound)).second == false)
                ;
            count--;
        }
        return container;
    }
} // namespace mad::random
