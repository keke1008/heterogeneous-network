#pragma once

#include "../address.h"
#include "../response.h"
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::wifi {
    class SendData {
        net::frame::FrameTransmissionRequest<Address> request_;

        nb::stream::FixedReadableBuffer<40> prefix_;
        nb::stream::DiscardingUntilByteWritableBuffer body_prompt_{'>'};

        // "SEND OK\r\n" or "SEND FAIL\r\n" のため11バイト必要
        nb::stream::MaxLengthSingleLineWrtableBuffer<11> response_;

      public:
        explicit SendData(
            net::frame::FrameTransmissionRequest<Address> &&request,
            uint16_t remote_port
        )
            : request_{etl::move(request)},
              prefix_{
                  R"(AT+CIPSEND=)", // フォーマッタ抑制用コメント
                  nb::buf::FormatDecimal(request_.reader.frame_length()),
                  R"(,")",
                  IPv4Address(request_.destination),
                  R"(",)",
                  nb::buf::FormatDecimal(remote_port),
                  "\r\n",
              } {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(prefix_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(body_prompt_.write_all_from(stream));
            POLL_UNWRAP_OR_RETURN(request_.reader.read_all_into(stream));

            while (true) {
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
                auto line = POLL_UNWRAP_OR_RETURN(response_.poll());

                if (Response<ResponseType::SEND_OK>::try_parse(line)) {
                    request_.success.set_value(true);
                    return nb::ready();
                }

                if (Response<ResponseType::SEND_FAIL>::try_parse(line)) {
                    request_.success.set_value(false);
                    return nb::ready();
                }

                response_.reset();
            }
        }
    };
} // namespace net::link::wifi
