#pragma once

#include <logger.h>
#include <nb/buf.h>
#include <stdint.h>

namespace net::frame {
    enum class ProtocolNumber : uint8_t {
        NoProtocol = 0x00,
        RoutingNeighbor = 0x01,
        RoutingReactive = 0x02,
        Rpc = 0x03,
    };

    inline constexpr uint8_t NUM_PROTOCOLS = 4;

    struct ProtocolNumberWriter {
        ProtocolNumber protocol_number;

        inline void write_to_builder(nb::buf::BufferBuilder &builder) {
            builder.append(static_cast<uint8_t>(protocol_number));
        }
    };

    struct ProtocolNumberParser {
        inline ProtocolNumber parse(nb::buf::BufferSplitter &splitter) {
            uint8_t value = splitter.split_1byte();
            ASSERT(value <= 0x02);
            return static_cast<ProtocolNumber>(value);
        }
    };
} // namespace net::frame
