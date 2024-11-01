/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

// Logging macros (for optionally evaluating the arguments)
// (mgilor): The reason we're going oldschool here is;
// Since lazy forwarding is yet to be included in C++ standard, we have to
// provide a mechanism for doing it so. There are some ongoing work
// (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0927r0.pdf) but I
// don't think it will be in standard C++ in any foreseeable future.

// For branch predictor optimization
#define MAD_LOG_LEVEL_LIKELINESS(X)      MAD_LOG_LEVEL_LIKELINESS_IMPL(X)
#define MAD_LOG_LEVEL_LIKELINESS_IMPL(X) MAD_LOG_LEVEL_LIKELINESS_##X
#define MAD_LOG_LEVEL_LIKELINESS_TRACE   [[unlikely]]
#define MAD_LOG_LEVEL_LIKELINESS_DEBUG   [[unlikely]]
#define MAD_LOG_LEVEL_LIKELINESS_INFO
#define MAD_LOG_LEVEL_LIKELINESS_WARN     [[likely]]
#define MAD_LOG_LEVEL_LIKELINESS_ERROR    [[likely]]
#define MAD_LOG_LEVEL_LIKELINESS_CRITICAL [[likely]]

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
#define MAD_LOG_I(INSTANCE, LEVEL, FMT, ...)                                   \
    do {                                                                       \
        if (INSTANCE.should_log(LEVEL)) {                                      \
            INSTANCE.template log<mad::meta::source_location{}, false>(        \
                LEVEL, FMT, ##__VA_ARGS__);                                    \
        }                                                                      \
    } while (0)

/**
 * @brief Macro function to log in trace level through given log printer
 * instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_TRACE_I(INSTANCE, FMT, ...)                                    \
    do {                                                                       \
        if (INSTANCE.should_log(::mad::log_level::trace))                      \
            MAD_LOG_LEVEL_LIKELINESS_TRACE {                                   \
                INSTANCE.template log_trace<::mad::meta::source_location{},    \
                                            false>(FMT, ##__VA_ARGS__);        \
            }                                                                  \
    } while (0)

/**
 * @brief Macro function to log in debug level through given log printer
 * instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_DEBUG_I(INSTANCE, FMT, ...)                                    \
    do {                                                                       \
        if (INSTANCE.should_log(::mad::log_level::debug))                      \
            MAD_LOG_LEVEL_LIKELINESS_DEBUG {                                   \
                INSTANCE.template log_debug<::mad::meta::source_location{},    \
                                            false>(FMT, ##__VA_ARGS__);        \
            }                                                                  \
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
#define MAD_LOG_INFO_I(INSTANCE, FMT, ...)                                     \
    do {                                                                       \
        if (INSTANCE.should_log(::mad::log_level::info))                       \
            MAD_LOG_LEVEL_LIKELINESS_INFO {                                    \
                INSTANCE                                                       \
                    .template log_info<::mad::meta::source_location{}, false>( \
                        FMT, ##__VA_ARGS__);                                   \
            }                                                                  \
    } while (0)

/**
 * @brief Macro function to log in warning level through given log printer
 * instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_WARN_I(INSTANCE, FMT, ...)                                     \
    do {                                                                       \
        if (INSTANCE.should_log(::mad::log_level::warn))                       \
            MAD_LOG_LEVEL_LIKELINESS_WARN {                                    \
                INSTANCE                                                       \
                    .template log_warn<::mad::meta::source_location{}, false>( \
                        FMT, ##__VA_ARGS__);                                   \
            }                                                                  \
    } while (0)

/**
 * @brief Macro function to log in error level through given log printer
 * instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_ERROR_I(INSTANCE, FMT, ...)                                    \
    do {                                                                       \
        if (INSTANCE.should_log(::mad::log_level::error))                      \
            MAD_LOG_LEVEL_LIKELINESS_ERROR {                                   \
                INSTANCE                                                       \
                    .template log_err<::mad::meta::source_location{}, false>(  \
                        FMT, ##__VA_ARGS__);                                   \
            }                                                                  \
    } while (0)

/**
 * @brief Macro function to log in critical level through given log printer
 * instance
 *
 * @param [in] INSTANCE  Log printer instance
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_CRITICAL_I(INSTANCE, FMT, ...)                                 \
    do {                                                                       \
        if (INSTANCE.should_log(::mad::log_level::critical))                   \
            MAD_LOG_LEVEL_LIKELINESS_CRITICAL {                                \
                INSTANCE.template log_critical<::mad::meta::source_location{}, \
                                               false>(FMT, ##__VA_ARGS__);     \
            }                                                                  \
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
 * @brief Macro function to log in trace level through current log printer
 * instance
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
 * @brief Macro function to log in debug level through current log printer
 * instance
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
 * @brief Macro function to log in info level through current log printer
 * instance
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
 * @brief Macro function to log in warning level through current log printer
 * instance
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
 * @brief Macro function to log in error level through current log printer
 * instance
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
 * @brief Macro function to log in critical level through current log printer
 * instance
 *
 * This implementation implicitly uses (*this) reference as an instance.
 *
 * @param [in] EID       Event identifier
 * @param [in] FMT       Format string
 * @param [in] ...       Varargs
 *
 */
#define MAD_LOG_CRITICAL(FMT, ...)                                             \
    MAD_LOG_CRITICAL_I((*this), FMT, ##__VA_ARGS__)
