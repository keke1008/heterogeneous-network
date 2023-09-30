#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <nb/serial.h>
#include <net/link/uhf/command/sn.h>

TEST_CASE("SN") {
    mock::MockReadableWritableStream stream{};

    net::link::uhf::SNExecutor executor;

    SUBCASE("SN=123456789") {
        stream.read_buffer_.write_str("*SN=123456789\r\n");

        auto result = executor.poll(stream);
        CHECK(result.is_ready());
        CHECK(util::as_str(stream.write_buffer_.written_bytes()) == "@SN\r\n");

        auto serial_number = result.unwrap();
        etl::array<uint8_t, 9> expected_serial_number{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        net::link::uhf::SerialNumber expected{etl::span(expected_serial_number)};
        CHECK_EQ(serial_number, expected);
    }
}
