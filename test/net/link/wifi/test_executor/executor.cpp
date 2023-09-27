#include <doctest.h>

#include <mock/stream.h>
#include <net/link/wifi/executor.h>

using namespace net::link;
using namespace net::link::wifi;

TEST_CASE("Executor") {
    mock::MockReadableWritableStream stream;
    uint16_t port = 19073;
    WifiExecutor executor{stream, port};

    SUBCASE("initialize") {
        auto result = executor.initialize();
        CHECK(result.is_ready());
        CHECK(result.unwrap().poll().is_pending());
    }

    SUBCASE("join_ap") {
        etl::array ssid = "ssid"_u8array;
        etl::array password = "password"_u8array;
        auto result = executor.join_ap(ssid, password);
        CHECK(result.is_ready());
        CHECK(result.unwrap().poll().is_pending());
    }

    SUBCASE("start_udp_server") {
        auto result = executor.start_udp_server();
        CHECK(result.is_ready());
        CHECK(result.unwrap().poll().is_pending());
    }

    SUBCASE("send_data") {
        uint8_t length = 10;
        IPv4Address remote_address{192, 168, 0, 1};
        auto result = executor.send_data(Address{remote_address}, length);
        CHECK(result.is_ready());
        CHECK(result.unwrap().body.poll().is_pending());
        CHECK(result.unwrap().success.poll().is_pending());
    }

    SUBCASE("receive data") {
        CHECK(executor.execute().is_pending());
        stream.write_to_read_buffer("+IPD,1,192.168.0.1,19073:0"_u8array);
        CHECK(executor.execute().is_ready());
    }

    SUBCASE("unknown message") {
        CHECK(executor.execute().is_pending());
        stream.write_to_read_buffer("+UNKNOWN_MSG"_u8array);
        CHECK(executor.execute().is_pending());
    }
}
