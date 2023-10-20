#include <doctest.h>

#include <nb/lock.h>

TEST_CASE("Instantiate lock") {
    nb::Lock lock(0);
}

TEST_CASE("Lock and unlock") {
    nb::Lock lock(0);
    auto guard = lock.lock();
    CHECK(guard.has_value());
}

TEST_CASE("Lock and unlock twice") {
    nb::Lock lock(0);
    auto guard = lock.lock();
    CHECK(guard.has_value());
    guard = lock.lock();
    CHECK_FALSE(guard.has_value());
}

TEST_CASE("Drop lock") {
    nb::Lock lock(0);
    {
        auto guard = lock.lock();
        CHECK(guard.has_value());
    }
    auto guard = lock.lock();
    CHECK(guard.has_value());
}
