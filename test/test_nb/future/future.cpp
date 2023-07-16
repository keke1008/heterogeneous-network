#include <doctest.h>

#include <nb/future.h>

TEST_CASE("empty future") {
    auto [future, promise] = nb::make_future_promise_pair<uint8_t>();
    CHECK_EQ(future.get(), nb::pending);
}

TEST_CASE("set value") {
    auto [future, promise] = nb::make_future_promise_pair<uint8_t>();
    promise.set_value(42);

    auto value = future.get();
    CHECK(value.is_ready());
    CHECK_EQ(value.unwrap(), 42);
}

TEST_CASE("move future") {
    auto [future, promise] = nb::make_future_promise_pair<uint8_t>();
    promise.set_value(42);

    auto future2 = etl::move(future);
    CHECK(future.is_closed());

    auto value = future2.get();
    CHECK(value.is_ready());
    CHECK_EQ(value.unwrap(), 42);
}

TEST_CASE("move promise") {
    auto [future, promise] = nb::make_future_promise_pair<uint8_t>();

    auto promise2 = etl::move(promise);
    CHECK(promise.is_closed());

    promise2.set_value(42);
    auto value = future.get();
    CHECK(value.is_ready());
    CHECK_EQ(value.unwrap(), 42);
}

TEST_CASE("destroy future") {
    auto [future, promise] = nb::make_future_promise_pair<uint8_t>();
    { auto future2 = etl::move(future); }
    CHECK(promise.is_closed());
    promise.set_value(42);
}

TEST_CASE("destroy promise") {
    auto [future, promise] = nb::make_future_promise_pair<uint8_t>();
    { auto promise2 = etl::move(promise); }
    CHECK(future.is_closed());
}
