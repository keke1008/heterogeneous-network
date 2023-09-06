#include <doctest.h>

#include "net/link/uhf/task/data_transmission.h"
#include <mock/serial.h>
#include <nb/lock.h>
#include <nb/serial.h>
#include <util/rand.h>
#include <util/u8_literal.h>

using namespace net::link::uhf;
using namespace net::link::uhf::data_transmisson;
using namespace util::u8_literal;

using Serial = nb::serial::Serial<mock::MockSerial>;

TEST_CASE("DataTransmissionTask") {
    SUBCASE("execute") {
        util::MockTime time{0};
        util::MockRandom rand{50};
        auto [f, p] = nb::make_future_promise_pair<CommandWriter<Serial>>();
        mock::MockSerial mock_serial{};
        nb::lock::Lock<memory::Owned<Serial>> serial{{mock_serial}};
        DataTransmissionTask task{etl::move(serial.lock().value()), ModemId{0x12}, 3, etl::move(p)};

        for (char ch : "*CS=DI\r\n*CS=EN\r\n*DT=03\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        while (f.poll().is_pending()) {
            time.set_now(time.get_now() + 100);
            task.poll(time, rand);
        }

        auto writer = f.poll().unwrap();
        for (char ch : "abc"_u8it) {
            writer.get().write(ch);
        }
        writer.get().close();

        auto result = task.poll(time, rand);
        while (result.is_pending()) {
            time.set_now(time.get_now() + 100);
            result = task.poll(time, rand);
        }

        CHECK(result.is_ready());
        CHECK_EQ(mock_serial.rx_buffer()->occupied_count(), 0);
    }
}
