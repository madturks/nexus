#include <mad/random.hpp>
#include <numeric>
#include <vector>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <gtest/gtest.h>

namespace mad::test {

    TEST(RandomTest, FillSpanWithArithmeticBoundary) {
        std::vector<int> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<int> bounds{1, 100};
        mad::random::fill_span<int>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithValueRange) {
        std::vector<int> vec(10);
        std::span vec_span{vec};
        std::vector<int> value_range = {1, 2, 3, 4, 5};
        mad::random::fill_span<int>(vec_span, value_range);

        for (const auto & val : vec) {
            ASSERT_NE(std::find(value_range.begin(), value_range.end(), val), value_range.end());
        }
    }

    TEST(RandomTest, FillSpanWithLargeArithmeticBoundary) {
        std::vector<int> vec(1000);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<int> bounds{1, 1000};
        mad::random::fill_span<int>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithLargeValueRange) {
        std::vector<int> vec(1000);
        std::vector<int> value_range(100);
        std::span vec_span{vec};
        std::iota(value_range.begin(), value_range.end(), 1); // Fill value_range with values from 1 to 100
        mad::random::fill_span<int>(vec_span, value_range);

        for (const auto & val : vec) {
            ASSERT_NE(std::find(value_range.begin(), value_range.end(), val), value_range.end());
        }
    }

    TEST(RandomTest, FillSpanWithFloats) {
        std::vector<float> vec(10);
        mad::random::arithmetic_boundary<float> bounds{1.0f, 100.0f};
        std::span vec_span{vec};
        mad::random::fill_span<float>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithDoubles) {
        std::vector<double> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<double> bounds{1.0, 100.0};
        mad::random::fill_span<double>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithNegativeBounds) {
        std::vector<int> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<int> bounds{-100, -1};
        mad::random::fill_span<int>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithMixedValueRange) {
        std::vector<int> vec(10);
        std::span vec_span{vec};
        std::vector<int> value_range = {-5, -3, 0, 3, 5};
        mad::random::fill_span<int>(vec_span, value_range);

        for (const auto & val : vec) {
            ASSERT_NE(std::find(value_range.begin(), value_range.end(), val), value_range.end());
        }
    }

    TEST(RandomTest, FillSpanWithEmptyValueRange) {
        std::vector<int> expected_vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        std::vector<int> vec          = expected_vec;

        std::span vec_span{vec};
        std::vector<int> value_range = {};
        ASSERT_EQ(expected_vec, vec);
    }

    TEST(RandomTest, FillSpanWithSingleValueRange) {
        std::vector<int> vec(10);
        std::span vec_span{vec};
        std::vector<int> value_range = {42};
        mad::random::fill_span<int>(vec_span, value_range);

        for (const auto & val : vec) {
            ASSERT_EQ(val, 42);
        }
    }

    // Additional tests for all arithmetic types with edge cases

    TEST(RandomTest, FillSpanWithCharBoundaryEdgeCases) {
        std::vector<char> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<char> bounds{std::numeric_limits<char>::min(), std::numeric_limits<char>::max()};
        mad::random::fill_span<char>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithInt8BoundaryEdgeCases) {
        std::vector<int8_t> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<int8_t> bounds{std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max()};
        mad::random::fill_span<int8_t>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithInt16BoundaryEdgeCases) {
        std::vector<int16_t> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<int16_t> bounds{std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()};
        mad::random::fill_span<int16_t>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithInt32BoundaryEdgeCases) {
        std::vector<int32_t> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<int32_t> bounds{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()};
        mad::random::fill_span<int32_t>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithInt64BoundaryEdgeCases) {
        std::vector<int64_t> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<int64_t> bounds{std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max()};
        mad::random::fill_span<int64_t>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithUInt8BoundaryEdgeCases) {
        std::vector<uint8_t> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<uint8_t> bounds{std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max()};
        mad::random::fill_span<uint8_t>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithUInt16BoundaryEdgeCases) {
        std::vector<uint16_t> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<uint16_t> bounds{std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()};
        mad::random::fill_span<uint16_t>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithUInt32BoundaryEdgeCases) {
        std::vector<uint32_t> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<uint32_t> bounds{std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max()};
        mad::random::fill_span<uint32_t>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithUInt64BoundaryEdgeCases) {
        std::vector<uint64_t> vec(10);
        std::span vec_span{vec};
        mad::random::arithmetic_boundary<uint64_t> bounds{std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max()};
        mad::random::fill_span<uint64_t>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithFloatBoundaryEdgeCases) {
        std::vector<float> vec(10);
        std::span vec_span{vec};
        // regarding max/2 see: https://timsong-cpp.github.io/cppwp/rand.dist.uni.real#2
        mad::random::arithmetic_boundary<float> bounds{-std::numeric_limits<float>::max()/2, std::numeric_limits<float>::max()/2};
        mad::random::fill_span<float>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }

    TEST(RandomTest, FillSpanWithDoubleBoundaryEdgeCases) {
        std::vector<double> vec(10);
        std::span vec_span{vec};
        // regarding max/2 see: https://timsong-cpp.github.io/cppwp/rand.dist.uni.real#2
        mad::random::arithmetic_boundary<double> bounds{-std::numeric_limits<double>::max()/2, std::numeric_limits<double>::max()/2};
        mad::random::fill_span<double>(vec_span, bounds);

        for (const auto & val : vec) {
            ASSERT_GE(val, bounds.lower);
            ASSERT_LE(val, bounds.upper);
        }
    }
} // namespace mad::test

int main(int argc, char ** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
