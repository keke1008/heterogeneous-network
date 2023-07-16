#pragma once

#include <nb/poll.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    class SerialNumber {
        collection::TinyBuffer<uint8_t, 9> serial_number_;

      public:
        SerialNumber(collection::TinyBuffer<uint8_t, 9> serial_number)
            : serial_number_{serial_number} {}

        const collection::TinyBuffer<uint8_t, 9> &get() const {
            return serial_number_;
        }

        inline constexpr bool operator==(const SerialNumber &other) const {
            return serial_number_ == other.serial_number_;
        }

        inline constexpr bool operator!=(const SerialNumber &other) const {
            return serial_number_ != other.serial_number_;
        }
    };

    class GetSerialNumberCommand {
        nb::stream::TinyByteReader<5> command_{'@', 'S', 'N', '\r', '\n'};

      public:
        GetSerialNumberCommand() = default;

        template <typename Writer>
        inline constexpr void write_to(Writer &writer) {
            nb::stream::pipe(command_, writer);
        }
    };

    class GetSerialNumberResponseBody {
        nb::stream::TinyByteWriter<9> serial_number_;
        nb::stream::TinyByteWriter<2> terminator_;

      public:
        template <typename Reader>
        nb::Poll<SerialNumber> read_from(Reader &reader) {
            nb::stream::pipe_writers(reader, serial_number_, terminator_);
            POLL_UNWRAP_OR_RETURN(terminator_.poll());
            auto serial_number = POLL_UNWRAP_OR_RETURN(serial_number_.poll());
            return SerialNumber{etl::move(serial_number)};
        }
    };
} // namespace media::uhf::modem
