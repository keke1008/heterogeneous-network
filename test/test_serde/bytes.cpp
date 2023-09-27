#include <doctest.h>

#include <serde/bytes.h>

TEST_CASE("Serialize") {
    SUBCASE("uint8_t") {
        auto result = serde::bytes::serialize<uint8_t>(0x01);
        CHECK_EQ(result, 0x01);
    }

    SUBCASE("uint16_t") {
        auto result = serde::bytes::serialize<uint16_t>(0x1234);
        CHECK_EQ(result[0], 0x34);
        CHECK_EQ(result[1], 0x12);
    }

    SUBCASE("uint32_t") {
        auto result = serde::bytes::serialize<uint32_t>(0x12345678);
        CHECK_EQ(result[0], 0x78);
        CHECK_EQ(result[1], 0x56);
        CHECK_EQ(result[2], 0x34);
        CHECK_EQ(result[3], 0x12);
    }
}

TEST_CASE("Deserialize") {
    SUBCASE("uint8_t") {
        CHECK_EQ(
            serde::bytes::deserialize<uint8_t>(collection::TinyBuffer<uint8_t, 1>(0x01)), 0x01
        );
    }

    SUBCASE("uint16_t") {
        CHECK_EQ(
            serde::bytes::deserialize<uint16_t>(collection::TinyBuffer<uint8_t, 2>(0x34, 0x12)),
            0x1234
        );
    }

    SUBCASE("uint32_t") {
        CHECK_EQ(
            serde::bytes::deserialize<uint32_t>(
                collection::TinyBuffer<uint8_t, 4>(0x78, 0x56, 0x34, 0x12)
            ),
            0x12345678
        );
    }
}

TEST_CASE("Serialize and deserialize") {
    SUBCASE("uint8_t") {
        CHECK_EQ(serde::bytes::deserialize<uint8_t>(serde::bytes::serialize<uint8_t>(0x01)), 0x01);
    }

    SUBCASE("uint16_t") {
        CHECK_EQ(
            serde::bytes::deserialize<uint16_t>(serde::bytes::serialize<uint16_t>(0x1234)), 0x1234
        );
    }

    SUBCASE("uint32_t") {
        CHECK_EQ(
            serde::bytes::deserialize<uint32_t>(serde::bytes::serialize<uint32_t>(0x12345678)),
            0x12345678
        );
    }
}

TEST_CASE("Deserialize and serialize") {
    SUBCASE("uint8_t") {
        CHECK_EQ(
            serde::bytes::serialize<uint8_t>(
                serde::bytes::deserialize<uint8_t>(collection::TinyBuffer<uint8_t, 1>(0x01))
            ),
            collection::TinyBuffer<uint8_t, 1>(0x01)
        );
    }

    SUBCASE("uint16_t") {
        CHECK_EQ(
            serde::bytes::serialize<uint16_t>(
                serde::bytes::deserialize<uint16_t>(collection::TinyBuffer<uint8_t, 2>(0x34, 0x12))
            ),
            collection::TinyBuffer<uint8_t, 2>(0x34, 0x12)
        );
    }

    SUBCASE("uint32_t") {
        CHECK_EQ(
            serde::bytes::serialize<uint32_t>(serde::bytes::deserialize<uint32_t>(
                collection::TinyBuffer<uint8_t, 4>(0x78, 0x56, 0x34, 0x12)
            )),
            collection::TinyBuffer<uint8_t, 4>(0x78, 0x56, 0x34, 0x12)
        );
    }
}
