#include <doctest.h>

#include <mock/stream.h>
#include <nb/serial.h>
#include <net/link/uhf/command/sn.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("SN") {
    mock::MockReadableWritableStream stream{};

    net::link::uhf::SNExecutor executor;

    SUBCASE("SN=123456789") {
        stream.write_to_read_buffer("*SN=123456789\r\n"_u8it);

        auto result = executor.poll(stream);
        CHECK(result.is_ready());

        for (auto ch : "@SN\r\n"_u8it) {
            auto cmd = stream.write_buffer_.read();
            CHECK(cmd == ch);
        }

        auto serial_number = result.unwrap();
        etl::array<uint8_t, 9> expected_serial_number{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        net::link::uhf::SerialNumber expected{etl::span(expected_serial_number)};
        CHECK_EQ(serial_number, expected);
    }
}
