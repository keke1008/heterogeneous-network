#include <doctest.h>

#include <nb/lock/mutex.h>

TEST_CASE("Initial mutex is not locked") {
    nb::lock::Mutex m;
    CHECK(!m.is_locked());
}

TEST_CASE("Locking an unlocked mutex succeeds") {
    nb::lock::Mutex m;
    CHECK(m.lock());
    CHECK(m.is_locked());
}

TEST_CASE("Locking a locked mutex fails") {
    nb::lock::Mutex m;
    m.lock();
    CHECK(!m.lock());
    CHECK(m.is_locked());
}

TEST_CASE("Unlocking a locked mutex succeeds") {
    nb::lock::Mutex m;
    m.lock();
    m.unlock();
    CHECK(!m.is_locked());
}

TEST_CASE("Locking twice fails") {
    nb::lock::Mutex m;
    m.lock();
    CHECK(!m.lock());
    CHECK(m.is_locked());
}
