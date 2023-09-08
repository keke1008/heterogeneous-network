#include <doctest.h>

#include <nb/stream.h>

using namespace nb::stream;

TEST_CASE("StreamReader") {
    SUBCASE("initialize with uint8_t") {
        FixedStreamReader<3> reader{1, 2, 3};
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(1));
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(2));
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(3));
        CHECK_EQ(reader.read(), nb::pending);
    }

    SUBCASE("initialize with span") {
        etl::array<uint8_t, 3> array{1, 2, 3};
        FixedStreamReader<6> reader{etl::span{array}, etl::span{array}};
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(1));
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(2));
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(3));
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(1));
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(2));
        CHECK_EQ(reader.read(), nb::ready<uint8_t>(3));
        CHECK_EQ(reader.read(), nb::pending);
    }

    SUBCASE("initialize with other reader") {
        FixedStreamReader<3> reader1{1, 2, 3};
        FixedStreamReader<3> reader2{1, 2, 3};
        FixedStreamReader<12> reader3{reader1, reader2, reader1};
        CHECK_EQ(reader3.read(), nb::ready<uint8_t>(1));
        CHECK_EQ(reader3.read(), nb::ready<uint8_t>(2));
        CHECK_EQ(reader3.read(), nb::ready<uint8_t>(3));
        CHECK_EQ(reader3.read(), nb::ready<uint8_t>(1));
        CHECK_EQ(reader3.read(), nb::ready<uint8_t>(2));
        CHECK_EQ(reader3.read(), nb::ready<uint8_t>(3));
        CHECK_EQ(reader3.read(), nb::pending);
    }

    SUBCASE("wait_until_empty") {
        FixedStreamReader<5> reader{1, 2, 3};

        for (int i = 0; i < 3; ++i) {
            CHECK_EQ(reader.wait_until_empty(), nb::pending);
            reader.read();
        }

        CHECK_EQ(reader.wait_until_empty(), nb::ready());
    }
}

TEST_CASE("FixedStreamWriter") {
    SUBCASE("initialize") {
        FixedStreamWriter<3> writer{};
        CHECK_EQ(writer.poll(), nb::pending);
    }

    SUBCASE("poll") {
        FixedStreamWriter<3> writer{};
        writer.drain_all(FixedStreamReader<3>{1, 2, 3});

        auto poll = writer.poll();
        CHECK(poll.is_ready());

        auto span = poll.unwrap();
        CHECK_EQ(span.size(), 3);
        CHECK_EQ(span[0], 1);
        CHECK_EQ(span[1], 2);
        CHECK_EQ(span[2], 3);
    }
}
