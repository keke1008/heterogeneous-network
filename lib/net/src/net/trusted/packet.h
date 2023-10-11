#pragma once

#include <etl/expected.h>
#include <nb/poll.h>
#include <net/frame/service.h>
#include <util/time.h>

namespace net::trusted {
    enum class TrustedError : uint8_t {
        ConnectionRefused,
        Timeout,
        BadNetwork,
    };

    enum class PacketType : uint8_t {
        SYN,
        ACK,
        NACK,
        DATA,
        FIN,
    };

    template <PacketType Type>
    constexpr uint8_t frame_length() {
        static_assert(Type != PacketType::DATA, "DATA packet type is not supported");
        return 1;
    }

    class PacketHeaderWriter {
        PacketType type_;

      public:
        explicit PacketHeaderWriter(PacketType type) : type_{type} {}

        inline void write_to_buffer(nb::buf::BufferBuilder &builder) const {
            builder.append(static_cast<uint8_t>(type_));
        }
    };

} // namespace net::trusted
