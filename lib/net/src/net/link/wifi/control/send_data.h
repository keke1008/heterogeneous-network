#pragma once

#include "../../media.h"
#include "../address.h"
#include "../response.h"
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::wifi {
    class SendData {
        Frame frame_;

        nb::stream::FixedReadableBuffer<40> prefix_;
        nb::stream::DiscardingUntilByteWritableBuffer body_prompt_{'>'};
        nb::stream::FixedReadableBuffer<1> protocol_;

        // "SEND OK\r\n" or "SEND FAIL\r\n" のため11バイト必要
        nb::stream::MaxLengthSingleLineWrtableBuffer<11> response_;

      public:
        explicit SendData(Frame &&frame, uint16_t remote_port)
            : frame_{etl::move(frame)},
              prefix_{
                  R"(AT+CIPSEND=)",
                  nb::buf::FormatDecimal<uint8_t>(frame_.length + frame::PROTOCOL_SIZE),
                  R"(,")",
                  IPv4Address(frame_.peer),
                  R"(",)",
                  nb::buf::FormatDecimal(remote_port),
                  "\r\n",
              },
              protocol_{frame::ProtocolNumberWriter(frame_.protocol_number)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(prefix_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(body_prompt_.write_all_from(stream));
            POLL_UNWRAP_OR_RETURN(protocol_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(frame_.reader.read_all_into(stream));

            while (true) {
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
                auto line = POLL_UNWRAP_OR_RETURN(response_.poll());

                if (Response<ResponseType::SEND_OK>::try_parse(line)) {
                    return nb::ready();
                }

                if (Response<ResponseType::SEND_FAIL>::try_parse(line)) {
                    return nb::ready();
                }

                response_.reset();
            }
        }
    };
} // namespace net::link::wifi
