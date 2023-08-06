#include <doctest.h>
#include <media/common.h>
#include <media/usb_serial.h>

TEST_CASE("Instantiate PacketSerializer") {
    media::usb_serial::UsbSerialPacketHeader header{
        media::UsbSerialAddress(0x12),
        media::UsbSerialAddress(0x34),
        0x5678,
    };

    media::usb_serial::PacketSerializer serializer(header);
    CHECK_EQ(serializer.read(), 0x12);
    CHECK_EQ(serializer.read(), 0x34);
    CHECK_EQ(serializer.read(), 0x78);
    CHECK_EQ(serializer.read(), 0x56);
}
