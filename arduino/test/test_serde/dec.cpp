#include <doctest.h>

#include <serde/dec.h>

TEST_CASE("serialize<uint8_t>") {
    SUBCASE("0") {
        uint8_t value = 0;
        etl::array<uint8_t, 3> result;

        CHECK(serde::dec::serialize(result, value) == 1);

        CHECK(result[0] == '0');
    }

    SUBCASE("1 digit") {
        uint8_t value = 1;
        etl::array<uint8_t, 3> result;

        CHECK(serde::dec::serialize(result, value) == 1);

        CHECK(result[0] == '1');
    }

    SUBCASE("2 digits") {
        uint8_t value = 12;
        etl::array<uint8_t, 3> result;

        CHECK(serde::dec::serialize(result, value) == 2);

        CHECK(result[0] == '1');
        CHECK(result[1] == '2');
    }

    SUBCASE("3 digits") {
        uint8_t value = 123;
        etl::array<uint8_t, 3> result;

        CHECK(serde::dec::serialize(result, value) == 3);

        CHECK(result[0] == '1');
        CHECK(result[1] == '2');
        CHECK(result[2] == '3');
    }
}

TEST_CASE("deserialize<uint8_t>") {
    SUBCASE("0") {
        etl::array<uint8_t, 1> data{'0'};

        CHECK(serde::dec::deserialize<uint8_t>(data) == 0);
    }

    SUBCASE("0 digits") {
        etl::array<uint8_t, 0> data{};

        CHECK(serde::dec::deserialize<uint8_t>(data) == 0);
    }

    SUBCASE("1 digit") {
        etl::array<uint8_t, 1> data{'1'};

        CHECK(serde::dec::deserialize<uint8_t>(data) == 1);
    }

    SUBCASE("2 digits") {
        etl::array<uint8_t, 2> data{'1', '2'};

        CHECK(serde::dec::deserialize<uint8_t>(data) == 12);
    }

    SUBCASE("3 digits") {
        etl::array<uint8_t, 3> data{'1', '2', '3'};

        CHECK(serde::dec::deserialize<uint8_t>(data) == 123);
    }
}
