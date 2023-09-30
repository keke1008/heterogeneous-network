#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <nb/future.h>
#include <net/frame/service.h>
#include <net/link/uhf/command/dt.h>
#include <test/net/frame.h>
#include <util/time.h>

TEST_CASE("DT") {
    mock::MockReadableWritableStream stream{};
    util::MockTime time{0};
    net::link::Address dest{net::link::uhf::ModemId{0x12}};
    constexpr uint8_t length = 3;
    uint8_t protocol = 034;

    auto [counter, buffer] = test::make_frame_buffer<length>();
    auto [transmission, request] =
        test::make_frame_transmission_request(protocol, dest, counter, buffer);
    transmission.writer.write_str("abc");
    net::link::uhf::DTExecutor executor{etl::move(request)};

    stream.read_buffer_.write_str("*DT=04\r\n");
    CHECK(executor.poll(stream, time).is_pending());
    CHECK(transmission.success.poll().is_pending());

    SUBCASE("success") {
        while (executor.poll(stream, time).is_pending()) {
            time.advance(util::Duration::from_millis(10));
        }
        CHECK(transmission.success.poll().is_ready());
        CHECK(util::as_str(stream.write_buffer_.written_bytes()) == "@DT04\034abc/R12\r\n");
    }

    SUBCASE("receive information response") {
        stream.read_buffer_.write_str("*IR=01\r\n");
        CHECK(executor.poll(stream, time).is_pending());

        while (executor.poll(stream, time).is_pending()) {
            time.advance(util::Duration::from_millis(10));
        }

        CHECK(transmission.success.poll().is_ready());
        CHECK_FALSE(transmission.success.poll().unwrap().get());
    }
}
