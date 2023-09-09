#pragma once

#include "./address.h"
#include "./buffer.h"
#include <nb/future.h>
#include <nb/stream.h>

namespace net::link {
    struct FrameHeader {
        Address source;
        Address destination;
        uint8_t body_total_length;

        inline FrameHeader(
            const Address &source,
            const Address &destination,
            uint8_t body_total_length
        )
            : source{source},
              destination{destination},
              body_total_length{body_total_length} {}

        inline FrameHeader(const FrameHeader &) = default;
        inline FrameHeader(FrameHeader &&) = default;
        inline FrameHeader &operator=(const FrameHeader &) = default;
        inline FrameHeader &operator=(FrameHeader &&) = default;

        constexpr bool operator==(const FrameHeader &other) const {
            return source == other.source && destination == other.destination &&
                   body_total_length == other.body_total_length;
        }

        inline constexpr bool operator!=(const FrameHeader &other) const {
            return !(*this == other);
        }

        inline constexpr uint8_t total_length() const {
            return source.total_length() + destination.total_length() + 1;
        }
    };

    class FrameHeaderDeserializer final : public nb::stream::WritableBuffer {
        AddressDeserializer source_;
        AddressDeserializer destination_;
        nb::stream::FixedWritableBuffer<1> body_total_length_;

      public:
        nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return nb::stream::write_all_from(source, source_, destination_, body_total_length_);
        }

        nb::Poll<FrameHeader> poll() {
            auto body_total_length = POLL_UNWRAP_OR_RETURN(body_total_length_.poll());
            return FrameHeader{
                source_.poll().unwrap(),
                destination_.poll().unwrap(),
                body_total_length[0],
            };
        }
    };

    class FrameHeaderSerializer final : public nb::stream::ReadableBuffer {
        AddressSerializer source_;
        AddressSerializer destination_;
        nb::stream::FixedReadableBuffer<1> body_total_length_;

      public:
        FrameHeaderSerializer(FrameHeader &header)
            : source_{header.source},
              destination_{header.destination},
              body_total_length_{header.body_total_length} {}

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return nb::stream::read_all_into(
                destination, source_, destination_, body_total_length_
            );
        }
    };

    struct Frame {
        FrameHeader header;
        FrameBufferReader buffer;
    };
} // namespace net::link
