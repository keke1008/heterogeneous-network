#include <doctest.h>

#include <media/uhf/executor.h>
#include <mock/serial.h>
#include <nb/future.h>
#include <nb/serial.h>

TEST_CASE("data_receiving") {
    using Serial = nb::serial::Serial<mock::MockSerial>;

    mock::MockSerial mock_serial;
    Serial serial{mock_serial};
    nb::lock::Lock<Serial> locked_serial{etl::move(serial)};
    auto rx_buffer = mock_serial.rx_buffer();

    auto [future, promise] = nb::make_future_promise_pair<media::uhf::common::DataReader<Serial>>();
    media::uhf::executor::DataReceivingExecutor<Serial> executor{
        etl::move(locked_serial.lock().value()), etl::move(promise)};

    SUBCASE("empty") {
        auto result = executor.execute();
        CHECK(result.is_ready());
        CHECK(result.unwrap().is_err());
    }

    SUBCASE("receive") {
        for (char ch : {'*', 'D', 'R', '=', '0', '2', 'A', 'B', '\r', '\n'}) {
            rx_buffer->push_back(ch);
        }
        CHECK(executor.execute().is_pending());

        auto reader = etl::move(future.get().unwrap());
        for (char ch : {'A', 'B'}) {
            auto result = reader.read();
            CHECK(result.has_value());
            CHECK_EQ(result.value(), ch);
        }
        CHECK(reader.is_reader_closed());

        auto result = executor.execute();
        CHECK(result.is_ready());
    }

    SUBCASE("invalid response name") {
        for (char ch : {'*', 'C', 'S', '=', 'E', 'N', '\r', '\n'}) {
            rx_buffer->push_back(ch);
        }
        auto result = executor.execute();
        CHECK(result.is_ready());
        CHECK(result.unwrap().is_err());
    }
}
