
#include <cstdlib>
#include <mad/random_string.hpp>

#include <span>

namespace mad::random {

#define NUMERIC_ASCII_CHARS   "0123456789"
#define LOWERCASE_ASCII_CHARS "abcdefghijklmnopqrstuvwxyz"
#define UPPERCASE_ASCII_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

    constinit static char alphanumeric_chars [] = NUMERIC_ASCII_CHARS LOWERCASE_ASCII_CHARS UPPERCASE_ASCII_CHARS;

    auto ascii_alphanumeric_charset() -> const std::span<char> & {
        constinit static std::span<char> charset{alphanumeric_chars};
        return charset;
    }

    auto ascii_number_charset() -> const std::span<char> & {
        constinit static std::span<char> charset{&alphanumeric_chars [0], sizeof(NUMERIC_ASCII_CHARS) - 1};
        return charset;
    }

    auto ascii_lowercase_charset() -> const std::span<char> & {
        constinit static std::span<char> charset{&alphanumeric_chars [sizeof(NUMERIC_ASCII_CHARS) - 1],
                                                 (sizeof(LOWERCASE_ASCII_CHARS) - 1)};
        return charset;
    }

    auto ascii_uppercase_charset() -> const std::span<char> & {
        constinit static std::span<char> charset{
            &alphanumeric_chars [(sizeof(NUMERIC_ASCII_CHARS) - 1) + (sizeof(LOWERCASE_ASCII_CHARS) - 1)],
            (sizeof(UPPERCASE_ASCII_CHARS) - 1)};
        return charset;
    }

} // namespace mad::random
