#include <doctest.h>

#include <media/uhf/modem.h>

TEST_CASE("Command") {
    using Serial = nb::serial::Serial<mock::MockSerial>;

    auto [future, promise] = nb::make_future_promise_pair<media::uhf::common::DataWriter<Serial>>();

    mock::MockSerial mock_serial;
    Serial serial{mock_serial};
    nb::lock::Lock<Serial> lock{etl::move(serial)};
    auto locked_serial = etl::move(lock.lock().value());
    auto owned_serial = memory::Owned{etl::move(locked_serial)};
    auto tx_buffer = mock_serial.tx_buffer();

    media::uhf::modem::DataTransmissionCommand<Serial> command{
        etl::move(promise), 2, media::uhf::common::ModemId{'A', 'B'}};

    REQUIRE(future.get().is_pending());

    SUBCASE("send") {
        CHECK_FALSE(command.write_to(owned_serial));

        for (char ch : {'@', 'D', 'T', '0', '2'}) {
            auto actual = tx_buffer->pop_front();
            CHECK(actual.has_value());
            CHECK_EQ(actual.value(), ch);
        }
        CHECK(tx_buffer->is_empty());
        CHECK(future.get().is_ready());

        auto writer = etl::move(future.get().unwrap());
        CHECK(writer.write('C'));
        CHECK(writer.write('D'));
        CHECK_FALSE(writer.write('E'));

        for (char ch : {'C', 'D'}) {
            auto actual = tx_buffer->pop_front();
            CHECK(actual.has_value());
            CHECK_EQ(actual.value(), ch);
        }
        CHECK(tx_buffer->is_empty());

        CHECK(command.write_to(owned_serial));
        for (char ch : {'/', 'R', 'A', 'B', '\r', '\n'}) {
            auto actual = tx_buffer->pop_front();
            CHECK(actual.has_value());
            CHECK_EQ(actual.value(), ch);
        }
        CHECK(tx_buffer->is_empty());
    }
}
