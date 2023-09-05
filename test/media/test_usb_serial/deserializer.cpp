#include <doctest.h>

#include <bytes/serde.h>
#include <etl/optional.h>
#include <media/common.h>
#include <media/usb_serial.h>
#include <nb/stream.h>

TEST_CASE("Instantate PacketDeserializer") {
    media::usb_serial::PacketDeserializer deserializer;
    CHECK_EQ(deserializer.read(), etl::nullopt);
}

TEST_CASE("Deserialize packet") {
    nb::stream::FixedByteReader<1> source{bytes::serialize<uint8_t>(01)};
    nb::stream::FixedByteReader<1> destination{bytes::serialize<uint8_t>(02)};
    nb::stream::FixedByteReader<2> size{bytes::serialize<uint16_t>(0x3456)};

    media::usb_serial::PacketDeserializer deserializer;
    CHECK(nb::stream::pipe_readers(deserializer, source, destination, size));

    auto packet = deserializer.read();
    CHECK(packet.has_value());
    CHECK_EQ(packet->source().value(), 0x01);
    CHECK_EQ(packet->destination().value(), 0x02);
    CHECK_EQ(packet->size(), 0x3456);
}
