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
        explicit AsyncSendRequestCommandSerializer(const WifiFrame &frame)
            : length_{frame.body_length()},
              address_{frame.remote.address()},
              port_{frame.remote.port()} {}

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
    class SendRequestControl {
        struct SendCommand {
            memory::Static<W> &writable;
            AsyncSendRequestCommandSerializer serializer;

            explicit SendCommand(memory::Static<W> &writable, const WifiFrame &frame)
                : writable{writable},
                  serializer{frame} {}
        };

        struct WaitingForOk {};

        struct WaitingForSendPrompt {};

        struct Done {
            bool success;
        };

        etl::variant<SendCommand, WaitingForOk, WaitingForSendPrompt, Done> state_;

      public:
        explicit SendRequestControl(memory::Static<W> &writable, const WifiFrame &frame)
            : state_{etl::in_place_type<SendCommand>, writable, frame} {}

        nb::Poll<bool> execute() {
            if (etl::holds_alternative<SendCommand>(state_)) {
                SendCommand &send = etl::get<SendCommand>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(send.serializer.serialize(*send.writable));
                if (result != nb::SerializeResult::Ok) {
                    return nb::ready(false);
                }

                state_ = WaitingForOk{};
            }

            if (etl::holds_alternative<WaitingForOk>(state_)) {
                return nb::pending;
            }

            if (etl::holds_alternative<WaitingForSendPrompt>(state_)) {
                return nb::pending;
            }

            if (etl::holds_alternative<Done>(state_)) {
                return etl::get<Done>(state_).success;
            }

            return nb::pending;
        }

        void handle_message(WifiMessage<R> &&message) {
            if (etl::holds_alternative<WaitingForOk>(state_)) {
                if (!etl::holds_alternative<WifiResponseMessage>(message)) {
                    state_ = Done{false};
                    return;
                }

                auto response = etl::get<WifiResponseMessage>(message);
                if (response == WifiResponseMessage::Ok) {
                    state_ = WaitingForSendPrompt{};
                    return;
                }
            } else if (etl::holds_alternative<WaitingForSendPrompt>(state_)) {
                if (!etl::holds_alternative<WifiResponseMessage>(message)) {
                    state_ = Done{false};
                    return;
                }

                auto prompt = etl::get<WifiResponseMessage>(message);
                if (prompt == WifiResponseMessage::SendPrompt) {
                    state_ = Done{true};
                    return;
                }
            }

            state_ = Done{false};
        }
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
                auto success = POLL_UNWRAP_OR_RETURN(send_request_.execute());
                if (!success) {
                    LOG_INFO(FLASH_STRING("Send request failed"));
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
