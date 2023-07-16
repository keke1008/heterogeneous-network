#pragma once

#include "../common.h"
#include "../modem.h"
#include "./error.h"
#include <memory/pair_shared.h>
#include <nb/future.h>
#include <nb/lock.h>
#include <nb/poll.h>
#include <serde/hex.h>
#include <util/visitor.h>

namespace media::uhf::executor {
    template <typename Writer>
    class DataTransmissionExecutor {
        using ResponseHeader = modem::ResponseHeader<Writer>;
        using CarrierSenseCommand = modem::CarrierSenseCommand;
        using CarrierSenseResponseBody = modem::CarrierSenseResponseBody;
        using DataTransmissionCommand = modem::DataTransmissionCommand<Writer>;
        using DataTransmissionResponseBody = modem::DataTransmissionResponseBody<Writer>;

        memory::Owned<nb::lock::Guard<Writer>> writer_;

        nb::Promise<common::DataWriter<Writer>> promise_;
        uint8_t length_;
        common::ModemId destination_;

        etl::variant<CarrierSenseCommand, DataTransmissionCommand> command_{CarrierSenseCommand{}};
        etl::variant<ResponseHeader, CarrierSenseResponseBody, DataTransmissionResponseBody>
            response_{ResponseHeader{}};

      public:
        DataTransmissionExecutor(
            nb::lock::Guard<Writer> &&writer,
            nb::Promise<common::DataWriter<Writer>> &&promise,
            uint8_t length_,
            common::ModemId destination_
        )
            : writer_{etl::move(writer)},
              promise_{etl::move(promise)},
              length_{length_},
              destination_{destination_} {};

        nb::Poll<nb::Result<nb::Empty, nb::Empty>> execute() {
            using Poll = nb::Poll<nb::Result<nb::Empty, nb::Empty>>;

            etl::visit(
                util::Visitor{
                    [&](CarrierSenseCommand &cmd) { cmd.write_to(*writer_.get()); },
                    [&](DataTransmissionCommand &cmd) { cmd.write_to(writer_); },
                },
                command_
            );

            return etl::visit(
                util::Visitor{
                    [&](ResponseHeader res) -> Poll {
                        auto name = POLL_UNWRAP_OR_RETURN(res.read_from(*writer_.get()));
                        if (!name.has_value()) {
                            return nb::Err{nb::empty};
                        }
                        if (name.value() == modem::ResponseType::CarrierSense) {
                            response_ = CarrierSenseResponseBody{};
                        } else if (name.value() == modem::ResponseType::DataTransmission) {
                            response_ = DataTransmissionResponseBody{};
                        } else {
                            return nb::Err{nb::empty};
                        }
                        return nb::pending;
                    },
                    [&](CarrierSenseResponseBody &res) -> Poll {
                        if (POLL_UNWRAP_OR_RETURN(res.read_from(*writer_.get()))) {
                            command_ =
                                DataTransmissionCommand{etl::move(promise_), length_, destination_};
                        } else {
                            command_ = CarrierSenseCommand{};
                        }
                        response_ = ResponseHeader{};
                        return Poll{nb::pending};
                    },
                    [&](DataTransmissionResponseBody &res) -> Poll {
                        POLL_UNWRAP_OR_RETURN(res.read_from(*writer_.get()));
                        return nb::Ok{nb::empty};
                    },
                },
                response_
            );
        }
    };

    template <typename Writer>
    class DataTransmissionTask {

        nb::Promise<common::DataWriter<Writer>> promise_;
        common::ModemId destination_;
        uint8_t length_;

      public:
        DataTransmissionTask(
            nb::Promise<common::DataWriter<Writer>> &&promise,
            common::ModemId destination,
            uint8_t length
        )
            : promise_{etl::move(promise)},
              destination_{destination},
              length_{length} {}

        DataTransmissionExecutor<Writer> into_executor(nb::lock::Guard<Writer> &&writer) {
            return DataTransmissionExecutor{
                memory::Owned{etl::move(writer)}, etl::move(promise_), length_, destination_};
        }
    };
} // namespace media::uhf::executor
