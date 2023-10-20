#include "etl/memory.h"
#include <doctest.h>

#include <memory/circular_buffer.h>

TEST_CASE("Instantiate CircularBuffer") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    auto buffer = memory::CircularBuffer<etl::unique_ptr<uint8_t[]>>::make_heap(10);
    CHECK_EQ(buffer.vacant_count(), 10);
    CHECK_EQ(buffer.occupied_count(), 0);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Instantiate CircularBuffer with stack buffer") {
    auto buffer = memory::CircularBuffer<uint8_t[]>::make_stack<10>();
    CHECK_EQ(buffer.vacant_count(), 10);
    CHECK_EQ(buffer.occupied_count(), 0);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Move CircularBuffer") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    auto buffer2 = etl::move(buffer);
    CHECK_EQ(buffer2.vacant_count(), 10);
    CHECK_EQ(buffer2.occupied_count(), 0);
    CHECK_EQ(buffer2.capacity(), 10);
    CHECK(buffer2.is_empty());
    CHECK_FALSE(buffer2.is_full());
}

TEST_CASE("Push back") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    buffer.push_back(1);
    CHECK_EQ(buffer.vacant_count(), 9);
    CHECK_EQ(buffer.occupied_count(), 1);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK_FALSE(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Push back full") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    for (uint8_t i = 0; i < 10; ++i) {
        buffer.push_back(i);
    }
    CHECK_EQ(buffer.vacant_count(), 0);
    CHECK_EQ(buffer.occupied_count(), 10);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK_FALSE(buffer.is_empty());
    CHECK(buffer.is_full());
    CHECK_FALSE(buffer.push_front(11));
    CHECK_FALSE(buffer.push_back(11));
}

TEST_CASE("Push front") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    buffer.push_front(1);
    CHECK_EQ(buffer.vacant_count(), 9);
    CHECK_EQ(buffer.occupied_count(), 1);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK_FALSE(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Push front full") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    for (uint8_t i = 0; i < 10; ++i) {
        buffer.push_front(i);
    }
    CHECK_EQ(buffer.vacant_count(), 0);
    CHECK_EQ(buffer.occupied_count(), 10);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK_FALSE(buffer.is_empty());
    CHECK(buffer.is_full());
    CHECK_FALSE(buffer.push_front(11));
    CHECK_FALSE(buffer.push_back(11));
}

TEST_CASE("Pop back") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    buffer.push_back(1);
    CHECK_EQ(buffer.pop_back(), 1);
    CHECK_EQ(buffer.vacant_count(), 10);
    CHECK_EQ(buffer.occupied_count(), 0);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Pop back empty") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    CHECK_EQ(buffer.pop_back(), etl::nullopt);
    CHECK_EQ(buffer.vacant_count(), 10);
    CHECK_EQ(buffer.occupied_count(), 0);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Pop front") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);

    buffer.push_back(1);
    CHECK_EQ(buffer.pop_front(), 1);
    CHECK_EQ(buffer.vacant_count(), 10);
    CHECK_EQ(buffer.occupied_count(), 0);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Pop front empty") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);

    CHECK_EQ(buffer.pop_front(), etl::nullopt);
    CHECK_EQ(buffer.vacant_count(), 10);
    CHECK_EQ(buffer.occupied_count(), 0);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());
}

TEST_CASE("Peek back") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    buffer.push_back(1);
    CHECK_EQ(buffer.peek_back(), 1);
    CHECK_EQ(buffer.vacant_count(), 9);
    CHECK_EQ(buffer.occupied_count(), 1);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK_FALSE(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());

    buffer.pop_back();
    CHECK_EQ(buffer.peek_back(), etl::nullopt);
}

TEST_CASE("Peek back empty") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    CHECK_EQ(buffer.peek_back(), etl::nullopt);
}

TEST_CASE("Peek front") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);
    buffer.push_back(1);
    CHECK_EQ(buffer.peek_front(), 1);
    CHECK_EQ(buffer.vacant_count(), 9);
    CHECK_EQ(buffer.occupied_count(), 1);
    CHECK_EQ(buffer.capacity(), 10);
    CHECK_FALSE(buffer.is_empty());
    CHECK_FALSE(buffer.is_full());

    buffer.pop_front();
    CHECK_EQ(buffer.peek_front(), etl::nullopt);
}

TEST_CASE("Peek front empty") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[10]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 10);

    CHECK_EQ(buffer.peek_front(), etl::nullopt);
}

TEST_CASE("Push back and front") {
    etl::unique_ptr<uint8_t[]> a(new uint8_t[3]);
    memory::CircularBuffer<etl::unique_ptr<uint8_t[]>> buffer(etl::move(a), 3);
    buffer.push_back(2);
    buffer.push_front(1);
    buffer.push_back(3);
    CHECK_EQ(buffer.peek_back(), 3);
    CHECK_EQ(buffer.peek_front(), 1);
    CHECK_EQ(buffer.vacant_count(), 0);
    CHECK_EQ(buffer.occupied_count(), 3);
    CHECK_EQ(buffer.capacity(), 3);
    CHECK_FALSE(buffer.is_empty());
    CHECK(buffer.is_full());
    CHECK_FALSE(buffer.push_back(4));
    CHECK_FALSE(buffer.push_front(4));
}
