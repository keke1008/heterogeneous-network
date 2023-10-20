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

TEST_CASE("Instantiation Poll<void>") {
    SUBCASE("Pending") {
        nb::Poll<void> poll{nb::Pending{}};
        CHECK_EQ(poll, nb::Poll<void>{nb::Pending{}});
        CHECK(poll.is_pending());
        CHECK_FALSE(poll.is_ready());
    }

    SUBCASE("Ready") {
        nb::Poll<void> poll{nb::Ready{}};
        CHECK_EQ(poll, nb::Poll<void>{nb::Ready{}});
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

TEST_CASE("POLL_UNWRAP_OR_RETURN") {
    SUBCASE("Ready") {
        auto f = []() {
            const nb::Poll<int> poll{nb::Ready{42}};
            int n = POLL_UNWRAP_OR_RETURN(poll);
            throw "CORRECT";
            return nb::pending;
        };

        CHECK_THROWS_WITH_AS(f(), "CORRECT", const char *);
    }

    SUBCASE("Pending") {
        auto f = []() {
            nb::Poll<int> poll{nb::Pending{}};
            int n = POLL_UNWRAP_OR_RETURN(poll);
            throw "INCORRECT";
            return nb::pending;
        };

        CHECK_NOTHROW(f());
    }

    SUBCASE("Ready<void>") {
        auto f = []() {
            nb::Poll<void> poll{nb::Ready{}};
            POLL_UNWRAP_OR_RETURN(poll);
            throw "CORRECT";
            return nb::pending;
        };

        CHECK_THROWS_WITH_AS(f(), "CORRECT", const char *);
    }

    SUBCASE("Pending void") {
        auto f = []() {
            nb::Poll<void> poll{nb::Pending{}};
            POLL_UNWRAP_OR_RETURN(poll);
            throw "INCORRECT";
            return nb::pending;
        };

        CHECK_NOTHROW(f());
    }
}
