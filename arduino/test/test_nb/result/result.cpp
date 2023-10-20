#include <doctest.h>

#include <nb/result.h>
#include <util/visitor.h>

TEST_CASE("Instantiation of Result with Ok") {
    nb::Result<int, int> result{nb::Ok{0}};
    CHECK(result.is_ok());
    CHECK_FALSE(result.is_err());
}

TEST_CASE("Instantiation of Result with Err") {
    nb::Result<int, int> result{nb::Err{0}};
    CHECK_FALSE(result.is_ok());
    CHECK(result.is_err());
}

TEST_CASE("visit") {
    SUBCASE("Result with Ok") {
        nb::Result<int, int> result{nb::Ok{0}};
        bool r = result.visit<bool>(util::Visitor{
            [](nb::Ok<int>) { return true; },
            [](nb::Err<int>) { return false; },
        });
        CHECK(r);
    }

    SUBCASE("Result with Err") {
        nb::Result<int, int> result{nb::Err{0}};
        bool r = result.visit<bool>(util::Visitor{
            [](nb::Ok<int>) { return false; },
            [](nb::Err<int>) { return true; },
        });
        CHECK(r);
    }

    SUBCASE("Infer type") {
        nb::Result<int, int> result{nb::Ok{0}};
        auto r = result.visit<nb::Result<int, int>>(util::Visitor{
            [](nb::Ok<int> ok) { return nb::Err(*ok + 1); },
            [](nb::Err<int>) { return nb::Ok{42}; },
        });
        CHECK(r.is_err());
        CHECK_EQ(r.unwrap_err(), 1);
    }
}

TEST_CASE("bind_ok") {
    SUBCASE("Ok -> Err") {
        nb::Result<int, int> result{nb::Ok{0}};
        nb::Result<int, int> bound = result.bind_ok<int>([](int ok) { return nb::Err(ok + 1); });
        CHECK_FALSE(bound.is_ok());
        CHECK(bound.is_err());
        CHECK_EQ(bound.unwrap_err(), 1);
    }

    SUBCASE("Err -> Err") {
        nb::Result<int, int> result{nb::Err{0}};
        nb::Result<int, int> bound = result.bind_ok([](int err) { return nb::Ok(err + 1); });
        CHECK_FALSE(bound.is_ok());
        CHECK(bound.is_err());
        CHECK_EQ(bound.unwrap_err(), 0);
    }

    SUBCASE("Infer type") {
        nb::Result<int, int> result{nb::Ok{0}};
        nb::Result<double, int> bound =
            result.bind_ok<double>([&](int ok) { return nb::Ok(ok + 1.5); });
        CHECK(bound.is_ok());
        CHECK_EQ(bound.unwrap_ok(), 1.5);
    }
}

TEST_CASE("bind_err") {
    SUBCASE("Ok -> Ok") {
        nb::Result<int, int> result{nb::Ok{0}};
        nb::Result<int, int> bound = result.bind_err<int>([](int ok) { return nb::Ok(ok + 1); });
        CHECK(bound.is_ok());
        CHECK_EQ(bound.unwrap_ok(), 0);
    }

    SUBCASE("Err -> Ok") {
        nb::Result<int, int> result{nb::Err{0}};
        nb::Result<int, int> bound = result.bind_err<int>([](int err) { return nb::Ok(err + 1); });
        CHECK(bound.is_ok());
        CHECK_EQ(bound.unwrap_ok(), 1);
    }

    SUBCASE("Infer type") {
        nb::Result<int, int> result{nb::Err{0}};
        nb::Result<int, double> bound =
            result.bind_err<double>([&](int err) { return nb::Ok(err + 1); });
        CHECK(bound.is_ok());
        CHECK_EQ(bound.unwrap_ok(), 1);
    }
}

TEST_CASE("map_ok") {
    SUBCASE("Ok") {
        nb::Result<int, int> result{nb::Ok{0}};
        nb::Result<int, int> mapped = result.map_ok([](int ok) { return ok + 1; });
        CHECK(mapped.is_ok());
        CHECK_FALSE(mapped.is_err());
        CHECK(mapped.unwrap_ok() == 1);
    }

    SUBCASE("Err") {
        nb::Result<int, int> result{nb::Err{0}};
        nb::Result<int, int> mapped = result.map_ok([](int ok) { return ok + 1; });
        CHECK_FALSE(mapped.is_ok());
        CHECK(mapped.is_err());
        CHECK(mapped.unwrap_err() == 0);
    }

    SUBCASE("Infer type") {
        nb::Result<int, int> result{nb::Ok{0}};
        nb::Result<double, int> mapped = result.map_ok([&](int ok) { return ok + 1.5; });
        CHECK(mapped.is_ok());
        CHECK_EQ(mapped.unwrap_ok(), 1.5);
    }
}

TEST_CASE("map_err") {
    SUBCASE("Ok") {
        nb::Result<int, int> result{nb::Ok{0}};
        nb::Result<int, int> mapped = result.map_err([](int err) { return err + 1; });
        CHECK(mapped.is_ok());
        CHECK_FALSE(mapped.is_err());
        CHECK(mapped.unwrap_ok() == 0);
    }

    SUBCASE("Err") {
        nb::Result<int, int> result{nb::Err{0}};
        nb::Result<int, int> mapped = result.map_err([](int err) { return err + 1; });
        CHECK_FALSE(mapped.is_ok());
        CHECK(mapped.is_err());
        CHECK(mapped.unwrap_err() == 1);
    }

    SUBCASE("Infer type") {
        nb::Result<int, int> result{nb::Err{0}};
        nb::Result<int, double> mapped = result.map_err([&](int err) { return err + 1.5; });
        CHECK(mapped.is_err());
        CHECK_EQ(mapped.unwrap_err(), 1.5);
    }
}
