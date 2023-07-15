#include <doctest.h>

#include <media/uhf/modem.h>

TEST_CASE("Command") {
    nb::stream::StreamReaderDelegate<media::uhf::modem::GetSerialNumberCommand> command;
    nb::stream::TinyByteWriter<5> output;
    nb::stream::pipe(command, output);

    CHECK(command.is_reader_closed());
    CHECK(output.is_writer_closed());

    auto serialized = output.poll();
    CHECK(serialized.is_ready());
    auto expected = collection::TinyBuffer<uint8_t, 5>{'@', 'S', 'N', '\r', '\n'};
    CHECK_EQ(serialized.unwrap(), expected);
}

TEST_CASE("Response") {
    nb::stream::StreamWriterDelegate<media::uhf::modem::GetSerialNumberResponse> response;
    auto input =
        nb::stream::TinyByteReader<11>{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', '\r', '\n'};
    nb::stream::pipe(input, response);

    CHECK(input.is_reader_closed());
    CHECK(response.is_writer_closed());
    auto actual = response.get_writer().poll();
    CHECK(actual.is_ready());
    collection::TinyBuffer<uint8_t, 9> expected{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};
    CHECK_EQ(actual.unwrap(), expected);
}
