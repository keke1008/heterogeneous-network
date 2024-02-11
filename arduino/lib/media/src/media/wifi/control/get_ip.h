#pragma once

#include "../frame.h"
#include "../message.h"
#include <nb/future.h>
#include <nb/serde.h>
#include <util/span.h>

namespace media::wifi {
    template <nb::AsyncReadable R, nb::AsyncWritable W>
    class GetIpControl {
        struct SendCommand {
            memory::Static<W> &writable;
            nb::ser::AsyncStaticSpanSerializer serializer{"AT+CIPSTA?\r\n"};
        };

        struct WaitingForIpResponse {};

        struct ReceiveIpResponse {
            // "255.255.255.255"\r\n
            static inline constexpr uint8_t MAX_BODY_LENGTH = 19;
            nb::de::AsyncMaxLengthSingleLineBytesDeserializer<19> response{};
            nb::LockGuard<etl::reference_wrapper<memory::Static<R>>> readable;
        };

        struct WaitingForResponseMessage {};

        struct Done {};

        etl::variant<
            SendCommand,
            WaitingForIpResponse,
            ReceiveIpResponse,
            WaitingForResponseMessage,
            Done>
            state_;
        nb::Promise<etl::optional<UdpIpAddress>> address_promise_;

      public:
        explicit GetIpControl(
            nb::Promise<etl::optional<UdpIpAddress>> &&promise,
            memory::Static<W> &writable
        )
            : state_{SendCommand{.writable = writable}},
              address_promise_{etl::move(promise)} {}

        nb::Poll<void> execute() {
            if (etl::holds_alternative<SendCommand>(state_)) {
                SendCommand &send = etl::get<SendCommand>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(send.serializer.serialize(*send.writable));
                if (result != nb::SerializeResult::Ok) {
                    address_promise_.set_value(etl::nullopt);
                    state_ = Done{};
                    return nb::ready();
                }

                state_ = WaitingForIpResponse{};
            }

            if (etl::holds_alternative<WaitingForIpResponse>(state_)) {
                return nb::pending;
            }

            if (etl::holds_alternative<ReceiveIpResponse>(state_)) {
                auto &[response, readable] = etl::get<ReceiveIpResponse>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(response.deserialize(*readable->get()));
                if (result != nb::DeserializeResult::Ok) {
                    address_promise_.set_value(etl::nullopt);
                    state_ = Done{};
                    return nb::ready();
                }

                etl::span<const uint8_t> line = response.result();
                if (util::as_str(line.first<2>()) != "ip") {
                    state_ = WaitingForIpResponse{};
                    return nb::pending;
                }

                // drop "ip:"" and "\r\n"
                etl::span<const uint8_t> body = line.subspan(4, line.size() - 6);
                AsyncUdpIpAddressDeserializer deserializer;
                auto poll_result = nb::deserialize_span(body, deserializer);
                if (poll_result.is_pending()) {
                    state_ = WaitingForIpResponse{};
                    return nb::pending;
                }
                if (poll_result.unwrap() != nb::DeserializeResult::Ok) {
                    address_promise_.set_value(etl::nullopt);
                    state_ = WaitingForResponseMessage{};
                    return nb::ready();
                }

                address_promise_.set_value(UdpIpAddress{response.result()});
                state_ = WaitingForResponseMessage{};
            }

            if (etl::holds_alternative<WaitingForResponseMessage>(state_)) {
                return nb::pending;
            }

            if (etl::holds_alternative<Done>(state_)) {
                return nb::ready();
            }

            return nb::pending;
        }

        inline void handle_message(WifiMessage<R> &&message) {
            if (etl::holds_alternative<WifiResponseMessage>(message)) {
                if (etl::holds_alternative<WaitingForResponseMessage>(state_)) {
                    state_ = Done{};
                } else {
                    address_promise_.set_value(etl::nullopt);
                    state_ = Done{};
                }
                return;
            }

            if (etl::holds_alternative<WifiMessageReport<R>>(message) &&
                etl::get<WifiMessageReport<R>>(message).type == WifiMessageReportType::CIPSTA &&
                etl::holds_alternative<WaitingForIpResponse>(state_)) {
                auto &&report = etl::get<WifiMessageReport<R>>(message);
                state_ = ReceiveIpResponse{.readable = etl::move(report.body)};
                return;
            }
        }
    };

} // namespace media::wifi
