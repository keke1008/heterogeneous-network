#include <doctest.h>

#include <nb/poll.h>

TEST_CASE("Instantiation") {
    SUBCASE("Pending") {
        nb::Poll<int> poll{nb::Pending{}};
        CHECK_EQ(poll, nb::Poll<int>{nb::Pending{}});
        CHECK(poll.is_pending());
        CHECK_FALSE(poll.is_ready());
    }

    SUBCASE("Ready") {
        nb::Poll<int> poll{nb::Ready{42}};
        CHECK_EQ(poll, nb::Poll<int>{nb::Ready{42}});
        CHECK_FALSE(poll.is_pending());
        CHECK(poll.is_ready());
    }
}

TEST_CASE("unwrap") {
    SUBCASE("Ready") {
        nb::Poll<int> poll{nb::Ready{42}};
        CHECK_EQ(poll.unwrap(), 42);
    }
}

TEST_CASE("unwrap_or") {
    SUBCASE("Ready") {
        nb::Poll<int> poll{nb::Ready{42}};
        CHECK_EQ(poll.unwrap_or(0), 42);
    }

    SUBCASE("Pending") {
        nb::Poll<int> poll{nb::Pending{}};
        CHECK_EQ(poll.unwrap_or(0), 0);
    }
}

TEST_CASE("unwrap_or_default") {
    SUBCASE("Ready") {
        nb::Poll<int> poll{nb::Ready{42}};
        CHECK_EQ(poll.unwrap_or_default(), 42);
    }

    SUBCASE("Pending") {
        nb::Poll<int> poll{nb::Pending{}};
        CHECK_EQ(poll.unwrap_or_default(), 0);
    }
}

TEST_CASE("map") {
    SUBCASE("Ready") {
        nb::Poll<int> poll{nb::Ready{42}};
        auto mapped = poll.map([](int v) { return v * 2; });
        CHECK_EQ(mapped, nb::Poll<int>{nb::Ready{84}});
    }

    SUBCASE("Pending") {
        nb::Poll<int> poll{nb::Pending{}};
        auto mapped = poll.map([](int v) { return v * 2; });
        CHECK_EQ(mapped, nb::Poll<int>{nb::Pending{}});
    }
}

TEST_CASE("bind_ready") {
    SUBCASE("Ready") {
        nb::Poll<int> poll{nb::Ready{42}};
        auto mapped = poll.bind_ready([](int v) { return nb::Poll<int>{nb::Ready{v * 2}}; });
        CHECK_EQ(mapped, nb::Poll<int>{nb::Ready{84}});
    }

    SUBCASE("Pending") {
        nb::Poll<int> poll{nb::Pending{}};
        auto mapped = poll.bind_ready([](int v) { return nb::Poll<int>{nb::Ready{v * 2}}; });
        CHECK_EQ(mapped, nb::Poll<int>{nb::Pending{}});
    }
}

TEST_CASE("bind_pending") {
    SUBCASE("Ready") {
        nb::Poll<int> poll{nb::Ready{42}};
        auto mapped = poll.bind_pending([]() { return nb::Poll<int>{nb::Pending{}}; });
        CHECK_EQ(mapped, nb::Poll<int>{nb::Ready{42}});
    }

    SUBCASE("Pending") {
        nb::Poll<int> poll{nb::Pending{}};
        auto mapped = poll.bind_pending([]() { return nb::Poll<int>{nb::Ready{42}}; });
        CHECK_EQ(mapped, nb::Poll<int>{nb::Ready{42}});
    }
}
