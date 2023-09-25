#include <doctest.h>

#include <mock/stream.h>
#include <net/link/wifi/control/send_data.h>
#include <util/u8_literal.h>

using namespace net::link::wifi;
using namespace util::u8_literal;

TEST_CASE("SendData") {
    mock::MockReadableWritableStream stream;

    auto [body_future, body_promise] = nb::make_future_promise_pair<net::link::DataWriter>();
    auto [result_future, result_promise] = nb::make_future_promise_pair<bool>();
    constexpr uint8_t length{10};
    net::link::IPv4Address remote_address{192, 168, 0, 1};
    uint16_t remote_port{1234};
    etl::span<uint8_t, length> body{"0123456789"_u8array};

    SendData send_data{
        etl::move(body_promise), etl::move(result_promise), length, remote_address, remote_port,
    };

    CHECK(send_data.execute(stream).is_pending());

    CHECK(stream.consume_write_buffer_and_equals_to(R"(AT+CIPSEND=10,"192.168.0.1",1234)"
                                                    "\r\n"));
    CHECK(body_future.poll().is_pending());
    CHECK(result_future.poll().is_pending());

    stream.read_buffer_.write("\r\nOK\r\n>"_u8array);
    CHECK(send_data.execute(stream).is_pending());
    CHECK(body_future.poll().is_ready());
    CHECK(result_future.poll().is_pending());

    auto writer = body_future.poll().unwrap();
    CHECK_EQ(writer.get().writable_count(), length);
    writer.get().write(body);
    CHECK_EQ(writer.get().writable_count(), 0);
    writer.get().close();

    SUBCASE("SEND OK") {
        stream.read_buffer_.write("\r\nRecv 1 bytes\r\n\r\nSEND OK\r\n"_u8array);
        CHECK(send_data.execute(stream).is_ready());
        CHECK(result_future.poll().is_ready());
        CHECK(result_future.poll().unwrap().get());
    }

    SUBCASE("SEND FAIL") {
        stream.read_buffer_.write("\r\nRecv 1 bytes\r\n\r\nSEND FAIL\r\n"_u8array);
        CHECK(send_data.execute(stream).is_ready());
        CHECK(result_future.poll().is_ready());
        CHECK_FALSE(result_future.poll().unwrap().get());
    }
}
