#include <doctest.h>

#include <serde/hex.h>

TEST_CASE("Serialize uint8_t") {
    auto result = serde::hex::serialize<uint8_t>(0x12);
    CHECK_EQ(result[0], '1');
    CHECK_EQ(result[1], '2');
}

TEST_CASE("Serialize uint16_t") {
    auto result = serde::hex::serialize<uint16_t>(0x1234);
    CHECK_EQ(result[0], '1');
    CHECK_EQ(result[1], '2');
    CHECK_EQ(result[2], '3');
    CHECK_EQ(result[3], '4');
}

TEST_CASE("Serialize uint32_t") {
    auto result = serde::hex::serialize<uint32_t>(0x12345678);
    CHECK_EQ(result[0], '1');
    CHECK_EQ(result[1], '2');
    CHECK_EQ(result[2], '3');
    CHECK_EQ(result[3], '4');
    CHECK_EQ(result[4], '5');
    CHECK_EQ(result[5], '6');
    CHECK_EQ(result[6], '7');
    CHECK_EQ(result[7], '8');
}

TEST_CASE("Deserialize uint8_t") {
    auto result = serde::hex::deserialize<uint8_t>(collection::TinyBuffer<uint8_t, 2>{'1', '2'});
    CHECK(result);
    CHECK_EQ(*result, 0x12);
}

TEST_CASE("Deserialize uint16_t") {
    auto result =
        serde::hex::deserialize<uint16_t>(collection::TinyBuffer<uint8_t, 4>{'1', '2', '3', '4'});
    CHECK(result);
    CHECK_EQ(*result, 0x1234);
}

TEST_CASE("Deserialize uint32_t") {
    auto result = serde::hex::deserialize<uint32_t>(collection::TinyBuffer<uint8_t, 8>{
        '1', '2', '3', '4', '5', '6', '7', '8'});
    CHECK(result);
    CHECK_EQ(*result, 0x12345678);
}
