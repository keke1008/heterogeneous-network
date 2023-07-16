#include <doctest.h>

#include <media/uhf/executor.h>
#include <mock/serial.h>
#include <nb/future.h>
#include <nb/serial.h>

TEST_CASE("set_equipment_id") {
    using Serial = nb::serial::Serial<mock::MockSerial>;

    mock::MockSerial mock_serial;
    Serial serial{mock_serial};
    nb::lock::Lock<Serial> locked_serial{etl::move(serial)};
    auto tx_buffer = mock_serial.tx_buffer();
    auto rx_buffer = mock_serial.rx_buffer();

    auto [future, promise] = nb::make_future_promise_pair<nb::Empty>();
    media::uhf::executor::SetEquipmentIdExecutor<Serial> executor{
        etl::move(locked_serial.lock().value()),
        etl::move(promise),
        media::uhf::common::ModemId{'A', 'B'},
    };

    REQUIRE(executor.execute().is_pending());

    SUBCASE("success") {
        for (char ch : {'@', 'E', 'I', 'A', 'B', '\r', '\n'}) {
            auto result = tx_buffer->pop_front();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), ch);
        }

        for (char ch : {'*', 'E', 'I', '=', 'A', 'B', '\r', '\n'}) {
            rx_buffer->push_back(ch);
        }
        auto result = executor.execute();
        CHECK(result.is_ready());
        CHECK(result.unwrap().is_ok());
    }
}
