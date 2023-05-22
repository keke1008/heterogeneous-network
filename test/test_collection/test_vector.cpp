#include <doctest.h>

#include <collection/vector.hpp>
#include <etl/optional.h>

TEST_CASE("create emtpy Vector") {
    auto vec = collection::Vector<int>();
    CHECK(vec.is_empty());
    CHECK_EQ(vec.size(), 0);
    CHECK_EQ(vec.capacity(), 0);
}

TEST_CASE("create Vector with capacity") {
    auto vec = collection::Vector<int>(42);
    CHECK(vec.is_empty());
    CHECK_EQ(vec.size(), 0);
    CHECK_EQ(vec.capacity(), 42);
}

TEST_CASE("create not empty Vector") {
    auto vec = collection::Vector<int>();
    vec.push_back(42);
    CHECK(!vec.is_empty());
}

TEST_CASE("shrink") {
    auto vec = collection::Vector<int>(42);
    vec.push_back(42);
    vec.shrink_to_fit();
    CHECK_EQ(vec.capacity(), 1);
}

TEST_CASE("push element") {
    auto vec = collection::Vector<int>();
    vec.push_back(42);

    CHECK_EQ(vec.at(0), 42);
    CHECK_EQ(vec.size(), 1);
    CHECK_GE(vec.capacity(), 1);
}

TEST_CASE("insert element without allocation") {
    auto vec = collection::Vector<int>(3);
    vec.push_back(42);
    vec.push_back(53);
    vec.insert(1, 64);

    CHECK_EQ(vec.at(1), 64);
    CHECK_EQ(vec.at(2), 53);
    CHECK_EQ(vec.size(), 3);
    CHECK_GE(vec.capacity(), 3);
}

TEST_CASE("insert element with allocation") {
    auto vec = collection::Vector<int>();
    vec.push_back(42);
    vec.push_back(53);
    vec.insert(1, 64);

    CHECK_EQ(vec.at(1), 64);
    CHECK_EQ(vec.at(2), 53);
    CHECK_EQ(vec.size(), 3);
    CHECK_GE(vec.capacity(), 3);
}

TEST_CASE("insert element at last of range") {
    auto vec = collection::Vector<int>();
    vec.push_back(42);
    vec.push_back(53);
    vec.insert(2, 64);

    CHECK_EQ(vec.at(1), 53);
    CHECK_EQ(vec.at(2), 64);
    CHECK_EQ(vec.size(), 3);
    CHECK_GE(vec.capacity(), 3);
}

TEST_CASE("insert element at out of range") {
    auto vec = collection::Vector<int>();
    vec.push_back(42);
    vec.push_back(53);
    CHECK_THROWS(vec.insert(3, 64));
}

collection::Vector<int> create_vector() {
    auto vec = collection::Vector<int>();
    vec.push_back(42);
    vec.push_back(53);
    vec.push_back(64);
    return vec;
}

TEST_CASE("pop element") {
    auto vec = create_vector();
    CHECK_EQ(vec.pop_back(), etl::optional(64));
    CHECK_EQ(vec.pop_back(), etl::optional(53));
    CHECK_EQ(vec.pop_back(), etl::optional(42));
    CHECK_EQ(vec.pop_back(), etl::nullopt);
}

TEST_CASE("erase element") {
    auto vec = create_vector();
    CHECK_EQ(vec.erase(2), 64);
    CHECK_EQ(vec.size(), 2);
}

TEST_CASE("erase element at out of range") {
    auto vec = create_vector();
    CHECK_THROWS(vec.erase(3));
}

TEST_CASE("index access at last of range") {
    auto vec = create_vector();
    CHECK_EQ(vec[2], 64);
}

TEST_CASE("index access at out of range") {
    auto vec = create_vector();
    CHECK_THROWS(vec[3]);
}

TEST_CASE("at access at last of range") {
    auto vec = create_vector();
    CHECK_EQ(vec.at(2), 64);
}

TEST_CASE("at access at out of range") {
    auto vec = create_vector();
    CHECK_EQ(vec.at(3), etl::nullopt);
}

TEST_CASE("get front element") {
    auto vec = create_vector();
    CHECK_EQ(vec.front(), 42);
}

TEST_CASE("get front element of empty vector") {
    auto vec = collection::Vector<int>();
    CHECK_EQ(vec.front(), etl::nullopt);
}

TEST_CASE("get last element") {
    auto vec = create_vector();
    CHECK_EQ(vec.back(), 64);
}

TEST_CASE("get back element of empty vector") {
    auto vec = collection::Vector<int>();
    CHECK_EQ(vec.back(), etl::nullopt);
}
