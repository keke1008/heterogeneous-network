#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/wifi/control/start_udp_server.h>

TEST_CASE("serialize") {
    mock::MockReadableWritableStream stream;
    auto [future, promise] = nb::make_future_promise_pair<bool>();
    net::link::wifi::WifiPort remote_port{1234};

    net::link::wifi::StartUdpServer command{etl::move(promise), remote_port};
    command.execute(stream);
    CHECK(
        util::as_str(stream.write_buffer_.written_bytes()) ==
        "AT+CIPSTART=\"UDP\",\"0.0.0.0\",1234,12345,2\r\n"
    );
}
