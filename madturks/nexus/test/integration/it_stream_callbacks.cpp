// StreamCallbackReceive function integration tests

#include <mad/macros.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_callback_function.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/random_string.hpp>

#include <catch2/catch_test_macros.hpp>
#include <flatbuffers/flatbuffer_builder.h>
#include <msquic.hpp>

#include <fbs-schemas/chat_generated.h>
#include <fbs-schemas/main_generated.h>
#include <fbs-schemas/monster_generated.h>

auto encode_msg() {
    return mad::nexus::quic_base::build_message(
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
}

auto encode_msg_large(std::size_t how_large) {
    return mad::nexus::quic_base::build_message(
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
}

namespace mad::nexus {

using receive_event = decltype(QUIC_STREAM_EVENT::RECEIVE);
extern QUIC_STATUS StreamCallbackReceive(stream_context & sctx,
                                         receive_event & event);

TEST_CASE("Ensure it can handle a single message",
          "[stream-callback-receive]") {

    auto lambda = +[](void * uptr,
                      std::span<const std::uint8_t> buf) -> std::size_t {
        auto & called = *static_cast<bool *>(uptr);
        assert(!called);
        called = true;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        REQUIRE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        REQUIRE(envelope->message_type() == mad::schemas::Message::Monster);
        auto monster = envelope->message_as_Monster();
        REQUIRE(monster->hp() == 120);
        REQUIRE(monster->mana() == 80);
        REQUIRE(monster->name()->string_view() == "Deruvish");
        mad::schemas::Vec3 coords{ 10, 20, 30 };
        REQUIRE(monster->pos()->x() == 10.0f);
        REQUIRE(monster->pos()->y() == 20.0f);
        REQUIRE(monster->pos()->z() == 30.0f);
        return 0;
    };

    bool called = false;
    connection_context cctx{ nullptr, {} };
    stream_context sctx{
        nullptr, cctx, quic_callback_function{ lambda, &called }
    };

    auto msg{ encode_msg() };

    auto data_span{ msg.data_span() };

    const QUIC_BUFFER buf{ static_cast<std::uint32_t>(data_span.size_bytes()),
                           data_span.data() };

    receive_event evt{ .AbsoluteOffset = 0,
                       .TotalBufferLength = data_span.size_bytes(),
                       .Buffers = &buf,
                       .BufferCount = 1,
                       .Flags = QUIC_RECEIVE_FLAG_NONE };

    StreamCallbackReceive(sctx, evt);
    REQUIRE(called);
}

TEST_CASE("Ensure it can handle multiple messages in a single buffer",
          "[stream-callback-receive]") {

    constexpr auto kHowManyMessages = 10;

    auto lambda = +[](void * uptr,
                      std::span<const std::uint8_t> buf) -> std::size_t {
        auto & called_times = *static_cast<std::uint32_t *>(uptr);
        called_times++;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        REQUIRE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        REQUIRE(envelope->message_type() == mad::schemas::Message::Monster);
        auto monster = envelope->message_as_Monster();
        REQUIRE(monster->hp() == 120);
        REQUIRE(monster->mana() == 80);
        REQUIRE(monster->name()->string_view() == "Deruvish");
        mad::schemas::Vec3 coords{ 10, 20, 30 };
        REQUIRE(monster->pos()->x() == 10.0f);
        REQUIRE(monster->pos()->y() == 20.0f);
        REQUIRE(monster->pos()->z() == 30.0f);
        return 0;
    };

    std::uint32_t called_times = 0;
    connection_context cctx{ nullptr, {} };
    stream_context sctx{
        nullptr, cctx, quic_callback_function{ lambda, &called_times }
    };

    std::vector<std::uint8_t> storage{};
    for (int i = 0; i < kHowManyMessages; i++) {
        auto msg{ encode_msg() };
        auto data_span{ msg.data_span() };
        // GCC does not support this
        // storage.append_range(data_span);
        storage.insert(storage.end(), data_span.begin(), data_span.end());
    }

    const QUIC_BUFFER buf{ static_cast<std::uint32_t>(storage.size()),
                           storage.data() };

    receive_event evt{ .AbsoluteOffset = 0,
                       .TotalBufferLength = storage.size(),
                       .Buffers = &buf,
                       .BufferCount = 1,
                       .Flags = QUIC_RECEIVE_FLAG_NONE };

    StreamCallbackReceive(sctx, evt);
    REQUIRE(called_times == kHowManyMessages);
}

TEST_CASE("Ensure it can handle a single message split into three buffers",
          "[stream-callback-receive]") {

    constexpr auto kHowManyMessages = 1;

    auto lambda = +[](void * uptr,
                      std::span<const std::uint8_t> buf) -> std::size_t {
        auto & called_times = *static_cast<std::uint32_t *>(uptr);
        called_times++;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        REQUIRE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        REQUIRE(envelope->message_type() == mad::schemas::Message::Monster);
        auto monster = envelope->message_as_Monster();
        REQUIRE(monster->hp() == 120);
        REQUIRE(monster->mana() == 80);
        REQUIRE(monster->name()->string_view() == "Deruvish");
        mad::schemas::Vec3 coords{ 10, 20, 30 };
        REQUIRE(monster->pos()->x() == 10.0f);
        REQUIRE(monster->pos()->y() == 20.0f);
        REQUIRE(monster->pos()->z() == 30.0f);
        return 0;
    };

    std::uint32_t called_times = 0;
    connection_context cctx{ nullptr, {} };
    stream_context sctx{
        nullptr, cctx, quic_callback_function{ lambda, &called_times }
    };

    std::vector<std::uint8_t> storage{};
    for (int i = 0; i < kHowManyMessages; i++) {
        auto msg{ encode_msg() };
        auto data_span{ msg.data_span() };

        storage.insert(storage.end(), data_span.begin(), data_span.end());
    }

    const QUIC_BUFFER buffers [3] = {
        { static_cast<std::uint32_t>(10),                  storage.data()      },
        { static_cast<std::uint32_t>(20),                  storage.data() + 10 },
        { static_cast<std::uint32_t>(storage.size() - 30),
         storage.data() + 30                                                   },
    };

    receive_event evt{ .AbsoluteOffset = 0,
                       .TotalBufferLength = storage.size(),
                       .Buffers = buffers,
                       .BufferCount = sizeof(buffers) / sizeof(QUIC_BUFFER),
                       .Flags = QUIC_RECEIVE_FLAG_NONE };

    StreamCallbackReceive(sctx, evt);
    REQUIRE(called_times == kHowManyMessages);
}

TEST_CASE("Ensure it can handle a single message split into a buffer per byte "
          "(torture test)",
          "[stream-callback-receive]") {

    constexpr auto kHowManyMessages = 1;

    auto lambda = +[](void * uptr,
                      std::span<const std::uint8_t> buf) -> std::size_t {
        auto & called_times = *static_cast<std::uint32_t *>(uptr);
        called_times++;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        REQUIRE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        REQUIRE(envelope->message_type() == mad::schemas::Message::Monster);
        auto monster = envelope->message_as_Monster();
        REQUIRE(monster->hp() == 120);
        REQUIRE(monster->mana() == 80);
        REQUIRE(monster->name()->string_view() == "Deruvish");
        mad::schemas::Vec3 coords{ 10, 20, 30 };
        REQUIRE(monster->pos()->x() == 10.0f);
        REQUIRE(monster->pos()->y() == 20.0f);
        REQUIRE(monster->pos()->z() == 30.0f);
        return 0;
    };

    std::uint32_t called_times = 0;
    connection_context cctx{ nullptr, {} };
    stream_context sctx{
        nullptr, cctx, quic_callback_function{ lambda, &called_times }
    };

    std::vector<std::uint8_t> storage{};
    for (int i = 0; i < kHowManyMessages; i++) {
        auto msg{ encode_msg() };
        auto data_span{ msg.data_span() };

        storage.insert(storage.end(), data_span.begin(), data_span.end());
    }

    QUIC_BUFFER * buffers = new QUIC_BUFFER [storage.size()];
    for (auto i = 0zu; i < storage.size(); i++) {
        buffers [i].Buffer = storage.data() + i;
        buffers [i].Length = 1;
    }

    receive_event evt{ .AbsoluteOffset = 0,
                       .TotalBufferLength = storage.size(),
                       .Buffers = buffers,
                       .BufferCount = static_cast<std::uint32_t>(
                           storage.size()),
                       .Flags = QUIC_RECEIVE_FLAG_NONE };

    StreamCallbackReceive(sctx, evt);
    delete [] buffers;
    REQUIRE(called_times == kHowManyMessages);
}

TEST_CASE("Ensure it can handle multiple messages in a single buffer but "
          "receive buffer has space for only 1 message",
          "[stream-callback-receive]") {

    constexpr auto kHowManyMessages = 10;

    auto lambda = +[](void * uptr,
                      std::span<const std::uint8_t> buf) -> std::size_t {
        auto & called_times = *static_cast<std::uint32_t *>(uptr);
        called_times++;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        REQUIRE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        REQUIRE(envelope->message_type() == mad::schemas::Message::Chat);
        auto chat = envelope->message_as_Chat();
        REQUIRE(chat->timestamp() == 123456789);
        REQUIRE(chat->message()->size() == 4000);
        return 0;
    };

    // The final payload will be over >40kbytes
    // but the receive buffer only gonna have 4096 bytes
    // of space, which means it can't pull all of them at
    // one go, and have to do partial pulls.
    std::vector<std::uint8_t> storage{};
    for (int i = 0; i < kHowManyMessages; i++) {
        auto msg{ encode_msg_large(4000) };
        auto data_span{ msg.data_span() };
        storage.insert(storage.end(), data_span.begin(), data_span.end());
    }

    std::uint32_t called_times = 0;
    connection_context cctx{ nullptr, {} };
    stream_context sctx{
        nullptr, cctx, quic_callback_function{ lambda, &called_times },
          4096
    };

    const QUIC_BUFFER buf{ static_cast<std::uint32_t>(storage.size()),
                           storage.data() };

    receive_event evt{ .AbsoluteOffset = 0,
                       .TotalBufferLength = storage.size(),
                       .Buffers = &buf,
                       .BufferCount = 1,
                       .Flags = QUIC_RECEIVE_FLAG_NONE };

    StreamCallbackReceive(sctx, evt);
    REQUIRE(called_times == kHowManyMessages);
}

TEST_CASE("Single message larger than the receive buffer",
          "[stream-callback-receive]") {

    constexpr auto kHowManyMessages = 1;

    auto lambda = +[](void * uptr,
                      std::span<const std::uint8_t> buf) -> std::size_t {
        auto & called_times = *static_cast<std::uint32_t *>(uptr);
        called_times++;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        REQUIRE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        REQUIRE(envelope->message_type() == mad::schemas::Message::Chat);
        auto chat = envelope->message_as_Chat();
        REQUIRE(chat->timestamp() == 123456789);
        REQUIRE(chat->message()->size() == 4000);
        return 0;
    };

    // The receive buffer simply don't have enough space to
    // pull a single message.
    std::vector<std::uint8_t> storage{};
    for (int i = 0; i < kHowManyMessages; i++) {
        auto msg{ encode_msg_large(5000) };
        auto data_span{ msg.data_span() };
        storage.insert(storage.end(), data_span.begin(), data_span.end());
    }

    std::uint32_t called_times = 0;
    connection_context cctx{ nullptr, {} };
    stream_context sctx{
        nullptr, cctx, quic_callback_function{ lambda, &called_times },
          4096
    };

    const QUIC_BUFFER buf{ static_cast<std::uint32_t>(storage.size()),
                           storage.data() };

    receive_event evt{ .AbsoluteOffset = 0,
                       .TotalBufferLength = storage.size(),
                       .Buffers = &buf,
                       .BufferCount = 1,
                       .Flags = QUIC_RECEIVE_FLAG_NONE };

    StreamCallbackReceive(sctx, evt);
    REQUIRE(called_times == 0);
}

TEST_CASE("Ensure it can handle a single message split into a buffer per byte "
          "and each one is arriving in an individual receive call"
          "(torture test)",
          "[stream-callback-receive]") {

    constexpr auto kHowManyMessages = 1;

    auto lambda = +[](void * uptr,
                      std::span<const std::uint8_t> buf) -> std::size_t {
        auto & called_times = *static_cast<std::uint32_t *>(uptr);
        called_times++;
        ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
        REQUIRE(mad::schemas::VerifyEnvelopeBuffer(vrf));
        auto envelope = mad::schemas::GetEnvelope(buf.data());
        REQUIRE(envelope->message_type() == mad::schemas::Message::Monster);
        auto monster = envelope->message_as_Monster();
        REQUIRE(monster->hp() == 120);
        REQUIRE(monster->mana() == 80);
        REQUIRE(monster->name()->string_view() == "Deruvish");
        mad::schemas::Vec3 coords{ 10, 20, 30 };
        REQUIRE(monster->pos()->x() == 10.0f);
        REQUIRE(monster->pos()->y() == 20.0f);
        REQUIRE(monster->pos()->z() == 30.0f);
        return 0;
    };

    std::uint32_t called_times = 0;
    connection_context cctx{ nullptr, {} };
    stream_context sctx{
        nullptr, cctx, quic_callback_function{ lambda, &called_times }
    };

    std::vector<std::uint8_t> storage{};
    for (int i = 0; i < kHowManyMessages; i++) {
        auto msg{ encode_msg() };
        auto data_span{ msg.data_span() };

        storage.insert(storage.end(), data_span.begin(), data_span.end());
    }

    QUIC_BUFFER * buffers = new QUIC_BUFFER [storage.size()];
    for (auto i = 0zu; i < storage.size(); i++) {
        buffers [i].Buffer = storage.data() + i;
        buffers [i].Length = 1;
    }

    for (auto i = 0zu; i < storage.size(); i++) {
        receive_event evt{ .AbsoluteOffset = 0,
                           .TotalBufferLength = 1,
                           .Buffers = &buffers [i],
                           .BufferCount = 1,
                           .Flags = QUIC_RECEIVE_FLAG_NONE };

        // The message will arrive in N different receive calls.
        StreamCallbackReceive(sctx, evt);
    }

    delete [] buffers;
    REQUIRE(called_times == kHowManyMessages);
}
} // namespace mad::nexus
