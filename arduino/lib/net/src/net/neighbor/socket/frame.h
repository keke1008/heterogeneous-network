#pragma once

#include <net/node.h>

namespace net::neighbor {
    struct ReceivedNeighborFrame {
        node::NodeId sender;
        frame::FrameBufferReader reader;
    };
} // namespace net::neighbor
