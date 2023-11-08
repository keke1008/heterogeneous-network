#include <doctest.h>
#include <util/doctest_ext.h>

#include <tl/optional.h>

TEST_CASE("constructor") {
    tl::Optional<int> opt1;
    tl::Optional<int> opt2{tl::nullopt};
    tl::Optional<int> opt3{1};
}

TEST_CASE("has_value") {
    tl::Optional<int> opt1;
    tl::Optional<int> opt2{tl::nullopt};
    tl::Optional<int> opt3{1};

    CHECK(opt1.has_value() == false);
    CHECK(opt2.has_value() == false);
    CHECK(opt3.has_value() == true);
}

TEST_CASE("value") {
    tl::Optional<int> opt1;
    tl::Optional<int> opt2{tl::nullopt};
    tl::Optional<int> opt3{1};

    CHECK_THROWS(opt1.value());
    CHECK_THROWS(opt2.value());
    CHECK(opt3.value() == 1);
}

TEST_CASE("copy constructor") {
    tl::Optional<int> opt1{1};
    tl::Optional<int> opt2{opt1};

    CHECK(opt1.value() == 1);
    CHECK(opt2.value() == 1);
}

TEST_CASE("move constructor") {
    tl::Optional<MoveOnly> opt1{MoveOnly{1}};
    tl::Optional<MoveOnly> opt2{etl::move(opt1)};

    CHECK(opt1.has_value() == false);
    CHECK(opt2.value().value == 1);
}

TEST_CASE("copy assignment") {
    tl::Optional<int> opt1{1};
    tl::Optional<int> opt2{2};
    opt2 = opt1;

    CHECK(opt1.value() == 1);
    CHECK(opt2.value() == 1);

    opt2 = tl::nullopt;
    CHECK(opt2.has_value() == false);
}

TEST_CASE("move assignment") {
    tl::Optional<MoveOnly> opt1{MoveOnly{1}};
    tl::Optional<MoveOnly> opt2{MoveOnly{2}};
    opt2 = etl::move(opt1);

    CHECK(opt1.has_value() == false);
    CHECK(opt2.value().value == 1);
}

TEST_CASE("destructor") {
    int count = 0;
    tl::Optional<DestroyCount> opt1{count};
    opt1 = tl::nullopt;

    CHECK(count == 1);
}

TEST_CASE("reset") {
    int count = 0;
    tl::Optional<DestroyCount> opt1{count};
    opt1.reset();

    CHECK(opt1.has_value() == false);
    CHECK(count == 1);
}

struct ManyParameter {
    int a;
    double b;
    bool c;

    explicit ManyParameter(int a, double b, bool c) : a{a}, b{b}, c{c} {}

    bool operator==(const ManyParameter &other) const {
        return a == other.a && b == other.b && c == other.c;
    }
};

TEST_CASE("emplace") {
    tl::Optional<ManyParameter> opt1;
    opt1.emplace(1, 0.0, false);
    CHECK(opt1.value() == ManyParameter{1, 0.0, false});
}

TEST_CASE("operator*") {
    tl::Optional<int> opt1{1};
    CHECK(*opt1 == 1);
}

TEST_CASE("operator->") {
    tl::Optional<ManyParameter> opt1{1, 0.0, false};
    CHECK(opt1->a == 1);
    CHECK(opt1->b == 0.0);
    CHECK(opt1->c == false);
}

TEST_CASE("value_or") {
    tl::Optional<int> opt1{1};
    tl::Optional<int> opt2{tl::nullopt};

    CHECK(opt1.value_or(2) == 1);
    CHECK(opt2.value_or(2) == 2);
}

TEST_CASE("operator==") {
    tl::Optional<int> opt1{1};
    tl::Optional<int> opt2{1};
    tl::Optional<int> opt3{2};
    tl::Optional<int> opt4{tl::nullopt};

    CHECK(opt1 == opt2);
    CHECK(opt1 != opt3);
    CHECK(opt1 != opt4);
    CHECK(opt2 != opt3);
    CHECK(opt2 != opt4);
    CHECK(opt3 != opt4);
}

TEST_CASE("operator<") {
    tl::Optional<int> opt1{1};
    tl::Optional<int> opt2{1};
    tl::Optional<int> opt3{2};
    tl::Optional<int> opt4{tl::nullopt};

    CHECK(!(opt1 < opt2));
    CHECK(opt1 < opt3);
    CHECK(!(opt1 < opt4));
    CHECK(opt2 < opt3);
    CHECK(!(opt2 < opt4));
    CHECK(!(opt3 < opt4));
}
