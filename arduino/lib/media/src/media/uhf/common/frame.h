#pragma once

#include "../../address/modem_id.h"
#include <net/frame.h>
#include <net/link.h>

namespace media::uhf {
    struct UhfFrame {
        net::frame::ProtocolNumber protocol_number;
        ModemId remote;
        net::frame::FrameBufferReader reader;

        static UhfFrame from_link_frame(net::link::LinkFrame &&frame) {
            return UhfFrame{
                .protocol_number = frame.protocol_number,
                .remote = ModemId{frame.remote},
                .reader = etl::move(frame.reader),
            };
        }

        UhfFrame clone() const {
            return UhfFrame{
                .protocol_number = protocol_number,
                .remote = remote,
                .reader = reader.make_initial_clone(),
            };
        }
    };
} // namespace media::uhf
