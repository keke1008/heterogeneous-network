#include <doctest.h>
#include <media/usb_serial.h>
#include <nb/stream.h>

TEST_CASE("Instantiate Receiver") {
    media::usb_serial::UsbSerialReceiver receiver;
}

TEST_CASE("Receive packet") {
    media::usb_serial::UsbSerialPacketHeader header{
        media::UsbSerialAddress(0x12),
        media::UsbSerialAddress(0x34),
        0x0005,
    };
    media::usb_serial::PacketSerializer serializer(header);
    auto payload = nb::stream::make_fixed_stream_reader(0x01, 0x02, 0x03, 0x04, 0x05);

    media::usb_serial::UsbSerialReceiver receiver;
    nb::stream::pipe_readers(receiver, serializer, payload);
    CHECK_EQ(serializer.readable_count(), 0);
    CHECK_EQ(payload.readable_count(), 5);

    auto packet = receiver.read();
    CHECK(packet.has_value());
    CHECK_EQ(packet->source().value(), 0x12);
    CHECK_EQ(packet->destination().value(), 0x34);

    nb::stream::pipe_readers(receiver, serializer, payload);
    CHECK_EQ(payload.readable_count(), 0);

    auto &data = packet->data();
    CHECK_EQ(data.readable_count(), 5);
    for (uint8_t i = 1; i <= 5; i++) {
        auto d = data.read();
        CHECK(d.has_value());
        CHECK_EQ(d, i);
    }

    CHECK_EQ(data.readable_count(), 0);
    CHECK_FALSE(data.read().has_value());
}
