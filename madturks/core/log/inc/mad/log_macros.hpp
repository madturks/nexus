#pragma once

// Logging macros (for optionally evaluating the arguments)
// (mgilor): The reason we're going oldschool here is;
// Since lazy forwarding is yet to be included in C++ standard, we have to provide a mechanism
// for doing it so.
// There are some ongoing work (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0927r0.pdf)
// but I don't think it will be in standard C++ in any foreseeable future.

// For branch predictor optimization
#define MAD_LOG_LEVEL_LIKELINESS(X)      MAD_LOG_LEVEL_LIKELINESS_IMPL(X)
#define MAD_LOG_LEVEL_LIKELINESS_IMPL(X) MAD_LOG_LEVEL_LIKELINESS_##X
#define MAD_LOG_LEVEL_LIKELINESS_trace   [[unlikely]]
#define MAD_LOG_LEVEL_LIKELINESS_debug   [[unlikely]]
#define MAD_LOG_LEVEL_LIKELINESS_info
#define MAD_LOG_LEVEL_LIKELINESS_warn     [[likely]]
#define MAD_LOG_LEVEL_LIKELINESS_error    [[likely]]
#define MAD_LOG_LEVEL_LIKELINESS_critical [[likely]]

/**
 * @brief Macro function to log through given log printer instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] LEVEL     Log level
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_I(INSTANCE, LEVEL, FMT, ...)                                                                                               \
    do {                                                                                                                                   \
        if (INSTANCE.should_log(LEVEL)) {                                                                                                  \
            INSTANCE.template log<mad::meta::source_location{}, false>(LEVEL, FMT, ##__VA_ARGS__);                                         \
        }                                                                                                                                  \
    } while (0)

/**
 * @brief Macro function to log in trace level through given log printer instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_TRACE_I(INSTANCE, FMT, ...)                                                                                                \
    do {                                                                                                                                   \
        constexpr auto trace = mad::log_level::trace;                                                                                      \
        if (INSTANCE.should_log(trace))                                                                                                    \
            MAD_LOG_LEVEL_LIKELINESS(trace) {                                                                                              \
                INSTANCE.template log_trace<mad::meta::source_location{}, false>(FMT, ##__VA_ARGS__);                                      \
            }                                                                                                                              \
    } while (0)

/**
 * @brief Macro function to log in debug level through given log printer instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_DEBUG_I(INSTANCE, FMT, ...)                                                                                                \
    do {                                                                                                                                   \
        constexpr auto debug = mad::log_level::debug;                                                                                      \
        if (INSTANCE.should_log(debug))                                                                                                    \
            MAD_LOG_LEVEL_LIKELINESS(debug) {                                                                                              \
                INSTANCE.template log_debug<mad::meta::source_location{}, false>(FMT, ##__VA_ARGS__);                                      \
            }                                                                                                                              \
    } while (0)

/**
 * @brief Macro function to log in info level through given log printer instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_INFO_I(INSTANCE, FMT, ...)                                                                                                 \
    do {                                                                                                                                   \
        constexpr auto info = mad::log_level::info;                                                                                        \
        if (INSTANCE.should_log(info))                                                                                                     \
            MAD_LOG_LEVEL_LIKELINESS(info) {                                                                                               \
                INSTANCE.template log_info<mad::meta::source_location{}, false>(FMT, ##__VA_ARGS__);                                       \
            }                                                                                                                              \
    } while (0)

/**
 * @brief Macro function to log in warning level through given log printer instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_WARN_I(INSTANCE, FMT, ...)                                                                                                 \
    do {                                                                                                                                   \
        constexpr auto warn = mad::log_level::warn;                                                                                        \
        if (INSTANCE.should_log(warn))                                                                                                     \
            MAD_LOG_LEVEL_LIKELINESS(warn) {                                                                                               \
                INSTANCE.template log_warn<mad::meta::source_location{}, false>(FMT, ##__VA_ARGS__);                                       \
            }                                                                                                                              \
    } while (0)

/**
 * @brief Macro function to log in error level through given log printer instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_ERROR_I(INSTANCE, FMT, ...)                                                                                                \
    do {                                                                                                                                   \
        constexpr auto error = mad::log_level::error;                                                                                      \
        if (INSTANCE.should_log(error))                                                                                                    \
            MAD_LOG_LEVEL_LIKELINESS(error) {                                                                                              \
                INSTANCE.template log_err<mad::meta::source_location{}, false>(FMT, ##__VA_ARGS__);                                        \
            }                                                                                                                              \
    } while (0)

/**
 * @brief Macro function to log in critical level through given log printer instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_CRITICAL_I(INSTANCE, FMT, ...)                                                                                             \
    do {                                                                                                                                   \
        constexpr auto critical = mad::log_level::critical;                                                                                \
        if (INSTANCE.should_log(critical))                                                                                                 \
            MAD_LOG_LEVEL_LIKELINESS(critical) {                                                                                           \
                INSTANCE.template log_critical<mad::meta::source_location{}, false>(FMT, ##__VA_ARGS__);                                   \
            }                                                                                                                              \
    } while (0)

/**
 * @brief Macro function to log through current log printer instance
 *
 * This implementation implicitly uses (*this) reference as an instance.
 *
 * @param [in] LEVEL     Log level
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG(LEVEL, FMT, ...) MAD_LOG_I((*this), LEVEL, FMT, ##__VA_ARGS__)

/**
 * @brief Macro function to log in trace level through current log printer instance
 *
 * This implementation implicitly uses (*this) reference as an instance.
 *
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_TRACE(FMT, ...) MAD_LOG_TRACE_I((*this), FMT, ##__VA_ARGS__)

/**
 * @brief Macro function to log in debug level through current log printer instance
 *
 * This implementation implicitly uses (*this) reference as an instance.
 *
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_DEBUG(FMT, ...) MAD_LOG_DEBUG_I((*this), FMT, ##__VA_ARGS__)

/**
 * @brief Macro function to log in info level through current log printer instance
 *
 * This implementation implicitly uses (*this) reference as an instance.
 *
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_INFO(FMT, ...) MAD_LOG_INFO_I((*this), FMT, ##__VA_ARGS__)

/**
 * @brief Macro function to log in warning level through current log printer instance
 *
 * This implementation implicitly uses (*this) reference as an instance.
 *
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_WARN(FMT, ...) MAD_LOG_WARN_I((*this), FMT, ##__VA_ARGS__)

/**
 * @brief Macro function to log in error level through current log printer instance
 *
 * This implementation implicitly uses (*this) reference as an instance.
 *
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_ERROR(FMT, ...) MAD_LOG_ERROR_I((*this), FMT, ##__VA_ARGS__)

/**
 * @brief Macro function to log in critical level through current log printer instance
 *
 * This implementation implicitly uses (*this) reference as an instance.
 *
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_CRITICAL(FMT, ...) MAD_LOG_CRITICAL_I((*this), FMT, ##__VA_ARGS__)

