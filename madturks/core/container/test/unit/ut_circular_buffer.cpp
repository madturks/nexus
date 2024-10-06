/**
 * ______________________________________________________
 *
 * @file 		circular_buffer_test.cpp
 * @project 	spectre/kol-framework/
 * @author 		mkg <hello@mkg.dev>
 * @date 		17.10.2019
 *
 * @brief
 *
 * @disclaimer
 * This file is part of SPECTRE MMORPG game server project.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @copyright		2012-2019 Mustafa K. GILOR, All rights reserved.
 *
 * ______________________________________________________
 */

#include <mad/circular_buffer.hpp>
#include <mad/circular_buffer_pow2.hpp>
#include <mad/circular_buffer_vm.hpp>
#include <mad/random_bytegen.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstring>

using namespace mad;

static inline constexpr auto page_size = 4096; // ?

template <typename T>
struct cb_fast_fixture : public ::testing::Test {

    using circular_buffer_t = T;

    static void SetUpTestCase() {
        ASSERT_EQ(page_size, getpagesize());
    }

    static void TearDownTestCase() {}
};

template <typename T, std::size_t S>
struct type_wrapper {
    using type = T;
    static constexpr auto size = S;
};

// mad::circular_buffer_pow2,
using MyTypes = ::testing::Types<mad::circular_buffer_vm<vm_cb_backend_shm>,
                                 mad::circular_buffer_vm<vm_cb_backend_mmap>,
                                 mad::circular_buffer>;

class NameGenerator {
public:
    template <typename T>
    static std::string GetName(int) {
        if constexpr (std::is_same_v<
                          T, mad::circular_buffer_vm<vm_cb_backend_shm>>)
            return "shm";
        if constexpr (std::is_same_v<
                          T, mad::circular_buffer_vm<vm_cb_backend_mmap>>)
            return "mmap";
        if constexpr (std::is_same_v<T, mad::circular_buffer_pow2>)
            return "pow2";
        if constexpr (std::is_same_v<T, mad::circular_buffer>)
            return "basic";
    }
};

TYPED_TEST_SUITE(cb_fast_fixture, MyTypes, NameGenerator);

TYPED_TEST(cb_fast_fixture, push) {
    TypeParam alloc{ page_size };

    std::array<std::array<unsigned char, page_size / 4>, 4> put_bufs = { {} };
    std::array<std::array<unsigned char, page_size / 4>, 4> get_bufs = { {} };

    mad::random::bytegen_n(
        put_bufs [0], put_bufs [1], put_bufs [2], put_bufs [3]);
    EXPECT_TRUE(
        alloc.put_all(put_bufs [0], put_bufs [1], put_bufs [2], put_bufs [3]));
    EXPECT_TRUE(
        alloc.get_all(get_bufs [0], get_bufs [1], get_bufs [2], get_bufs [3]));
    EXPECT_TRUE(std::equal(put_bufs.begin(), put_bufs.end(), get_bufs.begin()));
}

TYPED_TEST(cb_fast_fixture, push_full) {
    TypeParam alloc{ page_size };
    std::array<std::array<unsigned char, page_size / 4>, 4> put_bufs = { {} };

    mad::random::bytegen_n(
        put_bufs [0], put_bufs [1], put_bufs [2], put_bufs [3]);

    EXPECT_TRUE(
        alloc.put_all(put_bufs [0], put_bufs [1], put_bufs [2], put_bufs [3]));
    // Check we got no space
    ASSERT_EQ(alloc.empty_space(), 0);
    // Check we cannot push any more data (which would override unprocessed
    // data)
    ASSERT_FALSE(alloc.put(put_bufs [0].data(), 1));
}

TYPED_TEST(cb_fast_fixture, push_wrap_around) {
    TypeParam alloc{ page_size };

    std::array<std::array<unsigned char, page_size / 4>, 4> put_bufs = { {} };
    std::array<std::array<unsigned char, page_size / 4>, 4> get_bufs = { {} };

    mad::random::bytegen_n(
        put_bufs [0], put_bufs [1], put_bufs [2], put_bufs [3]);
    EXPECT_TRUE(
        alloc.put_all(put_bufs [0], put_bufs [1], put_bufs [2], put_bufs [3]));
    ASSERT_FALSE(alloc.empty_space());
    EXPECT_TRUE(alloc.get_all(get_bufs [0], get_bufs [1])); //
    // This will move head to center, next push will be between tail-head
    ASSERT_EQ(alloc.empty_space(), page_size / 2);

    EXPECT_TRUE(
        alloc.put_all(put_bufs [3], put_bufs [2])); // this will wrap around.
    EXPECT_TRUE(
        alloc.get_all(get_bufs [0], get_bufs [1], get_bufs [2], get_bufs [3]));

    EXPECT_TRUE(std::equal(
        get_bufs [0].begin(), get_bufs [0].end(), put_bufs [2].begin()));
    EXPECT_TRUE(std::equal(
        get_bufs [1].begin(), get_bufs [1].end(), put_bufs [3].begin()));
    EXPECT_TRUE(std::equal(
        get_bufs [2].begin(), get_bufs [2].end(), put_bufs [3].begin()));
    EXPECT_TRUE(std::equal(
        get_bufs [3].begin(), get_bufs [3].end(), put_bufs [2].begin()));
}

TYPED_TEST(cb_fast_fixture, ConstructDestruct) {
    TypeParam buffer_{ page_size };

    // Ensure buffer is constructed properly
    EXPECT_EQ(buffer_.empty_space(), 4096);
    EXPECT_EQ(buffer_.consumed_space(), 0);
}

TYPED_TEST(cb_fast_fixture, PutData) {
    TypeParam buffer_{ page_size };
    std::array<std::uint8_t, 100> data;
    std::fill(data.begin(), data.end(), 'A');

    // Test putting data into the buffer
    EXPECT_TRUE(buffer_.put(data.data(), data.size()));
    EXPECT_EQ(buffer_.consumed_space(), 100);
    EXPECT_EQ(buffer_.empty_space(), 3996);
}

TYPED_TEST(cb_fast_fixture, GetData) {
    TypeParam buffer_{ page_size };
    std::array<std::uint8_t, 100> data;

    std::fill(data.begin(), data.end(), 'A');

    // Put data into buffer
    EXPECT_TRUE(buffer_.put(data.data(), data.size()));

    // Get data from buffer
    std::array<std::uint8_t, 100> retrieved_data;
    EXPECT_TRUE(buffer_.get(retrieved_data.data(), retrieved_data.size()));
    EXPECT_EQ(buffer_.consumed_space(), 0);
    EXPECT_EQ(buffer_.empty_space(), 4096);
    EXPECT_EQ(std::memcmp(data.data(), retrieved_data.data(), data.size()), 0);
}

TYPED_TEST(cb_fast_fixture, PeekData) {
    TypeParam buffer_{ page_size };
    std::array<std::uint8_t, 100> data;
    std::fill(data.begin(), data.end(), 'A');

    // Put data into buffer
    EXPECT_TRUE(buffer_.put(data.data(), data.size()));

    // Peek data from buffer without removing it
    std::array<std::uint8_t, 100> peeked_data;
    EXPECT_TRUE(buffer_.peek(peeked_data.data(), peeked_data.size()));
    EXPECT_EQ(buffer_.consumed_space(), 100);
    EXPECT_EQ(std::memcmp(data.data(), peeked_data.data(), data.size()), 0);
}

TYPED_TEST(cb_fast_fixture, BufferOverflow) {
    TypeParam buffer_{ page_size };
    std::array<std::uint8_t, 5000> data;
    std::fill(data.begin(), data.end(), 'A');

    // Try to put more data than buffer can hold
    EXPECT_FALSE(buffer_.put(data.data(), data.size()));
    EXPECT_EQ(buffer_.consumed_space(), 0);
    EXPECT_EQ(buffer_.empty_space(), 4096);
}

TYPED_TEST(cb_fast_fixture, BufferUnderflow) {
    TypeParam buffer_{ page_size };
    std::array<std::uint8_t, 100> data;

    // Try to get data when buffer is empty
    EXPECT_FALSE(buffer_.get(data.data(), data.size()));
    EXPECT_EQ(buffer_.consumed_space(), 0);
    EXPECT_EQ(buffer_.empty_space(), 4096);
}

TYPED_TEST(cb_fast_fixture, TransferData) {
    if constexpr (std::is_same_v<TypeParam,
                                 mad::circular_buffer_vm<vm_cb_backend_mmap>> ||
                  std::is_same_v<TypeParam,
                                 mad::circular_buffer_vm<vm_cb_backend_shm>>) {
        TypeParam buffer_{ page_size };
        TypeParam buffer2{ 4096 };

        std::array<std::uint8_t, 100> data;
        std::fill(data.begin(), data.end(), 'A');

        // Put data into buffer 1
        EXPECT_TRUE(buffer_.put(data.data(), data.size()));

        // Transfer data to buffer 2
        buffer_.transfer(buffer2);
        EXPECT_EQ(buffer_.consumed_space(), 0);
        EXPECT_EQ(buffer2.consumed_space(), 100);

        // Retrieve data from buffer 2
        std::array<std::uint8_t, 100> retrieved_data;
        EXPECT_TRUE(buffer2.get(retrieved_data.data(), retrieved_data.size()));
        EXPECT_EQ(
            std::memcmp(data.data(), retrieved_data.data(), data.size()), 0);
    }
}

TYPED_TEST(cb_fast_fixture, AutoAlignToPage) {
    // Constructing buffer with size not aligned to page size
    if constexpr (std::is_same_v<TypeParam,
                                 mad::circular_buffer_vm<vm_cb_backend_mmap>> ||
                  std::is_same_v<TypeParam,
                                 mad::circular_buffer_vm<vm_cb_backend_shm>>) {

        {
            TypeParam aligned_buffer(
                1024, typename TypeParam::auto_align_to_page{});

            // Verify alignment to the page size
            EXPECT_EQ(aligned_buffer.empty_space() %
                          static_cast<std::size_t>(getpagesize()),
                      0);
            EXPECT_EQ(aligned_buffer.empty_space(), getpagesize());
        }

        {
            TypeParam aligned_buffer(static_cast<std::size_t>(getpagesize()),
                                     typename TypeParam::auto_align_to_page{});

            // Verify alignment to the page size
            EXPECT_EQ(aligned_buffer.empty_space() %
                          static_cast<std::size_t>(getpagesize()),
                      0);
            EXPECT_EQ(aligned_buffer.empty_space(), getpagesize());
        }

        {
            TypeParam aligned_buffer(
                static_cast<std::size_t>(getpagesize() + 1),
                typename TypeParam::auto_align_to_page{});

            // Verify alignment to the page size
            EXPECT_EQ(aligned_buffer.empty_space() %
                          static_cast<std::size_t>(getpagesize()),
                      0);
            EXPECT_EQ(aligned_buffer.empty_space(), getpagesize() * 2);
        }
    }
}

TYPED_TEST(cb_fast_fixture, ClearBuffer) {
    TypeParam buffer_{ page_size };
    std::array<std::uint8_t, 100> data;
    std::fill(data.begin(), data.end(), 'A');

    // Put data into buffer
    EXPECT_TRUE(buffer_.put(data.data(), data.size()));
    EXPECT_EQ(buffer_.consumed_space(), 100);

    // Clear buffer
    buffer_.clear();
    EXPECT_EQ(buffer_.consumed_space(), 0);
    EXPECT_EQ(buffer_.empty_space(), 4096);
}

TYPED_TEST(cb_fast_fixture, PeekAssign) {
    TypeParam buffer_{ page_size };
    std::array<std::uint8_t, 100> data;
    std::fill(data.begin(), data.end(), 'A');

    // Put data into buffer
    EXPECT_TRUE(buffer_.put(data.data(), data.size()));

    // Use peek_assign
    std::vector<std::uint8_t> assigned_data;
    EXPECT_TRUE(buffer_.peek_assign(assigned_data, data.size()));
    EXPECT_EQ(assigned_data.size(), data.size());
    EXPECT_EQ(std::memcmp(data.data(), assigned_data.data(), data.size()), 0);
    EXPECT_EQ(buffer_.consumed_space(), 100);
}

TYPED_TEST(cb_fast_fixture, LargeDataPutGet) {
    TypeParam buffer_{ page_size };
    // Test putting and getting very large data close to buffer's size
    std::vector<std::uint8_t> large_data(page_size - 1, 'L');

    // Fill buffer
    EXPECT_TRUE(buffer_.put(large_data.data(), large_data.size()));
    EXPECT_EQ(buffer_.consumed_space(), large_data.size());

    // Retrieve data
    std::vector<std::uint8_t> retrieved_data(page_size - 1);
    EXPECT_TRUE(buffer_.get(retrieved_data.data(), retrieved_data.size()));
    EXPECT_EQ(retrieved_data, large_data);
}

TYPED_TEST(cb_fast_fixture, SmallDataHighFrequency) {
    TypeParam buffer_{ page_size };
    // Insert and retrieve small data frequently
    std::array<std::uint8_t, 1> small_data = { 'S' };
    for (std::size_t i = 0; i < page_size; ++i) {
        EXPECT_TRUE(buffer_.put(small_data.data(), small_data.size()));
        std::array<std::uint8_t, 1> retrieved_data;
        EXPECT_TRUE(buffer_.get(retrieved_data.data(), retrieved_data.size()));
        EXPECT_EQ(retrieved_data [0], small_data [0]);
    }
}

TYPED_TEST(cb_fast_fixture, RapidPutAndClear) {
    TypeParam buffer_{ page_size };
    // Repeatedly put and clear data to test buffer's resilience
    for (int i = 0; i < 1000; ++i) {
        std::array<std::uint8_t, 100> data;
        std::fill(data.begin(), data.end(), 'R');

        EXPECT_TRUE(buffer_.put(data.data(), data.size()));
        EXPECT_EQ(buffer_.consumed_space(), data.size());

        buffer_.clear();
        EXPECT_EQ(buffer_.consumed_space(), 0);
        EXPECT_EQ(buffer_.empty_space(), page_size);
    }
}

TYPED_TEST(cb_fast_fixture, OverflowUnderflowStress) {
    TypeParam buffer_{ page_size };
    // Continuously try to overflow and underflow the buffer
    std::vector<std::uint8_t> large_data(
        page_size + 100, 'O'); // Intentionally larger than buffer
    for (int i = 0; i < 500; ++i) {
        EXPECT_FALSE(
            buffer_.put(large_data.data(),
                        large_data.size())); // Should fail due to overflow
    }

    std::array<std::uint8_t, 1> small_data;
    for (int i = 0; i < 500; ++i) {
        EXPECT_FALSE(
            buffer_.get(small_data.data(),
                        small_data.size())); // Should fail due to underflow
    }
}

TYPED_TEST(cb_fast_fixture, RapidTransfer) {
    if constexpr (std::is_same_v<TypeParam,
                                 mad::circular_buffer_vm<vm_cb_backend_mmap>> ||
                  std::is_same_v<TypeParam,
                                 mad::circular_buffer_vm<vm_cb_backend_shm>>) {
        TypeParam buffer_{ page_size };
        // Test rapid transfers between buffers
        TypeParam buffer2{ page_size };

        for (int i = 0; i < 500; ++i) {
            std::array<std::uint8_t, 100> data;
            std::fill(data.begin(), data.end(), 'T');

            EXPECT_TRUE(buffer_.put(data.data(), data.size()));
            buffer_.transfer(buffer2);

            EXPECT_EQ(buffer_.consumed_space(), 0);
            EXPECT_EQ(buffer2.consumed_space(), data.size());

            buffer2.clear();
        }
    }
}
