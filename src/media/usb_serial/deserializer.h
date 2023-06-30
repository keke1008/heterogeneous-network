#pragma once

#include "./packet_header.h"
#include <nb/stream.h>

namespace media::usb_serial {
    class PacketDeserializer {
        nb::stream::FixedByteWriter<1> source_;
        nb::stream::FixedByteWriter<1> destination_;
        nb::stream::FixedByteWriter<2> size_;

      public:
        using WritableStreamItem = uint8_t;
        using ReadableStreamItem = UsbSerialPacketHeader;

        PacketDeserializer() = default;
        PacketDeserializer(const PacketDeserializer &) = default;
        PacketDeserializer &operator=(const PacketDeserializer &) = default;
        PacketDeserializer(PacketDeserializer &&) = default;
        PacketDeserializer &operator=(PacketDeserializer &&) = default;

        size_t writable_count() const {
            return nb::stream::writable_count(source_, destination_, size_);
        }

        size_t readable_count() const {
            return nb::stream::is_closed(source_, destination_, size_) ? 1 : 0;
        }

        bool write(uint8_t data) {
            return nb::stream::write(data, source_, destination_, size_);
        }

        etl::optional<UsbSerialPacketHeader> read() {
            if (nb::stream::is_closed(source_, destination_, size_)) {
                UsbSerialAddress source{source_.get()};
                UsbSerialAddress destination{destination_.get()};
                uint16_t size{bytes::deserialize<uint16_t>(size_.get())};
                return UsbSerialPacketHeader{source, destination, size};
            } else {
                return etl::nullopt;
            }
        }

        bool is_closed() const {
            return false;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<PacketDeserializer, uint8_t>);
    static_assert(nb::stream::is_stream_reader_v<PacketDeserializer, UsbSerialPacketHeader>);
} // namespace media::usb_serial
