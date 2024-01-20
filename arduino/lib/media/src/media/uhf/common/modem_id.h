#pragma once

#include "../../address/modem_id.h"
#include <etl/array.h>
#include <logger.h>
#include <nb/serde.h>
#include <net/link.h>

namespace media::uhf {
    class AsyncModemIdDeserializer {
        nb::de::Hex<uint8_t> id_;

      public:
        inline ModemId result() const {
            return ModemId{id_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            return id_.deserialize(reader);
        }
    };

    class AsyncModemIdSerializer {
        nb::ser::Hex<uint8_t> id_;

      public:
        inline AsyncModemIdSerializer(const ModemId &id) : id_{id.get()} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writer) {
            return id_.serialize(writer);
        }

        inline constexpr uint8_t serialized_length() const {
            return id_.serialized_length();
        }
    };
} // namespace media::uhf
