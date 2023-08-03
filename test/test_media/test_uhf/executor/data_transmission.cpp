#include <doctest.h>

#include <media/uhf/executor.h>
#include <mock/serial.h>
#include <nb/future.h>
#include <nb/serial.h>

TEST_CASE("data_transmission") {
    using Serial = nb::serial::Serial<mock::MockSerial>;

    mock::MockSerial mock_serial;
    Serial serial{mock_serial};
    nb::lock::Lock<Serial> locked_serial{etl::move(serial)};
    auto tx_buffer = mock_serial.tx_buffer();
    auto rx_buffer = mock_serial.rx_buffer();

    auto [future, promise] = nb::make_future_promise_pair<media::uhf::common::DataWriter<Serial>>();
    uint8_t length = 2;
    media::uhf::common::ModemId destination{'A', 'B'};
    media::uhf::executor::DataTransmissionExecutor<Serial> executor{
        etl::move(locked_serial.lock().value()), etl::move(promise), length, destination};

    SUBCASE("transmit") {
        CHECK(executor.execute().is_pending());
        for (char ch : {'@', 'C', 'S', '\r', '\n'}) {
            auto actual = tx_buffer->pop_front();
            CHECK(actual.has_value());
            CHECK_EQ(actual.value(), ch);
        }
        CHECK(tx_buffer->is_empty());
        for (char ch : {'*', 'C', 'S', '=', 'D', 'I', '\r', '\n'}) {
            rx_buffer->push_back(ch);
        }

        CHECK(executor.execute().is_pending());
        CHECK(executor.execute().is_pending());
        CHECK(executor.execute().is_pending());
        for (char ch : {'@', 'C', 'S', '\r', '\n'}) {
            auto actual = tx_buffer->pop_front();
            CHECK(actual.has_value());
            CHECK_EQ(actual.value(), ch);
        }
        CHECK(tx_buffer->is_empty());
        for (char ch : {'*', 'C', 'S', '=', 'E', 'N', '\r', '\n'}) {
            rx_buffer->push_back(ch);
        }
        CHECK(future.get().is_pending());

        CHECK(executor.execute().is_pending());
        CHECK(executor.execute().is_pending());
        CHECK(executor.execute().is_pending());
        for (char ch : {'@', 'D', 'T'}) {
            auto actual = tx_buffer->pop_front();
            CHECK(actual.has_value());
            CHECK_EQ(actual.value(), ch);
        }
    }
}
