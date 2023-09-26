#pragma once

#include "./address.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <nb/future.h>
#include <net/link/frame.h>
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

    class ReceiveDataMessageHandler {
        static constexpr uint8_t HEADER_SIZE = 23;

        nb::stream::SentinelWritableBuffer<HEADER_SIZE> header_{':'};
        nb::Promise<Address> remote_address_promise_;
        nb::Promise<DataReader> body_promise_;
        etl::optional<nb::Future<void>> barrier_;

        struct ReceiveDataHeader {
            uint8_t length;
            IPv4Address remote_address;
            uint16_t remote_port;
        };

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
        explicit ReceiveDataMessageHandler(
            nb::Promise<DataReader> &&body_promise,
            nb::Promise<Address> &&remote_address_promise
        )
            : body_promise_{etl::move(body_promise)},
              remote_address_promise_{etl::move(remote_address_promise)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            if (!barrier_.has_value()) {
                POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));
                auto header = parse_header(header_.written_bytes());
                remote_address_promise_.set_value(Address(header.remote_address));

                auto [body_future, body_promise] = nb::make_future_promise_pair<void>();
                barrier_ = etl::move(body_future);
                body_promise_.set_value(DataReader{
                    etl::ref(stream),
                    etl::move(body_promise),
                    header.length,
                });
            }

            return barrier_.value().poll();
        }
    };
} // namespace net::link::wifi
