#pragma once

#include <logger.h>
#include <nb/serde.h>
#include <stdint.h>

namespace net::node {
    class ClusterId {
        friend class AsyncClusterIdDeserializer;
        friend class AsyncClusterIdSerializer;

        uint8_t id_;

        ClusterId(uint8_t id) : id_{id} {}

      public:
        static ClusterId default_id() {
            return ClusterId{0};
        }

        inline bool operator==(const ClusterId &other) const {
            return id_ == other.id_;
        }

        inline bool operator!=(const ClusterId &other) const {
            return id_ != other.id_;
        }

        inline friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const ClusterId &id) {
            return printer << id.id_;
        }
    };

    class AsyncClusterIdDeserializer {
        nb::de::Bin<uint8_t> id_;

      public:
        inline ClusterId result() const {
            return ClusterId{id_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            return id_.deserialize(r);
        }
    };

    class AsyncClusterIdSerializer {
        nb::ser::Bin<uint8_t> id_;

      public:
        inline AsyncClusterIdSerializer(const ClusterId &id) : id_{id.id_} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            return id_.serialize(w);
        }
    };
} // namespace net::node
