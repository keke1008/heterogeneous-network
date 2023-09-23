#include <doctest.h>

#include <mock/stream.h>
#include <net/link/address.h>
#include <net/link/wifi/control/start_udp.h>

TEST_CASE("serialize") {
    mock::MockReadableWritableStream stream;
    auto [future, promise] = nb::make_future_promise_pair<bool>();
    net::link::IPv4Address remote_ip{192, 168, 0, 1};
    uint16_t remote_port = 1234;
    uint16_t local_port = 5678;

    net::link::wifi::StartUdpConnection command{
        etl::move(promise), net::link::wifi::LinkId::Link0, remote_ip, remote_port, local_port};

    command.execute(stream);

    auto expected = R"(AT+CIPSTART=0,"UDP","192.168.0.1",1234,5678)"
                    "\r\n";
    auto expected_span = etl::span<const uint8_t>{
        reinterpret_cast<const uint8_t *>(expected),
        etl::char_traits<char>::length(expected),
    };
    CHECK(stream.consume_write_buffer_and_equals_to(expected_span));
}
