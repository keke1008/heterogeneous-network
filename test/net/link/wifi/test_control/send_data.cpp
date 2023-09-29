#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/wifi/control/send_data.h>
#include <test/net/frame.h>

using namespace net::link::wifi;

TEST_CASE("SendData") {
    mock::MockReadableWritableStream stream;
    constexpr uint8_t length{10};
    net::link::Address dest{IPv4Address{192, 168, 0, 1}};
    uint16_t remote_port{1234};

    auto [counter, buffer] = test::make_frame_buffer<length>();
    auto [transmission, request] = test::make_frame_transmission_request(dest, counter, buffer);

    etl::string_view data{"0123456789"};
    transmission.writer.write_str(data);
    SendData send_data{etl::move(request), remote_port};

    CHECK(send_data.execute(stream).is_pending());
    CHECK(
        util::as_str(stream.write_buffer_.written_bytes()) ==
        "AT+CIPSEND=10,\"192.168.0.1\",1234\r\n"
    );
    CHECK(transmission.success.poll().is_pending());
    stream.write_buffer_.reset();

    stream.read_buffer_.write_str("\r\nOK\r\n>");
    CHECK(send_data.execute(stream).is_pending());

    SUBCASE("SEND OK") {
        stream.read_buffer_.write_str("\r\nRecv 1 bytes\r\n\r\nSEND OK\r\n");
        CHECK(send_data.execute(stream).is_ready());
        CHECK(transmission.success.poll().is_ready());
        CHECK(transmission.success.poll().unwrap().get());
    }

    SUBCASE("SEND FAIL") {
        stream.read_buffer_.write_str("\r\nRecv 1 bytes\r\n\r\nSEND FAIL\r\n");
        CHECK(send_data.execute(stream).is_ready());
        CHECK(transmission.success.poll().is_ready());
        CHECK_FALSE(transmission.success.poll().unwrap().get());
    }
}
