#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/frame/service.h>
#include <net/link/serial.h>
#include <test/net/frame.h>

using namespace net::link::serial;
using namespace net::link;

TEST_CASE("Executor") {
    mock::MockReadableWritableStream stream;
    Address source{net::link::SerialAddress{012}};
    SerialExecutor executor{stream, net::link::SerialAddress{source}};
    Address remote{SerialAddress{034}};
    net::frame::FrameService<Address, 1, 1> frame_service;

    uint8_t protocol = 056;
    etl::array<uint8_t, PREAMBLE_LENGTH> PREAMBLE_VALUE;
    PREAMBLE_VALUE.fill(PREAMBLE);

    SUBCASE("send_data") {
        uint8_t length = 05;
        auto poll_transmission = frame_service.request_transmission(protocol, remote, length);
        auto transmission = etl::move(poll_transmission.unwrap());
        transmission.writer.write_str("abcde");

        executor.execute(frame_service);
        CHECK(frame_service.poll_transmission_request([](auto &) { return true; }).is_pending());
        CHECK_EQ(
            util::as_str(stream.write_buffer_.written_bytes().subspan(0, PREAMBLE_LENGTH)),
            util::as_str(PREAMBLE_VALUE)
        );
        CHECK_EQ(
            util::as_str(stream.write_buffer_.written_bytes().subspan(PREAMBLE_LENGTH)),
            "\056\012\034\005abcde"
        );

        auto poll_success = transmission.success.poll();
        CHECK(poll_success.is_ready());
        CHECK(poll_success.unwrap().get());
    }

    SUBCASE("receive data") {
        constexpr uint8_t length = 5;
        stream.read_buffer_.write_str("dummy");
        executor.execute(frame_service);
        CHECK(frame_service.poll_reception_notification().is_pending());

        stream.read_buffer_.write(PREAMBLE_VALUE);
        stream.read_buffer_.write_str("\056\034\012\005");
        executor.execute(frame_service);

        auto poll_reception_notification = frame_service.poll_reception_notification();
        CHECK(poll_reception_notification.is_ready());
        auto reception_notification = etl::move(poll_reception_notification.unwrap());
        CHECK(reception_notification.reader.frame_length() == length);
        CHECK(reception_notification.protocol == protocol);
        CHECK(reception_notification.source.poll().is_ready());
        CHECK(reception_notification.source.poll().unwrap().get() == remote);

        stream.read_buffer_.write_str("abcde");
        executor.execute(frame_service);
        etl::array<uint8_t, length> buffer;
        reception_notification.reader.read(buffer);
        CHECK(util::as_str(buffer) == "abcde");
    }

    SUBCASE("discard data") {
        constexpr uint8_t length = 5;
        stream.read_buffer_.write_str("dummy");
        stream.read_buffer_.write(PREAMBLE_VALUE);
        stream.read_buffer_.write_str("\056\034\077\005abcde");
        executor.execute(frame_service);
        CHECK(frame_service.poll_reception_notification().is_pending());
        CHECK(stream.read_buffer_.readable_count() == 0);

        stream.read_buffer_.write_str("dummy");
        stream.read_buffer_.write(PREAMBLE_VALUE);
        stream.read_buffer_.write_str("\056\034\012\005fghij");
        executor.execute(frame_service);
        auto poll_reception_notification = frame_service.poll_reception_notification();
        CHECK(poll_reception_notification.is_ready());
        auto reception_notification = etl::move(poll_reception_notification.unwrap());
        CHECK(reception_notification.reader.frame_length() == length);
        etl::array<uint8_t, length> buffer;
        reception_notification.reader.read(buffer);
        CHECK(util::as_str(buffer) == "fghij");
    }
}
