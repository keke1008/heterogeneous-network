#include <doctest.h>

#include <mock/stream.h>
#include <net/link/wifi/control/start_udp_server.h>

TEST_CASE("serialize") {
    mock::MockReadableWritableStream stream;
    auto [future, promise] = nb::make_future_promise_pair<bool>();
    uint16_t remote_port = 1234;

    net::link::wifi::StartUdpServer command{etl::move(promise), remote_port};

    command.execute(stream);

    auto expected = R"(AT+CIPSTART="UDP","0.0.0.0",1234,1234)"
                    "\r\n";
    CHECK(stream.consume_write_buffer_and_equals_to(expected));
}
