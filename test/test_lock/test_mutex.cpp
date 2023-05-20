#include <doctest.h>

#include <lock.hpp>

TEST_CASE("new mutex must be unlockable") {
    lock::Mutex mu;
    CHECK_FALSE(mu.is_locked());
}

TEST_CASE("lock mutex") {
    lock::Mutex mu;
    CHECK(mu.lock());
    CHECK(mu.is_locked());
}

TEST_CASE("locked mutex must be unlockable") {
    lock::Mutex mu;
    mu.lock();
    CHECK_FALSE(mu.lock());
}

TEST_CASE("unlock mutex") {
    lock::Mutex mu;
    mu.lock();
    mu.unlock();
    CHECK_FALSE(mu.is_locked());
}

TEST_CASE("unlocked mutex must be lockable") {
    lock::Mutex mu;
    mu.lock();
    mu.unlock();
    CHECK(mu.lock());
}
