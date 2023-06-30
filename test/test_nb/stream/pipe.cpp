#include <doctest.h>
#include <etl/array.h>
#include <nb/stream.h>

TEST_CASE("Pipe") {
    auto reader = nb::stream::make_fixed_stream_reader(1, 2, 3, 4);
    nb::stream::FixedByteWriter<3> writer{};
    CHECK(nb::stream::pipe(reader, writer));
    CHECK_EQ(writer.get(), etl::make_array<uint8_t>(1, 2, 3));
    CHECK_EQ(reader.readable_count(), 1);
}
