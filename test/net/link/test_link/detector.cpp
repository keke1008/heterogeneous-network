#include <doctest.h>

#include <mock/stream.h>
#include <net/link/detector.h>

using namespace net::link;

TEST_CASE("MediaDetector") {
    mock::MockReadableWritableStream stream;
    util::MockTime time{0};
    MediaDetector detector{stream, time};

    time.advance(util::Duration::from_seconds(200));
    CHECK(detector.poll(time).is_pending());
    CHECK(stream.consume_write_buffer_and_equals_to("AT\r\n"));

    SUBCASE("detect UHF") {
        stream.write_to_read_buffer("*ER=0\r\n");
        auto poll = detector.poll(time);
        CHECK(poll.is_ready());
        CHECK(poll.unwrap() == MediaType::UHF);
    }

    SUBCASE("detect Wifi") {
        stream.write_to_read_buffer("WIFI CONNECTED\r\nOK\r\nWIFI GOT IP\r\n");
        auto poll = detector.poll(time);
        CHECK(poll.is_ready());
        CHECK(poll.unwrap() == MediaType::Wifi);
    }

    SUBCASE("detect serial") {
        stream.write_to_read_buffer("dummy\r\ndummy\r\n");
        auto poll = detector.poll(time);
        CHECK(poll.is_ready());
        CHECK(poll.unwrap() == MediaType::Serial);
    }
}
