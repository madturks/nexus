
#pragma once

#include <mad/random.hpp>
#include <mad/random_arithmetic.hpp>

#include <span>
#include <type_traits>
#include <string>

namespace mad::random {

    /**
     * @brief Span of ascii alphanumeric characters
     *
     * @return A std::span which contains A-Za-z0-9 charset
     */
    [[nodiscard]] auto ascii_alphanumeric_charset() -> const std::span<char> &;

    /**
     * @brief Span of ascii number characters only
     *
     * @return A std::span which contains 0-9 charset
     */
    [[nodiscard]] auto ascii_number_charset() -> const std::span<char> &;

    /**
     * @brief Span of lowercase ascii characters only
     *
     * @return A std::span which contains a-z charset
     */
    [[nodiscard]] auto ascii_lowercase_charset() -> const std::span<char> &;

    /**
     * @brief Span of uppercase ascii characters only
     *
     * @return A std::span which contains A-Z charset
     */
    [[nodiscard]] auto ascii_uppercase_charset() -> const std::span<char> &;

    /**
     * Generate a single random std::string.
     *
     * @param [in] min_l Minimum length of the generated string. Default: `16`
     * @param [in] max_l Maximum length of the generated string. Default: `256`
     *
     * @return Random std::string with length in boundary of [min_l, max_l]
     */
    template <typename T>
    requires(std::is_same_v<std::string, T>)
    [[nodiscard]] inline auto generate(std::size_t min_l = 16, std::size_t max_l = 256,
                                       const std::span<char> & charset = ascii_alphanumeric_charset()) -> std::string {
        std::string v{};
        auto string_len = generate<std::size_t>(min_l, max_l);
        v.resize(string_len);
        std::span<std::string::value_type> result_span{v.data(), v.length()};
        fill_span(result_span, charset
                  // arithmetic_boundary<typename std::string::value_type> {
                  //     .lower = std::numeric_limits<typename std::string::value_type>::min(),
                  //     .upper= std::numeric_limits<typename std::string::value_type>::max()
                  // }
        );
        return v;
    }

    /**
     * Fill between [begin, end] with random elements of type T.
     *
     * (container type element overload)
     *
     * @tparam Iter Auto-deduced iterator type
     * @tparam ValueType Auto-deduced iterator value type
     *
     * @param [in,out] begin Iterator to the start of the fill range
     * @param [in,out] end  Iterator to the end of the fill range
     * @param [in] lower_bound Minimum length of the value generated
     * @param [in] upper_bound Maximum length of the value generated
     *
     */
    template <typename Iter, typename ValueType = typename Iter::value_type>
    requires(std::is_same_v<std::string, ValueType>)
    inline auto fill(Iter begin, Iter end, std::size_t min_length = 16, std::size_t max_length = 256) {
        while (begin != end) {
            *begin++ = generate<ValueType>(min_length, max_length);
        }
    }

    /**
     * Make a container filled with random `std::string` elements generated
     * in boundary of [min_length, max_length]
     *
     * (.push_back() overload)
     *
     * @tparam Container Container type
     * @tparam ValueType (auto-deduced) Value type
     *
     * @param [in] count Number of elements generated
     * @param [in] min_length (optional) Minimum length of generated strings (default=16)
     * @param [in] max_length (optional) Maximum length of generated strings (default=256)
     *
     * @return Container filled with `count` number of elements randomly generated
     * in length boundary of [lower_bound, upper_bound]
     */
    template <typename Container, typename ValueType = typename Container::value_type>
    requires(std::is_same_v<std::string, ValueType> && has_push_back<Container> && std::is_default_constructible_v<Container>)
    [[nodiscard]] inline auto make(std::size_t count, std::size_t min_length = 16, std::size_t max_length = 256) {
        Container container;

        while (count--) {
            container.push_back(generate<ValueType>(min_length, max_length));
        }

        return container;
    }

    /**
     * Make a container filled with random `std::string` elements generated
     * in boundary of [min_length, max_length]
     *
     * (.insert() overload)
     *
     * @tparam Container Container type
     * @tparam ValueType (auto-deduced) Value type
     * @tparam BoundType Bound type
     *
     * @param [in] count Number of elements generated
     * @param [in] min_length (optional) Minimum length of generated strings (default=16)
     * @param [in] max_length (optional) Maximum length of generated strings (default=256)
     *
     * @return Container filled with `count` number of elements randomly generated
     * in length boundary of [lower_bound, upper_bound]
     */
    template <typename Container, typename ValueType = typename Container::value_type, typename BoundType = std::size_t>
    requires(std::is_same_v<std::string, ValueType> && has_unary_insert<Container> && std::is_default_constructible_v<Container>)
    [[nodiscard]]
    inline auto make(std::size_t count, BoundType min_length = 16, BoundType max_length = 256) {
        Container container;
        while (count) {
            while (container.insert(generate<ValueType>(min_length, max_length)).second == false)
                ;
            count--;
        }
        return container;
    }

} // namespace mad
