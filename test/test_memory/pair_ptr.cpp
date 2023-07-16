#include <doctest.h>

#include <etl/utility.h>
#include <memory/pair_ptr.h>

class P1;
class P2;

class P1 : public memory::PairPtr<P1, P2> {};

class P2 : public memory::PairPtr<P2, P1> {};

TEST_CASE("PairPtr") {
    P1 p1{nullptr};
    P2 p2{nullptr};
    p1.unsafe_set_pair(&p2);
    p2.unsafe_set_pair(&p1);
    CHECK_EQ(p1.unsafe_get_pair(), &p2);
    CHECK_EQ(p2.unsafe_get_pair(), &p1);
}

etl::pair<P1, P2> make_pair_ptr() {
    P1 p1{nullptr};
    P2 p2{nullptr};
    p1.unsafe_set_pair(&p2);
    p2.unsafe_set_pair(&p1);
    return {etl::move(p1), etl ::move(p2)};
}

TEST_CASE("move") {
    auto [p1, p2] = make_pair_ptr();
    CHECK_EQ(p1.unsafe_get_pair(), &p2);
    CHECK_EQ(p2.unsafe_get_pair(), &p1);
}

TEST_CASE("move constructor") {
    auto [p1, p2] = make_pair_ptr();
    P1 p3{etl::move(p1)};
    CHECK_FALSE(p1.has_pair());
    CHECK_EQ(p2.unsafe_get_pair(), &p3);
    CHECK_EQ(p3.unsafe_get_pair(), &p2);
}

TEST_CASE("move assign") {
    auto [p1, p2] = make_pair_ptr();
    auto [p3, p4] = make_pair_ptr();
    p1 = etl::move(p3);
    CHECK_EQ(p1.unsafe_get_pair(), &p4);
    CHECK_FALSE(p2.has_pair());
    CHECK_FALSE(p3.has_pair());
    CHECK_EQ(p4.unsafe_get_pair(), &p1);
}

TEST_CASE("unpair") {
    auto [p1, p2] = make_pair_ptr();
    p1.unpair();
    CHECK_FALSE(p1.has_pair());
    CHECK_FALSE(p2.has_pair());
}
