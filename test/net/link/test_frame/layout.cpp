#include <doctest.h>

#include <net/link/frame/layout.h>

using namespace net::link;

TEST_CASE("FrameHeader") {
    const Address source{AddressType::IPv4, {0x01, 0x02, 0x03, 0x04}};
    const Address destination{AddressType::IPv4, {0x05, 0x06, 0x07, 0x08}};
    const uint8_t body_total_length = 0x09;
    FrameHeader header{source, destination, body_total_length};

    SUBCASE("constructor") {
        CHECK_EQ(header.source, source);
        CHECK_EQ(header.destination, destination);
        CHECK_EQ(header.body_total_length, body_total_length);
    }

    SUBCASE("total_length") {
        CHECK_EQ(header.total_length(), 11);
    }

    SUBCASE("serialize") {
        FrameHeaderSerializer serializer{header};
        nb::stream::FixedWritableBuffer<11> writer{};

        auto poll = serializer.read_all_into(writer);
        CHECK(poll.is_ready());

        auto poll_span = writer.poll();
        CHECK(poll_span.is_ready());

        auto span = poll_span.unwrap();
        CHECK_EQ(span.size(), header.total_length());

        etl::array<uint8_t, 11> expected{
            static_cast<uint8_t>(AddressType::IPv4),
            0x01,
            0x02,
            0x03,
            0x04,
            static_cast<uint8_t>(AddressType::IPv4),
            0x05,
            0x06,
            0x07,
            0x08,
            body_total_length,
        };
        CHECK(etl::equal(span, etl::span(expected)));
    }

    SUBCASE("deserialize") {
        FrameHeaderDeserializer deserializer{};
        nb::stream::FixedReadableBuffer<11> reader{
            static_cast<uint8_t>(AddressType::IPv4),
            0x01,
            0x02,
            0x03,
            0x04,
            static_cast<uint8_t>(AddressType::IPv4),
            0x05,
            0x06,
            0x07,
            0x08,
            body_total_length,
        };

        auto poll = deserializer.write_all_from(reader);
        CHECK(poll.is_ready());

        auto poll_header = deserializer.poll();
        CHECK(poll_header.is_ready());

        auto actual_header = poll_header.unwrap();
        CHECK_EQ(header, actual_header);
    }
}
