#include <doctest.h>

#include <mock/stream.h>
#include <net/frame/service.h>
#include <net/link/wifi/executor.h>

using namespace net::link;
using namespace net::link::wifi;

TEST_CASE("Executor") {
    mock::MockReadableWritableStream stream;
    uint16_t port = 19073;
    WifiExecutor executor{stream, port};
    net::frame::FrameService<Address, 1, 1> frame_service{};

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
        Address remote_address{IPv4Address{192, 168, 0, 1}};
        frame_service.request_transmission(remote_address, length);

        executor.execute(frame_service);

        CHECK(frame_service.poll_transmission_request([](auto &) { return true; }).is_pending());
    }

    SUBCASE("receive data") {
        stream.write_to_read_buffer("+IPD,1,192.168.0.1,19073:A"_u8array);
        executor.execute(frame_service);

        auto poll_reception_notification = frame_service.poll_reception_notification();
        CHECK(poll_reception_notification.is_ready());

        auto reception_notification = etl::move(poll_reception_notification.unwrap());
        auto poll_source = reception_notification.source.poll();
        CHECK(poll_source.is_ready());
        CHECK(poll_source.unwrap().get() == Address{IPv4Address{192, 168, 0, 1}});

        CHECK(reception_notification.reader.frame_length() == 1);
        CHECK(reception_notification.reader.read() == 'A');
    }

    SUBCASE("unknown message") {
        stream.write_to_read_buffer("+UNKNOWN_MSG"_u8array);
        executor.execute(frame_service);
        CHECK(frame_service.poll_reception_notification().is_pending());
    }
}
