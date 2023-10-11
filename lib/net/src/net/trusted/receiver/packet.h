#pragma once

#include "../packet.h"
#include <nb/channel.h>

namespace net::trusted::receiver {
    class DataPacketReceiver {
        nb::OneBufferReceiver<frame::FrameBufferReader> reader_rx_;

      public:
        DataPacketReceiver() = delete;
        DataPacketReceiver(const DataPacketReceiver &) = delete;
        DataPacketReceiver(DataPacketReceiver &&) = default;
        DataPacketReceiver &operator=(const DataPacketReceiver &) = delete;
        DataPacketReceiver &operator=(DataPacketReceiver &&) = default;

        explicit DataPacketReceiver(nb::OneBufferReceiver<frame::FrameBufferReader> &&reader_rx)
            : reader_rx_{etl::move(reader_rx)} {}

        nb::Poll<frame::FrameBufferReader> receive_frame() {
            return reader_rx_.receive();
        }
    };

    static_assert(frame::IFrameReceiver<DataPacketReceiver>);
} // namespace net::trusted::receiver
