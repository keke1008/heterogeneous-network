#include <doctest.h>

#include <mock/serial.h>
#include <nb/serial.h>
#include <net/link/uhf/command/sn.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("SN") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};

    net::link::uhf::SNExecutor executor;

    SUBCASE("SN=123456789") {
        for (auto ch : "*SN=123456789\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        auto result = executor.poll(serial);
        while (result.is_pending()) {
            result = executor.poll(serial);
        }

        for (auto ch : "@SN\r\n"_u8it) {
            auto cmd = mock_serial.tx_buffer()->pop_front();
            CHECK(cmd == ch);
        }

        auto serial_number = result.unwrap();
        net::link::uhf::SerialNumber expected{{'1', '2', '3', '4', '5', '6', '7', '8', '9'}};
        CHECK_EQ(serial_number, expected);
    }
}
