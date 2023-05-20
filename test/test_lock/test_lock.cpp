#include <doctest.h>

#include "lock.hpp"

TEST_CASE("new lock must be unlocked") {
    lock::Lock<int> l(42);
    CHECK_FALSE(l.is_locked());
}

TEST_CASE("new lock must be lockable") {
    lock::Lock<int> l(42);
    auto g = l.lock();
    CHECK(l.is_locked());
    CHECK(g.has_value());
}

TEST_CASE("locked lock must be non-lockable") {
    lock::Lock<int> l(42);
    auto g = l.lock();
    auto h = l.lock();
    CHECK(g.has_value());
    CHECK_FALSE(h.has_value());
}

TEST_CASE("destruction of guard makes lock lockable") {
    lock::Lock<int> l(42);
    { auto g = l.lock(); }
    CHECK_FALSE(l.is_locked());
}
