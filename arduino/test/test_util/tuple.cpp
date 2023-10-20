#include <doctest.h>

#include <util/tuple.h>

TEST_CASE("Tuple") {
    util::Tuple<int, float, double> t{1, 2.0f, 3.0};
    CHECK_EQ(util::get<0>(t), 1);
    CHECK_EQ(util::get<1>(t), 2.0f);
    CHECK_EQ(util::get<2>(t), 3.0);
    util::get<0>(t) = 4;
    CHECK_EQ(util::get<0>(t), 4);
    CHECK_EQ(util::get<1>(t), 2.0f);
    CHECK_EQ(util::get<2>(t), 3.0);
    util::get<1>(t) = 5.0f;
    CHECK_EQ(util::get<0>(t), 4);
    CHECK_EQ(util::get<1>(t), 5.0f);
    CHECK_EQ(util::get<2>(t), 3.0);
    util::get<2>(t) = 6.0;
    CHECK_EQ(util::get<0>(t), 4);
    CHECK_EQ(util::get<1>(t), 5.0f);
    CHECK_EQ(util::get<2>(t), 6.0);
}

TEST_CASE("move") {
    class UnCopyable {
        UnCopyable(const UnCopyable &) = delete;
        UnCopyable &operator=(const UnCopyable &) = delete;

      public:
        UnCopyable() = default;
        UnCopyable(UnCopyable &&) = default;
        UnCopyable &operator=(UnCopyable &&) = default;
    };

    util::Tuple<UnCopyable, UnCopyable, UnCopyable> t{};
    auto t2 = std::move(t);
}

TEST_CASE("last") {
    util::Tuple<int, float, double> t{1, 2.0f, 3.0};
    CHECK_EQ(t.last(), 3.0);
    t.last() = 4.0;
    CHECK_EQ(t.last(), 4.0);
}

TEST_CASE("structured bindings") {
    util::Tuple<int, float, double> t{1, 2.0f, 3.0};
    auto &[i, f, d] = t;
    CHECK_EQ(i, 1);
    CHECK_EQ(f, 2.0f);
    CHECK_EQ(d, 3.0);
    i = 4;
    CHECK_EQ(i, 4);
    CHECK_EQ(f, 2.0f);
    CHECK_EQ(d, 3.0);
    f = 5.0f;
    CHECK_EQ(i, 4);
    CHECK_EQ(f, 5.0f);
    CHECK_EQ(d, 3.0);
    d = 6.0;
    CHECK_EQ(i, 4);
    CHECK_EQ(f, 5.0f);
    CHECK_EQ(d, 6.0);
}

TEST_CASE("structure bindings const") {
    const util::Tuple<int, float, double> t{1, 2.0f, 3.0};
    auto &[i, f, d] = t;
    CHECK_EQ(i, 1);
    CHECK_EQ(f, 2.0f);
    CHECK_EQ(d, 3.0);
}

TEST_CASE("apply") {
    util::Tuple<int, float, double> t{1, 2.0f, 3.0};
    auto f = [](auto &i, auto &f, auto &d) {
        i = 4;
        f = 5.0f;
        d = 6.0;
    };
    util::apply(f, t);
    CHECK_EQ(util::get<0>(t), 4);
    CHECK_EQ(util::get<1>(t), 5.0f);
    CHECK_EQ(util::get<2>(t), 6.0);
}
