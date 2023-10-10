#pragma once

#include "../packet.h"
#include <memory/pair_shared.h>
#include <net/frame/service.h>
#include <net/link.h>

namespace net::trusted {
    template <frame::IFrameBufferRequester LowerRequester>
    class DataPacketBufferRequester {
        LowerRequester requester_;

        inline void write_header(frame::FrameBufferWriter &&writer) {
            writer.build(PacketHeaderWriter{PacketType::DATA});
        }

      public:
        explicit DataPacketBufferRequester(frame::FrameService<link::Address> &frame_service)
            : requester_{frame_service} {}

        inline nb::Poll<frame::FrameBufferWriter> request_frame_writer(uint8_t length) {
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(requester_.request_frame_writer(length));
            write_header(etl::move(writer));
            return etl::move(writer);
        }

        inline nb::Poll<frame::FrameBufferWriter> request_max_length_frame_writer() {
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(requester_.request_max_length_frame_writer());
            write_header(etl::move(writer));
            return etl::move(writer);
        }
    };

    class DataPacketBufferSender {
        memory::Reference<etl::optional<frame::FrameBufferReader>> transmit_reader_;

      public:
        DataPacketBufferSender() = delete;
        DataPacketBufferSender(const DataPacketBufferSender &) = delete;
        DataPacketBufferSender(DataPacketBufferSender &&) = default;
        DataPacketBufferSender &operator=(const DataPacketBufferSender &) = delete;
        DataPacketBufferSender &operator=(DataPacketBufferSender &&) = default;

        explicit DataPacketBufferSender(
            memory::Reference<etl::optional<frame::FrameBufferReader>> &&transmit_reader
        )
            : transmit_reader_{etl::move(transmit_reader)} {}

        nb::Poll<void> send_frame(frame::FrameBufferReader &&reader) {
            if (!transmit_reader_.has_pair()) {
                return nb::ready();
            }

            auto &transmit_reader = transmit_reader_.get().value().get();
            if (!transmit_reader.has_value()) {
                return nb::pending;
            }
            transmit_reader = etl::move(reader);
            return nb::ready();
        }

        inline void close() && {
            transmit_reader_.unpair();
        }
    };

    static_assert(frame::IFrameSender<DataPacketBufferSender>);

} // namespace net::trusted
