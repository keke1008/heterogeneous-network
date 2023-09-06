#include <doctest.h>

#include <mock/serial.h>
#include <nb/serial.h>
#include <net/link/uhf/command/cs.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("CS") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};

    net::link::uhf::CSExecutor executor;

    SUBCASE("*CS=EN") {
        for (auto ch : "*CS=EN\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        auto result = executor.poll(serial);
        while (result.is_pending()) {
            result = executor.poll(serial);
        }

        for (auto ch : "@CS\r\n"_u8it) {
            auto cmd = mock_serial.tx_buffer()->pop_front();
            CHECK(cmd == ch);
        }

        CHECK(result.unwrap());
    }

    SUBCASE("*CS=DI") {
        for (auto ch : "*CS=DI\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        auto result = executor.poll(serial);
        while (result.is_pending()) {
            result = executor.poll(serial);
        }

        CHECK_FALSE(result.unwrap());
    }
}
