#pragma once

#include <mad/macro>

#include <flatbuffers/default_allocator.h>

#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

namespace mad::nexus {
/**
 * It's flatbuffers::detachedbuffer but:
 *
 * - without a allocator
 * - with a toggle to control the deallocation
 *   on destruction
 *
 * @tparam AutoCleanup Whether to destroy the buffer
 * upon destruction or not.
 */
template <bool AutoCleanup = true>
struct send_buffer {
    static constexpr std::size_t k_QuicBufStructSize = 16;
    [[maybe_unused]] static constexpr auto k_QuicBufStructAlignment = 8;
    /**
     * Sentinel value used as a placeholder for QUIC_BUFFER's space
     * until the real one is written.
     */
    [[maybe_unused]] constexpr static std::uint8_t
        quic_buf_sentinel [k_QuicBufStructSize] = { 0xDE, 0xAD, 0xBE, 0xEF,
                                                    0xBA, 0xAD, 0xC0, 0xDE,
                                                    0xCA, 0xFE, 0xBA, 0xBE,
                                                    0xDE, 0xAD, 0xFA, 0xCE };
    std::uint8_t * buf{ nullptr };
    std::size_t offset{ 0 };
    std::size_t buf_size{ 0 };

    send_buffer() {}

    send_buffer(const send_buffer &) = delete;
    send_buffer & operator=(const send_buffer &) = delete;
    send_buffer & operator=(send_buffer &&) = delete;

    auto size() const noexcept {
        return buf_size - offset;
    }

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

    std::uint32_t encoded_data_size() const noexcept {
        MAD_EXPECTS(size() >= sizeof(std::uint32_t));
        std::uint32_t val{};
        std::memcpy(&val, buf + offset, sizeof(val));
        return val;
    }

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

    template <bool B>
    send_buffer(send_buffer<B> && other) noexcept {
        // There's no possibility of being the same object
        // when the types are different for this class.
        std::swap(buf, other.buf);
        std::swap(offset, other.offset);
        std::swap(buf_size, other.buf_size);
    }

    ~send_buffer() {
        if (AutoCleanup) {
            // Deallocating a nullptr is defined behavior.
            ::flatbuffers::DefaultAllocator::dealloc(buf, buf_size);
        }
    }
};
} // namespace mad::nexus
