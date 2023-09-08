#include <doctest.h>

#include <nb/stream.h>

using namespace nb::stream;

TEST_CASE("StreamReader") {
    SUBCASE("initialize with uint8_t") {
        FixedReadableBuffer<3> reader{1, 2, 3};
        CHECK_EQ(reader.read(), 1);
        CHECK_EQ(reader.read(), 2);
        CHECK_EQ(reader.read(), 3);
        CHECK_EQ(reader.readable_count(), 0);
    }

    SUBCASE("initialize with span") {
        etl::array<uint8_t, 3> array{1, 2, 3};
        FixedReadableBuffer<6> reader{etl::span{array}, etl::span{array}};
        CHECK_EQ(reader.read(), 1);
        CHECK_EQ(reader.read(), 2);
        CHECK_EQ(reader.read(), 3);
        CHECK_EQ(reader.read(), 1);
        CHECK_EQ(reader.read(), 2);
        CHECK_EQ(reader.read(), 3);
        CHECK_EQ(reader.readable_count(), 0);
    }

    SUBCASE("initialize with other reader") {
        FixedReadableBuffer<3> reader1{1, 2, 3};
        FixedReadableBuffer<3> reader2{1, 2, 3};
        FixedReadableBuffer<12> reader3{reader1, reader2, reader1};
        CHECK_EQ(reader3.read(), 1);
        CHECK_EQ(reader3.read(), 2);
        CHECK_EQ(reader3.read(), 3);
        CHECK_EQ(reader3.read(), 1);
        CHECK_EQ(reader3.read(), 2);
        CHECK_EQ(reader3.read(), 3);
        CHECK_EQ(reader3.readable_count(), 0);
    }

    SUBCASE("wait_until_empty") {
        FixedReadableBuffer<5> reader{1, 2, 3};
        CHECK_EQ(reader.readable_count(), 3);

        CHECK_EQ(reader.read(), 1);
        CHECK_EQ(reader.read(), 2);
        CHECK_EQ(reader.read(), 3);

        CHECK_EQ(reader.readable_count(), 0);
    }
}

TEST_CASE("FixedStreamWriter") {
    SUBCASE("initialize") {
        FixedWritableBuffer<3> writer{};
        CHECK_EQ(writer.poll(), nb::pending);
        CHECK_EQ(writer.writable_count(), 3);
    }

    SUBCASE("initialize with length") {
        FixedWritableBuffer<3> writer{2};
        CHECK_EQ(writer.poll(), nb::pending);
        CHECK_EQ(writer.writable_count(), 2);
    }

    SUBCASE("poll") {
        FixedWritableBuffer<5> writer{3};
        FixedReadableBuffer<3>{1, 2, 3}.read_all_into(writer);

        auto poll = writer.poll();
        CHECK(poll.is_ready());

        auto span = poll.unwrap();
        CHECK_EQ(span.size(), 3);
        CHECK_EQ(span[0], 1);
        CHECK_EQ(span[1], 2);
        CHECK_EQ(span[2], 3);
    }
}
