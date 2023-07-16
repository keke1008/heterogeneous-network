#include <doctest.h>

#include <media/uhf/modem.h>

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
    media::uhf::modem::CarrierSenseResponseBody response;
    nb::stream::TinyByteReader<4> input{'E', 'N', '\r', '\n'};

    auto actual = response.read_from(input);
    CHECK_EQ(input.readable_count(), 0);

    CHECK(actual.is_ready());
    CHECK(actual.unwrap());
}

TEST_CASE("Response disabled") {
    media::uhf::modem::CarrierSenseResponseBody response;
    nb::stream::TinyByteReader<4> input{'D', 'I', '\r', '\n'};

    auto actual = response.read_from(input);
    CHECK_EQ(input.readable_count(), 0);

    CHECK(actual.is_ready());
    CHECK_FALSE(actual.unwrap());
}
