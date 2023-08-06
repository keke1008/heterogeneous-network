#include <doctest.h>

#include <media/uhf/command/cs.h>
#include <mock/serial.h>
#include <nb/serial.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("CS") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};

    media::uhf::CSExecutor executor{etl::move(serial)};

    SUBCASE("*CS=EN") {
        for (auto ch : "*CS=EN\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        auto result = executor.poll();
        while (result.is_pending()) {
            result = executor.poll();
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

        auto result = executor.poll();
        while (result.is_pending()) {
            result = executor.poll();
        }

        CHECK_FALSE(result.unwrap());
    }
}
