#pragma once

#include "./address.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <nb/future.h>
#include <net/frame/service.h>
#include <stdint.h>
#include <util/span.h>

namespace net::link::wifi {
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
            if (util::span::equal(buffer_.written_bytes(), "+IPD,")) {
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
    };

    class ReceiveDataMessageHandler {
        static constexpr uint8_t HEADER_SIZE = 23;

        nb::stream::SentinelWritableBuffer<HEADER_SIZE> header_{':'};
        etl::optional<ReceiveDataHeader> header_parsed_;
        etl::optional<net::frame::FrameReception<Address>> reception_;

        ReceiveDataHeader parse_header(etl::span<const uint8_t> span) {
            DEBUG_ASSERT(span.size() <= HEADER_SIZE, "Invalid header size");

            auto length_span = util::span::take_until(span, ',');
            DEBUG_ASSERT(length_span.has_value());

            auto remote_address_span = util::span::take_until(span, ',');
            DEBUG_ASSERT(remote_address_span.has_value());
            auto remote_address = IPv4Address::try_parse_pretty(remote_address_span.value());
            DEBUG_ASSERT(remote_address.has_value());

            auto remote_port_span = span;

            return ReceiveDataHeader{
                .length = serde::dec::deserialize<uint8_t>(length_span.value()),
                .remote_address = remote_address.value(),
                .remote_port = serde::dec::deserialize<uint16_t>(remote_port_span),
            };
        }

      public:
        ReceiveDataMessageHandler() = default;
        ReceiveDataMessageHandler(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler(ReceiveDataMessageHandler &&) = default;
        ReceiveDataMessageHandler &operator=(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler &operator=(ReceiveDataMessageHandler &&) = default;

        template <net::frame::IFrameService<Address> FrameService>
        nb::Poll<void> execute(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (!header_parsed_.has_value()) {
                POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));
                header_parsed_ = parse_header(header_.written_bytes());
            }

            if (!reception_.has_value()) {
                uint8_t length = header_parsed_->length;
                reception_ = POLL_MOVE_UNWRAP_OR_RETURN(service.notify_reception(length));
                reception_.value().source.set_value(Address{header_parsed_->remote_address});
            }

            return reception_->writer.write_all_from(stream);
        }
    };
} // namespace net::link::wifi
