/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/log_config.hpp>
#include <mad/log_level.hpp>
#include <mad/meta/source_location.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <exception>
#include <iostream>
#include <memory>
#include <source_location>
#include <string_view>

namespace mad {

using log_level = mad::log_level;

/**
 * @brief A class which provides logging facility.
 *
 * @mgilor:
 *
 * This class uses;
 * - `spdlog` for logger registry management and printing the logs to the
 * defined sinks
 * - `spdlog_setup` for populating spdlog loggers from toml based log
 * configuration files
 * - `fmt` for formatting log strings
 *
 * This class aims to hide any third party dependencies from the consumer. It
 * succeeds to hide `spdlog` and `spdlog_setup` dependencies.
 *
 * Currently, the only exposed dependency is `fmt`. Since fmtlib is compliant
 * with C++20 standard format library(1), we will replace it with standard
 * library's format implementation when it is available.
 *
 * (1): `fmt` is actually the library that standard library proposal is based
 * upon.
 *
 * TODO(mgilor): Add `source_location` support
 */
class log_printer {
public:
    /**
     * @brief Retrieve a logger from logger registry and then initialize
     * log printer with that logger
     *
     * @param [in] logger_name Logger name in registry. If logger with given
     * name is not found in registry, log_printer will use default logger
     * instead.
     *
     */
    log_printer(std::string_view logger_name,
                log_level level = log_level::info);

    /**
     * @brief Initialize log printer with a logger
     *
     * @param [in] logger Logger instance. If `logger` is nullptr, log_printer
     * will use default logger instead.
     *
     */
    log_printer(std::shared_ptr<void> logger,
                log_level level = log_level::info);

    log_printer(const log_printer &) = default;
    log_printer & operator=(const log_printer &) = default;
    log_printer(log_printer &&) = default;
    log_printer & operator=(log_printer &&) = default;

    /**
     * Destructor
     */
    virtual ~log_printer();

    /**
     * @brief Populate global logger registry from configuration file
     *
     * @param [in] configuration_file Primary configuration file
     * @param [in] override_configuration_file Override file (optional)
     *
     * @exception May throw when file in given path(s) are not present or not in
     * expected toml structure.
     */
    static void load_configuration_file(
        std::string_view configuration_file,
        std::string_view override_configuration_file = {}) noexcept(false);

    /**
     * @brief Write a log through logger instance.
     * w/ source location support
     *
     * @tparam EventCode Event code of the log being logged
     * @tparam FormatString Format string type
     * @tparam Args Format string argument types
     *
     * @param [in] level Log level
     * @param [in] sloc Source location info
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     * @exception Exception-safe. Will swallow any exception thrown internally
     * and dump any failure to std::clog.
     */
    template <mad::meta::source_location Sloc = mad::meta::source_location{},
              bool CheckLogLevel = true, bool Enabled = MAD_LOG_ENABLED,
              typename FormatString, typename... Args>
    inline void log(log_level level, const FormatString & fmt,
                    Args &&... args) const noexcept {

        if constexpr (CheckLogLevel) {
            if (!should_log(level))
                return;
        }

        if constexpr (Enabled) {
            fmt::memory_buffer buf;

            try {
                // FIXME(mgilor): To benefit from compile time checks,
                // Find a way to perfectly forward string literal into this
                fmt::format_to(std::back_inserter(buf), fmt::runtime(fmt),
                               std::forward<Args>(args)...);
                log_impl(
                    level, Sloc, std::string_view{ buf.data(), buf.size() });
            }
            catch (const fmt::format_error & fe) {
                dump_format_error_to_clog(fe, fmt, std::forward<Args>(args)...);
                // LCOV_EXCL_START
            }
            catch (const std::exception & ex) {
                dump_any_error_to_clog(
                    ex, std::string_view{ buf.data(), buf.size() });
            }
            catch (...) {
                std::clog << "Logging failed, either format or log threw a "
                             "foreign object.\n";
            }
            // LCOV_EXCL_STOP
        } else {
            (void) fmt;
            (void) sizeof...(args);
        }
    }

    /**
     * @brief Write a log through logger instance, with log level of
     * `log_level::trace`.
     *
     * @tparam EventCode Event code of the exception being raised
     * @tparam FormatString Format string type
     * @tparam Args Format argument types
     *
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     * @exception Exception-safe. Will swallow any exception thrown internally
     * and dump any failure to std::clog.
     */
    template <mad::meta::source_location Sloc = mad::meta::source_location{},
              bool CheckLogLevel = true,
              bool Enabled = MAD_LOG_LEVEL_TRACE_ENABLED, typename FormatString,
              typename... Args>
    inline void log_trace(const FormatString & fmt,
                          Args &&... args) const noexcept {
        if constexpr (Enabled) {
            log<Sloc, CheckLogLevel>(
                log_level::trace, fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Write a log through logger instance, with log level of
     * `log_level::debug`.
     *
     * @tparam EventCode Event code of the exception being raised
     * @tparam FormatString Format string type
     * @tparam Args Format argument types
     *
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     * @exception Exception-safe. Will swallow any exception thrown internally
     * and dump any failure to std::clog.
     */
    template <mad::meta::source_location Sloc = mad::meta::source_location{},
              bool CheckLogLevel = true,
              bool Enabled = MAD_LOG_LEVEL_DEBUG_ENABLED, typename FormatString,
              typename... Args>
    inline void log_debug(const FormatString & fmt,
                          Args &&... args) const noexcept {
        if constexpr (Enabled) {
            log<Sloc, CheckLogLevel>(
                log_level::debug, fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * Write a log through logger instance, with log level of `log_level::info`.
     *
     * @tparam EventCode Event code of the exception being raised
     * @tparam FormatString Format string type
     * @tparam Args Format argument types
     *
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     * @exception Exception-safe. Will swallow any exception thrown internally
     * and dump any failure to std::clog.
     */
    template <mad::meta::source_location Sloc = mad::meta::source_location{},
              bool CheckLogLevel = true,
              bool Enabled = MAD_LOG_LEVEL_INFO_ENABLED, typename FormatString,
              typename... Args>
    inline void log_info(const FormatString & fmt,
                         Args &&... args) const noexcept {
        if constexpr (Enabled) {
            log<Sloc, CheckLogLevel>(
                log_level::info, fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Write a log through logger instance, with log level of
     * `log_level::warn`.
     *
     * @tparam EventCode Event code of the exception being raised
     * @tparam FormatString Format string type
     * @tparam Args Format argument types
     *
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     * @exception Exception-safe. Will swallow any exception thrown internally
     * and dump any failure to std::clog.
     */
    template <mad::meta::source_location Sloc = mad::meta::source_location{},
              bool CheckLogLevel = true,
              bool Enabled = MAD_LOG_LEVEL_WARN_ENABLED, typename FormatString,
              typename... Args>
    inline void log_warn(const FormatString & fmt,
                         Args &&... args) const noexcept {
        if constexpr (Enabled) {
            log<Sloc, CheckLogLevel>(
                log_level::warn, fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Write a log through logger instance, with log level of
     * `log_level::error`.
     *
     * @tparam EventCode Event code of the exception being raised
     * @tparam FormatString Format string type
     * @tparam Args Format argument types
     *
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     * @exception Exception-safe. Will swallow any exception thrown internally
     * and dump any failure to std::clog.
     */
    template <mad::meta::source_location Sloc = mad::meta::source_location{},
              bool CheckLogLevel = true,
              bool Enabled = MAD_LOG_LEVEL_ERROR_ENABLED, typename FormatString,
              typename... Args>
    inline void log_err(const FormatString & fmt,
                        Args &&... args) const noexcept {
        if constexpr (Enabled) {
            log<Sloc, CheckLogLevel>(
                log_level::error, fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Write a log through logger instance, with log level of
     * `log_level::critical`.
     *
     * @tparam EventCode Event code of the exception being raised
     * @tparam FormatString Format string type
     * @tparam Args Format argument types
     *
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     * @exception Exception-safe. Will swallow any exception thrown internally
     * and dump any failure to std::clog.
     */
    template <mad::meta::source_location Sloc = mad::meta::source_location{},
              bool CheckLogLevel = true,
              bool Enabled = MAD_LOG_LEVEL_CRITICAL_ENABLED,
              typename FormatString, typename... Args>
    inline void log_critical(const FormatString & fmt,
                             Args &&... args) const noexcept {
        if constexpr (Enabled) {
            log<Sloc, CheckLogLevel>(
                log_level::critical, fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Get current log level
     *
     * @return Current log level
     */
    [[nodiscard]] log_level get_log_level() const;

    /**
     * @brief Set log level to @p `level`
     *
     * @param [in] level Desired log level
     */
    void set_log_level(log_level level);

    /**
     * @brief Check whether logging is enabled for specific log level
     *
     * @param [in] message_log_level Log level of the message

     * @return True if a log should be written for given message log level,
     false otherwise.
     */
    [[nodiscard]] inline bool should_log(log_level message_log_level) const {
        return message_log_level >= current_level;
    }

private:
    /**
     * @brief Default source location to be used when not provided
     */
    static inline constexpr std::source_location default_sloc{};

    /**
     * @brief Log a message through `logger_instance`.
     *
     * @param [in] level Log level
     * @param [in] ec Event code
     * @param [in] sloc Call site source location
     * @param [in] message Message to log
     */
    void log_impl(log_level level, const mad::meta::source_location & sloc,
                  std::string_view message) const;

    /**
     * @brief Fallback log function to be used on format exceptions.
     *
     * @tparam FormatString Format string type
     * @tparam Args Format argument types
     *
     * @param [in] fe Thrown exception
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     */
    template <typename FormatString, typename... Args>
    static inline void dump_format_error_to_clog(const fmt::format_error & fe,
                                                 const FormatString & fmt,
                                                 Args &&... args) {
        // LCOV_EXCL_START
        std::clog << "Failed to format content, dumping params:\n";
        std::clog << "\nFormat error cause: " << fe.what() << "\n";
        std::clog << "Format string: " << fmt << "\n";
        std::clog << "Format arguments:";
        ((std::clog << ',' << std::forward<Args>(args)), ...);
        // LCOV_EXCL_STOP
    }

    // LCOV_EXCL_START
    /**
     * @brief Fallback log function to be used on exceptions.
     *
     * @tparam FormatString Format string type
     * @tparam Args Format argument types
     *
     * @param [in] ex Thrown exception
     * @param [in] fmt Format string
     * @param [in] args Format string arguments
     *
     */
    static inline void dump_any_error_to_clog(const std::exception & ex,
                                              std::string_view message) {
        std::clog << "Logging failed, dumping formatted message:\n";
        std::clog.write(
            message.data(), static_cast<std::streamsize>(message.size()));
        std::clog << "\n";
        std::clog << "\nError cause: " << ex.what() << "\n";
    }

    // LCOV_EXCL_STOP

    /**
     * @brief Spdlog logger object
     */
    mutable std::shared_ptr<void> logger_instance{};

    /**
     * @brief Current log level
     */
    log_level current_level{ mad::log_level::info };
};
} // namespace mad

// Macro definitions
#include <mad/log_macros.hpp>
