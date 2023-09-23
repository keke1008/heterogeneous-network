#include <doctest.h>

#include <net/link/address.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("equality") {
    net::link::IPv4Address a{192, 168, 0, 1};
    net::link::IPv4Address b{192, 168, 0, 1};
    net::link::IPv4Address c{192, 168, 0, 2};

    CHECK(a == b);
    CHECK(a != c);
}

TEST_CASE("serialize") {
    etl::array<uint8_t, 15> buffer;
    nb::stream::BufferBuilder builder{buffer};

    net::link::IPv4Address a{192, 168, 0, 1};
    auto actual = nb::stream::build_buffer(buffer, a);

    auto expected = "192.168.0.1"_u8array;
    CHECK(etl::equal(etl::span(actual), etl::span(expected)));
}

TEST_CASE("deserialize") {
    auto bytes = "192.168.0.11"_u8array;
    auto address = net::link::IPv4Address::deserialize_from_pretty_format(bytes);

    CHECK(address == net::link::IPv4Address{192, 168, 0, 11});
}
