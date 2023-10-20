#include <doctest.h>

#include <serde/bin.h>

TEST_CASE("serialize<uint8_t>") {
    uint8_t value = 0x12;
    etl::array<uint8_t, 1> buffer;
    CHECK(serde::bin::serialize(buffer, value) == 1);
    CHECK(buffer[0] == 0x12);
}

TEST_CASE("serialize<uint16_t>") {
    uint16_t value = 0x1234;
    etl::array<uint8_t, 2> buffer;
    CHECK(serde::bin::serialize(buffer, value) == 2);
    CHECK(buffer[0] == 0x34);
    CHECK(buffer[1] == 0x12);
}

TEST_CASE("serialize<uint32_t>") {
    uint32_t value = 0x12345678;
    etl::array<uint8_t, 4> buffer;
    CHECK(serde::bin::serialize(buffer, value) == 4);
    CHECK(buffer[0] == 0x78);
    CHECK(buffer[1] == 0x56);
    CHECK(buffer[2] == 0x34);
    CHECK(buffer[3] == 0x12);
}

TEST_CASE("serialize<uint64_t>") {
    uint64_t value = 0x123456789ABCDEF0;
    etl::array<uint8_t, 8> buffer;
    CHECK(serde::bin::serialize(buffer, value) == 8);
    CHECK(buffer[0] == 0xF0);
    CHECK(buffer[1] == 0xDE);
    CHECK(buffer[2] == 0xBC);
    CHECK(buffer[3] == 0x9A);
    CHECK(buffer[4] == 0x78);
    CHECK(buffer[5] == 0x56);
    CHECK(buffer[6] == 0x34);
    CHECK(buffer[7] == 0x12);
}

TEST_CASE("deserialize<uint8_t>") {
    etl::array<uint8_t, 1> buffer{0x12};
    CHECK(serde::bin::deserialize<uint8_t>(buffer) == 0x12);
}

TEST_CASE("deserialize<uint16_t>") {
    etl::array<uint8_t, 2> buffer{0x34, 0x12};
    CHECK(serde::bin::deserialize<uint16_t>(buffer) == 0x1234);
}

TEST_CASE("deserialize<uint32_t>") {
    etl::array<uint8_t, 4> buffer{0x78, 0x56, 0x34, 0x12};
    CHECK(serde::bin::deserialize<uint32_t>(buffer) == 0x12345678);
}

TEST_CASE("deserialize<uint64_t>") {
    etl::array<uint8_t, 8> buffer{0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12};
    CHECK(serde::bin::deserialize<uint64_t>(buffer) == 0x123456789ABCDEF0);
}
