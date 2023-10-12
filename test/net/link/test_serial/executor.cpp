#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/frame/service.h>
#include <net/link/serial.h>

using namespace net::link::serial;
using namespace net::link;

TEST_CASE("Executor") {
    mock::MockReadableWritableStream stream;
    Address self{SerialAddress{012}};
    Address peer{SerialAddress{034}};
    SerialExecutor executor{stream, net::link::SerialAddress{self}};
    net::frame::FrameService<Address, 1, 1> frame_service;

    auto protocol = net::frame::ProtocolNumber{001};
    etl::array<uint8_t, PREAMBLE_LENGTH> PREAMBLE_VALUE;
    PREAMBLE_VALUE.fill(PREAMBLE);

    SUBCASE("send_data") {
        uint8_t length = 05;
        auto writer = etl::move(frame_service.request_frame_writer(length).unwrap());
        writer.write_str("abcde");
        Frame frame{
            .protocol_number = protocol,
            .peer = peer,
            .length = length,
            .reader = etl::move(writer.make_initial_reader()),
        };

        executor.send_frame(etl::move(frame));
        executor.execute(frame_service);
        CHECK_EQ(
            util::as_str(stream.write_buffer_.written_bytes().subspan(0, PREAMBLE_LENGTH)),
            util::as_str(PREAMBLE_VALUE)
        );
        CHECK_EQ(
            util::as_str(stream.write_buffer_.written_bytes().subspan(PREAMBLE_LENGTH)),
            "\001\012\034\005abcde"
        );
    }

    SUBCASE("receive data") {
        constexpr uint8_t length = 5;
        stream.read_buffer_.write_str("dummy");
        executor.execute(frame_service);
        CHECK(executor.receive_frame().is_pending());

        stream.read_buffer_.write(PREAMBLE_VALUE);
        stream.read_buffer_.write_str("\001\034\012\005");
        executor.execute(frame_service);

        auto poll_frame = executor.receive_frame();
        CHECK(poll_frame.is_ready());

        auto frame = etl::move(poll_frame.unwrap());
        CHECK(frame.length == length);
        CHECK(frame.protocol_number == protocol);
        CHECK(frame.peer == peer);

        stream.read_buffer_.write_str("abcde");
        executor.execute(frame_service);
        CHECK(util::as_str(frame.reader.written_buffer()) == "abcde");
    }

    SUBCASE("discard data") {
        constexpr uint8_t length = 5;
        stream.read_buffer_.write_str("dummy");
        stream.read_buffer_.write(PREAMBLE_VALUE);
        stream.read_buffer_.write_str("\056\034\077\005abcde");
        executor.execute(frame_service);
        CHECK(executor.receive_frame().is_pending());

        stream.read_buffer_.write_str("dummy");
        stream.read_buffer_.write(PREAMBLE_VALUE);
        stream.read_buffer_.write_str("\056\034\012\005fghij");
        executor.execute(frame_service);
        auto poll_frame = executor.receive_frame();
        CHECK(poll_frame.is_ready());
        auto frame = etl::move(poll_frame.unwrap());
        CHECK(frame.length == length);
        CHECK(util::as_str(frame.reader.written_buffer()) == "fghij");
    }
}
