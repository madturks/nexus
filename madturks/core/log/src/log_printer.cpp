

#include "mad/meta/source_location.hpp"
#include <mad/log_printer.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#define CPPTOML_NO_RTTI
#include <spdlog_setup/spdlog_setup.hpp>

#pragma GCC diagnostic pop

#include <string>
#include <memory>

// (mgilor): We need to find a way to avoid spdlog's global registry
// since it is static. It causes all kind of weird issues when being
// linked through another static library.

auto spdlog_get_logger(auto var) {
    return spdlog::get(std::string{var});
}

auto spdlog_default_logger() {
    static auto default_logger = []() {
#ifdef _WIN32
        auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
        auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif

        const char * default_logger_name = "";

        // spdlog::set_formatter(std::make_unique<spdlog::pattern_formatter>());
        return std::make_shared<spdlog::logger>(default_logger_name, std::move(color_sink));
    }();
    return default_logger;
}

namespace mad {

    log_printer::log_printer(std::string_view logger_name) : log_printer::log_printer(spdlog_get_logger(logger_name)) {}

    log_printer::log_printer(std::shared_ptr<void> logger) {
        if (logger_instance = logger; nullptr == logger_instance) {
            // (mgilor): We cannot use default logger as a fallback since
            // spdlog::default_logger() lives in this library's address space
            // and cannot be used across library boundaries.
            // see: https://github.com/gabime/spdlog/issues/1154

            // Thus, we have to roll our own default instance.
            logger_instance = spdlog_default_logger();
        }
        assert(logger_instance);
        // Save current log level
        current_level = static_cast<log_level>(static_cast<spdlog::logger *>(logger_instance.get())->level());
    }

    log_printer::~log_printer() = default;

    void log_printer::load_configuration_file(std::string_view configuration_file,
                                              std::string_view override_configuration_file) noexcept(false) {
        if (override_configuration_file.empty()) {
            spdlog_setup::from_file(std::string{configuration_file});
            return;
        }
        // LCOV_EXCL_START
        spdlog_setup::from_file_and_override(std::string{configuration_file}, std::string{override_configuration_file});
        // LCOV_EXCL_STOP
    }

    void log_printer::log_impl(log_level level,  const mad::meta::source_location & sloc, std::string_view message) const {
        assert(logger_instance);

        struct logger_type : public spdlog::logger {
            using spdlog::logger::sink_it_; // hack to call sink_it_ directly
        };

        auto * logger = static_cast<logger_type *>(logger_instance.get());

        spdlog::source_loc spdlog_sloc(sloc.file, sloc.line, sloc.function);

        spdlog::details::log_msg log_msg{spdlog_sloc, logger->name(), static_cast<spdlog::level::level_enum>(level), message};
        // (mgilor): To avoid spdlog internal level check and such, call sink_it directly
        logger->sink_it_(log_msg);
    }

    log_level log_printer::get_log_level() const {
        return current_level;
    }

    void log_printer::set_log_level(log_level level) {
        assert(logger_instance);
        try {
            static_cast<spdlog::logger *>(logger_instance.get())->set_level(static_cast<spdlog::level::level_enum>(level));
            current_level = static_cast<log_level>(static_cast<spdlog::logger *>(logger_instance.get())->level());
        }
        catch (...) {
        }
    }
} // namespace mad
