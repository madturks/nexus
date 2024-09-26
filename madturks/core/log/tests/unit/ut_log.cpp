

#include <mad/log_printer.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <filesystem>
#include <cstring>
#include <fstream>

#include <mad/temputils.hpp>

static const char * default_config = R"(# level is optional for both sinks and loggers
# level for error logging is 'err', not 'error'
# _st => single threaded, _mt => multi threaded
# syslog_sink is automatically thread-safe by default, no need for _mt suffix

# max_size supports suffix
# - T (terabyte)
# - G (gigabyte)
# - M (megabyte)
# - K (kilobyte)
# - or simply no suffix (byte)

# check out https: // github.com/gabime/spdlog/wiki/3.-Custom-formatting
global_pattern = "[%Y-%m-%dT%T%z] [%L] <%n> < %s:%# %!> [%t]: %v"

# Sink declarations
[[sink]]
name = "color_console_st"
type = "color_stdout_sink_st"
level = "trace"

# Log patterns
[[pattern]]
name = "default_module_pattern"
value = "%c-%L: %v"

# Loggers

# Classifier modules
[[logger]]
name = "console"
sinks = [ "color_console_st" ]
level = "trace"

)";

enum class example_log_event_id : std::uint32_t
{
    eid_0,
    eid_1,
    eid_2
};

struct value_provider {
    MOCK_METHOD0(get_text, const char *(void) );
};

class log_level_fixture : public ::testing::TestWithParam<mad::log_level> {
public:
    virtual void SetUp() override {
        uut.set_log_level(mad::log_level::off);
    }

    mad::log_printer uut{"console"};
    value_provider vp;
};

TEST_P(log_level_fixture, level_check_happy_path) {
    EXPECT_CALL(vp, get_text()).Times(1).WillOnce(testing::Return("Test"));
    uut.set_log_level(GetParam());
    ASSERT_EQ(GetParam(), uut.get_log_level());
    MAD_LOG_I(uut, GetParam(), "This should be printed: {}", vp.get_text());
}

TEST_P(log_level_fixture, level_check_false_path) {
    EXPECT_CALL(vp, get_text()).Times(0);
    uut.set_log_level(static_cast<mad::log_level>(std::to_underlying(GetParam()) + 1));
    ASSERT_EQ(static_cast<mad::log_level>(std::to_underlying(GetParam()) + 1), uut.get_log_level());
    MAD_LOG_I(uut, GetParam(), "This should not be printed: {}", vp.get_text());
}

INSTANTIATE_TEST_SUITE_P(validate_level, log_level_fixture,
                         ::testing::Values(mad::log_level::trace, mad::log_level::debug, mad::log_level::info, mad::log_level::warn,
                                           mad::log_level::error, mad::log_level::critical));

TEST(log_printer, default_logger) {
    mad::log_printer def{std::shared_ptr<void>(nullptr)};
    ASSERT_NO_THROW(def.set_log_level(mad::log_level::info));
    ASSERT_NO_THROW(def.log_info<>("aa"));
    ASSERT_NO_THROW(def.log_info<>("aa"));
    ASSERT_NO_THROW(def.log_trace<>("aa"));
    ASSERT_NO_THROW(def.set_log_level(mad::log_level::critical));
}

class log_fixture : public ::testing::Test {
public:
    virtual void SetUp() override {
        uut.set_log_level(mad::log_level::off);
    }

    mad::log_printer uut{"console"};
    value_provider vp;
};

TEST_F(log_fixture, cause_format_exception) {
    try {
        uut.set_log_level(mad::log_level::info);
        MAD_LOG_INFO_I(uut, "This should cause format exception: {}{}{}");
        SUCCEED();
    }
    catch (...) {
        FAIL();
    }
}

#define LOG_TEST_CASE_SHOULD_LOG(LEVEL, MACRO_PREFIX, MACRO_SUFFIX)                                                                        \
    TEST_F(log_fixture, should_log_##MACRO_PREFIX##MACRO_SUFFIX) {                                                                         \
        int rem = std::to_underlying(LEVEL);                                                                                               \
        EXPECT_CALL(vp, get_text()).Times(rem + 1);                                                                                        \
        ON_CALL(vp, get_text()).WillByDefault(testing::Return("Mock Me"));                                                                 \
        while (rem >= 0) {                                                                                                                 \
            uut.set_log_level(static_cast<mad::log_level>(rem));                                                                           \
            MACRO_PREFIX##MACRO_SUFFIX##_I(uut, "This should be logged {}", vp.get_text());                                             \
            --rem;                                                                                                                         \
        }                                                                                                                                  \
        SUCCEED();                                                                                                                         \
    }

#define LOG_TEST_CASE_SHOULD_NOT_LOG(LEVEL, MACRO_PREFIX, MACRO_SUFFIX)                                                                    \
    TEST_F(log_fixture, should_not_log_##MACRO_PREFIX##MACRO_SUFFIX) {                                                                     \
        EXPECT_CALL(vp, get_text()).Times(0);                                                                                              \
        for (int i = std::to_underlying(LEVEL) + 1; i < std::to_underlying(mad::log_level::max); i++) {                                    \
            uut.set_log_level(static_cast<mad::log_level>(i));                                                                             \
            MACRO_PREFIX##MACRO_SUFFIX##_I(uut, "This should not be logged {}", vp.get_text());                                         \
        }                                                                                                                                  \
        SUCCEED();                                                                                                                         \
    }

LOG_TEST_CASE_SHOULD_LOG(mad::log_level::trace, MAD_LOG_, TRACE)
LOG_TEST_CASE_SHOULD_LOG(mad::log_level::debug, MAD_LOG_, DEBUG)
LOG_TEST_CASE_SHOULD_LOG(mad::log_level::info, MAD_LOG_, INFO)
LOG_TEST_CASE_SHOULD_LOG(mad::log_level::warn, MAD_LOG_, WARN)
LOG_TEST_CASE_SHOULD_LOG(mad::log_level::error, MAD_LOG_, ERROR)
LOG_TEST_CASE_SHOULD_LOG(mad::log_level::critical, MAD_LOG_, CRITICAL)

LOG_TEST_CASE_SHOULD_NOT_LOG(mad::log_level::trace, MAD_LOG_, TRACE)
LOG_TEST_CASE_SHOULD_NOT_LOG(mad::log_level::debug, MAD_LOG_, DEBUG)
LOG_TEST_CASE_SHOULD_NOT_LOG(mad::log_level::info, MAD_LOG_, INFO)
LOG_TEST_CASE_SHOULD_NOT_LOG(mad::log_level::warn, MAD_LOG_, WARN)
LOG_TEST_CASE_SHOULD_NOT_LOG(mad::log_level::error, MAD_LOG_, ERROR)
LOG_TEST_CASE_SHOULD_NOT_LOG(mad::log_level::critical, MAD_LOG_, CRITICAL)
// LOG_TEST_CASE_SHOULD_NOT_LOG(mad::log_level::error,  MAD_RAISE_,  ERROR)
// LOG_TEST_CASE_SHOULD_NOT_LOG(mad::log_level::critical,MAD_RAISE_, CRITICAL)

static auto toml_file_path = std::filesystem::temp_directory_path().append("ncf.log.default.toml");

static void write_default_config() {

    if (auto result = mad::make_temp_file()) {
        auto & [path, ofs] = (*result);
        ofs.write(default_config, static_cast<std::streamsize>(std::strlen(default_config)));
        ofs.close();
        std::filesystem::rename(path, toml_file_path);
    }
}

int main(int argc, char * argv []) {
    write_default_config();
    mad::log_printer::load_configuration_file(toml_file_path.string());
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
