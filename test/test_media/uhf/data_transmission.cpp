#include <doctest.h>

#include <media/uhf/modem.h>

TEST_CASE("Command") {
    nb::stream::StreamReaderDelegate<
        media::uhf::modem::DataTransmissionCommand<nb::stream::TinyByteReader<2>>>
        command{'A', 'B'};

    nb::stream::TinyByteWriter<9> output;
    nb::stream::pipe(command, output);

    CHECK(command.is_reader_closed());
    CHECK(output.is_writer_closed());

    auto serialized = output.poll();
    CHECK(serialized.is_ready());
    auto expected =
        collection::TinyBuffer<uint8_t, 9>{'@', 'D', 'T', '0', '2', 'A', 'B', '\r', '\n'};
    CHECK_EQ(serialized.unwrap(), expected);
}
