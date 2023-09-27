#include <doctest.h>

#include <nb/time.h>

TEST_CASE("Delay") {
    util::MockTime time{0};
    nb::Delay delay{time, util::Duration::from_millis(10)};

    CHECK(delay.poll(time).is_pending());

    time.set_now_ms(5);
    CHECK(delay.poll(time).is_pending());

    time.set_now_ms(10);
    CHECK(delay.poll(time).is_ready());
}
