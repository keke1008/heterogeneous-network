#pragma once

#include "./common.h"
#include "./communicator.h"
#include "./transaction.h"
#include <nb/lock.h>
#include <nb/poll.h>
#include <nb/stream.h>

namespace media::uhf {
    template <typename Serial>
    class GetSerialNumberTransaction {
        nb::lock::Guard<ModemCommunicator<Serial>> communicator_;
        CommandTransactionPolling<Serial> command_transaction_{'S', 'N'};
        CommandPolling<Serial> command_polling_;
        ResponsePolling response_{ResponseName::GetSerialNumber};
        nb::stream::TinyByteWriter<9> serial_number_writer_;

      public:
        GetSerialNumberTransaction(nb::lock::Guard<ModemCommunicator<Serial>> &&communicator)
            : communicator_{etl::move(communicator)} {}

        nb::Poll<UHFResult<SerialNumber>> execute() {
            communicator_->execute();
            auto transaction = POLL_UNWRAP_OR_RETURN(command_transaction_.poll(communicator_));
            auto command = POLL_UNWRAP_OR_RETURN(command_polling_.poll(transaction));
            command.get().body().close();

            auto response_body = POLL_RESULT_UNWRAP_OR_RETURN(response_.poll(transaction.get()));
            nb::stream::pipe(response_body.get(), serial_number_writer_);
            auto serial_number = POLL_UNWRAP_OR_RETURN(serial_number_writer_.poll());
            return nb::Ok{SerialNumber{serial_number}};
        }
    };
} // namespace media::uhf
