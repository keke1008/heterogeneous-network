#include <doctest.h>

#include <collection/raw_buffer.hpp>

TEST_CASE("create empty RawBuffer") {
    auto buf = collection::RawBuffer<int>();
    CHECK_EQ(buf.capacity(), 0);
}

TEST_CASE("create Rawbuffer with capacity") {
    auto buf = collection::RawBuffer<int>(42);
    CHECK_EQ(buf.capacity(), 42);
}

TEST_CASE("write buffer") {
    auto buf = collection::RawBuffer<int>(1);
    buf.set(0, 42);
    CHECK_EQ(buf.at(0), 42);
}

TEST_CASE("move element") {
    auto buf = collection::RawBuffer<int>(1);
    buf.set(0, 42);
    auto e = buf.move_at(0);
    CHECK_EQ(e, 42);
}

TEST_CASE("move_from") {
    auto buf1 = collection::RawBuffer<int>(5);
    buf1.set(0, 0);
    buf1.set(1, 1);
    buf1.set(2, 2);

    auto buf2 = collection::RawBuffer<int>(3);
    buf2.move_from(1, 0, 2, buf1);

    CHECK_EQ(buf2.at(1), 0);
    CHECK_EQ(buf2.at(2), 1);
}

TEST_CASE("move buffer to right") {
    auto buf1 = collection::RawBuffer<int>(5);
    buf1.set(0, 0);
    buf1.set(1, 1);
    buf1.set(2, 2);

    buf1.move_from(2, 0, 3, buf1);
    CHECK_EQ(buf1.at(2), 0);
    CHECK_EQ(buf1.at(3), 1);
    CHECK_EQ(buf1.at(4), 2);
}

TEST_CASE("move buffer to left") {
    auto buf1 = collection::RawBuffer<int>(5);
    buf1.set(2, 0);
    buf1.set(3, 1);
    buf1.set(4, 2);

    buf1.move_from(0, 2, 3, buf1);
    CHECK_EQ(buf1.at(0), 0);
    CHECK_EQ(buf1.at(1), 1);
    CHECK_EQ(buf1.at(2), 2);
}
