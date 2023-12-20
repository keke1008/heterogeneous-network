#pragma once

#include "../event.h"
#include "../frame.h"
#include "../response.h"
#include <etl/optional.h>
#include <nb/poll.h>
#include <nb/serde.h>
#include <net/frame.h>

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
        explicit AsyncSendRequestCommandSerializer(const WifiAddress &address, uint8_t body_length)
            : length_{static_cast<uint16_t>(body_length)},
              address_{address.address_part()},
              port_{address.port_part()} {}

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
        explicit SendRequest(const WifiDataFrame &frame)
            : command_{frame.remote, frame.body_length()} {}

        explicit SendRequest(const WifiAddress &destination, const WifiControlFrame &frame)
            : command_{destination, etl::visit([](auto &f) { return f.body_length(); }, frame)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<bool> execute(RW &rw) {
            POLL_UNWRAP_OR_RETURN(command_.serialize(rw));

            while (true) {
                auto poll_result = response_.deserialize(rw);

                auto bytes = response_.written_bytes();
                if (bytes.size() == 2 && bytes[0] == '>' && bytes[1] == ' ') {
                    return nb::ready(true);
                }

                if (POLL_UNWRAP_OR_RETURN(poll_result) != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                AsyncBufferedResponseTypeDeserializer de;
                poll_result = nb::deserialize_span(response_.result(), de);
                if (POLL_UNWRAP_OR_RETURN(poll_result) != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                auto response = de.result();
                if (response == ResponseType::OK) {
                    response_.reset();
                    continue;
                }

                return nb::ready(false);
            }
        }
    };

    class SendDataFrame {
        WifiAddress destination_;

        AsyncWifiDataFrameSerializer serializer_;
        AsyncResponseTypeDeserializer response_;

      public:
        explicit SendDataFrame(WifiDataFrame &&frame)
            : destination_{frame.remote},
              serializer_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<WifiAddress> execute(RW &rw) {
            POLL_UNWRAP_OR_RETURN(serializer_.serialize(rw));
            POLL_UNWRAP_OR_RETURN(response_.deserialize(rw));
            return destination_;
        }
    };

    class SendControlFrame {
        AsyncWifiControlFrameSerializer serializer_;
        AsyncResponseTypeDeserializer response_;

      public:
        explicit SendControlFrame(WifiControlFrame &&frame) : serializer_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(RW &rw) {
            POLL_UNWRAP_OR_RETURN(serializer_.serialize(rw));
            POLL_UNWRAP_OR_RETURN(response_.deserialize(rw));
            return nb::ready();
        }
    };

    class SendWifiFrame {
        enum class State : uint8_t {
            SendRequest,
            SendData,
        } state_{State::SendRequest};

        SendRequest send_request_;
        etl::variant<SendDataFrame, SendControlFrame> send_frame_;

      public:
        explicit SendWifiFrame(WifiDataFrame &&frame)
            : send_request_{frame},
              send_frame_{SendDataFrame{etl::move(frame)}} {}

        explicit SendWifiFrame(const WifiAddress &destination, WifiControlFrame &&frame)
            : send_request_{destination, frame},
              send_frame_{SendControlFrame{etl::move(frame)}} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<etl::optional<WifiEvent>> execute(RW &rw) {
            if (state_ == State::SendRequest) {
                bool success = POLL_UNWRAP_OR_RETURN(send_request_.execute(rw));
                if (!success) {
                    return etl::optional<WifiEvent>{};
                }

                state_ = State::SendData;
            }

            return etl::visit(
                util::Visitor{
                    [&](SendDataFrame &frame) -> nb::Poll<etl::optional<WifiEvent>> {
                        const auto &address = POLL_UNWRAP_OR_RETURN(frame.execute(rw));
                        return etl::optional<WifiEvent>{SentDataFrame{.destination = address}};
                    },
                    [&](SendControlFrame &frame) -> nb::Poll<etl::optional<WifiEvent>> {
                        POLL_UNWRAP_OR_RETURN(frame.execute(rw));
                        return etl::optional<WifiEvent>{};
                    },
                },
                send_frame_
            );
        }
    };
} // namespace net::link::wifi
