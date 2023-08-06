#include <doctest.h>

#include <media/uhf/command/ei.h>
#include <mock/serial.h>
#include <nb/serial.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("EI") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};

    SUBCASE("success") {
        media::uhf::common::ModemId equipment_id{'1', '2'};
        media::uhf::EIExecutor executor{etl::move(serial), equipment_id};

        for (auto ch : "*EI=12\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        auto result = executor.poll();
        while (result.is_pending()) {
            result = executor.poll();
        }

        for (auto ch : "@EI12\r\n"_u8it) {
            auto cmd = mock_serial.tx_buffer()->pop_front();
            CHECK(cmd == ch);
        }
    }
}
