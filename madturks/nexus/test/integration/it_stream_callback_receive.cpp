// StreamCallbackReceive function integration tests

#include <mad/macros.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_callback_function.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/random_string.hpp>

#include <flatbuffers/flatbuffer_builder.h>
#include <gtest/gtest.h>
#include <msquic.hpp>

#include <fbs-schemas/chat_generated.h>
#include <fbs-schemas/main_generated.h>
#include <fbs-schemas/monster_generated.h>

auto encode_monster_msg() {
    static auto validator =
        +[](void * uptr, std::span<const std::uint8_t> buf) -> std::size_t {
        auto & times = *static_cast<std::uint32_t *>(uptr);
        times++;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        EXPECT_TRUE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        EXPECT_EQ(envelope->message_type(), mad::schemas::Message::Monster);
        auto monster = envelope->message_as_Monster();
        EXPECT_EQ(monster->hp(), 120);
        EXPECT_EQ(monster->mana(), 80);
        EXPECT_EQ(monster->name()->string_view(), "Deruvish");
        mad::schemas::Vec3 coords{ 10, 20, 30 };
        EXPECT_EQ(monster->pos()->x(), 10.0f);
        EXPECT_EQ(monster->pos()->y(), 20.0f);
        EXPECT_EQ(monster->pos()->z(), 30.0f);
        return 0;
    };

    auto message = mad::nexus::quic_base::build_message(
        [](::flatbuffers::FlatBufferBuilder & fbb) {
            mad::schemas::Vec3 coords{ 10, 20, 30 };
            auto name = fbb.CreateString("Deruvish");
            mad::schemas::MonsterBuilder mb{ fbb };
            mb.add_hp(120);
            mb.add_mana(80);
            mb.add_name(name);
            mb.add_pos(&coords);
            auto f = mb.Finish();
            mad::schemas::EnvelopeBuilder env{ fbb };
            env.add_message(f.Union());
            env.add_message_type(mad::schemas::Message::Monster);
            return env.Finish();
        });

    return std::make_pair(std::move(message), validator);
}

auto encode_chat_message(std::size_t how_large) {
    static auto validator =
        +[](void * uptr, std::span<const std::uint8_t> buf) -> std::size_t {
        auto & times = *static_cast<std::uint32_t *>(uptr);
        times++;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        EXPECT_TRUE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        EXPECT_TRUE(envelope->message_type() == mad::schemas::Message::Chat);
        auto chat = envelope->message_as_Chat();
        EXPECT_TRUE(chat->timestamp() == 123456789);
        EXPECT_TRUE(chat->message()->size() == 4000);
        return 0;
    };

    auto message = mad::nexus::quic_base::build_message(
        [&](::flatbuffers::FlatBufferBuilder & fbb) {
            //
            auto random_string = mad::random::generate<std::string>(
                how_large, how_large);
            auto content = fbb.CreateString(random_string);
            mad::schemas::ChatBuilder chabby{ fbb };
            chabby.add_message(content);
            chabby.add_timestamp(123456789);
            auto f = chabby.Finish();
            mad::schemas::EnvelopeBuilder env{ fbb };
            env.add_message(f.Union());
            env.add_message_type(mad::schemas::Message::Chat);
            return env.Finish();
        });

    return std::make_pair(std::move(message), validator);
}

namespace mad::nexus {

using receive_event = decltype(QUIC_STREAM_EVENT::RECEIVE);
extern QUIC_STATUS StreamCallbackReceive(stream_context & sctx,
                                         receive_event & event);

struct StreamCallback : public ::testing::Test {

    struct message_object {
        std::vector<QUIC_BUFFER> buffers{};
        std::vector<std::uint8_t> storage{};

        std::size_t per_message_size = 0;
        std::size_t (*validator)(void *,
                                 std::span<const std::uint8_t>) = nullptr;
        std::size_t called_times;
        stream_context sctx{
            nullptr, message_object::cctx,
            quic_callback_function{ validator, &called_times }
        };

        std::size_t total_length() const {
            return std::transform_reduce(buffers.begin(), buffers.end(), 0u,
                                         std::plus<>(), [](const auto & v) {
                                             return v.Length;
                                         });
        }

        receive_event get_receive_event() {
            receive_event evt{ .AbsoluteOffset = 0,
                               .TotalBufferLength = total_length(),
                               .Buffers = buffers.data(),
                               .BufferCount = static_cast<std::uint32_t>(
                                   buffers.size()),
                               .Flags = QUIC_RECEIVE_FLAG_NONE };
            return evt;
        }

    private:
        static inline connection_context cctx{ nullptr };
    };

    auto generate_message_object(
        std::size_t how_many_messages,
        std::size_t segmentation_size = std::size_t{ ~0u },
        std::pair<send_buffer<true>,
                  std::size_t (*)(void *, std::span<const std::uint8_t>)>
            msg_obj = encode_monster_msg()) {
        message_object result{};
        auto & [msg, validator] = msg_obj;
        auto data_span{ msg.data_span() };
        for (std::size_t i = 0; i < how_many_messages; i++) {

            result.storage.insert(
                result.storage.end(), data_span.begin(), data_span.end());
        }
        result.per_message_size = msg.data_span().size_bytes();
        result.validator = validator;
        result.sctx.on_data_received =
            quic_callback_function{ result.validator, &result.called_times };

        auto remanining_bytes = result.storage.size();
        auto itr = result.storage.begin();

        while (remanining_bytes > 0) {
            const auto append_amount = std::min(
                segmentation_size, remanining_bytes);
            result.buffers.emplace_back(append_amount, &*itr);
            std::advance(itr, append_amount);
            remanining_bytes -= append_amount;
        }
        return result;
    }
};

TEST_F(StreamCallback, SingleMessageSingleBuffer) {
    constexpr auto kHowManyMessages = 1u;
    auto obj = generate_message_object(kHowManyMessages);
    auto evt = obj.get_receive_event();
    StreamCallbackReceive(obj.sctx, evt);
    EXPECT_EQ(obj.called_times, kHowManyMessages);
}

TEST_F(StreamCallback, MultipleMessagesSingleBuffer) {
    constexpr auto kHowManyMessages = 10u;
    auto obj = generate_message_object(kHowManyMessages);
    auto evt = obj.get_receive_event();
    StreamCallbackReceive(obj.sctx, evt);
    EXPECT_EQ(obj.called_times, kHowManyMessages);
}

TEST_F(StreamCallback, SingleMessageMultipleBuffers) {
    constexpr auto kHowManyMessages = 1u;
    constexpr auto kSegmentationSize = 10u;
    auto obj = generate_message_object(kHowManyMessages, kSegmentationSize);
    ASSERT_GE(obj.per_message_size, kSegmentationSize);
    auto evt = obj.get_receive_event();
    StreamCallbackReceive(obj.sctx, evt);
    EXPECT_EQ(obj.called_times, kHowManyMessages);
}

TEST_F(StreamCallback, SingleMessageMultipleBuffersTorture) {
    constexpr auto kHowManyMessages = 1u;
    constexpr auto kSegmentationSize = 1u;
    auto obj = generate_message_object(kHowManyMessages, kSegmentationSize);
    ASSERT_GE(obj.per_message_size, kSegmentationSize);
    auto evt = obj.get_receive_event();
    StreamCallbackReceive(obj.sctx, evt);
    EXPECT_EQ(obj.called_times, kHowManyMessages);
}

TEST_F(StreamCallback, MultipleMessagesRecvBufferHasSpaceForOne) {
    // Multiple messages, but receive buffer only has space for one message.
    constexpr auto kHowManyMessages = 10u;
    constexpr auto kChatRandomTextLength = 4000u;
    auto obj = generate_message_object(
        kHowManyMessages, std::size_t{ ~0u },
        encode_chat_message(kChatRandomTextLength));
    auto evt = obj.get_receive_event();
    StreamCallbackReceive(obj.sctx, evt);
    EXPECT_EQ(obj.called_times, kHowManyMessages);
}

TEST_F(StreamCallback, SingleMessageLargerThanReceiveBuffer) {
    constexpr auto kHowManyMessages = 1u;
    constexpr auto kChatRandomTextLength = 5000u;
    auto obj = generate_message_object(
        kHowManyMessages, std::size_t{ ~0u },
        encode_chat_message(kChatRandomTextLength));
    auto evt = obj.get_receive_event();
    stream_context custom_sctx{ nullptr, obj.sctx.connection(),
                                obj.sctx.on_data_received,
                                obj.per_message_size / 2 };
    StreamCallbackReceive(custom_sctx, evt);
    EXPECT_EQ(obj.called_times, 0);
}

TEST_F(StreamCallback, SingleMessageBufferPerByteArrivingIndividually) {
    constexpr auto kHowManyMessages = 10u;
    auto obj = generate_message_object(kHowManyMessages, std::size_t{ ~0u });

    for (const auto & buffer : obj.buffers) {

        for (auto i = 0u; i < buffer.Length; i++) {

            QUIC_BUFFER buf{ .Length = 1, .Buffer = &buffer.Buffer [i] };

            receive_event evt{ .AbsoluteOffset = 0,
                               .TotalBufferLength = 1,
                               .Buffers = &buf,
                               .BufferCount = 1,
                               .Flags = QUIC_RECEIVE_FLAG_NONE };
            StreamCallbackReceive(obj.sctx, evt);
        }
    }

    EXPECT_EQ(obj.called_times, kHowManyMessages);
}
} // namespace mad::nexus
