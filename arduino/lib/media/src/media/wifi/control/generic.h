#pragma once

#include "../message.h"
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/serde.h>

namespace media::wifi {
    template <nb::AsyncReadable R, nb::AsyncWritable W, typename Command>
    class GenericEmptyResponseControl {
        struct SendCommand {
            memory::Static<W> &writable;
            Command serializer;

            template <typename... Args>
            SendCommand(memory::Static<W> &writable, Args &&...args)
                : writable{writable},
                  serializer{etl::forward<Args>(args)...} {}
        };

        struct WaitingForResponse {};

        struct Done {};

        nb::Promise<bool> promise_;

        etl::variant<SendCommand, WaitingForResponse, Done> state_;

        WifiResponseMessage expected_response_;

      public:
        template <typename... Args>
        explicit GenericEmptyResponseControl(
            memory::Static<W> &writable,
            nb::Promise<bool> &&promise,
            WifiResponseMessage expected_response,
            Args &&...args
        )
            : promise_{etl::move(promise)},
              state_{
                  etl::in_place_type<SendCommand>,
                  writable,
                  etl::forward<Args>(args)...,
              },
              expected_response_{expected_response} {}

        nb::Poll<void> execute() {
            if (etl::holds_alternative<SendCommand>(state_)) {
                SendCommand &send = etl::get<SendCommand>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(send.serializer.serialize(*send.writable));
                if (result != nb::SerializeResult::Ok) {
                    return nb::ready();
                }

                state_ = WaitingForResponse{};
            }

            if (etl::holds_alternative<WaitingForResponse>(state_)) {

                return nb::pending;
            }

            if (etl::holds_alternative<Done>(state_)) {
                return nb::ready();
            }

            return nb::pending;
        }

        inline void handle_message(WifiMessage<R> &&message) {
            if (etl::holds_alternative<WaitingForResponse>(state_) &&
                etl::holds_alternative<WifiResponseMessage>(message)) {
                auto response = etl::get<WifiResponseMessage>(message);
                promise_.set_value(response == expected_response_);
            } else {
                promise_.set_value(false);
            }

            state_ = Done{};
        }
    };

    template <nb::AsyncReadable R, nb::AsyncWritable W, typename Command>
    class GenericEmptyResponseSyncControl {

        struct SendCommand {
            memory::Static<W> &writable;
            Command serializer;
        };

        struct WaitingForResponse {};

        struct Done {};

        struct Failed {};

        etl::variant<SendCommand, WaitingForResponse, Done, Failed> state_;

        WifiResponseMessage expected_response_;

      public:
        template <typename... Args>
        explicit GenericEmptyResponseSyncControl(
            memory::Static<W> &writable,
            WifiResponseMessage expected_response,
            Args &&...args
        )
            : state_{SendCommand{
                  .writable{writable},
                  .serializer = Command{etl::forward<Args>(args)...},
              }},
              expected_response_{expected_response} {}

        nb::Poll<bool> execute() {
            if (etl::holds_alternative<SendCommand>(state_)) {
                SendCommand &send = etl::get<SendCommand>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(send.serializer.serialize(*send.writable));
                if (result != nb::SerializeResult::Ok) {
                    return nb::ready(false);
                }

                state_ = WaitingForResponse{};
            }

            if (etl::holds_alternative<WaitingForResponse>(state_)) {
                return nb::pending;
            }

            if (etl::holds_alternative<Done>(state_)) {
                return nb::ready(true);
            }

            if (etl::holds_alternative<Failed>(state_)) {
                return nb::ready(false);
            }

            return nb::pending;
        }

        void handle_message(WifiMessage<R> &&message) {
            if (etl::holds_alternative<WaitingForResponse>(state_) &&
                etl::holds_alternative<WifiResponseMessage>(message)) {
                auto response = etl::get<WifiResponseMessage>(message);
                if (response == expected_response_) {
                    state_ = Done{};
                } else {
                    state_ = Failed{};
                }
            } else {
                state_ = Failed{};
            }
        }
    };
} // namespace media::wifi
