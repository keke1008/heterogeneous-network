#pragma once

#include "../packet.h"
#include <nb/channel.h>
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
        nb::OneBufferSender<frame::FrameBufferReader> reader_tx_;

      public:
        DataPacketBufferSender() = delete;
        DataPacketBufferSender(const DataPacketBufferSender &) = delete;
        DataPacketBufferSender(DataPacketBufferSender &&) = default;
        DataPacketBufferSender &operator=(const DataPacketBufferSender &) = delete;
        DataPacketBufferSender &operator=(DataPacketBufferSender &&) = default;

        explicit DataPacketBufferSender(nb::OneBufferSender<frame::FrameBufferReader> reader_tx)
            : reader_tx_{etl::move(reader_tx)} {}

        nb::Poll<void> send_frame(frame::FrameBufferReader &&reader) {
            if (reader_tx_.is_closed()) {
                return nb::ready();
            }
            return reader_tx_.send(etl::move(reader));
        }

        inline void close() && {
            etl::move(reader_tx_).close();
        }
    };

    static_assert(frame::IFrameSender<DataPacketBufferSender>);

} // namespace net::trusted
