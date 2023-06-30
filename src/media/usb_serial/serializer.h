#pragma once

#include "./packet_header.h"
#include <bytes/serde.h>
#include <nb/stream.h>

namespace media::usb_serial {
    class PacketSerializer {
        nb::stream::FixedByteReader<1> source_;
        nb::stream::FixedByteReader<1> destination_;
        nb::stream::FixedByteReader<2> size_;

      public:
        using ReadableStreamItem = uint8_t;

        PacketSerializer() = delete;
        PacketSerializer(const PacketSerializer &) = default;
        PacketSerializer(PacketSerializer &&) = default;
        PacketSerializer &operator=(const PacketSerializer &) = default;
        PacketSerializer &operator=(PacketSerializer &&) = default;

        PacketSerializer(UsbSerialPacketHeader &header)
            : source_{header.source().serialize()},
              destination_{header.destination().serialize()},
              size_{bytes::serialize(header.size())} {}

        size_t readable_count() const {
            return nb::stream::readable_count(source_, destination_, size_);
        }

        etl::optional<uint8_t> read() {
            return nb::stream::read(source_, destination_, size_);
        }

        bool is_closed() const {
            return nb::stream::is_closed(source_, destination_, size_);
        }
    };

    static_assert(nb::stream::is_stream_reader_v<PacketSerializer, uint8_t>);
} // namespace media::usb_serial
