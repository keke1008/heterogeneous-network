#include <doctest.h>

#include "lock.hpp"
#include <etl/type_traits.h>

TEST_CASE("new guard") {
    lock::Mutex mu;
    int n;

    CHECK_NOTHROW(lock::Guard(&mu, &n));
}

TEST_CASE("create guard with locked mutex must fail") {
    lock::Mutex mu;
    mu.lock();
    int n;

    CHECK_THROWS(lock::Guard(&mu, &n));
}

TEST_CASE("destructing guard must unlock mutex") {
    lock::Mutex mu;
    int n;

    { lock::Guard guard(&mu, &n); }

    CHECK_FALSE(mu.is_locked());
}

TEST_CASE("deference guard must get reference of internal value") {
    lock::Mutex mu;
    int n;
    lock::Guard guard(&mu, &n);

    CHECK(etl::is_same_v<decltype(*guard), int &>);
}
