#pragma once

#include "../packet.h"
#include <memory/pair_shared.h>

namespace net::trusted::receiver {
    class DataPacketReceiver {
        memory::Owned<etl::optional<frame::FrameBufferReader>> reader_;

      public:
        DataPacketReceiver() = delete;
        DataPacketReceiver(const DataPacketReceiver &) = delete;
        DataPacketReceiver(DataPacketReceiver &&) = default;
        DataPacketReceiver &operator=(const DataPacketReceiver &) = delete;
        DataPacketReceiver &operator=(DataPacketReceiver &&) = default;

        explicit DataPacketReceiver(memory::Owned<etl::optional<frame::FrameBufferReader>> &&reader)
            : reader_{etl::move(reader)} {}

        nb::Poll<frame::FrameBufferReader> execute() {
            if (auto &pair = reader_.get(); pair.has_value()) {
                auto reader = etl::move(pair.value());
                pair = etl::nullopt;
                return nb::ready(etl::move(reader));
            } else {
                return nb::pending;
            }
        }
    };
} // namespace net::trusted::receiver
