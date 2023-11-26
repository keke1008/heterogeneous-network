#pragma once

#include "../../media.h"
#include "../frame.h"
#include "../response.h"
#include "./generic.h"
#include <etl/optional.h>
#include <nb/poll.h>
#include <nb/serde.h>
#include <net/frame/service.h>

namespace net::link::wifi {
    class AsyncSendRequestCommandSerializer {
        nb::ser::AsyncStaticSpanSerializer prefix_{R"(AT+CIPSEND=)"};
        nb::ser::Dec<uint16_t> length_;
        nb::ser::AsyncStaticSpanSerializer comma_{R"(,")"};
        AsyncWifiIpV4AddressSerializer address_;
        nb::ser::AsyncStaticSpanSerializer comma2_{R"(",)"};
        AsyncWifiPortSerializer port_;
        nb::ser::AsyncStaticSpanSerializer trailer_{"\r\n"};

      public:
        AsyncSendRequestCommandSerializer(const WifiFrame &frame)
            : length_{static_cast<uint16_t>(frame.reader.buffer_length() + frame::PROTOCOL_SIZE)},
              address_{frame.remote.address_part()},
              port_{frame.remote.port_part()} {}

        template <nb::AsyncReadable R>
        nb::Poll<nb::SerializeResult> serialize(R &reader) {
            SERDE_SERIALIZE_OR_RETURN(prefix_.serialize(reader));
            SERDE_SERIALIZE_OR_RETURN(length_.serialize(reader));
            SERDE_SERIALIZE_OR_RETURN(comma_.serialize(reader));
            SERDE_SERIALIZE_OR_RETURN(address_.serialize(reader));
            SERDE_SERIALIZE_OR_RETURN(comma2_.serialize(reader));
            SERDE_SERIALIZE_OR_RETURN(port_.serialize(reader));
            return trailer_.serialize(reader);
        }

        uint8_t serialized_length() const {
            return prefix_.serialized_length() + length_.serialized_length() +
                comma_.serialized_length() + address_.serialized_length() +
                comma2_.serialized_length() + port_.serialized_length() +
                trailer_.serialized_length();
        }
    };

    class SendRequest {
        AsyncSendRequestCommandSerializer command_;
        AsyncResponseTypeBytesDeserializer response_;

      public:
        explicit SendRequest(const WifiFrame &frame) : command_{frame} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<bool> execute(RW &rw) {
            POLL_UNWRAP_OR_RETURN(command_.serialize(rw));

            while (true) {
                auto poll_result = response_.deserialize(rw);

                auto bytes = response_.written_bytes();
                if (bytes.size() == 1 && bytes[0] == '>') {
                    return nb::ready(true);
                }

                if (POLL_UNWRAP_OR_RETURN(poll_result) != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                AsyncResponseTypeDeserializer de;
                poll_result = nb::deserialize_span(response_.result(), de);
                if (POLL_UNWRAP_OR_RETURN(poll_result) != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                return nb::ready(false);
            }
        }
    };

    class AsyncFrameSerializer {
        frame::AsyncProtocolNumberSerializer protocol_;
        frame::AsyncFrameBufferReaderSerializer reader_;

      public:
        explicit AsyncFrameSerializer(WifiFrame &&frame)
            : protocol_{frame.protocol_number},
              reader_{etl::move(frame.reader)} {}

        template <nb::AsyncReadable R>
        nb::Poll<nb::SerializeResult> serialize(R &reader) {
            SERDE_SERIALIZE_OR_RETURN(protocol_.serialize(reader));
            return reader_.serialize(reader);
        }

        uint8_t serialized_length() const {
            return protocol_.serialized_length() + reader_.serialized_length();
        }
    };

    class SendFrame {
        AsyncFrameSerializer serializer_;
        AsyncResponseTypeDeserializer response_;

      public:
        explicit SendFrame(WifiFrame &&frame) : serializer_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(RW &rw) {
            POLL_UNWRAP_OR_RETURN(serializer_.serialize(rw));
            POLL_UNWRAP_OR_RETURN(response_.deserialize(rw));
            return nb::ready();
        }
    };

    class SendData {
        enum class State : uint8_t {
            SendRequest,
            SendData,
        } state_{State::SendRequest};

        SendRequest send_request_;
        SendFrame send_frame_;

      public:
        explicit SendData(WifiFrame &&frame)
            : send_request_{frame},
              send_frame_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(RW &rw) {
            if (state_ == State::SendRequest) {
                bool success = POLL_UNWRAP_OR_RETURN(send_request_.execute(rw));
                if (!success) {
                    return nb::ready();
                }

                state_ = State::SendData;
            }

            return send_frame_.execute(rw);
        }
    };
} // namespace net::link::wifi
