#include <doctest.h>
#include <util/doctest_ext.h>

#include <net/link/wifi.h>

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

    auto expected = "192.168.0.1";
    CHECK(util::as_str(actual) == "192.168.0.1");
}

TEST_CASE("deserialize") {
    auto bytes = util::as_bytes("192.168.0.11,");
    nb::buf::BufferSplitter splitter{bytes};
    auto address = splitter.parse<IPv4AddressWithTrailingCommaParser>();
    CHECK(address == IPv4Address{192, 168, 0, 11});
}
