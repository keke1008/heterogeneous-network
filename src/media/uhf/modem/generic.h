#pragma once

#include "./error.h"
#include <collection/tiny_buffer.h>
#include <etl/array.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>
#include <util/visitor.h>

namespace media::uhf::modem {
    class ModemId {
        collection::TinyBuffer<uint8_t, 2> value_;

      public:
        ModemId() = delete;
        ModemId(const ModemId &) = default;
        ModemId(ModemId &&) = default;
        ModemId &operator=(const ModemId &) = default;
        ModemId &operator=(ModemId &&) = default;

        ModemId(const collection::TinyBuffer<uint8_t, 2> &value) : value_{value} {}

        template <typename... Ts>
        ModemId(Ts... args) : value_{args...} {}

        bool operator==(const ModemId &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const ModemId &other) const {
            return value_ != other.value_;
        }

        const collection::TinyBuffer<uint8_t, 2> &serialize() const {
            return value_;
        }
    };

    class CommandName {
      public:
        enum Value {
            CarrierSense,
            SerialNumber,
            DataTransmission,
            EquipmentId,
            DestinationId,
        };

      private:
        Value value_;

      public:
        CommandName() = delete;
        CommandName(const CommandName &) = default;
        CommandName(CommandName &&) = default;
        CommandName &operator=(const CommandName &) = default;
        CommandName &operator=(CommandName &&) = default;

        constexpr CommandName(Value value) : value_{value} {}

        operator bool() = delete;

        bool operator==(const CommandName &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const CommandName &other) const {
            return value_ != other.value_;
        }

        collection::TinyBuffer<uint8_t, 2> to_bytes() const {
            switch (value_) {
            case CarrierSense:
                return {'C', 'S'};
            case SerialNumber:
                return {'S', 'N'};
            case DataTransmission:
                return {'D', 'T'};
            case EquipmentId:
                return {'E', 'I'};
            case DestinationId:
                return {'D', 'I'};
            }
        }
    };

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
        Command(CommandName name_, Ts &&...args)
            : command_name_{name_.to_bytes()},
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

    class ResponseName {
      public:
        enum Value {
            SerialNumber,
            DataTransmission,
            EquipmentId,
            DestinationId,
            DataReceived,
            Error,
            InformationResponse,
        };

      private:
        Value value_;

      public:
        ResponseName() = delete;
        ResponseName(const ResponseName &) = default;
        ResponseName(ResponseName &&) = default;
        ResponseName &operator=(const ResponseName &) = default;
        ResponseName &operator=(ResponseName &&) = default;

        ResponseName(Value value) : value_{value} {}

        operator bool() const = delete;

        constexpr bool operator==(const ResponseName &other) const {
            return value_ == other.value_;
        }

        constexpr bool operator!=(const ResponseName &other) const {
            return value_ != other.value_;
        }

        static etl::optional<ResponseName>
        from_bytes(const collection::TinyBuffer<uint8_t, 2> &bytes) {
            uint8_t c1 = bytes[0];
            uint8_t c2 = bytes[1];
            if (c1 == 'S' && c2 == 'N') {
                return {Value::SerialNumber};
            }
            if (c1 == 'D' && c2 == 'T') {
                return {Value::DataTransmission};
            }
            if (c1 == 'E' && c2 == 'I') {
                return {Value::EquipmentId};
            }
            if (c1 == 'D' && c2 == 'I') {
                return {Value::DestinationId};
            }
            if (c1 == 'D' && c2 == 'R') {
                return {Value::DataReceived};
            }
            if (c1 == 'E' && c2 == 'R') {
                return {Value::Error};
            }
            if (c1 == 'I' && c2 == 'R') {
                return {Value::InformationResponse};
            }
            return etl::nullopt;
        }
    };

    class ResponseHeader {
        nb::stream::DiscardingStreamWriter prefix_{1};
        nb::stream::TinyByteWriter<2> response_name_;
        nb::stream::DiscardingStreamWriter equal_sign{1};

      public:
        inline constexpr const nb::stream::TupleStreamWriter<
            const nb::stream::DiscardingStreamWriter &,
            const nb::stream::TinyByteWriter<2> &,
            const nb::stream::DiscardingStreamWriter &>
        delegate() const {
            return {prefix_, response_name_, equal_sign};
        }

        inline constexpr nb::stream::TupleStreamWriter<
            nb::stream::DiscardingStreamWriter &,
            nb::stream::TinyByteWriter<2> &,
            nb::stream::DiscardingStreamWriter &>
        delegate() {
            return {prefix_, response_name_, equal_sign};
        }

        nb::Poll<ModemResult<ResponseName>> poll() {
            auto &&name_bytes = POLL_UNWRAP_OR_RETURN(response_name_.poll());
            auto name = ResponseName::from_bytes(name_bytes);
            if (name.has_value()) {
                return nb::Ok{name.value()};
            }
            return nb::Err{ModemError::invalid_response_name(name_bytes)};
        }
    };

    static_assert(nb::stream::is_stream_writer_v<nb::stream::StreamWriterDelegate<ResponseHeader>>);
    static_assert(nb::is_future_v<ResponseHeader, ModemResult<ResponseName>>);

    template <typename ResponseBody>
    class Response {
        static_assert(nb::stream::is_stream_writer_v<ResponseBody>);

        ResponseBody body_;
        nb::stream::DiscardingStreamWriter terminator_{2};

      public:
        Response() = default;

        template <typename... Ts>
        Response(Ts &&...args) : body_{etl::forward<Ts>(args)...} {}

        inline constexpr const nb::stream::
            TupleStreamWriter<const ResponseBody &, const nb::stream::DiscardingStreamWriter &>
            delegate() const {
            return {body_, terminator_};
        }

        inline constexpr nb::stream::
            TupleStreamWriter<ResponseBody &, nb::stream::DiscardingStreamWriter &>
            delegate() {
            return {body_, terminator_};
        }

      protected:
        inline const ResponseBody &get_body() const {
            return body_;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<
                  nb::stream::StreamWriterDelegate<Response<nb::stream::EmptyStreamWriter>>>);
}; // namespace media::uhf::modem
