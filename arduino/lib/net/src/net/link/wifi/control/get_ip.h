#pragma once

#include "../frame.h"
#include <nb/future.h>
#include <nb/stream.h>
#include <util/span.h>

namespace net::link::wifi {
    inline etl::optional<WifiIpV4Address>
    try_parse_ip_address_from_response(etl::span<const uint8_t> response) {
        // 22 == IPアドレスのレスポンスの最小の長さ: +CIPSTA:ip:"0.0.0.0"\r\n
        if (response.size() < 22) {
            return etl::nullopt;
        }

        auto message_type = response.subspan(7, 4); // drop +CIPSTA
        if (util::as_str(message_type) != ":ip:") {
            return etl::nullopt;
        }

        uint8_t address_begin = 11;
        uint8_t address_end = response.size() - 2; // drop "\r\n
        auto address_span = response.subspan(address_begin, address_end - address_begin);

        nb::buf::BufferSplitter splitter{address_span};
        return WifiIpV4AddressDeserializer{}.parse(splitter);
    }

    class GetIp {
        nb::stream::FixedReadableBuffer<12> command_{"AT+CIPSTA?\r\n"};
        // +CIPSTA:ip:"255.255.255.255"\r\n
        nb::stream::MaxLengthSingleLineWrtableBuffer<30> response_;

        nb::Promise<WifiIpV4Address> address_promise_;

      public:
        GetIp() = delete;
        GetIp(const GetIp &) = delete;
        GetIp(GetIp &&) = delete;
        GetIp &operator=(const GetIp &) = delete;
        GetIp &operator=(GetIp &&) = delete;

        explicit GetIp(nb::Promise<WifiIpV4Address> &&promise)
            : address_promise_{etl::move(promise)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(command_.read_all_into(stream));

            while (stream.readable_count() > 0) {
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));

                auto opt_bytes = response_.written_bytes();
                if (!opt_bytes.has_value()) {
                    response_.reset();
                    continue;
                }

                if (opt_bytes->size() == 4 && util::as_str(*opt_bytes) == "OK\r\n") {
                    return nb::ready();
                }

                auto address = try_parse_ip_address_from_response(*opt_bytes);
                if (address) {
                    address_promise_.set_value(etl::move(*address));
                } else {
                    response_.reset();
                }
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
