#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/detector.h>

using namespace net::link;

TEST_CASE("MediaDetector") {
    mock::MockReadableWritableStream stream;
    util::MockTime time{0};
    MediaDetector detector{stream, time};

    time.advance(util::Duration::from_millis(200));
    CHECK(detector.poll(time).is_pending());
    CHECK(util::as_str(stream.write_buffer_.written_bytes()) == "AT\r\n");
    stream.write_buffer_.reset();

    SUBCASE("detect UHF") {
        stream.read_buffer_.write_str("*ER=0\r\n");
        auto poll = detector.poll(time);
        CHECK(poll.is_ready());
        CHECK(poll.unwrap() == MediaType::UHF);
    }

    SUBCASE("detect Wifi") {
        stream.read_buffer_.write_str("WIFI CONNECTED\r\nOK\r\nWIFI GOT IP\r\n");
        auto poll = detector.poll(time);
        CHECK(poll.is_ready());
        CHECK(poll.unwrap() == MediaType::Wifi);
    }

    SUBCASE("detect serial") {
        stream.read_buffer_.write_str("dummy\r\ndummy\r\n");
        CHECK(detector.poll(time).is_pending());

        time.advance(util::Duration::from_millis(100));
        auto poll = detector.poll(time);
        CHECK(poll.is_ready());
        CHECK(poll.unwrap() == MediaType::Serial);
    }
}
