#include <doctest.h>
#include <media/packet.h>
#include <media/usb_serial.h>
#include <mock/serial.h>
#include <nb/stream.h>

TEST_CASE("Instantiate UsbSerial") {
    mock::MockSerial mock_serial;
    nb::serial::Serial serial(mock_serial);
    media::UsbSerial usb_serial{etl::move(serial)};
}

TEST_CASE("Send packet") {
    mock::MockSerial mock_serial;
    nb::serial::Serial serial(mock_serial);
    media::UsbSerial usb_serial{etl::move(serial)};

    auto [writer, reader] = nb::stream::make_heap_stream<uint8_t>(5);
    for (int i = 0; i < 5; i++) {
        writer.write(i);
    }

    media::UsbSerialPacket packet{
        media::UsbSerialAddress(0x12),
        media::UsbSerialAddress(0x34),
        etl::move(reader),
    };
    CHECK(usb_serial.write(etl::move(packet)));

    auto tx_buffer = mock_serial.tx_buffer();
    auto packet_bytes = {
        0x12, 0x34, 0x05, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
    };
    for (int b : packet_bytes) {
        usb_serial.execute();
        CHECK_EQ(tx_buffer->pop_front(), b);
    }
}

TEST_CASE("2 UsbSerials") {
    auto [mock_serial1, mock_serial2] = mock::make_connected_mock_serials();
    nb::serial::Serial serial1(mock_serial1);
    media::UsbSerial usb_serial1{etl::move(serial1)};
    nb::serial::Serial serial2(mock_serial2);
    media::UsbSerial usb_serial2{etl::move(serial2)};

    auto [writer, reader] = nb::stream::make_heap_stream<uint8_t>(5);
    for (int i = 0; i < 5; i++) {
        writer.write(i);
    }
    media::UsbSerialPacket tx_packet{
        media::UsbSerialAddress(0x12),
        media::UsbSerialAddress(0x34),
        etl::move(reader),
    };
    CHECK(usb_serial1.write(etl::move(tx_packet)));

    while (usb_serial2.readable_count() == 0) {
        usb_serial1.execute();
        usb_serial2.execute();
    }
    auto rx_packet = usb_serial2.read();
    CHECK(rx_packet.has_value());
    CHECK_EQ(rx_packet->source(), media::UsbSerialAddress(0x12));
    CHECK_EQ(rx_packet->destination(), media::UsbSerialAddress(0x34));
    auto &data = rx_packet->data();
    for (int i = 0; i < 5; i++) {
        usb_serial2.execute();
        CHECK_EQ(data.read(), etl::optional<uint8_t>(i));
    }
    CHECK_FALSE(data.read().has_value());
}
