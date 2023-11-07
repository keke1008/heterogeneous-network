#include <doctest.h>
#include <tl/variant.h>
#include <variant>

struct UnCopyable {
    int value;

    explicit UnCopyable(int value) : value{value} {}

    UnCopyable(const UnCopyable &) = delete;

    UnCopyable(UnCopyable &&other) {
        value = other.value;
        other.value = -1;
    }

    UnCopyable &operator=(const UnCopyable &) = delete;

    UnCopyable &operator=(UnCopyable &&other) {
        value = other.value;
        other.value = -1;
        return *this;
    }
};

TEST_CASE("instantiation") {
    tl::Variant<int, float> v{1};
    CHECK(v.index() == 0);
    CHECK(v.get<int>() == 1);

    v = 2.0f;
    CHECK(v.index() == 1);
    CHECK(v.get<float>() == 2.0f);
}

TEST_CASE("default constructor") {
    tl::Variant<tl::Monostate, int> v;
    CHECK(v.index() == 0);
}

TEST_CASE("holds_alternative") {
    tl::Variant<int, float> v{1};
    CHECK(v.holds_alternative<int>());
    CHECK(!v.holds_alternative<float>());

    v = 2.0f;
    CHECK(!v.holds_alternative<int>());
    CHECK(v.holds_alternative<float>());
}

TEST_CASE("get") {
    tl::Variant<int, float> v{1};
    CHECK(v.get<int>() == 1);

    v = 2.0f;
    CHECK(v.get<float>() == 2.0f);
}

TEST_CASE("copy") {
    tl::Variant<int, float> v{1};
    CHECK(v.get<int>() == 1);

    auto v2 = v;
    CHECK(v2.get<int>() == 1);

    v = 2.0f;
    CHECK(v.get<float>() == 2.0f);
    CHECK(v2.get<int>() == 1);
}

TEST_CASE("move") {
    tl::Variant<UnCopyable, int> v1{UnCopyable{1}};
    CHECK(v1.get<UnCopyable>().value == 1);

    auto v2 = etl::move(v1);
    CHECK(v1.get<UnCopyable>().value == -1);
    CHECK(v2.get<UnCopyable>().value == 1);

    int x = 2;
    v1 = x;
    CHECK(v1.get<int>() == 2);
    CHECK(v2.get<UnCopyable>().value == 1);
}

TEST_CASE("uncopyable") {
    tl::Variant<UnCopyable> v1{UnCopyable{1}};
    CHECK(v1.get<UnCopyable>().value == 1);

    auto v2 = etl::move(v1);
    CHECK(v1.get<UnCopyable>().value == -1);
    CHECK(v2.get<UnCopyable>().value == 1);

    v1 = UnCopyable{2};
    CHECK(v1.get<UnCopyable>().value == 2);
    CHECK(v2.get<UnCopyable>().value == 1);
}

struct A {
    int &ref;
    UnCopyable uncopyable;

    ~A() {
        ref++;
    }
};

TEST_CASE("emplace") {
    tl::Variant<int, A> v{1};
    CHECK(v.get<int>() == 1);

    int counter = 0;
    v.emplace<A>(counter, UnCopyable{2});
    CHECK(v.get<A>().ref == 0);
    CHECK(v.get<A>().uncopyable.value == 2);

    v.emplace<A>(counter, UnCopyable{3});
    CHECK(counter == 1);
    CHECK(v.get<A>().uncopyable.value == 3);

    v.emplace<int>(4);
    CHECK(counter == 2);
    CHECK(v.get<int>() == 4);
}

TEST_CASE("visit method") {
    tl::Variant<int, UnCopyable> v1{UnCopyable{2}};
    CHECK(v1.visit([](int x) { return x; }, [](UnCopyable &u) { return u.value; }) == 2);

    v1 = 1;
    CHECK(v1.visit([](int x) { return x; }, [](UnCopyable &u) { return u.value; }) == 1);

    tl::Variant<int, float> v2{1};
    CHECK(v2.visit([](auto x) { return x * 2; }) == 2);
    CHECK(v2.visit<double>([](auto x) { return x * 2; }) == 2.0);
}

TEST_CASE("visit function") {
    tl::Variant<int, UnCopyable> v1{UnCopyable{1}};
    CHECK(
        tl::visit(
            util::Visitor{[](int x) { return x; }, [](UnCopyable &u) { return u.value; }}, v1
        ) == 1
    );
}

TEST_CASE("get") {
    tl::Variant<int, UnCopyable> v1{1};
    CHECK(tl::get<int>(v1) == 1);
    CHECK_THROWS(tl::get<UnCopyable>(v1));

    v1 = UnCopyable{2};
    CHECK(tl::get<UnCopyable>(v1).value == 2);
    CHECK_THROWS(tl::get<int>(v1));
}
