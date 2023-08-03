#include <doctest.h>

#include <media/uhf/modem.h>
#include <nb/future.h>

TEST_CASE("Command") {
    media::uhf::modem::CarrierSenseCommand command;
    nb::stream::TinyByteWriter<5> output;
    command.write_to(output);

    CHECK(output.is_writer_closed());

    auto serialized = output.poll();
    CHECK(serialized.is_ready());
    auto expected = collection::TinyBuffer<uint8_t, 5>{'@', 'C', 'S', '\r', '\n'};
    CHECK_EQ(serialized.unwrap(), expected);
}

TEST_CASE("Response enabled") {
    media::uhf::modem::CarrierSenseResponse response;
    nb::stream::TinyByteReader<4> input{'E', 'N', '\r', '\n'};

    auto actual = response.read_from(input);
    CHECK_EQ(input.readable_count(), 0);

    CHECK(actual.is_ready());
    CHECK(actual.unwrap());
}

TEST_CASE("Response disabled") {
    media::uhf::modem::CarrierSenseResponse response;
    nb::stream::TinyByteReader<4> input{'D', 'I', '\r', '\n'};

    auto actual = response.read_from(input);
    CHECK_EQ(input.readable_count(), 0);

    CHECK(actual.is_ready());
    CHECK_FALSE(actual.unwrap());
}

TEST_CASE("Task") {
    auto [f, p] = nb::make_future_promise_pair<bool>();
    media::uhf::modem::CarrierSenseTask task{etl::move(p)};

    mock::MockSerial mock_serial;
    nb::serial::Serial serial{mock_serial};

    SUBCASE("output valid command") {
        task.execute(serial);

        auto data = mock_serial.tx_buffer();
        for (auto ch : {'@', 'C', 'S', '\r', '\n'}) {
            CHECK_EQ(data->pop_front(), ch);
        }
    }

    SUBCASE("`execute` returns Pending before receive response") {
        auto result = task.execute(serial);
        CHECK(result.is_pending());
    }

    // SUBCASE("`execute` returns Pending during receiving response") {
    //     for (auto ch : {'*', 'C', 'S'}) {
    //         mock_serial.rx_buffer()->push_back(ch);
    //     }
    //     CHECK(task.execute(serial).is_pending());
    // }
}
