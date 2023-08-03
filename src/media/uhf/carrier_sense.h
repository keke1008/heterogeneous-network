#pragma once

#include "./communicator.h"
#include "./error.h"
#include "./transaction.h"
#include <nb/lock.h>
#include <nb/poll.h>

namespace media::uhf {
    class CarrierSenseResponseWriter {
        nb::stream::TinyByteWriter<2> response_writer_;

      public:
        template <typename Serial>
        nb::Poll<bool> poll(common::DataReader<Serial> &reader) {
            nb::stream::pipe(reader, response_writer_);
            const auto &body = POLL_UNWRAP_OR_RETURN(response_writer_.poll());
            reader.close();

            bool enabled = body.get<0>() == 'E' && body.get<1>() == 'N';
            return nb::Ready{enabled};
        }
    };

    template <typename Serial>
    class CarrierSenseTransaction {
        nb::lock::Guard<ModemCommunicator<Serial>> communicator_;
        CommandTransactionPolling<Serial> command_transaction_{'C', 'S'};
        CommandPolling<Serial> command_polling_;
        ResponsePolling response_{ResponseName::CarrierSense};
        CarrierSenseResponseWriter response_writer_;

      public:
        explicit CarrierSenseTransaction(nb::lock::Guard<ModemCommunicator<Serial>> &&communicator)
            : communicator_{etl::move(communicator)} {}

        nb::Poll<UHFResult<bool>> execute() {
            communicator_->execute();
            auto transaction = POLL_UNWRAP_OR_RETURN(command_transaction_.poll(communicator_));
            auto command = POLL_UNWRAP_OR_RETURN(command_polling_.poll(transaction.get()));
            command.get().body().close();
            auto response_body = POLL_RESULT_UNWRAP_OR_RETURN(response_.poll(transaction.get()));
            auto result = POLL_UNWRAP_OR_RETURN(response_writer_.poll(response_body.get()));
            return nb::Ok{result};
        }
    };
} // namespace media::uhf
