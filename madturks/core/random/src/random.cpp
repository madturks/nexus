
#include <mad/random.hpp>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <random>
#include <type_traits>

namespace mad::random {

static auto & get_engine() {
    thread_local auto engine = []() {
        std::mt19937 eng;
        eng.seed(static_cast<std::uint32_t>(std::time(nullptr)));
        return eng;
    }();
    return engine;
}

template <typename T>
auto fill_span(std::span<T> & span, arithmetic_boundary<T> bounds) -> void {

    auto rnb = [&bounds]() {
        if constexpr (std::is_same_v<T, char>) {
            // The standard committee and its fucking pecularities.
            // char returns true for std::is_integral_v but
            // std::uniform_int_distribution rejects it regardless..
            return [&bounds]() {
                return static_cast<char>(
                    std::uniform_int_distribution<unsigned char>{
                        static_cast<unsigned char>(bounds.upper),
                        static_cast<unsigned char>(bounds.lower) }
                        .operator()(get_engine()));
            };
        }

        else if constexpr (std::is_integral_v<T>) {
            return [&bounds]() {
                return std::uniform_int_distribution<T>{ bounds.lower,
                                                         bounds.upper }
                    .operator()(get_engine());
            };
        }

        else {
            return [&bounds]() {
                return std::uniform_real_distribution<T>{ bounds.lower,
                                                          bounds.upper }
                    .operator()(get_engine());
            };
        }
    };

    std::generate(std::begin(span), std::end(span), rnb());
}

template <typename T>
auto fill_span(std::span<T> & span,
               const std::span<const T> & value_range) -> void {

    if (value_range.empty())
        return;

    auto rnb = [&value_range]() {
        return [&value_range]() {
            return value_range
                [std::uniform_int_distribution<decltype(value_range.size())>{
                    0, value_range.size() - 1 }
                     .operator()(get_engine())];
        };
    };

    std::generate(std::begin(span), std::end(span), rnb());
}

} // namespace mad::random

template auto
mad::random::fill_span(std::span<char> & span,
                       mad::random::arithmetic_boundary<char> bounds) -> void;
template auto mad::random::fill_span(
    std::span<std::int8_t> & span,
    mad::random::arithmetic_boundary<std::int8_t> bounds) -> void;
template auto mad::random::fill_span(
    std::span<std::int16_t> & span,
    mad::random::arithmetic_boundary<std::int16_t> bounds) -> void;
template auto mad::random::fill_span(
    std::span<std::int32_t> & span,
    mad::random::arithmetic_boundary<std::int32_t> bounds) -> void;
template auto mad::random::fill_span(
    std::span<std::int64_t> & span,
    mad::random::arithmetic_boundary<std::int64_t> bounds) -> void;
template auto mad::random::fill_span(
    std::span<std::uint8_t> & span,
    mad::random::arithmetic_boundary<std::uint8_t> bounds) -> void;
template auto mad::random::fill_span(
    std::span<std::uint16_t> & span,
    mad::random::arithmetic_boundary<std::uint16_t> bounds) -> void;
template auto mad::random::fill_span(
    std::span<std::uint32_t> & span,
    mad::random::arithmetic_boundary<std::uint32_t> bounds) -> void;
template auto mad::random::fill_span(
    std::span<std::uint64_t> & span,
    mad::random::arithmetic_boundary<std::uint64_t> bounds) -> void;
template auto
mad::random::fill_span(std::span<float> & span,
                       mad::random::arithmetic_boundary<float> bounds) -> void;
template auto
mad::random::fill_span(std::span<double> & span,
                       mad::random::arithmetic_boundary<double> bounds) -> void;

template auto
mad::random::fill_span(std::span<char> & span,
                       const std::span<const char> & value_range) -> void;
template auto mad::random::fill_span(
    std::span<std::int8_t> & span,
    const std::span<const std::int8_t> & value_range) -> void;
template auto mad::random::fill_span(
    std::span<std::int16_t> & span,
    const std::span<const std::int16_t> & value_range) -> void;
template auto mad::random::fill_span(
    std::span<std::int32_t> & span,
    const std::span<const std::int32_t> & value_range) -> void;
template auto mad::random::fill_span(
    std::span<std::int64_t> & span,
    const std::span<const std::int64_t> & value_range) -> void;
template auto mad::random::fill_span(
    std::span<std::uint8_t> & span,
    const std::span<const std::uint8_t> & value_range) -> void;
template auto mad::random::fill_span(
    std::span<std::uint16_t> & span,
    const std::span<const std::uint16_t> & value_range) -> void;
template auto mad::random::fill_span(
    std::span<std::uint32_t> & span,
    const std::span<const std::uint32_t> & value_range) -> void;
template auto mad::random::fill_span(
    std::span<std::uint64_t> & span,
    const std::span<const std::uint64_t> & value_range) -> void;
template auto
mad::random::fill_span(std::span<float> & span,
                       const std::span<const float> & value_range) -> void;
template auto
mad::random::fill_span(std::span<double> & span,
                       const std::span<const double> & value_range) -> void;
