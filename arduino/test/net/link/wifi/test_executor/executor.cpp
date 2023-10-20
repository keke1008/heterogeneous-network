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
        Address peer{IPv4Address{192, 168, 0, 1}};
        auto protocol = net::frame::ProtocolNumber{001};

        auto writer = etl::move(frame_service.request_frame_writer(length).unwrap());
        Frame frame{
            .protocol_number = protocol,
            .peer = peer,
            .length = length,
            .reader = etl::move(writer.make_initial_reader()),
        };

        CHECK(executor.send_frame(etl::move(frame)).is_ready());
    }

    SUBCASE("receive data") {
        auto protocol = net::frame::ProtocolNumber{001};
        Address peer{IPv4Address{192, 168, 0, 1}};
        stream.read_buffer_.write_str("+IPD,2,192.168.0.1,19073:\001A");
        executor.execute(frame_service);

        auto poll_frame = executor.receive_frame();
        CHECK(poll_frame.is_ready());

        auto frame = etl::move(poll_frame.unwrap());
        CHECK(frame.length == 1);
        CHECK(frame.protocol_number == protocol);
        CHECK(frame.peer == peer);
        CHECK(util::as_str(frame.reader.written_buffer()) == "A");
    }

    SUBCASE("unknown message") {
        stream.read_buffer_.write_str("+UNKNOWN_MSG");
        executor.execute(frame_service);
        CHECK(executor.receive_frame().is_pending());
    }
}
