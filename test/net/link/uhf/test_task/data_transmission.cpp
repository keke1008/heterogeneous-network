#include <doctest.h>

#include "net/link/uhf/task/data_transmission.h"
#include <mock/stream.h>
#include <nb/lock.h>
#include <util/rand.h>

using namespace net::link::uhf;
using namespace net::link::uhf::data_transmisson;

TEST_CASE("DataTransmissionTask") {
    util::MockTime time{0};
    util::MockRandom rand{50};
    mock::MockReadableWritableStream stream{};
    net::link::Address dest{ModemId{0x12}};
    constexpr uint8_t length = 3;
    auto protocol = net::frame::ProtocolNumber{001};
    net::frame::FrameService<net::link::Address, 1, 1> frame_service;

    auto writer = etl::move(frame_service.request_frame_writer(length).unwrap());
    net::link::Frame frame{
        .protocol_number = protocol,
        .peer = dest,
        .length = length,
        .reader = writer.make_initial_reader(),
    };

    DataTransmissionTask task{etl::move(frame)};
    stream.read_buffer_.write_str("*CS=DI\r\n*CS=EN\r\n*DT=04\r\n");
    writer.write_str("abc");

    while (task.poll(stream, time, rand).is_pending()) {
        time.advance(util::Duration::from_millis(100));
    }
    CHECK(stream.read_buffer_.readable_count() == 0);
}
