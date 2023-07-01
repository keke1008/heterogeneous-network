#include <doctest.h>
#include <etl/array.h>
#include <nb/stream.h>

TEST_CASE("Pipe") {
    auto reader = nb::stream::make_fixed_stream_reader(1, 2, 3, 4);
    nb::stream::FixedByteWriter<3> writer{};
    nb::stream::pipe(reader, writer);
    CHECK_EQ(writer.get(), etl::make_array<uint8_t>(1, 2, 3));
    CHECK_EQ(reader.readable_count(), 1);
}

TEST_CASE("Pipe readers") {
    auto reader1 = nb::stream::make_fixed_stream_reader(1, 2, 3, 4);
    auto reader2 = nb::stream::make_fixed_stream_reader(5, 6, 7, 8);
    nb::stream::FixedByteWriter<7> writer{};
    CHECK(nb::stream::pipe_readers(writer, reader1, reader2));
    CHECK_EQ(writer.get(), etl::make_array<uint8_t>(1, 2, 3, 4, 5, 6, 7));
    CHECK_EQ(reader1.readable_count(), 0);
    CHECK_EQ(reader2.readable_count(), 1);
}

TEST_CASE("Pipe writers") {
    auto reader = nb::stream::make_fixed_stream_reader(1, 2, 3, 4, 5, 6, 7);
    nb::stream::FixedByteWriter<3> writer1{};
    nb::stream::FixedByteWriter<3> writer2{};
    CHECK_FALSE(nb::stream::pipe_writers(reader, writer1, writer2));
    CHECK_EQ(writer1.get(), etl::make_array<uint8_t>(1, 2, 3));
    CHECK_EQ(writer2.get(), etl::make_array<uint8_t>(4, 5, 6));
    CHECK_EQ(reader.readable_count(), 1);
}
