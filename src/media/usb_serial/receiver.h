#pragma once

#include "./deserializer.h"
#include "media/packet.h"
#include <etl/optional.h>
#include <etl/variant.h>
#include <nb/stream.h>

namespace media::usb_serial {
    using ReceiverState = etl::variant<PacketDeserializer, nb::stream::HeapStreamWriter<uint8_t>>;

    class UsbSerialReceiver {
        ReceiverState state_{PacketDeserializer{}};

      public:
        UsbSerialReceiver() = default;
        UsbSerialReceiver(const UsbSerialReceiver &) = delete;
        UsbSerialReceiver(UsbSerialReceiver &&) = default;
        UsbSerialReceiver &operator=(const UsbSerialReceiver &) = delete;
        UsbSerialReceiver &operator=(UsbSerialReceiver &&) = default;

        using WritableStreamItem = uint8_t;
        using ReadableStreamItem = UsbSerialPacket;

        size_t writable_count() const {
            return etl::visit([](const auto &state) { return state.writable_count(); }, state_);
        }

        size_t readable_count() const {
            if (etl::holds_alternative<PacketDeserializer>(state_)) {
                return etl::get<PacketDeserializer>(state_).readable_count();
            } else {
                return 0;
            }
        }

        bool write(uint8_t data) {
            if (etl::holds_alternative<PacketDeserializer>(state_)) {
                return etl::get<PacketDeserializer>(state_).write(data);
            }

            auto &stream_writer = etl::get<nb::stream::HeapStreamWriter<uint8_t>>(state_);
            if (!stream_writer.write(data)) {
                auto parser = PacketDeserializer{};
                parser.write(data);
                state_ = parser;
            }
            return true;
        }

        etl::optional<UsbSerialPacket> read() {
            if (!etl::holds_alternative<PacketDeserializer>(state_)) {
                return etl::nullopt;
            }

            auto packet_header = etl::get<PacketDeserializer>(state_).read();
            if (!packet_header) {
                return etl::nullopt;
            }

            auto [writer, packet] = packet_header->make_packet();
            state_ = etl::move(writer);
            return etl::move(packet);
        }

        bool is_closed() const {
            return false;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<UsbSerialReceiver, uint8_t>);
    static_assert(nb::stream::is_stream_reader_v<UsbSerialReceiver, UsbSerialPacket>);
} // namespace media::usb_serial
