#pragma once

#include "../frame.h"
#include "./generic.h"
#include <etl/optional.h>
#include <nb/poll.h>
#include <nb/serde.h>
#include <net/frame.h>

namespace media::wifi {
    class AsyncSendRequestCommandSerializer {
        nb::ser::AsyncStaticSpanSerializer prefix_{R"(AT+CIPSEND=)"};
        nb::ser::Dec<uint16_t> length_;
        nb::ser::AsyncStaticSpanSerializer comma_{R"(,")"};
        AsyncIpV4AddressSerializer address_;
        nb::ser::AsyncStaticSpanSerializer comma2_{R"(",)"};
        AsyncUdpPortSerializer port_;
        nb::ser::AsyncStaticSpanSerializer trailer_{"\r\n"};

      public:
        explicit AsyncSendRequestCommandSerializer(const UdpAddress &address, uint8_t body_length)
            : length_{static_cast<uint16_t>(body_length)},
              address_{address.address()},
              port_{address.port()} {}

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

    template <nb::AsyncReadable R, nb::AsyncWritable W>
    struct SendRequestControl
        : public GenericEmptyResponseSyncControl<R, W, AsyncSendRequestCommandSerializer> {
        explicit SendRequestControl(memory::Static<W> &writable, const WifiFrame &frame)
            : GenericEmptyResponseSyncControl<R, W, AsyncSendRequestCommandSerializer>{
                  writable,
                  WifiResponseMessage::SendPrompt,
                  frame.remote,
                  frame.body_length(),
              } {}
    };

    template <nb::AsyncReadable R, nb::AsyncWritable W>
    struct SendFrameControl
        : public GenericEmptyResponseSyncControl<R, W, AsyncWifiFrameSerializer> {
        explicit SendFrameControl(memory::Static<W> &writable, WifiFrame &&frame)
            : GenericEmptyResponseSyncControl<R, W, AsyncWifiFrameSerializer>{
                  writable,
                  WifiResponseMessage::SendOk,
                  etl::move(frame),
              } {}
    };

    template <nb::AsyncReadable R, nb::AsyncWritable W>
    class SendWifiFrameControl {
        memory::Static<W> &writable_;
        SendRequestControl<R, W> send_request_;
        etl::variant<WifiFrame, SendFrameControl<R, W>> send_frame_;

      public:
        explicit SendWifiFrameControl(memory::Static<W> &writable, WifiFrame &&frame)
            : writable_{writable},
              send_request_{writable, frame},
              send_frame_{etl::in_place_type<WifiFrame>, etl::move(frame)} {}

        nb::Poll<void> execute() {
            if (etl::holds_alternative<WifiFrame>(send_frame_)) {
                auto &&success = POLL_MOVE_UNWRAP_OR_RETURN(send_request_.execute());
                if (!success) {
                    return nb::ready();
                }

                send_frame_ = SendFrameControl<R, W>{
                    writable_,
                    etl::move(etl::get<WifiFrame>(send_frame_)),
                };
            }

            auto &send_frame = etl::get<SendFrameControl<R, W>>(send_frame_);
            POLL_UNWRAP_OR_RETURN(send_frame.execute());
            return nb::ready();
        }

        void handle_message(WifiMessage<R> &&message) {
            if (etl::holds_alternative<WifiFrame>(send_frame_)) {
                send_request_.handle_message(etl::move(message));
            } else {
                auto &send_frame = etl::get<SendFrameControl<R, W>>(send_frame_);
                send_frame.handle_message(etl::move(message));
            }
        }
    };
} // namespace media::wifi
