#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/wifi/control/send_data.h>

using namespace net::link::wifi;

TEST_CASE("SendData") {
    mock::MockReadableWritableStream stream;
    constexpr uint8_t length{5};
    net::link::Address peer{IPv4Address{192, 168, 0, 1}};
    uint16_t remote_port{1234};
    auto protocol = net::frame::ProtocolNumber{001};
    net::frame::FrameService<net::link::Address, 1, 1> frame_service;

    auto writer = etl::move(frame_service.request_frame_writer(length).unwrap());
    writer.write_str("abcde");
    net::link::Frame frame{
        .protocol_number = protocol,
        .peer = peer,
        .length = length,
        .reader = writer.make_initial_reader(),
    };
    SendData send_data{etl::move(frame), remote_port};

    CHECK(send_data.execute(stream).is_pending());
    CHECK(
        util::as_str(stream.write_buffer_.written_bytes()) ==
        "AT+CIPSEND=6,\"192.168.0.1\",1234\r\n"
    );
    stream.write_buffer_.reset();

    stream.read_buffer_.write_str("\r\nOK\r\n>");
    CHECK(send_data.execute(stream).is_pending());

    SUBCASE("SEND OK") {
        stream.read_buffer_.write_str("\r\nRecv 1 bytes\r\n\r\nSEND OK\r\n");
        CHECK(send_data.execute(stream).is_ready());
        CHECK(util::as_str(stream.write_buffer_.written_bytes()) == "\001abcde");
        CHECK(stream.read_buffer_.readable_count() == 0);
    }

    SUBCASE("SEND FAIL") {
        stream.read_buffer_.write_str("\r\nRecv 1 bytes\r\n\r\nSEND FAIL\r\n");
        CHECK(send_data.execute(stream).is_ready());
        CHECK(stream.read_buffer_.readable_count() == 0);
    }
}
