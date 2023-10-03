#pragma once

#include "./address.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <nb/buf.h>
#include <nb/future.h>
#include <net/frame/service.h>
#include <stdint.h>
#include <util/span.h>

namespace net::link::wifi {
    static constexpr uint8_t MAX_HEADER_SIZE = 23;

    enum class MessageType : uint8_t {
        Unknown,
        DataReceived,
    };

    class MessageDetector {
        nb::stream::MinLengthSingleLineWritableBuffer<5> buffer_;
        etl::optional<nb::stream::DiscardSingleLineWritableBuffer> discarder_;

      public:
        nb::Poll<MessageType> execute(nb::stream::ReadableStream &stream) {
            if (discarder_.has_value()) {
                POLL_UNWRAP_OR_RETURN(discarder_.value().write_all_from(stream));
                discarder_.reset();
                return nb::ready(MessageType::Unknown);
            }

            POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(stream));
            if (util::as_str(buffer_.written_bytes()) == "+IPD,") {
                return nb::ready(MessageType::DataReceived);
            } else {
                discarder_.emplace();
                return nb::pending;
            }
        }
    };

    struct ReceiveDataHeader {
        uint8_t length;
        IPv4Address remote_address;
        uint16_t remote_port;

        static ReceiveDataHeader parse(etl::span<const uint8_t> span) {
            DEBUG_ASSERT(span.size() <= MAX_HEADER_SIZE, "Invalid header size");
            nb::buf::BufferSplitter splitter{span};
            return ReceiveDataHeader{
                .length = serde::dec::deserialize<uint8_t>(splitter.split_sentinel(',')),
                .remote_address = splitter.parse<IPv4AddressWithTrailingCommaParser>(),
                .remote_port = serde::dec::deserialize<uint16_t>(splitter.split_remaining()),
            };
        }
    };

    class ReceiveDataMessageHandler {
        nb::stream::SentinelWritableBuffer<MAX_HEADER_SIZE> header_{':'};
        etl::optional<ReceiveDataHeader> header_parsed_;
        nb::stream::FixedWritableBuffer<1> protocol_;
        etl::optional<net::frame::FrameReception<Address>> reception_;

      public:
        ReceiveDataMessageHandler() = default;
        ReceiveDataMessageHandler(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler(ReceiveDataMessageHandler &&) = default;
        ReceiveDataMessageHandler &operator=(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler &operator=(ReceiveDataMessageHandler &&) = default;

        template <net::frame::IFrameService FrameService>
        nb::Poll<void> execute(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (!header_parsed_.has_value()) {
                POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));
                header_parsed_ = ReceiveDataHeader::parse(header_.written_bytes());
            }

            if (!reception_.has_value()) {
                POLL_UNWRAP_OR_RETURN(protocol_.write_all_from(stream));
                uint8_t protocol = protocol_.written_bytes()[0];
                uint8_t length = header_parsed_->length - frame::PROTOCOL_SIZE;
                reception_ = POLL_MOVE_UNWRAP_OR_RETURN(service.notify_reception(protocol, length));
                reception_.value().source.set_value(Address{header_parsed_->remote_address});
            }

            return reception_->writer.write_all_from(stream);
        }
    };
} // namespace net::link::wifi
