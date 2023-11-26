#pragma once

#include "../frame.h"
#include "../response.h"
#include <nb/future.h>
#include <nb/serde.h>
#include <util/span.h>

namespace net::link::wifi {
    class AsyncIpV4AddressResponseDeserializer {
        AsyncWifiIpV4AddressDeserializer deserializer_;

      public:
        template <nb::AsyncBufferedReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &readable) {
            // 22 == IPアドレスのレスポンスの最小の長さ: +CIPSTA:ip:"0.0.0.0"\r\n
            SERDE_DESERIALIZE_OR_RETURN(readable.poll_readable(22));

            readable.read_span_unchecked(7); // drop +CIPSTA

            auto message_type = readable.read_span_unchecked(4); // expect :ip:
            if (util::as_str(message_type) != ":ip:") {
                return nb::DeserializeResult::Invalid;
            }

            readable.read_span_unchecked(1); // drop "
            return deserializer_.deserialize(readable);
        }

        inline WifiIpV4Address result() const {
            return deserializer_.result();
        }
    };

    class GetIp {
        nb::ser::AsyncStaticSpanSerializer command_{"AT+CIPSTA?\r\n"};

        // +CIPSTA:ip:"255.255.255.255"\r\n
        nb::de::AsyncMaxLengthSingleLineBytesDeserializer<30> response_;

        nb::Promise<WifiIpV4Address> address_promise_;

      public:
        GetIp() = delete;
        GetIp(const GetIp &) = delete;
        GetIp(GetIp &&) = delete;
        GetIp &operator=(const GetIp &) = delete;
        GetIp &operator=(GetIp &&) = delete;

        explicit GetIp(nb::Promise<WifiIpV4Address> &&promise)
            : address_promise_{etl::move(promise)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(RW &rw) {
            POLL_UNWRAP_OR_RETURN(command_.serialize(rw));

            while (true) {
                if (POLL_UNWRAP_OR_RETURN(response_.deserialize(rw)) != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                auto line = response_.result();
                auto poll_result =
                    nb::deserialize_span(line, AsyncBufferedResponseTypeDeserializer{});
                if (POLL_UNWRAP_OR_RETURN(poll_result) == nb::DeserializeResult::Ok) {
                    return nb::ready();
                }

                AsyncIpV4AddressResponseDeserializer deserializer;
                poll_result = nb::deserialize_span(line, deserializer);
                if (POLL_UNWRAP_OR_RETURN(poll_result) != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                address_promise_.set_value(etl::move(deserializer.result()));
                return nb::ready();
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
