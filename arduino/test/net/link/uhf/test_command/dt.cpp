#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <nb/future.h>
#include <net/frame/service.h>
#include <net/link/uhf/command/dt.h>
#include <util/time.h>

TEST_CASE("DT") {
    mock::MockReadableWritableStream stream{};
    util::MockTime time{0};
    net::link::uhf::ModemId peer{0x12};
    constexpr uint8_t length = 3;
    auto protocol = net::frame::ProtocolNumber{001};

    memory::Static<net::frame::MultiSizeFrameBufferPool<1, 1>> buffer_pool;
    net::frame::FrameService frame_service{buffer_pool};

    auto writer = etl::move(frame_service.request_frame_writer(length).unwrap());
    writer.write_str("abc");
    net::link::uhf::UhfFrame frame{
        .protocol_number = protocol,
        .remote = peer,
        .reader = writer.make_initial_reader(),
    };
    net::link::uhf::DTExecutor executor{etl::move(frame)};

    stream.read_buffer_.write_str("*DT=04\r\n");
    CHECK(executor.poll(stream, time).is_pending());

    SUBCASE("success") {
        while (executor.poll(stream, time).is_pending()) {
            time.advance(util::Duration::from_millis(10));
        }
        CHECK(util::as_str(stream.write_buffer_.written_bytes()) == "@DT04\001abc/R12\r\n");
    }

    SUBCASE("receive information response") {
        stream.read_buffer_.write_str("*IR=01\r\n");
        CHECK(executor.poll(stream, time).is_pending());

        while (executor.poll(stream, time).is_pending()) {
            time.advance(util::Duration::from_millis(10));
        }

        CHECK(stream.read_buffer_.readable_count() == 0);
    }
}
