#pragma once

#include "../address.h"
#include <nb/stream.h>
#include <util/span.h>

namespace net::link::wifi {
    class GetIp {
        nb::stream::FixedReadableBuffer<12> command_{"AT+CIPSTA?\r\n"};
        // +CIPSTA:ip:"255.255.255.255"\r\n
        nb::stream::MaxLengthSingleLineWrtableBuffer<30> response_;

        etl::optional<IPv4Address> result_;

      public:
        GetIp() = default;

        nb::Poll<etl::optional<IPv4Address>> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(command_.read_all_into(stream));
            while (stream.readable_count() > 0) {
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));

                auto bytes = response_.written_bytes();
                if (bytes.size() == 4 && util::as_str(bytes) == "OK\r\n") {
                    return result_;
                }

                // 22 == IPアドレスのレスポンスの最小の長さ: +CIPSTA:ip:"0.0.0.0"\r\n
                if (result_.has_value() || bytes.size() < 22) {
                    response_.reset();
                    continue;
                }

                auto message_type = bytes.subspan(7, 4); // drop +CIPSTA
                if (util::as_str(message_type) != ":ip:") {
                    response_.reset();
                    continue;
                }

                uint8_t address_begin = 11;
                uint8_t address_end = bytes.size() - 3; // drop "\r\n
                auto address_span = bytes.subspan(address_begin, address_end - address_begin);

                nb::buf::BufferSplitter splitter{address_span};
                result_ = IPv4AddressParser{}.parse(splitter);
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
