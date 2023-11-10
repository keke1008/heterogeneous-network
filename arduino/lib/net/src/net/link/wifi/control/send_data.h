#pragma once

#include "../../media.h"
#include "../frame.h"
#include "../response.h"
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::wifi {
    class SendData {
        frame::FrameBufferReader reader_;

        // AT+CIPSEND=40,"255.255.255.255",65535\r\n
        nb::stream::FixedReadableBuffer<40> prefix_;
        nb::stream::DiscardingUntilByteWritableBuffer body_prompt_{'>'};
        nb::stream::FixedReadableBuffer<1> protocol_;

        // "SEND OK\r\n" or "SEND FAIL\r\n" のため11バイト必要
        nb::stream::MaxLengthSingleLineWrtableBuffer<11> response_;

      public:
        explicit SendData(WifiFrame &&frame)
            : reader_{etl::move(frame.reader)},
              prefix_{
                  R"(AT+CIPSEND=)",
                  nb::buf::FormatDecimal<uint8_t>(reader_.frame_length() + frame::PROTOCOL_SIZE),
                  R"(,")",
                  frame.remote.address_part(),
                  R"(",)",
                  frame.remote.port_part(),
                  "\r\n",
              },
              protocol_{frame::ProtocolNumberWriter(frame.protocol_number)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(prefix_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(body_prompt_.write_all_from(stream));
            POLL_UNWRAP_OR_RETURN(protocol_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(reader_.read_all_into(stream));

            while (true) {
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
                auto opt_line = response_.written_bytes();
                if (!opt_line.has_value()) {
                    response_.reset();
                    continue;
                }

                if (Response<ResponseType::SEND_OK>::try_parse(*opt_line)) {
                    return nb::ready();
                }
                if (Response<ResponseType::SEND_FAIL>::try_parse(*opt_line)) {
                    return nb::ready();
                }

                response_.reset();
            }
        }
    };
} // namespace net::link::wifi
