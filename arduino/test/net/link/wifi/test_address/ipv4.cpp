#include <doctest.h>
#include <util/doctest_ext.h>

#include <net/link/wifi.h>

using namespace net::link::wifi;

TEST_CASE("WifiIpV4Address") {
    WifiIpV4Address a{192, 168, 0, 1};
    WifiIpV4Address b{192, 168, 0, 1};
    WifiIpV4Address c{192, 168, 0, 2};

    SUBCASE("equality") {
        CHECK(a == b);
        CHECK(a != c);
    }

    SUBCASE("deserialize") {
        auto buffer = util::as_bytes("192.168.0.1");
        auto address = nb::buf::parse<WifiIpV4AddressDeserializer>(buffer);
        CHECK(address == a);
    }

    SUBCASE("serialize") {
        etl::array<uint8_t, 15> buffer;
        auto result = nb::buf::build_buffer(buffer, a);
        CHECK(util::as_str(result) == "192.168.0.1");
    }
}

TEST_CASE("WifiPort") {
    WifiPort a{1234};
    WifiPort b{1234};
    WifiPort c{1235};

    SUBCASE("equality") {
        CHECK(a == b);
        CHECK(a != c);
    }

    SUBCASE("deserialize") {
        auto buffer = util::as_bytes("1234");
        auto port = nb::buf::parse<WifiPortDeserializer>(buffer);
        CHECK(port == a);
    }

    SUBCASE("serialize") {
        etl::array<uint8_t, 5> buffer;
        auto result = nb::buf::build_buffer(buffer, a);
        CHECK(util::as_str(result) == "1234");
    }
}

TEST_CASE("WifiAddress") {
    WifiAddress a{WifiIpV4Address{192, 168, 0, 1}, WifiPort{1234}};
    WifiAddress b{WifiIpV4Address{192, 168, 0, 1}, WifiPort{1234}};
    WifiAddress c{WifiIpV4Address{192, 168, 0, 2}, WifiPort{1234}};

    SUBCASE("equality") {
        CHECK(a == b);
        CHECK(a != c);
    }

    SUBCASE("deserialize") {
        auto buffer = util::as_bytes("192.168.0.1:1234");
        WifiAddressDeserializer deserializer{':'};

        auto address = nb::buf::parse(buffer, deserializer);
        CHECK(address == a);
    }
}
