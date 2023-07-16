#pragma once

#include <memory/pair_shared.h>
#include <nb/lock.h>
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/serial.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    class CarrierSenseCommand {
        nb::stream::TinyByteReader<5> command_{'@', 'C', 'S', '\r', '\n'};

      public:
        CarrierSenseCommand() = default;

        template <typename Writer>
        bool write_to(Writer &writer) {
            nb::stream::pipe(command_, writer);
            return command_.is_reader_closed();
        }
    };

    class CarrierSenseResponseBody {
        nb::stream::TinyByteWriter<2> result_;
        nb::stream::TinyByteWriter<2> terminator_;

      public:
        template <typename Reader>
        nb::Poll<bool> read_from(Reader &reader) {
            nb::stream::pipe_writers(reader, result_, terminator_);
            const auto &response = POLL_UNWRAP_OR_RETURN(result_.poll());
            return response.get<0>() == 'E' && response.get<1>() == 'N';
        }
    };
} // namespace media::uhf::modem
