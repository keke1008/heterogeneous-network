#include <doctest.h>

#include <data/cache_set.h>

using namespace data;

TEST_CASE("full") {
    CacheSet<uint8_t, 1> cache_set{};
    CHECK(cache_set.full() == false);

    cache_set.push(0);
    CHECK(cache_set.full() == true);
}

TEST_CASE("get empty") {
    CacheSet<uint8_t, 1> cache_set{};
    CHECK(cache_set.get(0) == etl::nullopt);
}

TEST_CASE("get out of bounds") {
    CacheSet<uint8_t, 1> cache_set{};
    cache_set.push(0);
    CHECK(cache_set.get(1) == etl::nullopt);
}

TEST_CASE("push") {
    CacheSet<uint8_t, 2> cache_set{};
    cache_set.push(0);
    cache_set.push(1);

    CHECK(cache_set.get(0).has_value());
    CHECK(cache_set.get(0).value() == 0);

    CHECK(cache_set.get(1).has_value());
    CHECK(cache_set.get(1).value() == 1);
}

TEST_CASE("pop oldest") {
    CacheSet<uint8_t, 2> cache_set{};
    cache_set.push(0);
    cache_set.push(1);
    cache_set.pop_oldest();

    CHECK(cache_set.get(0).has_value());
    CHECK(cache_set.get(0).value() == 1);
    CHECK(cache_set.get(1) == etl::nullopt);
}

TEST_CASE("push circular") {
    CacheSet<uint8_t, 2> cache_set{};
    cache_set.push(0);
    cache_set.push(1);
    cache_set.push(2);

    CHECK(cache_set.get(0).has_value());
    CHECK(cache_set.get(0).value() == 1);
    CHECK(cache_set.get(1).has_value());
    CHECK(cache_set.get(1).value() == 2);
}

TEST_CASE("push many") {
    CacheSet<uint8_t, 4> cache_set{};
    for (uint8_t i = 0; i < 10; i++) {
        cache_set.push(i);
    }

    CHECK(cache_set.get(0).has_value());
    CHECK(cache_set.get(0).value() == 6);
    CHECK(cache_set.get(1).has_value());
    CHECK(cache_set.get(1).value() == 7);
    CHECK(cache_set.get(2).has_value());
    CHECK(cache_set.get(2).value() == 8);
    CHECK(cache_set.get(3).has_value());
    CHECK(cache_set.get(3).value() == 9);
}

TEST_CASE("remove") {
    CacheSet<uint8_t, 2> cache_set{};
    cache_set.push(0);
    cache_set.push(1);
    cache_set.remove(0);

    CHECK(cache_set.get(0).has_value());
    CHECK(cache_set.get(0).value() == 1);
    CHECK(cache_set.get(1) == etl::nullopt);
}

TEST_CASE("find") {
    CacheSet<uint8_t, 4> cache_set{};
    cache_set.push(0);
    cache_set.push(1);
    cache_set.push(2);
    cache_set.push(3);

    CHECK(cache_set.find_if([](uint8_t entry) { return entry == 0; }) == true);
    CHECK(cache_set.find_if([](uint8_t entry) { return entry == 4; }) == false);
}

TEST_CASE("complex 1") {
    CacheSet<uint8_t, 3> cache_set{};
    cache_set.push(0);      // => [0]
    cache_set.push(1);      // => [0, 1]
    cache_set.push(2);      // => [0, 1, 2]
    cache_set.push(3);      // => [1, 2, 3]
    cache_set.pop_oldest(); // => [2, 3]
    cache_set.push(4);      // => [2, 3, 4]
    cache_set.remove(1);    // => [2, 4]

    CHECK(cache_set.get(0).has_value());
    CHECK(cache_set.get(0).value() == 2);
    CHECK(cache_set.get(1).has_value());
    CHECK(cache_set.get(1).value() == 4);
    CHECK(cache_set.get(2) == etl::nullopt);
}

TEST_CASE("complex 2") {
    CacheSet<uint8_t, 3> cache_set{};
    cache_set.push(0);      // => [0]
    cache_set.push(1);      // => [0, 1]
    cache_set.push(2);      // => [0, 1, 2]
    cache_set.push(3);      // => [1, 2, 3]
    cache_set.push(4);      // => [2, 3, 4]
    cache_set.remove(1);    // => [2, 4]
    cache_set.push(5);      // => [2, 4, 5]
    cache_set.push(6);      // => [4, 5, 6]
    cache_set.pop_oldest(); // => [5, 6]
    cache_set.pop_oldest(); // => [6]
    cache_set.push(7);      // => [6, 7]
    cache_set.push(8);      // => [6, 7, 8]

    CHECK(cache_set.get(0).has_value());
    CHECK(cache_set.get(0).value() == 6);
    CHECK(cache_set.get(1).has_value());
    CHECK(cache_set.get(1).value() == 7);
    CHECK(cache_set.get(2).has_value());
    CHECK(cache_set.get(2).value() == 8);
    CHECK(cache_set.get(3) == etl::nullopt);
}
