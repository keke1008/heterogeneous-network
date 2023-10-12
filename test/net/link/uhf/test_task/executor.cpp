#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/uhf/executor.h>
#include <util/rand.h>

using namespace net::link::uhf;

TEST_CASE("UHFExecutor") {
    util::MockTime time{0};
    util::MockRandom rand{50};
    net::frame::FrameService<net::link::Address, 2, 1> frame_service;

    mock::MockReadableWritableStream stream{};
    UHFExecutor uhf_executor{stream};

    SUBCASE("get_serial_number") {
        auto poll_fut = uhf_executor.get_serial_number();
        CHECK(poll_fut.is_ready());
        CHECK(poll_fut.unwrap().poll().is_pending());

        stream.read_buffer_.write_str("*SN=123456789\r\n");
        auto fut = etl::move(poll_fut.unwrap());
        CHECK(fut.poll().is_pending());

        uhf_executor.execute(frame_service, time, rand);

        CHECK(fut.poll().is_ready());
        auto sn = etl::move(fut.poll().unwrap());
        CHECK(sn.get() == SerialNumber{util::as_bytes("123456789")});
    }

    SUBCASE("set_equipment_id") {
        ModemId equipment_id{util::as_bytes("AB")};

        auto poll_fut = uhf_executor.set_equipment_id(equipment_id);
        CHECK(poll_fut.is_ready());
        auto fut = etl::move(poll_fut.unwrap());

        stream.read_buffer_.write_str("*EI=AB\r\n");
        uhf_executor.execute(frame_service, time, rand);
        CHECK(fut.poll().is_ready());
    }

    SUBCASE("data_transmission") {
        net::link::Address dest{ModemId{util::as_bytes("AB")}};
        uint8_t length = 5;
        auto protocol = net::frame::ProtocolNumber{001};

        auto writer = etl::move(frame_service.request_frame_writer(length).unwrap());
        writer.write_str("abcde");
        net::link::Frame frame{
            .protocol_number = protocol,
            .peer = dest,
            .length = length,
            .reader = writer.make_initial_reader(),
        };
        auto writer2 = etl::move(frame_service.request_frame_writer(length).unwrap());
        writer2.write_str("abcde");
        net::link::Frame frame2{
            .protocol_number = protocol,
            .peer = dest,
            .length = length,
            .reader = writer2.make_initial_reader(),
        };

        CHECK(uhf_executor.send_frame(etl::move(frame)).is_ready());
        uhf_executor.execute(frame_service, time, rand);
        CHECK(uhf_executor.send_frame(etl::move(frame2)).is_pending());

        stream.read_buffer_.write_str("*CS=EN\r\n*DT=06\r\n");
        uhf_executor.execute(frame_service, time, rand);
        CHECK(uhf_executor.send_frame(etl::move(frame2)).is_pending());

        time.advance(util::Duration::from_seconds(1));
        uhf_executor.execute(frame_service, time, rand);
        CHECK(uhf_executor.send_frame(etl::move(frame2)).is_ready());
    }

    SUBCASE("data_receiving") {
        constexpr uint8_t length = 5;
        auto protocol = net::frame::ProtocolNumber{001};
        net::link::Address peer{ModemId{util::as_bytes("AB")}};

        stream.read_buffer_.write_str("*DR=06\001abcde\\RAB\r\n");
        uhf_executor.execute(frame_service, time, rand);

        auto poll_frame = uhf_executor.receive_frame();
        CHECK(poll_frame.is_ready());
        auto frame = etl::move(poll_frame.unwrap());
        CHECK(frame.protocol_number == protocol);
        CHECK(frame.peer == peer);
        CHECK(frame.length == length);
        CHECK(util::as_str(frame.reader.written_buffer()) == "abcde");
    }
}
