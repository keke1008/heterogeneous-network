#include <doctest.h>

#include <media/uhf/modem.h>

TEST_CASE("Command") {
    nb::stream::StreamReaderDelegate<media::uhf::modem::SetModemIdCommand> command{
        media::uhf::modem::CommandName::SetEquipmentId, media::uhf::modem::ModemId{{'A', 'B'}}};
    nb::stream::TinyByteWriter<7> output;
    nb::stream::pipe(command, output);

    CHECK(command.is_reader_closed());
    CHECK(output.is_writer_closed());

    auto serialized = output.poll();
    CHECK(serialized.is_ready());
    auto expected = collection::TinyBuffer<uint8_t, 7>{'@', 'E', 'I', 'A', 'B', '\r', '\n'};
    CHECK_EQ(serialized.unwrap(), expected);
}

TEST_CASE("Response") {
    nb::stream::StreamWriterDelegate<media::uhf::modem::SetModemIdResponse> response;
    nb::stream::TinyByteReader<4> input{collection::TinyBuffer<uint8_t, 4>{'A', 'B', '\r', '\n'}};
    nb::stream::pipe(input, response);

    CHECK(input.is_reader_closed());
    CHECK(response.is_writer_closed());

    auto serialized = response.get_writer().poll();
    CHECK(serialized.is_ready());
    CHECK_EQ(serialized.unwrap(), media::uhf::modem::ModemId{'A', 'B'});
}
