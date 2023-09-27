#include <doctest.h>

#include <net/link/wifi.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;
using namespace net::link::wifi;

TEST_CASE("equality") {
    IPv4Address a{192, 168, 0, 1};
    IPv4Address b{192, 168, 0, 1};
    IPv4Address c{192, 168, 0, 2};

    CHECK(a == b);
    CHECK(a != c);
}

TEST_CASE("serialize") {
    etl::array<uint8_t, 15> buffer;

    IPv4Address a{192, 168, 0, 1};
    auto actual = nb::buf::build_buffer(buffer, a);

    auto expected = "192.168.0.1"_u8array;
    CHECK(etl::equal(etl::span(actual), etl::span(expected)));
}

TEST_CASE("deserialize") {
    auto bytes = "192.168.0.11"_u8array;
    auto address = IPv4Address::try_parse_pretty(bytes).value();

    CHECK(address == IPv4Address{192, 168, 0, 11});
}
