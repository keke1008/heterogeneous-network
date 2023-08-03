#pragma once

#include "./common/data.h"
#include "./communicator.h"
#include "./error.h"
#include <etl/functional.h>
#include <nb/lock.h>
#include <nb/poll.h>

namespace media::uhf {
    template <typename Serial>
    class CommandTransactionPolling {
        uint8_t command_name1_;
        uint8_t command_name2_;

        etl::optional<CommandTransaction<Serial>> command_transaction_{etl::nullopt};

      public:
        explicit CommandTransactionPolling(uint8_t command_name1, uint8_t command_name2)
            : command_name1_{command_name1},
              command_name2_{command_name2} {}

        nb::Poll<etl::reference_wrapper<CommandTransaction<Serial>>>
        poll(nb::lock::Guard<ModemCommunicator<Serial>> &communicator) {
            if (!command_transaction_.has_value()) {
                command_transaction_ = communicator->send_command(command_name1_, command_name2_);
            }
            if (command_transaction_.has_value()) {
                return nb::Ready{etl::ref(command_transaction_.value())};
            }
            return nb::pending;
        }
    };

    template <typename Serial>
    nb::Poll<etl::reference_wrapper<CommandTransaction<Serial>>> poll_command_transaction(
        CommandTransactionPolling<Serial> &polling,
        nb::lock::Guard<ModemCommunicator<Serial>> &communicator
    ) {
        return polling.poll(communicator);
    }

    template <typename Serial>
    class CommandPolling {
      public:
        nb::Poll<etl::reference_wrapper<Command<Serial>>>
        poll(CommandTransaction<Serial> &transaction) {
            auto command_ref = POLL_UNWRAP_OR_RETURN(transaction.command().poll());
            return nb::Ready{command_ref};
        }
    };

    class ResponsePolling {
        ResponseName expected_name_;

      public:
        explicit ResponsePolling(ResponseName expected_name) : expected_name_{expected_name} {}

        template <typename Serial>
        nb::Poll<UHFResult<etl::reference_wrapper<common::DataReader<Serial>>>>
        poll(CommandTransaction<Serial> &transaction) {
            auto response_ref = POLL_UNWRAP_OR_RETURN(transaction.response().poll());
            auto &response = response_ref.get();
            if (response.name() != expected_name_) {
                return nb::Err{UHFError{UnhandlableResponseNameError{response.name()}}};
            }
            return nb::Ok{etl::ref(response.body())};
        }
    };
} // namespace media::uhf
