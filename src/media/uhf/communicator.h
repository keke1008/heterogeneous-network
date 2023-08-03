#pragma once

#include "./common/data.h"
#include "./error.h"
#include <etl/optional.h>
#include <memory/pair_shared.h>
#include <nb/future.h>
#include <nb/result.h>
#include <nb/serial.h>
#include <util/tuple.h>

namespace media::uhf {
    template <typename Serial>
    class Command {
        common::DataWriter<Serial> body_;

      public:
        explicit inline Command(memory::Reference<Serial> &&body)
            : body_{common::DataWriter{etl::move(body)}} {}

        inline common::DataWriter<Serial> &body() const {
            return body_;
        }

        inline common::DataWriter<Serial> &body() {
            return body_;
        }
    };

    template <typename Serial>
    class ModemSerialCommand {
        enum class State : uint8_t {
            BeforeBody,
            CreateReference,
            Body,
            Suffix,
            Done,
        } state_{State::BeforeBody};

        nb::stream::TinyByteReader<1> prefix_{'@'};
        nb::stream::TinyByteReader<2> command_name_;
        nb::stream::TinyByteReader<2> suffix_{'\r', '\n'};

        nb::Promise<Command<Serial>> promise_;

      public:
        explicit ModemSerialCommand(
            uint8_t command_name1,
            uint8_t command_name2,
            nb::Promise<Command<Serial>> &&promise
        )
            : command_name_{command_name1, command_name2},
              promise_{etl::move(promise)} {}

        bool execute(memory::Owned<Serial> &serial) {
            switch (state_) {
            case State::BeforeBody: {
                nb::stream::pipe_readers(serial.get(), prefix_, command_name_);
                if (!command_name_.is_reader_closed()) {
                    return false;
                }

                state_ = State::CreateReference;
                [[fallthrough]];
            }
            case State::CreateReference: {
                auto reference = etl::move(serial.try_create_pair());
                if (!reference.has_value()) {
                    return false;
                }

                state_ = State::Body;
                promise_.set_value(etl::move(Command{etl::move(reference.value())}));
                [[fallthrough]];
            }
            case State::Body: {
                if (serial.has_pair()) {
                    return false;
                }

                state_ = State::Suffix;
                [[fallthrough]];
            }
            case State::Suffix: {
                nb::stream::pipe(suffix_, serial.get());
                if (!suffix_.is_reader_closed()) {
                    return false;
                }

                state_ = State::Done;
                [[fallthrough]];
            }
            case State::Done: {
                return true;
            }
            }
        }
    };

    ResponseName parse_response_name(const collection::TinyBuffer<uint8_t, 2> &name);

    template <typename Serial>
    class Response {
        ResponseName name_;
        common::DataReader<Serial> body_;

      public:
        explicit inline Response(ResponseName name, common::DataReader<Serial> &&body)
            : name_{name},
              body_{etl::move(body)} {}

        inline ResponseName name() const {
            return name_;
        }

        inline common::DataReader<Serial> &body() const {
            return body_;
        }

        inline common::DataReader<Serial> &body() {
            return body_;
        }
    };

    template <typename Serial>
    class ModemSerialResponse {
        enum class State : uint8_t {
            BeforeBody,
            CreateReference,
            Body,
            Suffix,
            Done,
            InvalidTerminator,
        } state_{State::BeforeBody};

        nb::stream::TinyByteWriter<1> prefix_;
        nb::stream::TinyByteWriter<2> command_name_;
        nb::stream::TinyByteWriter<1> equal_sign_;
        nb::stream::TinyByteWriter<2> suffix_;

        nb::Promise<Response<Serial>> promise_;

      public:
        explicit ModemSerialResponse(nb::Promise<Response<Serial>> &&promise)
            : promise_{etl::move(promise)} {}

        nb::Poll<nb::Result<nb::Empty, InvalidTerminatorError>>
        execute(memory::Owned<Serial> &serial) {
            switch (state_) {
            case State::BeforeBody: {
                nb::stream::pipe_writers(serial.get(), prefix_, command_name_, equal_sign_);
                if (!equal_sign_.is_writer_closed()) {
                    return nb::pending;
                }

                state_ = State::CreateReference;
                [[fallthrough]];
            }
            case State::CreateReference: {
                auto reference = etl::move(serial.try_create_pair());
                if (!reference.has_value()) {
                    return nb::pending;
                }

                state_ = State::Body;
                auto name = parse_response_name(command_name_.poll().unwrap());
                auto data_reader = common::DataReader{etl::move(reference.value())};
                promise_.set_value(Response{name, etl::move(data_reader)});
                [[fallthrough]];
            }
            case State::Body: {
                if (serial.has_pair()) {
                    return nb::pending;
                }

                state_ = State::Suffix;
                [[fallthrough]];
            }
            case State::Suffix: {
                nb::stream::pipe(serial.get(), suffix_);
                auto &&suffix = POLL_UNWRAP_OR_RETURN(suffix_.poll());
                if (suffix.template get<0>() != '\r' || suffix.template get<1>() != '\n') {
                    state_ = State::InvalidTerminator;
                    return nb::Err{InvalidTerminatorError{}};
                }

                state_ = State::Done;
                [[fallthrough]];
            }
            case State::Done: {
                return nb::Ok{nb::Empty{}};
            }
            case State::InvalidTerminator: {
                return nb::Err{InvalidTerminatorError{}};
            }
            }
        }
    };

    template <typename Serial>
    class CommandTransaction {
        nb::Future<Command<Serial>> command_;
        nb::Future<Response<Serial>> response_;

      public:
        explicit CommandTransaction(
            nb::Future<Command<Serial>> &&command,
            nb::Future<Response<Serial>> &&response
        )
            : command_{etl::move(command)},
              response_{etl::move(response)} {}

        inline nb::Future<Command<Serial>> &command() {
            return command_;
        }

        inline nb::Future<Response<Serial>> &response() {
            return response_;
        }
    };

    template <typename Serial>
    class ModemCommunicator {
        etl::optional<ModemSerialCommand<Serial>> command_{etl::nullopt};
        etl::optional<ModemSerialResponse<Serial>> response_{etl::nullopt};
        memory::Owned<Serial> serial_;

      public:
        explicit ModemCommunicator(Serial &&serial) : serial_{memory::Owned{etl::move(serial)}} {}

        etl::optional<CommandTransaction<Serial>>
        send_command(uint8_t command_name1, uint8_t command_name2) {
            if (command_.has_value() || response_.has_value()) {
                return etl::nullopt;
            }

            auto [c_future, c_promise] = nb::make_future_promise_pair<Command<Serial>>();
            command_ = ModemSerialCommand(command_name1, command_name2, etl::move(c_promise));
            auto [r_future, r_promise] = nb::make_future_promise_pair<Response<Serial>>();
            response_ = ModemSerialResponse(etl::move(r_promise));

            return etl::optional(CommandTransaction{etl::move(c_future), etl::move(r_future)});
        }

        etl::optional<nb::Future<Response<Serial>>> try_receive_response() {
            if (command_.has_value() || response_.has_value() || !serial_.get().is_readable()) {
                return etl::nullopt;
            }

            auto [future, promise] = nb::make_future_promise_pair<Response<Serial>>();
            response_ = ModemSerialResponse<Serial>(etl::move(promise));
            return etl::optional(etl::move(future));
        }

        void execute() {
            if (command_.has_value()) {
                if (command_.value().execute(serial_)) {
                    command_ = etl::nullopt;
                }
            }
            if (response_.has_value()) {
                if (response_.value().execute(serial_).is_ready()) {
                    response_ = etl::nullopt;
                }
            }
        }
    };
} // namespace media::uhf
