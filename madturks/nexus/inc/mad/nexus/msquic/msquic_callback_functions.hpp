#pragma once

// WORK-IN-PROGRESS

#include <bit>
#include <cstdint>

namespace mad::nexus {

enum class callback_status : std::uint32_t
{
    success = 0
};

/**
 * @brief The callback function for incoming stream data.
 *
 * The callback transfers the received data to stream-specific
 * circular receive buffer for parsing.
 *
 * @param sctx The owning stream
 * @param event The receive event details
 *
 * @return QUIC_STATUS Return code indicating callback result
 */
callback_status StreamCallbackReceive(auto & sctx, auto & event) {

    MAD_EXPECTS(sctx.on_data_received);
    MAD_EXPECTS(event.BufferCount > 0);
    MAD_EXPECTS(event.TotalBufferLength > 0);

    std::size_t buffer_offset = 0;

    // FIXME: Optimize this.
    for (std::uint32_t buffer_idx = 0; buffer_idx < event.BufferCount;) {

        const auto & buffer = event.Buffers [buffer_idx];

        const auto pull_amount = std::min(
            sctx.rbuf().empty_space(),
            static_cast<std::size_t>(buffer.Length - buffer_offset));

        MAD_LOG_DEBUG_I(stream_logger(),
                        "Pulled {} byte(s) into the receive buffer (rb "
                        "allocation size: {})",
                        pull_amount, sctx.rbuf().total_size());

        // assert(pull_amount > 0);
        if (pull_amount == 0) {
            MAD_LOG_ERROR_I(
                stream_logger(), "No empty space left in the receive buffer!");
            // FIXME: What to do here? close stream? close connection?
            // skip the message?
            // Probably corrupt message, disconnect and break
            break;
        }
        assert(sctx.rbuf().put(buffer.Buffer + buffer_offset, pull_amount));
        buffer_offset += pull_amount;

        std::uint32_t push_payload_cnt = 0;

        // Deliver all complete messages to the app layer
        for (auto available_span = sctx.rbuf().available_span();
             available_span.size_bytes() >= sizeof(std::uint32_t);
             available_span = sctx.rbuf().available_span()) {
            // Read the size of the message
            auto size = *reinterpret_cast<const std::uint32_t *>(
                available_span.data());
            // Size is little-endian.
            if constexpr (std::endian::native == std::endian::big) {
                size = std::byteswap(size);
            }

            MAD_LOG_DEBUG_I(stream_logger(), "Message size {}", size);
            if ((available_span.size_bytes() - sizeof(std::uint32_t)) >= size) {
                auto message = available_span.subspan(
                    sizeof(std::uint32_t), size);

                // Only deliver complete messages to the application layer.
                [[maybe_unused]] auto consumed_bytes = sctx.on_data_received(
                    message);
                push_payload_cnt++;
                MAD_LOG_DEBUG_I(
                    stream_logger(), "Push payload count {}", push_payload_cnt);

                sctx.rbuf().mark_as_read(sizeof(std::uint32_t) +
                                         message.size_bytes());
                continue;
            }
            MAD_LOG_DEBUG_I(
                stream_logger(),
                "Partial data received({}), need {} more byte(s)",
                available_span.size_bytes() - sizeof(std::uint32_t),
                (size + sizeof(std::uint32_t)) - available_span.size_bytes());
            break;
        }

        // Not sure about this
        if (buffer_offset == buffer.Length) {
            buffer_idx++;
            buffer_offset = 0;
            MAD_LOG_DEBUG_I(
                stream_logger(), "proceed to next buffer(idx: {})", buffer_idx);
        }
    }

    MAD_LOG_DEBUG_I(stream_logger(),
                    "Processed {} buffers in grand total of {} byte(s). "
                    "Receive buffer has {} byte(s) inside.",
                    event.BufferCount, event.TotalBufferLength,
                    sctx.rbuf().consumed_space());
    return callback_status::success;
}
} // namespace mad::nexus
