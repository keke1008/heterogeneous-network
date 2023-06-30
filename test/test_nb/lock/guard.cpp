#include <doctest.h>

#include <etl/utility.h>
#include <nb/lock/guard.h>

int value = 0;

TEST_CASE("Instantiate guard") {
    nb::lock::Mutex mutex;
    mutex.lock();
    nb::lock::Guard<int> guard{&mutex, &value};
    CHECK(guard.operator->() == &value);
    CHECK(guard.operator*() == 0);
}

TEST_CASE("Move construct guard") {
    nb::lock::Mutex mutex;
    mutex.lock();
    nb::lock::Guard<int> guard{&mutex, &value};
    nb::lock::Guard<int> guard2{etl::move(guard)};
    CHECK(mutex.is_locked());
}

TEST_CASE("Destroy guard") {
    nb::lock::Mutex mutex;
    mutex.lock();
    {
        nb::lock::Guard<int> guard{&mutex, &value};
        CHECK(mutex.is_locked());
    }
    CHECK_FALSE(mutex.is_locked());
}

TEST_CASE("Move assign guard") {
    nb::lock::Mutex mutex;
    mutex.lock();
    nb::lock::Guard<int> guard{&mutex, &value};
    nb::lock::Guard<int> guard2{&mutex, &value};
    guard2 = etl::move(guard);
    CHECK(mutex.is_locked());
}

TEST_CASE("Move assign two mutexes") {
    nb::lock::Mutex mutex;
    mutex.lock();
    nb::lock::Mutex mutex2;
    mutex2.lock();
    nb::lock::Guard<int> guard{&mutex, &value};
    nb::lock::Guard<int> guard2{&mutex2, &value};
    guard2 = etl::move(guard);
    CHECK(mutex.is_locked());
    CHECK_FALSE(mutex2.is_locked());
}

TEST_CASE("Move assign two mutexes, then destroy") {
    nb::lock::Mutex mutex;
    mutex.lock();
    nb::lock::Mutex mutex2;
    mutex2.lock();
    {
        nb::lock::Guard<int> guard{&mutex, &value};
        nb::lock::Guard<int> guard2{&mutex2, &value};
        guard2 = etl::move(guard);
        CHECK(mutex.is_locked());
        CHECK_FALSE(mutex2.is_locked());
    }
    CHECK_FALSE(mutex.is_locked());
    CHECK_FALSE(mutex2.is_locked());
}
