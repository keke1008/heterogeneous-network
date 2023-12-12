#pragma once

#include <logger.h>
#include <nb/serde.h>
#include <stdint.h>

namespace net::frame {
    enum class ProtocolNumber : uint8_t {
        NoProtocol = 0x00,
        RoutingNeighbor = 0x01,
        Discover = 0x02,
        Rpc = 0x03,
        Observer = 0x04,
    };

    inline constexpr uint8_t NUM_PROTOCOLS = 5;

    inline constexpr bool is_valid_protocol_number(uint8_t protocol_number) {
        return protocol_number <= static_cast<uint8_t>(ProtocolNumber::Rpc);
    }

    class AsyncProtocolNumberSerializer {
        nb::ser::Bin<uint8_t> protocol_number_;

      public:
        inline AsyncProtocolNumberSerializer(ProtocolNumber protocol_number)
            : protocol_number_(static_cast<uint8_t>(protocol_number)) {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writer) {
            return protocol_number_.serialize(writer);
        }

        inline constexpr uint8_t serialized_length() const {
            return protocol_number_.serialized_length();
        }
    };

    class AsyncProtocolNumberDeserializer {
        nb::de::Bin<uint8_t> protocol_number_;

      public:
        inline ProtocolNumber result() const {
            return static_cast<ProtocolNumber>(protocol_number_.result());
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(protocol_number_.deserialize(reader));
            return is_valid_protocol_number(protocol_number_.result())
                ? nb::de::DeserializeResult::Ok
                : nb::de::DeserializeResult::Invalid;
        }
    };
} // namespace net::frame
