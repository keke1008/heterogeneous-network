#include <doctest.h>

#include <net/routing/link_state/resolver.h>

using namespace net::routing::resolver;

TEST_CASE("simple 2 vertexes graph") {
    // A(0) -- 10 -- B(0)

    constexpr uint8_t size = 2;
    uint8_t A = 0;
    uint8_t B = 1;

    net::routing::resolver::AdjacencyCost<size> cost;
    cost.set_vertex(A, Cost{0});
    cost.set_vertex(B, Cost{0});
    cost.set(A, B, Cost(10));
    etl::array<bool, size> valid_vertex{true, true};

    auto result = resolve_gateway_vertex<size>(cost, valid_vertex, A, B);
    CHECK(result.has_value());
    CHECK(result.value() == B);
}

TEST_CASE("bypass") {
    // A(0) -------- 10 -------- B(0)
    // \                          /
    //  \--- 4 --- C(0) --- 4 ---/

    constexpr uint8_t size = 3;
    uint8_t A = 0;
    uint8_t B = 1;
    uint8_t C = 2;

    AdjacencyCost<size> cost;
    cost.set_vertex(A, Cost{0});
    cost.set_vertex(B, Cost{0});
    cost.set_vertex(C, Cost{0});
    cost.set(A, B, Cost(10));
    cost.set(A, C, Cost(4));
    cost.set(C, B, Cost(4));
    etl::array<bool, size> valid_vertex{true, true, true};

    auto result = resolve_gateway_vertex<size>(cost, valid_vertex, A, B);
    CHECK(result.has_value());
    CHECK(result.value() == C);
}

TEST_CASE("no route") {
    // A(0) -- 10 -- B(0)
    //
    // C(0)

    constexpr uint8_t size = 3;
    uint8_t A = 0;
    uint8_t B = 1;
    uint8_t C = 2;

    AdjacencyCost<size> cost;
    cost.set_vertex(A, Cost{0});
    cost.set_vertex(B, Cost{0});
    cost.set_vertex(C, Cost{0});
    cost.set(A, B, Cost(10));
    etl::array<bool, size> valid_vertex{true, true, true};

    auto result = resolve_gateway_vertex<size>(cost, valid_vertex, A, C);
    CHECK(!result.has_value());
}

TEST_CASE("contains invalid node") {
    // A(0) -------- 10 -------- B(0)
    // \                          /
    //  \--- 4 --- C(0) --- 4 ---/
    // where C is invalid

    constexpr uint8_t size = 3;
    uint8_t A = 0;
    uint8_t B = 1;
    uint8_t C = 2;

    AdjacencyCost<size> cost;
    cost.set_vertex(A, Cost{0});
    cost.set_vertex(B, Cost{0});
    cost.set_vertex(C, Cost{0});
    cost.set(A, B, Cost(10));
    cost.set(A, C, Cost(4));
    cost.set(C, B, Cost(4));
    etl::array<bool, size> valid_vertex{true, true, false};

    auto result = resolve_gateway_vertex<size>(cost, valid_vertex, A, B);
    CHECK(result.has_value());
    CHECK(result.value() == B);
}

TEST_CASE("calculation considers node cost") {
    // A(0) ----- 5 ---- B(1)
    // |                   |
    // 2                   5
    // |                   |
    // C(8) ---- 2 ---- D(0)

    constexpr uint8_t size = 4;
    uint8_t A = 0;
    uint8_t B = 1;
    uint8_t C = 2;
    uint8_t D = 3;

    AdjacencyCost<size> cost;
    cost.set_vertex(A, Cost{0});
    cost.set_vertex(B, Cost{1});
    cost.set_vertex(C, Cost{8});
    cost.set_vertex(D, Cost{0});
    cost.set(A, B, Cost(5));
    cost.set(B, D, Cost(5));
    cost.set(A, C, Cost(2));
    cost.set(C, D, Cost(2));
    etl::array<bool, size> valid_vertex{true, true, true, true};

    auto result = resolve_gateway_vertex<size>(cost, valid_vertex, A, D);
    CHECK(result.has_value());
    CHECK(result.value() == B);
}
