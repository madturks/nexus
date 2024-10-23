#pragma once

// cppstd
#include <algorithm>
#include <climits>
#include <random>

// mad
namespace mad::random {

static inline std::independent_bits_engine<std::default_random_engine, CHAR_BIT,
                                           unsigned char>
    rbe;

template <typename T, size_t N>
void bytegen(T (&buf) [N]) {
    std::generate(&buf [0], &buf [N], std::ref(rbe));
}

template <typename T>
void bytegen(T & buf) {
    std::generate(begin(buf), end(buf), std::ref(rbe));
}

template <size_t N>
void bytegen(std::array<std::uint8_t, N> & buf) {
    std::generate(begin(buf), end(buf), std::ref(rbe));
}

template <typename... Args>
void bytegen_n(Args &&... args) {
    // expansion pattern expansion
    (std::generate(begin(std::forward<Args>(args)),
                   end(std::forward<Args>(args)), std::ref(rbe)),
     ...);
}

template <typename T>
inline auto bytegen_n_all(T & arrays) -> void {
    std::for_each(begin(arrays), end(arrays), [](auto & v) {
        bytegen(v);
    });
}
} // namespace mad::random
