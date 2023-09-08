#include <doctest.h>

#include <net/link/frame/address.h>

using namespace net::link;

TEST_CASE("Address") {
    SUBCASE("constructor") {
        Address address{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04}};
        CHECK_EQ(address.type(), AddressType::IPv4);

        auto span = address.address();
        CHECK_EQ(span.size(), 4);
        CHECK_EQ(span[0], 0x01);
        CHECK_EQ(span[1], 0x02);
        CHECK_EQ(span[2], 0x03);
        CHECK_EQ(span[3], 0x04);
    }

    SUBCASE("equality") {
        Address address1{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04}};
        Address address2{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04}};
        Address address3{AddressType::IPv4, {0x01, 0x02, 0x03, 0x05}};
        CHECK_EQ(address1, address2);
        CHECK_NE(address1, address3);
    }
}

TEST_CASE("AddressSerializer") {
    SUBCASE("serialize Serial") {
        Address address{AddressType::Serial, {0x01}};
        AddressSerializer serializer{address};
        nb::stream::FixedWritableBuffer<2> writer{};

        auto poll = serializer.read_all_into(writer);
        CHECK(poll.is_ready());

        auto poll_span = writer.poll();
        CHECK(poll_span.is_ready());

        auto span = poll_span.unwrap();
        CHECK_EQ(span.size(), 2);
        CHECK_EQ(span[0], static_cast<uint8_t>(AddressType::Serial));
        CHECK_EQ(span[1], 0x01);
    }

    SUBCASE("serialize IPv4") {
        Address address{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04}};
        AddressSerializer serializer{address};
        nb::stream::FixedWritableBuffer<5> writer{};

        auto poll = serializer.read_all_into(writer);
        CHECK(poll.is_ready());

        auto poll_span = writer.poll();
        CHECK(poll_span.is_ready());

        auto span = poll_span.unwrap();
        CHECK_EQ(span.size(), 5);
        CHECK_EQ(span[0], static_cast<uint8_t>(AddressType::IPv4));
        CHECK_EQ(span[1], 0x01);
        CHECK_EQ(span[2], 0x02);
        CHECK_EQ(span[3], 0x03);
        CHECK_EQ(span[4], 0x04);
    }
}

TEST_CASE("AddressDeserializer") {
    SUBCASE("deserialize Serial") {
        AddressDeserializer deserializer{};
        nb::stream::FixedReadableBuffer<2> reader{static_cast<uint8_t>(AddressType::Serial), 0x01};

        auto poll = deserializer.write_all_from(reader);
        CHECK(poll.is_ready());

        auto poll_address = deserializer.poll();
        CHECK(poll_address.is_ready());

        auto address = poll_address.unwrap();
        CHECK_EQ(address.type(), AddressType::Serial);

        auto span = address.address();
        CHECK_EQ(span.size(), 1);
        CHECK_EQ(span[0], 0x01);
    }

    SUBCASE("deserialize IPv4") {
        AddressDeserializer deserializer{};
        nb::stream::FixedReadableBuffer<5> reader{
            static_cast<uint8_t>(AddressType::IPv4), 0x01, 0x02, 0x03, 0x04};

        auto poll = deserializer.write_all_from(reader);
        CHECK(poll.is_ready());

        auto poll_address = deserializer.poll();
        CHECK(poll_address.is_ready());

        auto address = poll_address.unwrap();
        CHECK_EQ(address.type(), AddressType::IPv4);

        auto span = address.address();
        CHECK_EQ(span.size(), 4);
        CHECK_EQ(span[0], 0x01);
        CHECK_EQ(span[1], 0x02);
        CHECK_EQ(span[2], 0x03);
        CHECK_EQ(span[3], 0x04);
    }
}
