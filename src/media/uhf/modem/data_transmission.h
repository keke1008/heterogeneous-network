#pragma once

#include "../common.h"
#include <nb/result.h>
#include <nb/stream.h>
#include <serde/hex.h>
#include <util/visitor.h>

namespace media::uhf::modem {
    enum class DataTransmissionState : uint8_t {
        SendBeforeData,
        SendData,
        SendAfterData,
        Completed,
    };

    template <typename Writer>
    class DataTransmissionCommand {
        DataTransmissionState state_{DataTransmissionState::SendBeforeData};

        size_t length_number_;

        nb::stream::TinyByteReader<1> prefix_{'@'};
        nb::stream::TinyByteReader<2> command_name_{'D', 'T'};
        nb::stream::TinyByteReader<2> length_;
        nb::Promise<common::DataWriter<Writer>> promise_;
        nb::stream::TinyByteReader<2> route_prefix_{'/', 'R'};
        nb::stream::TinyByteReader<2> destination_;
        nb::stream::TinyByteReader<2> suffix_{'\r', '\n'};

      public:
        DataTransmissionCommand(
            nb::Promise<common::DataWriter<Writer>> &&promise_,
            uint8_t length,
            common::ModemId destination
        )
            : length_number_{length},
              promise_{etl::move(promise_)},
              length_{serde::hex::serialize(length)},
              destination_{destination.serialize()} {}

        bool write_to(memory::Owned<nb::lock::Guard<Writer>> &serial) {
            switch (state_) {
            case DataTransmissionState::SendBeforeData: {
                nb::stream::pipe_readers(*serial.get(), prefix_, command_name_, length_);
                if (!length_.is_reader_closed()) {
                    break;
                }
                state_ = DataTransmissionState::SendData;
                promise_.set_value(common::DataWriter<Writer>{
                    etl::move(serial.try_create_pair().value()), length_number_});
                [[fallthrough]];
            }
            case DataTransmissionState::SendData: {
                if (serial.has_pair()) {
                    break;
                }
                state_ = DataTransmissionState::SendAfterData;
                [[fallthrough]];
            }
            case DataTransmissionState::SendAfterData: {
                nb::stream::pipe_readers(*serial.get(), route_prefix_, destination_, suffix_);
                if (!suffix_.is_reader_closed()) {
                    break;
                }
                state_ = DataTransmissionState::Completed;
                [[fallthrough]];
            }
            case DataTransmissionState::Completed: {
                return true;
            }
            }
            return false;
        }
    };

    template <typename Reader>
    class DataTransmissionResponseBody {
        nb::stream::TinyByteWriter<2> length_;
        nb::stream::TinyByteWriter<2> terminator_;

      public:
        DataTransmissionResponseBody() = default;

        nb::Poll<nb::Empty> read_from(Reader &reader) {
            nb::stream::pipe_writers(reader, length_, terminator_);
            POLL_UNWRAP_OR_RETURN(terminator_.poll());
            return nb::empty;
        }
    };
} // namespace media::uhf::modem
