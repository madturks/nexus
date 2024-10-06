
#pragma once

#include <cstdint>
#include <span>
#include <type_traits>

namespace mad::random {

/**
 * A type which represents a boundary in
 * values can be represented with type T.
 *
 * @tparam T Value type of the boundary
 */
template <typename T>
requires(std::is_arithmetic_v<T>)
struct arithmetic_boundary {
    const T lower;
    const T upper;
};

/**
 * Fill a span with random values generated in boundary of `bounds`.
 *
 * @tparam T Element type
 *
 * @param [in,out] span Contiguous range of elements to fill
 * @param [in] bounds Bounds to generate random T value in
 *
 */
template <typename T>
auto fill_span(std::span<T> & span, arithmetic_boundary<T> bounds) -> void;

/**
 * Fill a span with random values selected from @p `value_range`.
 *
 * @tparam T Element type
 *
 * @param [in,out] span Contiguous range of elements to fill
 * @param [in] value_range Value range which elements of span will be randomly
 * selected from
 *
 */
template <typename T>
auto fill_span(std::span<T> & span,
               const std::span<const T> & value_range) -> void;
} // namespace mad::random

extern template auto
mad::random::fill_span(std::span<char> & span,
                       mad::random::arithmetic_boundary<char> bounds) -> void;
extern template auto mad::random::fill_span(
    std::span<std::int8_t> & span,
    mad::random::arithmetic_boundary<std::int8_t> bounds) -> void;
extern template auto mad::random::fill_span(
    std::span<std::int16_t> & span,
    mad::random::arithmetic_boundary<std::int16_t> bounds) -> void;
extern template auto mad::random::fill_span(
    std::span<std::int32_t> & span,
    mad::random::arithmetic_boundary<std::int32_t> bounds) -> void;
extern template auto mad::random::fill_span(
    std::span<std::int64_t> & span,
    mad::random::arithmetic_boundary<std::int64_t> bounds) -> void;
extern template auto mad::random::fill_span(
    std::span<std::uint8_t> & span,
    mad::random::arithmetic_boundary<std::uint8_t> bounds) -> void;
extern template auto mad::random::fill_span(
    std::span<std::uint16_t> & span,
    mad::random::arithmetic_boundary<std::uint16_t> bounds) -> void;
extern template auto mad::random::fill_span(
    std::span<std::uint32_t> & span,
    mad::random::arithmetic_boundary<std::uint32_t> bounds) -> void;
extern template auto mad::random::fill_span(
    std::span<std::uint64_t> & span,
    mad::random::arithmetic_boundary<std::uint64_t> bounds) -> void;
extern template auto
mad::random::fill_span(std::span<float> & span,
                       mad::random::arithmetic_boundary<float> bounds) -> void;
extern template auto
mad::random::fill_span(std::span<double> & span,
                       mad::random::arithmetic_boundary<double> bounds) -> void;

extern template auto
mad::random::fill_span(std::span<char> & span,
                       const std::span<const char> & value_range) -> void;
extern template auto mad::random::fill_span(
    std::span<std::int8_t> & span,
    const std::span<const std::int8_t> & value_range) -> void;
extern template auto mad::random::fill_span(
    std::span<std::int16_t> & span,
    const std::span<const std::int16_t> & value_range) -> void;
extern template auto mad::random::fill_span(
    std::span<std::int32_t> & span,
    const std::span<const std::int32_t> & value_range) -> void;
extern template auto mad::random::fill_span(
    std::span<std::int64_t> & span,
    const std::span<const std::int64_t> & value_range) -> void;
extern template auto mad::random::fill_span(
    std::span<std::uint8_t> & span,
    const std::span<const std::uint8_t> & value_range) -> void;
extern template auto mad::random::fill_span(
    std::span<std::uint16_t> & span,
    const std::span<const std::uint16_t> & value_range) -> void;
extern template auto mad::random::fill_span(
    std::span<std::uint32_t> & span,
    const std::span<const std::uint32_t> & value_range) -> void;
extern template auto mad::random::fill_span(
    std::span<std::uint64_t> & span,
    const std::span<const std::uint64_t> & value_range) -> void;
extern template auto
mad::random::fill_span(std::span<float> & span,
                       const std::span<const float> & value_range) -> void;
extern template auto
mad::random::fill_span(std::span<double> & span,
                       const std::span<const double> & value_range) -> void;
