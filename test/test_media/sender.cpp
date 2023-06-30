#include "media/packet.h"
#include <doctest.h>
#include <media/usb_serial.h>

TEST_CASE("Instantiate UsbSerialSender") {
    media::usb_serial::UsbSerialSender sender;
}

TEST_CASE("Send data") {
    media::usb_serial::UsbSerialPacketHeader header{
        media::UsbSerialAddress(0x12),
        media::UsbSerialAddress(0x34),
        0x0005,
    };
    auto [payload_writer, packet] = header.make_packet();
    auto payload = nb::stream::make_fixed_stream_reader(0x01, 0x02, 0x03, 0x04, 0x05);
    nb::stream::pipe(payload, payload_writer);

    media::usb_serial::UsbSerialSender sender;
    sender.write(etl::move(packet));

    CHECK_EQ(sender.read(), 0x12);
    CHECK_EQ(sender.read(), 0x34);
    CHECK_EQ(sender.read(), 0x05);
    CHECK_EQ(sender.read(), 0x00);
    CHECK_EQ(sender.read(), 0x01);
    CHECK_EQ(sender.read(), 0x02);
    CHECK_EQ(sender.read(), 0x03);
    CHECK_EQ(sender.read(), 0x04);
    CHECK_EQ(sender.read(), 0x05);
}
