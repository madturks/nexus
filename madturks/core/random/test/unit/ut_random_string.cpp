#include <mad/random_string.hpp>

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace mad::random::test {

TEST(RandomStringTest, AsciiAlphanumericCharset) {
    const auto & charset = ascii_alphanumeric_charset();
    ASSERT_FALSE(charset.empty());
    for (char c : charset) {
        ASSERT_TRUE((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z'));
    }
}

TEST(RandomStringTest, AsciiNumberCharset) {
    const auto & charset = ascii_number_charset();
    ASSERT_FALSE(charset.empty());
    for (char c : charset) {
        ASSERT_TRUE(c >= '0' && c <= '9');
    }
}

TEST(RandomStringTest, AsciiLowercaseCharset) {
    const auto & charset = ascii_lowercase_charset();
    ASSERT_FALSE(charset.empty());
    for (char c : charset) {
        ASSERT_TRUE(c >= 'a' && c <= 'z');
    }
}

TEST(RandomStringTest, AsciiUppercaseCharset) {
    const auto & charset = ascii_uppercase_charset();
    ASSERT_FALSE(charset.empty());
    for (char c : charset) {
        ASSERT_TRUE(c >= 'A' && c <= 'Z');
    }
}

TEST(RandomStringTest, GenerateRandomString) {
    auto random_string = generate<std::string>(16, 256);
    ASSERT_GE(random_string.length(), 16);
    ASSERT_LE(random_string.length(), 256);
}

TEST(RandomStringTest, FillRandomStrings) {
    std::vector<std::string> strings(10);
    fill(strings.begin(), strings.end(), 16, 256);
    for (const auto & str : strings) {
        ASSERT_GE(str.length(), 16);
        ASSERT_LE(str.length(), 256);
    }
}

TEST(RandomStringTest, MakeRandomStringVector) {
    auto strings = make<std::vector<std::string>>(10, 16, 256);
    ASSERT_EQ(strings.size(), 10);
    for (const auto & str : strings) {
        ASSERT_GE(str.length(), 16);
        ASSERT_LE(str.length(), 256);
    }
}

TEST(RandomStringTest, MakeRandomStringSet) {
    auto strings = make<std::set<std::string>>(10, 16, 256);
    ASSERT_EQ(strings.size(), 10);
    for (const auto & str : strings) {
        ASSERT_GE(str.length(), 16);
        ASSERT_LE(str.length(), 256);
    }
}

} // namespace mad::random::test
