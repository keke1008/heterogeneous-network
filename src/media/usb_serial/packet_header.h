#pragma once

#include "media/packet.h"
#include <etl/utility.h>
#include <nb/stream.h>

namespace media::usb_serial {
    class UsbSerialPacketHeader {
        UsbSerialAddress source_;
        UsbSerialAddress destination_;
        uint16_t size_;

      public:
        UsbSerialPacketHeader() = delete;
        UsbSerialPacketHeader(const UsbSerialPacketHeader &) = default;
        UsbSerialPacketHeader(UsbSerialPacketHeader &&) = default;
        UsbSerialPacketHeader &operator=(const UsbSerialPacketHeader &) = default;
        UsbSerialPacketHeader &operator=(UsbSerialPacketHeader &&) = default;

        UsbSerialPacketHeader(UsbSerialAddress source, UsbSerialAddress dest, uint16_t size)
            : source_{source},
              destination_{dest},
              size_{size} {}

        UsbSerialAddress source() const {
            return source_;
        }

        UsbSerialAddress destination() const {
            return destination_;
        }

        uint16_t size() const {
            return size_;
        }

        etl::pair<nb::stream::HeapStreamWriter<uint8_t>, UsbSerialPacket> make_packet() const {
            auto [writer, reader] = nb::stream::make_heap_stream<uint8_t>(size_);
            UsbSerialPacket packet{source_, destination_, etl::move(reader)};
            return {etl::move(writer), etl::move(packet)};
        }

        static etl::pair<nb::stream::HeapStreamReader<uint8_t>, UsbSerialPacketHeader>
        from_packet(UsbSerialPacket &&packet) {
            auto source = packet.source();
            auto destination = packet.destination();
            auto size = static_cast<uint16_t>(packet.data().readable_count());
            return {etl::move(packet.data()), UsbSerialPacketHeader{source, destination, size}};
        }
    };
} // namespace media::usb_serial
