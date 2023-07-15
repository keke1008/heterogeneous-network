#include <doctest.h>

#include <media/uhf/modem.h>

TEST_CASE("Command") {
    nb::stream::StreamReaderDelegate<media::uhf::modem::CarrierSenseCommand> command;
    nb::stream::TinyByteWriter<5> output;
    nb::stream::pipe(command, output);

    CHECK(command.is_reader_closed());
    CHECK(output.is_writer_closed());

    auto serialized = output.poll();
    CHECK(serialized.is_ready());
    auto expected = collection::TinyBuffer<uint8_t, 5>{'@', 'C', 'S', '\r', '\n'};
    CHECK_EQ(serialized.unwrap(), expected);
}

TEST_CASE("Response enabled") {
    nb::stream::StreamWriterDelegate<media::uhf::modem::CarrierSenseResponse> response;
    auto input = nb::stream::TinyByteReader<4>{'E', 'N', '\r', '\n'};
    nb::stream::pipe(input, response);

    CHECK_EQ(input.readable_count(), 0);
    CHECK(response.is_writer_closed());

    auto actual = response.get_writer().poll();
    CHECK(actual.is_ready());
    CHECK(actual.unwrap().is_ok());
}

TEST_CASE("Response disabled") {
    nb::stream::StreamWriterDelegate<media::uhf::modem::CarrierSenseResponse> response;
    auto input = nb::stream::TinyByteReader<4>{'D', 'I', '\r', '\n'};
    nb::stream::pipe(input, response);

    CHECK_EQ(input.readable_count(), 0);
    CHECK(response.is_writer_closed());

    auto actual = response.get_writer().poll();
    CHECK(actual.is_ready());
    CHECK(actual.unwrap().is_err());
}
