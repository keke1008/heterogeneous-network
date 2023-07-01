#pragma once

#include "./packet_header.h"
#include "./serializer.h"
#include "media/packet.h"
#include <etl/optional.h>
#include <etl/variant.h>
#include <nb/stream.h>
#include <stddef.h>

namespace media::usb_serial {
    class UsbSerialSender {
        etl::optional<etl::pair<PacketSerializer, nb::stream::HeapStreamReader<uint8_t>>> state_;

      public:
        UsbSerialSender() = default;
        UsbSerialSender(const UsbSerialSender &) = delete;
        UsbSerialSender(UsbSerialSender &&) = default;
        UsbSerialSender &operator=(const UsbSerialSender &) = delete;
        UsbSerialSender &operator=(UsbSerialSender &&) = default;

        using StreamWriterItem = UsbSerialPacket;
        using StreamReaderItem = uint8_t;

        bool is_writable() const {
            return !state_;
        }

        size_t writable_count() const {
            if (state_) {
                return 0;
            } else {
                return 1;
            }
        }

        bool write(UsbSerialPacket &&packet) {
            if (state_) {
                return false;
            }
            auto [reader, header] = UsbSerialPacketHeader::from_packet(etl::move(packet));
            state_ = etl::make_pair(PacketSerializer{header}, etl::move(reader));
            return true;
        }

        bool is_readable() const {
            if (state_) {
                return nb::stream::is_readable(state_->first, state_->second);
            } else {
                return false;
            }
        }

        size_t readable_count() const {
            if (state_) {
                return nb::stream::readable_count(state_->first, state_->second);
            } else {
                return 0;
            }
        }

        etl::optional<uint8_t> read() {
            if (!state_) {
                return etl::nullopt;
            }

            auto data = nb::stream::read(state_->first, state_->second);
            if (!data) {
                state_ = etl::nullopt;
            }
            return data;
        }

        bool is_closed() const {
            return false;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<UsbSerialSender>);
    static_assert(nb::stream::is_stream_reader_v<UsbSerialSender>);
} // namespace media::usb_serial
