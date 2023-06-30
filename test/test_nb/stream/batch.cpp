#include "nb/stream/batch.h"
#include <doctest.h>
#include <nb/stream.h>

TEST_CASE("writable_count") {
    nb::stream::FixedByteWriter<1> w1{};
    nb::stream::FixedByteWriter<2> w2{};
    nb::stream::FixedByteWriter<4> w3{};
    CHECK_EQ(nb::stream::writable_count(w1, w2, w3), 7);
}

TEST_CASE("readable_count") {
    auto r1 = nb::stream::make_fixed_stream_reader(1);
    auto r2 = nb::stream::make_fixed_stream_reader(1, 2);
    auto r3 = nb::stream::make_fixed_stream_reader(1, 2, 3, 4);
    CHECK_EQ(nb::stream::readable_count(r1, r2, r3), 7);
}

TEST_CASE("write") {
    nb::stream::FixedByteWriter<1> w1{};
    nb::stream::FixedByteWriter<2> w2{};
    nb::stream::FixedByteWriter<4> w3{};

    CHECK(nb::stream::write(1, w1, w2, w3));
    CHECK_EQ(w1.get()[0], 1);

    CHECK(nb::stream::write(2, w1, w2, w3));
    CHECK_EQ(w2.get()[0], 2);
    CHECK(nb::stream::write(3, w1, w2, w3));
    CHECK_EQ(w2.get()[1], 3);

    CHECK(nb::stream::write(4, w1, w2, w3));
    CHECK_EQ(w3.get()[0], 4);
    CHECK(nb::stream::write(5, w1, w2, w3));
    CHECK_EQ(w3.get()[1], 5);
    CHECK(nb::stream::write(6, w1, w2, w3));
    CHECK_EQ(w3.get()[2], 6);
    CHECK(nb::stream::write(7, w1, w2, w3));
    CHECK_EQ(w3.get()[3], 7);

    CHECK_FALSE(nb::stream::write(8, w1, w2, w3));
}

TEST_CASE("read") {
    auto r1 = nb::stream::make_fixed_stream_reader(1);
    auto r2 = nb::stream::make_fixed_stream_reader(2, 3);
    auto r3 = nb::stream::make_fixed_stream_reader(4, 5, 6, 7);
    for (int i = 1; i <= 7; i++) {
        CHECK_EQ(nb::stream::read(r1, r2, r3), i);
    }
    CHECK_FALSE(nb::stream::read(r1, r2, r3).has_value());
}
