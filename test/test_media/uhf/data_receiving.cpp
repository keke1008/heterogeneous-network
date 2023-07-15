#include <doctest.h>

#include <media/uhf/modem.h>

TEST_CASE("Response") {
    nb::stream::StreamWriterDelegate<
        media::uhf::modem::DataReceivingResponse<nb::stream::TinyByteWriter<2>>>
        response;
    nb::stream::TinyByteReader<6> input{'0', '2', 'A', 'B', '\r', '\n'};
    nb::stream::pipe(input, response);

    CHECK(input.is_reader_closed());
    CHECK(response.is_writer_closed());

    auto serialized = response.get_writer().get_data().poll();
    CHECK(serialized.is_ready());
    CHECK_EQ(serialized.unwrap(), collection::TinyBuffer<uint8_t, 2>{'A', 'B'});
}
