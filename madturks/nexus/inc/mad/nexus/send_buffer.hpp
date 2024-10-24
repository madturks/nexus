/******************************************************
 * Raw ptr/size wrapper type.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/macro>

#include <flatbuffers/default_allocator.h>

#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

namespace mad::nexus {
/******************************************************
 * QUIC send buffer type.
 *
 * @tparam AutoCleanup Whether to delete the owned resource
 * on destruction.
 ******************************************************/
template <bool AutoCleanup = true>
struct send_buffer {

    /******************************************************
     * The size of the QUIC_BUFFER type.
     ******************************************************/
    static constexpr std::size_t k_QuicBufStructSize = 16;

    /******************************************************
     * The alignment requirement of the QUIC_BUFFER type.
     ******************************************************/
    [[maybe_unused]] static constexpr auto k_QuicBufStructAlignment = 8;

    /******************************************************
     * Sentinel value used as a placeholder for QUIC_BUFFER's space
     * until the real one is written.
     ******************************************************/
    [[maybe_unused]] constexpr static std::uint8_t
        quic_buf_sentinel [k_QuicBufStructSize] = { 0xDE, 0xAD, 0xBE, 0xEF,
                                                    0xBA, 0xAD, 0xC0, 0xDE,
                                                    0xCA, 0xFE, 0xBA, 0xBE,
                                                    0xDE, 0xAD, 0xFA, 0xCE };

    /******************************************************
     * The default constructor.
     ******************************************************/
    send_buffer() {}

    send_buffer(const send_buffer &) = delete;
    send_buffer & operator=(const send_buffer &) = delete;
    send_buffer & operator=(send_buffer &&) = delete;

    /******************************************************
     * The space used.
     ******************************************************/
    auto size() const noexcept {
        return buf_size - offset;
    }

    /******************************************************
     * The reserved space for the QUIC_BUFFER.
     ******************************************************/
    std::span<std::uint8_t> quic_buffer_span() noexcept {
        MAD_EXPECTS(buf);
        MAD_EXPECTS(buf_size >= k_QuicBufStructSize);
        MAD_EXPECTS(size() >= k_QuicBufStructSize);
        auto ptr{ buf + buf_size - k_QuicBufStructSize };
        MAD_ENSURES(ptr >= buf && ptr <= (buf + buf_size));

        MAD_ENSURES(0 == std::memcmp(ptr, quic_buf_sentinel,
                                     sizeof(quic_buf_sentinel)));

        return { ptr, k_QuicBufStructSize };
    }

    /******************************************************
     * The size which is embedded into the data itself.
     ******************************************************/
    std::uint32_t encoded_data_size() const noexcept {
        MAD_EXPECTS(size() >= sizeof(std::uint32_t));
        std::uint32_t val{};
        std::memcpy(&val, buf + offset, sizeof(val));
        return val;
    }

    /******************************************************
     * User data part of the buffer.
     ******************************************************/
    std::span<std::uint8_t> data_span() const noexcept {
        MAD_EXPECTS(buf);
        MAD_EXPECTS(offset <= buf_size);
        MAD_EXPECTS(size() >= k_QuicBufStructSize);
        auto ptr{ buf + offset };
        auto sz{ size() - k_QuicBufStructSize };
        MAD_ENSURES(ptr >= buf && ptr <= (buf + buf_size));
        MAD_ENSURES(sz == encoded_data_size() + sizeof(std::uint32_t));
        return { ptr, sz };
    }

    /******************************************************
     * Move constructor
     *
     * @param other from
     ******************************************************/
    template <bool B>
    send_buffer(send_buffer<B> && other) noexcept {
        // There's no possibility of being the same object
        // when the types are different for this class.
        std::swap(buf, other.buf);
        std::swap(offset, other.offset);
        std::swap(buf_size, other.buf_size);
    }

    /******************************************************
     * Destroy the send buffer object.
     ******************************************************/
    ~send_buffer() {
        if (AutoCleanup) {
            // Deallocating a nullptr is defined behavior.
            ::flatbuffers::DefaultAllocator::dealloc(buf, buf_size);
        }
    }

    std::uint8_t * buf{ nullptr };
    std::size_t offset{ 0 };
    std::size_t buf_size{ 0 };
};
} // namespace mad::nexus
