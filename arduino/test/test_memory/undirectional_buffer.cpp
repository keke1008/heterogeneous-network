#include <doctest.h>

#include <memory/unidirectional_buffer.h>

TEST_CASE("Instantiate unidirectional buffer") {
    memory::UnidirectionalBuffer<uint8_t> buffer{10};
    CHECK_EQ(buffer.writable_count(), 10);
    CHECK_EQ(buffer.readable_count(), 0);
    CHECK(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Push and shift") {
    memory::UnidirectionalBuffer<uint8_t> buffer{10};
    CHECK(buffer.push(1));
    CHECK_EQ(buffer.writable_count(), 9);
    CHECK_EQ(buffer.readable_count(), 1);
    CHECK_FALSE(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());

    CHECK_EQ(buffer.shift(), etl::optional<uint8_t>{1});
    CHECK_EQ(buffer.writable_count(), 9);
    CHECK_EQ(buffer.readable_count(), 0);
    CHECK(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Push multiple and shift multiple") {
    memory::UnidirectionalBuffer<uint8_t> buffer{10};
    for (uint8_t i = 0; i < 10; i++) {
        CHECK(buffer.push(i));
    }
    CHECK_EQ(buffer.writable_count(), 0);
    CHECK_EQ(buffer.readable_count(), 10);
    CHECK_FALSE(buffer.is_empty());
    CHECK(buffer.is_full());
    CHECK_FALSE(buffer.push(11));

    for (uint8_t i = 0; i < 10; i++) {
        CHECK_EQ(buffer.shift(), etl::optional<uint8_t>{i});
    }
    CHECK_EQ(buffer.writable_count(), 0);
    CHECK_EQ(buffer.readable_count(), 0);
    CHECK(buffer.is_empty());
    CHECK(buffer.is_full());
}
