#include <doctest.h>

#include "net/link/uhf/task/data_transmission.h"
#include <mock/stream.h>
#include <nb/lock.h>
#include <test/net/frame.h>
#include <util/rand.h>

using namespace net::link::uhf;
using namespace net::link::uhf::data_transmisson;

TEST_CASE("DataTransmissionTask") {
    util::MockTime time{0};
    util::MockRandom rand{50};
    mock::MockReadableWritableStream stream{};
    net::link::Address dest{ModemId{0x12}};

    constexpr uint8_t length = 3;
    auto [counter, buffer] = test::make_frame_buffer<length>();
    auto [transmission, request] = test::make_frame_transmission_request(dest, counter, buffer);
    DataTransmissionTask task{etl::move(request)};
    stream.read_buffer_.write_str("*CS=DI\r\n*CS=EN\r\n*DT=03\r\n");
    transmission.writer.write_str("abc");

    while (task.poll(stream, time, rand).is_pending()) {
        time.advance(util::Duration::from_millis(100));
    }
    CHECK(transmission.success.poll().is_ready());
    CHECK(transmission.success.poll().unwrap().get());
}
