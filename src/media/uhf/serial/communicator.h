#pragma once

#include "media/uhf/common/data.h"
#include <etl/optional.h>
#include <memory/pair_shared.h>
#include <nb/future.h>
#include <nb/serial.h>
#include <util/tuple.h>

namespace media::uhf::header {
    template <typename Serial>
    class ModemSerialCommand {
        enum class State : uint8_t {
            BeforeBody,
            Body,
            Suffix,
            Done,
        } state_{State::BeforeBody};

        nb::stream::TinyByteReader<1> prefix_{'@'};
        nb::stream::TinyByteReader<2> command_name_;
        nb::stream::TinyByteReader<2> suffix_{'\r', '\n'};

        nb::Promise<common::DataWriter<Serial>> promise_;

      public:
        explicit ModemSerialCommand(
            uint8_t command_name1,
            uint8_t command_name2,
            nb::Promise<common::DataWriter<Serial>> &&promise
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

                state_ = State::Body;
                auto reference = etl::move(serial.try_create_pair().value());
                promise_.set_value(etl::move(common::DataWriter<Serial>(etl::move(reference))));
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

    enum class ResponseName {
        CarrierSense,
        GetSerialNumber,
        SetEquipmentId,
        DataTransmission,
        DataReceiving,
        Error,
        Information,
        Other,
    };

    ResponseName parse_response_name(uint8_t command_name1, uint8_t command_name2);

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
            Body,
            Suffix,
            Done,
        } state_{State::BeforeBody};

        nb::stream::TinyByteWriter<1> prefix_;
        nb::stream::TinyByteWriter<2> command_name_;
        nb::stream::TinyByteWriter<1> equal_sign_;
        nb::stream::TinyByteWriter<2> suffix_;

        nb::Promise<Response<Serial>> promise_;

        void set_promise(memory::Owned<Serial> &serial) {
            auto &&name_buffer = command_name_.poll().unwrap();
            auto name = parse_response_name(name_buffer.get<0>(), name_buffer.get<1>());
            auto data_reader = common::DataReader{etl::move(serial.try_create_pair().value())};
            promise_.set_value(Response{name, etl::move(data_reader)});
        }

      public:
        explicit ModemSerialResponse(nb::Promise<Response<Serial>> &&promise)
            : promise_{etl::move(promise)} {}

        bool execute(memory::Owned<Serial> &serial) {
            switch (state_) {
            case State::BeforeBody: {
                nb::stream::pipe_writers(serial.get(), prefix_, command_name_, equal_sign_);
                if (!equal_sign_.is_writer_closed()) {
                    return false;
                }

                state_ = State::Body;
                set_promise(serial);
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
                nb::stream::pipe(serial.get(), suffix_);
                if (!suffix_.is_writer_closed()) {
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

    template <typename Serial>
    class ModemCommunicator {
        etl::optional<ModemSerialCommand<Serial>> command_{etl::nullopt};
        etl::optional<ModemSerialResponse<Serial>> response_{etl::nullopt};
        memory::Owned<Serial> serial_;

      public:
        explicit ModemCommunicator(Serial &&serial) : serial_{memory::Owned{etl::move(serial)}} {}

        bool send_command(
            uint8_t command_name1,
            uint8_t command_name2,
            nb::Promise<common::DataWriter<Serial>> &&command_body,
            nb::Promise<Response<Serial>> &&response
        ) {
            if (command_.has_value() || response_.has_value()) {
                return false;
            }
            command_ =
                ModemSerialCommand<Serial>(command_name1, command_name2, etl::move(command_body));
            response_ = ModemSerialResponse<Serial>(etl::move(response));
            return true;
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
                if (response_.value().execute(serial_)) {
                    response_ = etl::nullopt;
                }
            }
        }
    };
} // namespace media::uhf
