#include <doctest.h>
#include <util/doctest_ext.h>

#include <tl/vec.h>

using namespace tl;

struct UnCopyable {
    int value_;

    UnCopyable() = delete;
    UnCopyable(const UnCopyable &) = delete;
    UnCopyable(UnCopyable &&) = default;
    UnCopyable &operator=(const UnCopyable &) = delete;
    UnCopyable &operator=(UnCopyable &&) = default;

    explicit UnCopyable(int value) : value_{value} {}
};

TEST_CASE("default constructor") {
    Vec<int, 5> v;
    CHECK(v.size() == 0);
}

TEST_CASE("copy constructor") {
    Vec<int, 5> v1;
    v1.push_back(1);
    v1.push_back(2);

    Vec<int, 5> v2{v1};
    CHECK(v2.size() == 2);
    CHECK(v2[0] == 1);
    CHECK(v2[1] == 2);
}

TEST_CASE("move constructor") {
    Vec<UnCopyable, 5> v1;
    v1.emplace_back(1);
    v1.emplace_back(2);

    Vec<UnCopyable, 5> v2{etl::move(v1)};
    CHECK(v1.size() == 0);
    CHECK(v2.size() == 2);
    CHECK(v2[0].value_ == 1);
    CHECK(v2[1].value_ == 2);
}

TEST_CASE("copy assignment") {
    Vec<int, 5> v1;
    v1.push_back(1);
    v1.push_back(2);

    Vec<int, 5> v2;
    v2 = v1;
    CHECK(v2.size() == 2);
    CHECK(v2[0] == 1);
    CHECK(v2[1] == 2);
}

TEST_CASE("move assignment") {
    Vec<UnCopyable, 5> v1;
    v1.emplace_back(1);
    v1.emplace_back(2);

    Vec<UnCopyable, 5> v2;
    v2 = etl::move(v1);
    CHECK(v1.size() == 0);
    CHECK(v2.size() == 2);
    CHECK(v2[0].value_ == 1);
    CHECK(v2[1].value_ == 2);
}

TEST_CASE("push_back") {
    Vec<int, 5> v;
    v.push_back(1);
    v.push_back(2);
    CHECK(v.size() == 2);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
}

TEST_CASE("emplace_back") {
    Vec<int, 5> v;
    v.emplace_back(1);
    v.emplace_back(2);
    CHECK(v.size() == 2);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
}

TEST_CASE("pop_back") {
    int count = 0;
    Vec<DestroyCount, 5> v;
    v.emplace_back(count);
    v.emplace_back(count);
    v.pop_back();
    CHECK(v.size() == 1);
    CHECK(count == 1);
}

TEST_CASE("clear") {
    int count = 0;
    Vec<DestroyCount, 5> v;
    v.emplace_back(count);
    v.emplace_back(count);
    v.clear();

    CHECK(v.size() == 0);
    CHECK(count == 2);
}

TEST_CASE("insert") {
    int count = 0;
    Vec<DestroyCount, 5> v;
    v.emplace_back(count);
    v.emplace_back(count);

    v.insert(1, DestroyCount{count});
    CHECK(v.size() == 3);
    CHECK(count == 0);
}

TEST_CASE("remove") {
    int count = 0;
    Vec<DestroyCount, 5> v;
    v.emplace_back(count);
    v.emplace_back(count);
    v.emplace_back(count);

    v.remove(1);
    CHECK(v.size() == 2);
    CHECK(count == 1);
}

TEST_CASE("swap remove") {
    int count = 0;
    Vec<DestroyCount, 5> v;
    v.emplace_back(count, 1);
    v.emplace_back(count, 2);
    v.emplace_back(count, 3);
    v.emplace_back(count, 4);

    v.swap_remove(1);
    CHECK(v.size() == 3);
    CHECK(count == 1);
    CHECK(v[0].value_ == 1);
    CHECK(v[1].value_ == 4);
}
