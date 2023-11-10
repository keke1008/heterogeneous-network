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
    ModemId dest{0x12};
    constexpr uint8_t length = 3;
    auto protocol = net::frame::ProtocolNumber{001};
    memory::Static<net::frame::MultiSizeFrameBufferPool<1, 1>> buffer_pool;
    net::frame::FrameService frame_service{buffer_pool};

    auto writer = etl::move(frame_service.request_frame_writer(length).unwrap());
    net::link::uhf::UhfFrame frame{
        .protocol_number = protocol,
        .remote = dest,
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
