#include <doctest.h>

#include <media/uhf/modem.h>
#include <memory/pair_shared.h>
#include <mock/serial.h>
#include <nb/future.h>
#include <nb/lock.h>
#include <nb/serial.h>

TEST_CASE("Response") {
    using Serial = nb::serial::Serial<mock::MockSerial>;

    auto [future, promise] = nb::make_future_promise_pair<media::uhf::common::DataReader<Serial>>();
    media::uhf::modem::DataReceivingResponseBody<Serial> response{etl::move(promise)};

    mock::MockSerial mock_serial;
    Serial serial{mock_serial};
    nb::lock::Lock<Serial> lock{etl::move(serial)};
    auto locked_serial = etl::move(lock.lock().value());
    auto owned_serial = memory::Owned{etl::move(locked_serial)};
    auto rx_buffer = mock_serial.rx_buffer();

    REQUIRE(response.read_from(owned_serial).is_pending());
    REQUIRE(future.get().is_pending());

    SUBCASE("sequential") {
        rx_buffer->push_back('0');
        rx_buffer->push_back('2');
        CHECK(response.read_from(owned_serial).is_pending());
        CHECK(future.get().is_ready());

        auto reader = etl::move(future.get().unwrap());
        rx_buffer->push_back('A');
        rx_buffer->push_back('B');
        CHECK(response.read_from(owned_serial).is_pending());

        auto ch1 = reader.read();
        CHECK(ch1.has_value());
        CHECK_EQ(ch1.value(), 'A');
        CHECK_FALSE(reader.is_reader_closed());

        auto ch2 = reader.read();
        CHECK(ch2.has_value());
        CHECK_EQ(ch2.value(), 'B');
        CHECK(reader.is_reader_closed());

        rx_buffer->push_back('\r');
        rx_buffer->push_back('\n');
        CHECK(response.read_from(owned_serial).is_ready());
        CHECK(reader.is_reader_closed());
    }

    SUBCASE("batch") {
        rx_buffer->push_back('0');
        rx_buffer->push_back('2');
        rx_buffer->push_back('A');
        rx_buffer->push_back('B');
        rx_buffer->push_back('\r');
        rx_buffer->push_back('\n');
        CHECK_EQ(rx_buffer->occupied_count(), 6);

        CHECK(response.read_from(owned_serial).is_pending());
        CHECK(future.get().is_ready());
        CHECK_EQ(rx_buffer->occupied_count(), 4);

        auto reader = etl::move(future.get().unwrap());
        auto ch1 = reader.read();
        CHECK(ch1.has_value());
        CHECK_EQ(ch1.value(), 'A');
        CHECK_FALSE(reader.is_reader_closed());
        CHECK_EQ(rx_buffer->occupied_count(), 3);

        auto ch2 = reader.read();
        CHECK(ch2.has_value());
        CHECK_EQ(ch2.value(), 'B');
        CHECK(reader.is_reader_closed());
        CHECK_EQ(rx_buffer->occupied_count(), 2);

        CHECK(response.read_from(owned_serial).is_ready());
        CHECK(reader.is_reader_closed());
        CHECK_EQ(rx_buffer->occupied_count(), 0);
    }

    SUBCASE("abort") {
        rx_buffer->push_back('0');
        rx_buffer->push_back('2');
        rx_buffer->push_back('A');
        rx_buffer->push_back('B');
        rx_buffer->push_back('\r');
        rx_buffer->push_back('\n');
        CHECK(response.read_from(owned_serial).is_pending());

        {
            auto reader = etl::move(future.get().unwrap());
            reader.read();
            CHECK_EQ(rx_buffer->occupied_count(), 3);
        }

        CHECK(response.read_from(owned_serial).is_ready());
        CHECK_EQ(rx_buffer->occupied_count(), 0);
    }
}
