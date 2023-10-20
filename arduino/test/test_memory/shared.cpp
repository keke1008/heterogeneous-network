#include <doctest.h>
#include <memory/shared.h>

TEST_CASE("Instantiation") {
    memory::Shared<int> shared(1);
    CHECK_EQ(shared.ref_count(), 1);
    CHECK(shared.is_unique());
}

TEST_CASE("Copy constructor") {
    memory::Shared<int> shared(1);
    memory::Shared<int> shared2(shared);
    CHECK_EQ(shared.ref_count(), 2);
    CHECK_EQ(shared2.ref_count(), 2);
    CHECK_FALSE(shared.is_unique());
    CHECK_FALSE(shared2.is_unique());
}

TEST_CASE("Copy assignment") {
    memory::Shared<int> shared(1);
    memory::Shared<int> shared2(2);
    shared2 = shared;
    CHECK_EQ(shared.ref_count(), 2);
    CHECK_EQ(shared2.ref_count(), 2);
    CHECK_FALSE(shared.is_unique());
    CHECK_FALSE(shared2.is_unique());
}
