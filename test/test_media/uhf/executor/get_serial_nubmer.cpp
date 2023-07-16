#include <doctest.h>

#include <media/uhf/executor.h>
#include <mock/serial.h>
#include <nb/future.h>
#include <nb/serial.h>

TEST_CASE("get_serial_number") {
    using Serial = nb::serial::Serial<mock::MockSerial>;

    mock::MockSerial mock_serial;
    Serial serial{mock_serial};
    nb::lock::Lock<Serial> locked_serial{etl::move(serial)};
    auto tx_buffer = mock_serial.tx_buffer();
    auto rx_buffer = mock_serial.rx_buffer();

    auto [future, promise] = nb::make_future_promise_pair<media::uhf::modem::SerialNumber>();
    media::uhf::executor::GetSerialNumberExecutor<Serial> executor{
        etl::move(locked_serial.lock().value()), etl::move(promise)};

    REQUIRE(executor.execute().is_pending());

    SUBCASE("execute") {
        for (char ch : {'@', 'S', 'N', '\r', '\n'}) {
            auto result = tx_buffer->pop_front();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), ch);
        }

        for (char ch : "*SN=123456789\r\n") {
            if (ch != '\0') {
                rx_buffer->push_back(ch);
            }
        }

        CHECK(executor.execute().is_ready());
        auto result = future.get();
        CHECK(result.is_ready());
        CHECK_EQ(
            result.unwrap(),
            media::uhf::modem::SerialNumber{{'1', '2', '3', '4', '5', '6', '7', '8', '9'}}
        );
    }
}
