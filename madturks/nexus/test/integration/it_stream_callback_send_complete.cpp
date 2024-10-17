#include <mad/nexus/quic_stream_context.hpp>

#include <flatbuffers/detached_buffer.h>
#include <gmock/gmock.h>

#include <msquic.h>

class MockDeleter {
public:
    MOCK_METHOD(void, DeleteArray, (void *), ());
};

MockDeleter mockDeleter;

void operator delete [](void * ptr) noexcept {
    mockDeleter.DeleteArray(ptr);
    ::operator delete [](ptr); // Call the global delete[] to free memory.
}

namespace mad::nexus {
extern QUIC_STATUS
StreamCallbackSendComplete(stream_context & sctx,
                           decltype(QUIC_STREAM_EVENT::SEND_COMPLETE) & event);
}
