#include <bytes/serde.h>
#include <doctest.h>

TEST_CASE("Serialize") {
    SUBCASE("uint8_t") {
        CHECK_EQ(bytes::serialize<uint8_t>(0x01), etl::make_array<uint8_t>(0x01));
    }

    SUBCASE("uint16_t") {
        CHECK_EQ(bytes::serialize<uint16_t>(0x1234), etl::make_array<uint8_t>(0x34, 0x12));
    }

    SUBCASE("uint32_t") {
        CHECK_EQ(
            bytes::serialize<uint32_t>(0x12345678), etl::make_array<uint8_t>(0x78, 0x56, 0x34, 0x12)
        );
    }
}

TEST_CASE("Deserialize") {
    SUBCASE("uint8_t") {
        CHECK_EQ(bytes::deserialize<uint8_t>(etl::make_array<uint8_t>(0x01)), 0x01);
    }

    SUBCASE("uint16_t") {
        CHECK_EQ(bytes::deserialize<uint16_t>(etl::make_array<uint8_t>(0x34, 0x12)), 0x1234);
    }

    SUBCASE("uint32_t") {
        CHECK_EQ(
            bytes::deserialize<uint32_t>(etl::make_array<uint8_t>(0x78, 0x56, 0x34, 0x12)),
            0x12345678
        );
    }
}

TEST_CASE("Serialize and deserialize") {
    SUBCASE("uint8_t") {
        CHECK_EQ(bytes::deserialize<uint8_t>(bytes::serialize<uint8_t>(0x01)), 0x01);
    }

    SUBCASE("uint16_t") {
        CHECK_EQ(bytes::deserialize<uint16_t>(bytes::serialize<uint16_t>(0x1234)), 0x1234);
    }

    SUBCASE("uint32_t") {
        CHECK_EQ(bytes::deserialize<uint32_t>(bytes::serialize<uint32_t>(0x12345678)), 0x12345678);
    }
}

TEST_CASE("Deserialize and serialize") {
    SUBCASE("uint8_t") {
        CHECK_EQ(
            bytes::serialize<uint8_t>(bytes::deserialize<uint8_t>(etl::make_array<uint8_t>(0x01))),
            etl::make_array<uint8_t>(0x01)
        );
    }

    SUBCASE("uint16_t") {
        CHECK_EQ(
            bytes::serialize<uint16_t>(
                bytes::deserialize<uint16_t>(etl::make_array<uint8_t>(0x34, 0x12))
            ),
            etl::make_array<uint8_t>(0x34, 0x12)
        );
    }

    SUBCASE("uint32_t") {
        CHECK_EQ(
            bytes::serialize<uint32_t>(
                bytes::deserialize<uint32_t>(etl::make_array<uint8_t>(0x78, 0x56, 0x34, 0x12))
            ),
            etl::make_array<uint8_t>(0x78, 0x56, 0x34, 0x12)
        );
    }
}
