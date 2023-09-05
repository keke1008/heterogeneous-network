#include <doctest.h>

#include <nb/stream.h>

TEST_CASE("Instantiation of EmptyStreamReader") {
    nb::stream::EmptyStreamReader reader{};
    CHECK_FALSE(reader.is_readable());
    CHECK(reader.readable_count() == 0);
    CHECK_FALSE(reader.read().has_value());
    CHECK(reader.is_reader_closed());
}

TEST_CASE("Instantiation of EmptyStreamWriter") {
    nb::stream::EmptyStreamWriter writer{};
    CHECK_FALSE(writer.is_writable());
    CHECK(writer.writable_count() == 0);
    CHECK_FALSE(writer.write(0));
    CHECK(writer.is_writer_closed());
}
