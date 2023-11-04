#include "./frame.h"

namespace net::routing::neighbor {
    etl::optional<NeighborFrame> parse_frame(etl::span<const uint8_t> buffer) {
        nb::buf::BufferSplitter splitter{buffer};

        auto type = static_cast<FrameType>(splitter.split_1byte());
        if (type == FrameType::HELLO || type == FrameType::HELLO_ACK) {
            return NeighborFrame{HelloFrame::parse(type, splitter)};
        } else if (type == FrameType::GOODBYE) {
            return NeighborFrame{GoodbyeFrame::parse(splitter)};
        } else {
            return etl::nullopt;
        }
    }
} // namespace net::routing::neighbor
