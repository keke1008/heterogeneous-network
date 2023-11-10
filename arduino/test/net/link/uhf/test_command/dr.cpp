#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <nb/future.h>
#include <net/frame/service.h>
#include <net/link/uhf/command/dr.h>

using namespace net::link;

TEST_CASE("DR") {
    mock::MockReadableWritableStream stream{};
    memory::Static<net::frame::MultiSizeFrameBufferPool<1, 1>> buffer_pool;
    net::frame::FrameService frame_service{buffer_pool};
    uhf::ModemId remote{0xAB};
    auto protocol_number = net::frame::ProtocolNumber{001};

    SUBCASE("receive 'abc'") {
        net::link::uhf::DRExecutor executor{};

        stream.read_buffer_.write_str("*DR=04\001abc\\RAB\r\n");
        auto poll_frame = executor.poll(frame_service, stream);
        CHECK(poll_frame.is_ready());
        auto frame = etl::move(poll_frame.unwrap());

        CHECK(frame.protocol_number == protocol_number);
        CHECK(frame.remote == remote);
        CHECK(util::as_str(frame.reader.written_buffer()) == "abc");
    }
}
