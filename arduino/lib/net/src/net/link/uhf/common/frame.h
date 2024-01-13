#pragma once

#include "./modem_id.h"
#include <net/frame.h>

namespace net::link::uhf {
    struct UhfFrame {
        frame::ProtocolNumber protocol_number;
        ModemId remote;
        frame::FrameBufferReader reader;

        static UhfFrame from_link_frame(LinkFrame &&frame) {
            return UhfFrame{
                .protocol_number = frame.protocol_number,
                .remote = ModemId{frame.remote},
                .reader = etl::move(frame.reader),
            };
        }

        explicit operator LinkFrame() && {
            return LinkFrame{
                .protocol_number = protocol_number,
                .remote = Address{remote},
                .reader = etl::move(reader),
            };
        }
    };
} // namespace net::link::uhf
