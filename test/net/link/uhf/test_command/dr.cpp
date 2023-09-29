#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <nb/future.h>
#include <net/frame/service.h>
#include <net/link/uhf/command/dr.h>

using namespace net::link;

TEST_CASE("DR") {
    mock::MockReadableWritableStream stream{};
    net::frame::FrameService<Address, 1, 1> frame_service;
    net::link::uhf::DRExecutor executor;

    SUBCASE("receive 'abc'") {
        // stream.read_buffer_.write_str("*DR=03abc\\RAB\r\n");
        // CHECK(executor.poll(frame_service, stream).is_ready());

        // auto poll_reception_notification = frame_service.poll_reception_notification();
        // CHECK(poll_reception_notification.is_ready());
        // auto reception_notification = etl::move(poll_reception_notification.unwrap());
        // CHECK(reception_notification.reader.frame_length() == 3);
        //
        // etl::span<uint8_t, 3> buffer;
        // reception_notification.reader.read(buffer);
        // CHECK(util::as_str(buffer) == "abc");
        //
        // auto poll_source = reception_notification.source.poll();
        // CHECK(poll_source.is_ready());
        // CHECK(poll_source.unwrap().get() == Address{uhf::ModemId{0xAB}});
    }
}
