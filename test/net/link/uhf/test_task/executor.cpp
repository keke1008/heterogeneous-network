#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/uhf/executor.h>
#include <util/rand.h>

using namespace net::link::uhf;

TEST_CASE("UHFExecutor") {
    util::MockTime time{0};
    util::MockRandom rand{50};
    net::frame::FrameService<net::link::Address, 1, 1> frame_service;

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
        uint8_t protocol = 034;

        auto poll_transmisson = frame_service.request_transmission(protocol, dest, length);
        auto transmission = etl::move(poll_transmisson.unwrap());
        transmission.writer.write_str("abcde");

        uhf_executor.execute(frame_service, time, rand);
        CHECK(frame_service.poll_transmission_request([](auto &) { return true; }).is_pending());
        stream.read_buffer_.write_str("*CS=EN\r\n*DT=06\r\n");
        uhf_executor.execute(frame_service, time, rand);
        time.advance(util::Duration::from_seconds(1));
        uhf_executor.execute(frame_service, time, rand);

        CHECK(transmission.success.poll().is_ready());
        CHECK(transmission.success.poll().unwrap().get());
    }

    SUBCASE("data_receiving") {
        constexpr uint8_t length = 5;
        uint8_t protocol = 034;

        stream.read_buffer_.write_str("*DR=06\034abcde\\RAB\r\n");
        uhf_executor.execute(frame_service, time, rand);

        auto poll_reception_notification = frame_service.poll_reception_notification();
        CHECK(poll_reception_notification.is_ready());
        auto reception_notification = etl::move(poll_reception_notification.unwrap());
        CHECK(reception_notification.reader.frame_length() == length);

        etl::array<uint8_t, length> buffer;
        reception_notification.reader.read(buffer);
        CHECK(util::as_str(buffer) == "abcde");

        CHECK(reception_notification.protocol == protocol);
        auto poll_source = reception_notification.source.poll();
        CHECK(poll_source.is_ready());
        CHECK(poll_source.unwrap().get() == net::link::Address{ModemId{util::as_bytes("AB")}});
    }
}
