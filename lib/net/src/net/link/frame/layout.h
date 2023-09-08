#pragma once

#include "./address.h"
#include "./buffer.h"
#include <nb/stream.h>

namespace net::link {
    struct FrameHeader {
        Address source;
        Address destination;
        uint8_t body_octet_length;

        inline FrameHeader(Address source, Address destination)
            : source{source},
              destination{destination} {}

        inline FrameHeader(const FrameHeader &) = default;
        inline FrameHeader(FrameHeader &&) = default;
        inline FrameHeader &operator=(const FrameHeader &) = default;
        inline FrameHeader &operator=(FrameHeader &&) = default;
    };

    struct FrameHeaderDeserializer {
        AddressDeserializer source_;
        AddressDeserializer destination_;
        nb::stream::TinyByteWriter<sizeof(uint8_t)> body_octet_length_;

      public:
        template <typename Reader>
        nb::Poll<Reader> deserialize(Reader &reader) {
            auto poll = source_.poll(reader);
            if (poll.is_pending()) {
                return poll;
            }
            poll = destination_.poll(poll.value());
            if (poll.is_pending()) {
                return poll;
            }
            poll = body_octet_length_.poll(poll.value());
            if (poll.is_pending()) {
                return poll;
            }
            return nb::Poll<Reader>{reader};
        }
    };

    struct Frame {
        FrameHeader header;
        FrameBufferReader buffer;
    };
} // namespace net::link
