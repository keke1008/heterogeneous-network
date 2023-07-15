#pragma once

#include <collection/tiny_buffer.h>
#include <nb/stream.h>
#include <stdint.h>

namespace media::uhf::modem {
    enum class CommandName : uint8_t {
        CarrierSense,
        SerialNumber,
        DataTransmission,
        SetEquipmentId,
        SetDestinationId,
    };

    collection::TinyBuffer<uint8_t, 2> serialize_command_name(CommandName command_name);

    template <typename CommandBody>
    class Command {
        static_assert(nb::stream::is_stream_reader_v<CommandBody>);

        nb::stream::TinyByteReader<1> prefix_{'@'};
        nb::stream::TinyByteReader<2> command_name_;
        CommandBody body_;
        nb::stream::TinyByteReader<2> terminator_{'\r', '\n'};

      public:
        Command() = delete;

        template <typename... Ts>
        inline Command(CommandName name_, Ts &&...args)
            : command_name_{serialize_command_name(name_)},
              body_{etl::forward<Ts>(args)...} {}

        inline constexpr const nb::stream::TupleStreamReader<
            const nb::stream::TinyByteReader<1> &,
            const nb::stream::TinyByteReader<2> &,
            const CommandBody &,
            const nb::stream::TinyByteReader<2> &>
        delegate() const {
            return {prefix_, command_name_, body_, terminator_};
        }

        inline constexpr nb::stream::TupleStreamReader<
            nb::stream::TinyByteReader<1> &,
            nb::stream::TinyByteReader<2> &,
            CommandBody &,
            nb::stream::TinyByteReader<2> &>
        delegate() {
            return {prefix_, command_name_, body_, terminator_};
        }
    };

    static_assert(nb::stream::is_stream_reader_v<
                  nb::stream::StreamReaderDelegate<Command<nb::stream::EmptyStreamReader>>>);

} // namespace media::uhf::modem
