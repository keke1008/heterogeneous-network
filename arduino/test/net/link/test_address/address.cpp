#include <doctest.h>

#include <net/link/address.h>

using namespace net::link;

TEST_CASE("Address") {
    SUBCASE("constructor") {
        Address address{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
        CHECK_EQ(address.type(), AddressType::IPv4);

        auto span = address.address();
        CHECK_EQ(span.size(), 6);
        CHECK_EQ(span[0], 0x01);
        CHECK_EQ(span[1], 0x02);
        CHECK_EQ(span[2], 0x03);
        CHECK_EQ(span[3], 0x04);
        CHECK_EQ(span[4], 0x05);
        CHECK_EQ(span[5], 0x06);
    }

    SUBCASE("equality") {
        Address address1{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
        Address address2{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
        Address address3{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04, 0x05, 0x07}};
        CHECK_EQ(address1, address2);
        CHECK_NE(address1, address3);
    }

    SUBCASE("total_length") {
        Address address{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
        CHECK_EQ(address.total_length(), 7);

        Address address2{AddressType::Serial, {0x01}};
        CHECK_EQ(address2.total_length(), 2);
    }
}

TEST_CASE("AddressSerializer") {
    SUBCASE("serialize Serial") {
        Address address{AddressType::Serial, {0x01}};
        etl::array<uint8_t, 5> buffer;
        auto span = nb::buf::build_buffer(buffer, address);

        CHECK_EQ(span.size(), 2);
        CHECK_EQ(span[0], static_cast<uint8_t>(AddressType::Serial));
        CHECK_EQ(span[1], 0x01);
    }

    SUBCASE("serialize IPv4") {
        Address address{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
        etl::array<uint8_t, 7> buffer;
        auto span = nb::buf::build_buffer(buffer, address);

        CHECK_EQ(span.size(), 7);
        CHECK_EQ(span[0], static_cast<uint8_t>(AddressType::IPv4));
        CHECK_EQ(span[1], 0x01);
        CHECK_EQ(span[2], 0x02);
        CHECK_EQ(span[3], 0x03);
        CHECK_EQ(span[4], 0x04);
        CHECK_EQ(span[5], 0x05);
        CHECK_EQ(span[6], 0x06);
    }
}

TEST_CASE("AddressDeserializer") {
    SUBCASE("deserialize Serial") {
        etl::array<uint8_t, 2> reader{static_cast<uint8_t>(AddressType::Serial), 0x01};
        nb::buf::BufferSplitter splitter{reader};

        auto address = splitter.parse<AddressDeserializer>();
        CHECK_EQ(address.type(), AddressType::Serial);

        auto span = address.address();
        CHECK_EQ(span.size(), 1);
        CHECK_EQ(span[0], 0x01);
    }

    SUBCASE("deserialize IPv4") {
        etl::array<uint8_t, 7> reader{
            static_cast<uint8_t>(AddressType::IPv4), 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        nb::buf::BufferSplitter splitter{reader};

        auto address = splitter.parse<AddressDeserializer>();
        CHECK_EQ(address.type(), AddressType::IPv4);

        auto span = address.address();
        CHECK_EQ(span.size(), 6);
        CHECK_EQ(span[0], 0x01);
        CHECK_EQ(span[1], 0x02);
        CHECK_EQ(span[2], 0x03);
        CHECK_EQ(span[3], 0x04);
        CHECK_EQ(span[4], 0x05);
        CHECK_EQ(span[5], 0x06);
    }
}
