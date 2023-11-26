#pragma once

#include <etl/functional.h>
#include <nb/poll.h>
#include <nb/serde.h>
#include <stdint.h>

namespace net::link::uhf {
    template <uint8_t ResponseBodySize>
    class AsyncFixedReponseDeserializer {
        nb::de::Array<nb::de::Bin<uint8_t>, 4> prefix_{4};
        nb::de::Array<nb::de::Bin<uint8_t>, ResponseBodySize> body_{ResponseBodySize};
        nb::de::Array<nb::de::Bin<uint8_t>, 2> suffix_{2};

      public:
        AsyncFixedReponseDeserializer() = default;

        inline etl::span<const uint8_t> result() const {
            return body_.result();
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(prefix_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(body_.deserialize(reader));
            return suffix_.deserialize(reader);
        }
    };

    template <typename CommandBody, uint8_t ResponseBodySize>
    class AsyncFixedExecutor {
        nb::ser::AsyncSpanSerializer<3> command_prefix_;
        CommandBody command_;
        nb::ser::AsyncSpanSerializer<2> command_suffix_{'\r', '\n'};

        AsyncFixedReponseDeserializer<ResponseBodySize> response_;

      public:
        template <typename... Args>
        AsyncFixedExecutor(uint8_t command_prefix1, uint8_t command_prefix2, Args &&...command)
            : command_prefix_{static_cast<uint8_t>('@'), command_prefix1, command_prefix2},
              command_{etl::forward<Args>(command)...} {}

        template <nb::AsyncReadableWritable RW>
            requires nb::AsyncSerializable<CommandBody, RW>
        nb::Poll<etl::span<const uint8_t>> poll(RW &rw) {
            POLL_UNWRAP_OR_RETURN(command_.serialize(rw));
            POLL_UNWRAP_OR_RETURN(response_.deserialize(rw));
            return response_.result();
        }
    };
} // namespace net::link::uhf
