#include <doctest.h>
#include <nb/stream.h>

TEST_CASE("FixedByteReader") {
    SUBCASE("Instantiate") {
        nb::stream::FixedByteReader<1> reader{etl::make_array<uint8_t>(0x01)};
        CHECK_EQ(reader.readable_count(), 1);
        CHECK_FALSE(reader.is_reader_closed());
    }

    SUBCASE("Read") {
        nb::stream::FixedByteReader<1> reader{etl::make_array<uint8_t>(0x01)};
        CHECK_EQ(reader.read(), 0x01);
        CHECK_EQ(reader.readable_count(), 0);
        CHECK(reader.is_reader_closed());
        CHECK_EQ(reader.read(), etl::nullopt);
    }

    SUBCASE("Read multiple") {
        nb::stream::FixedByteReader<2> reader{etl::make_array<uint8_t>(0x01, 0x02)};
        CHECK_EQ(reader.read(), 0x01);
        CHECK_EQ(reader.readable_count(), 1);
        CHECK_FALSE(reader.is_reader_closed());

        CHECK_EQ(reader.read(), 0x02);
        CHECK_EQ(reader.readable_count(), 0);
        CHECK(reader.is_reader_closed());
        CHECK_EQ(reader.read(), etl::nullopt);
    }
}

TEST_CASE("FixedByteWriter") {
    SUBCASE("Instantiate") {
        nb::stream::FixedByteWriter<1> writer{};
        CHECK_EQ(writer.writable_count(), 1);
        CHECK_FALSE(writer.is_writer_closed());
    }

    SUBCASE("Write") {
        nb::stream::FixedByteWriter<1> writer{};
        CHECK(writer.write(0x01));
        CHECK_EQ(writer.writable_count(), 0);
        CHECK(writer.is_writer_closed());
        CHECK_FALSE(writer.write(0x02));
    }

    SUBCASE("Write multiple") {
        nb::stream::FixedByteWriter<2> writer{};
        CHECK(writer.write(0x01));
        CHECK(writer.write(0x02));
        CHECK_EQ(writer.writable_count(), 0);
        CHECK(writer.is_writer_closed());
        CHECK_FALSE(writer.write(0x03));
    }
}
