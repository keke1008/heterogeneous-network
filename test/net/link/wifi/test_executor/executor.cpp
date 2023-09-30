#include <doctest.h>
#include <util/doctest_ext.h>

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
        auto result = executor.join_ap(util::as_bytes("ssid"), util::as_bytes("password"));
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
        uint8_t protocol = 056;
        frame_service.request_transmission(protocol, remote_address, length);

        executor.execute(frame_service);

        CHECK(frame_service.poll_transmission_request([](auto &) { return true; }).is_pending());
    }

    SUBCASE("receive data") {
        uint8_t protocol = 056;
        stream.read_buffer_.write_str("+IPD,2,192.168.0.1,19073:\056A");
        executor.execute(frame_service);

        auto poll_reception_notification = frame_service.poll_reception_notification();
        CHECK(poll_reception_notification.is_ready());

        auto reception_notification = etl::move(poll_reception_notification.unwrap());
        auto poll_source = reception_notification.source.poll();
        CHECK(poll_source.is_ready());
        CHECK(poll_source.unwrap().get() == Address{IPv4Address{192, 168, 0, 1}});

        CHECK(reception_notification.protocol == protocol);
        CHECK(reception_notification.reader.frame_length() == 1);
        CHECK(reception_notification.reader.read() == 'A');
    }

    SUBCASE("unknown message") {
        stream.read_buffer_.write_str("+UNKNOWN_MSG");
        executor.execute(frame_service);
        CHECK(frame_service.poll_reception_notification().is_pending());
    }
}
