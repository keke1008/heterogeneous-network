#include <doctest.h>

#include <mock/serial.h>
#include <nb/serial.h>
#include <net/link/uhf/command/ei.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("EI") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};

    SUBCASE("success") {
        net::link::uhf::ModemId equipment_id{0x12};
        net::link::uhf::EIExecutor executor{equipment_id};

        for (auto ch : "*EI=12\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        auto result = executor.poll(serial);
        while (result.is_pending()) {
            result = executor.poll(serial);
        }

        for (auto ch : "@EI12\r\n"_u8it) {
            auto cmd = mock_serial.tx_buffer()->pop_front();
            CHECK(cmd == ch);
        }
    }
}
