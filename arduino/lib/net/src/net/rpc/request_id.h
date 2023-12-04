#pragma once

#include <nb/serde.h>
#include <stdint.h>

namespace net::rpc {
    class RequestId {
        friend class AsyncRequestIdDeserializer;
        friend class AsyncRequestIdSerializer;

        uint16_t id_;

        explicit RequestId(uint16_t id) : id_(id) {}
    };

    class AsyncRequestIdDeserializer {
        nb::de::Bin<uint16_t> id_;

      public:
        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return id_.deserialize(r);
        }

        inline RequestId result() const {
            return RequestId{id_.result()};
        }
    };

    class AsyncRequestIdSerializer {
        nb::ser::Bin<uint16_t> id_;

      public:
        explicit AsyncRequestIdSerializer(RequestId id) : id_{id.id_} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            return id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return id_.serialized_length();
        }
    };
} // namespace net::rpc
