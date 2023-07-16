#pragma once

#include "../common/modem_id.h"
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    class SetEquipmentIdCommand {
        nb::stream::TinyByteReader<3> header_{'@', 'E', 'I'};
        nb::stream::TinyByteReader<2> equipment_id_;
        nb::stream::TinyByteReader<2> terminator_{'\r', '\n'};

      public:
        SetEquipmentIdCommand(common::ModemId id) : equipment_id_{id.serialize()} {}

        template <typename Writer>
        inline void write_to(Writer &writer) {
            nb::stream::pipe_readers(writer, header_, equipment_id_, terminator_);
        }
    };

    class SetEquipmentIdResponseBody {
        nb::stream::TinyByteWriter<2> equipment_id_;
        nb::stream::TinyByteWriter<2> terminator_;

      public:
        template <typename Reader>
        inline nb::Poll<nb::Empty> read_from(Reader &reader) {
            nb::stream::pipe_writers(reader, equipment_id_, terminator_);
            POLL_UNWRAP_OR_RETURN(terminator_.poll());
            return nb::empty;
        }
    };
} // namespace media::uhf::modem
