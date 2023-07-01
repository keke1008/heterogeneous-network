#include <doctest.h>

#include <nb/lock.h>

TEST_CASE("Instantiate lock") {
    nb::lock::Lock lock(0);
}

TEST_CASE("Lock and unlock") {
    nb::lock::Lock lock(0);
    auto guard = lock.lock();
    CHECK(guard.has_value());
}

TEST_CASE("Lock and unlock twice") {
    nb::lock::Lock lock(0);
    auto guard = lock.lock();
    CHECK(guard.has_value());
    guard = lock.lock();
    CHECK_FALSE(guard.has_value());
}

TEST_CASE("Drop lock") {
    nb::lock::Lock lock(0);
    {
        auto guard = lock.lock();
        CHECK(guard.has_value());
    }
    auto guard = lock.lock();
    CHECK(guard.has_value());
}
