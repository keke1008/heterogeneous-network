#include <doctest.h>

#include <memory/track_ptr.h>

TEST_CASE("Instantiate TrackPtr") {
    auto [t, u] = memory::make_track_ptr(1, 2);
    CHECK_EQ(t.get(), 1);
    CHECK_EQ(u.get(), 2);
    CHECK_EQ(t.try_get_pair()->get(), 2);
    CHECK_EQ(u.try_get_pair()->get(), 1);
}

TEST_CASE("Drop TrackPtr") {
    auto [t, u] = memory::make_track_ptr(1, 2);
    {
        auto [t2, u2] = memory::make_track_ptr(3, 4);
        t = etl::move(t2);
        CHECK_EQ(t.try_get_pair()->get(), 4);
        CHECK_FALSE(u.try_get_pair().has_value());
    }
    CHECK_FALSE(t.try_get_pair().has_value());
    CHECK_FALSE(u.try_get_pair().has_value());
}

TEST_CASE("Drop TrackPtr partial") {
    auto [t, u] = memory::make_track_ptr(1, 2);
    {
        auto [t2, u2] = memory::make_track_ptr(3, 4);
        t = etl::move(t2);
        CHECK_EQ(t.try_get_pair()->get(), 4);
        CHECK_FALSE(u.try_get_pair().has_value());
        CHECK_EQ(u2.try_get_pair()->get(), 3);
    }
    CHECK_FALSE(t.try_get_pair().has_value());
    CHECK_FALSE(u.try_get_pair().has_value());
}

TEST_CASE("Move TrackPtr") {
    auto [t, u] = memory::make_track_ptr(1, 2);
    auto [t2, u2] = memory::make_track_ptr(3, 4);
    t = etl::move(t2);
    u = etl::move(u2);
    CHECK_EQ(t.get(), 3);
    CHECK_EQ(u.get(), 4);
    CHECK_EQ(t.try_get_pair()->get(), 4);
    CHECK_EQ(u.try_get_pair()->get(), 3);
}

TEST_CASE("Move TrackPtr partial") {
    auto [t, u] = memory::make_track_ptr(1, 2);
    auto [t2, u2] = memory::make_track_ptr(3, 4);
    t = etl::move(t2);
    CHECK_EQ(t.try_get_pair()->get(), 4);
    CHECK_FALSE(u.try_get_pair().has_value());
    CHECK_EQ(u2.try_get_pair()->get(), 3);
}

TEST_CASE("Move TrackPtr to self") {
    auto [t, u] = memory::make_track_ptr(1, 2);
    t = etl::move(t);
    CHECK_EQ(t.get(), 1);
    CHECK_EQ(u.get(), 2);
    CHECK_EQ(t.try_get_pair()->get(), 2);
    CHECK_EQ(u.try_get_pair()->get(), 1);
}

TEST_CASE("Move TrackPtr to self partial") {
    auto [t, u] = memory::make_track_ptr(1, 2);
    auto [t2, u2] = memory::make_track_ptr(3, 4);
    t = etl::move(t);
    CHECK_EQ(t.get(), 1);
    CHECK_EQ(u.get(), 2);
    CHECK_EQ(t.try_get_pair()->get(), 2);
    CHECK_EQ(u.try_get_pair()->get(), 1);
    t = etl::move(t2);
    CHECK_EQ(t.try_get_pair()->get(), 4);
    CHECK_FALSE(u.try_get_pair().has_value());
    CHECK_EQ(u2.try_get_pair()->get(), 3);
}
