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
    namespace {
        class IPv4AddressPartWriter {
            etl::span<const uint8_t, 4> address_;

          public:
            explicit IPv4AddressPartWriter(etl::span<const uint8_t, 4> address)
                : address_{address} {}

            void write_to_builder(nb::buf::BufferBuilder &builder) {
                DEBUG_ASSERT(builder.writable_count() >= 15);

                auto write_byte = [](uint8_t byte) {
                    return [byte](auto span) { return serde::dec::serialize(span, byte); };
                };

                for (uint8_t i = 0; i < 3; ++i) {
                    builder.append(write_byte(address_[i]));
                    builder.append(static_cast<uint8_t>('.'));
                }
                builder.append(write_byte(address_[3]));
            }
        };
    } // namespace

    class SendData {
        frame::FrameBufferReader reader_;

        nb::stream::FixedReadableBuffer<40> prefix_;
        nb::stream::DiscardingUntilByteWritableBuffer body_prompt_{'>'};
        nb::stream::FixedReadableBuffer<1> protocol_;

        // "SEND OK\r\n" or "SEND FAIL\r\n" のため11バイト必要
        nb::stream::MaxLengthSingleLineWrtableBuffer<11> response_;

      public:
        explicit SendData(
            frame::ProtocolNumber protocol_number,
            const IPv4Address &peer,
            frame::FrameBufferReader &&reader_ref
        )
            : reader_{etl::move(reader_ref)},
              prefix_{
                  R"(AT+CIPSEND=)",
                  nb::buf::FormatDecimal<uint8_t>(reader_.frame_length() + frame::PROTOCOL_SIZE),
                  R"(,")",
                  IPv4AddressPartWriter{peer.address()},
                  R"(",)",
                  nb::buf::FormatDecimal(peer.port()),
                  "\r\n",
              },
              protocol_{frame::ProtocolNumberWriter(protocol_number)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(prefix_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(body_prompt_.write_all_from(stream));
            POLL_UNWRAP_OR_RETURN(protocol_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(reader_.read_all_into(stream));

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
