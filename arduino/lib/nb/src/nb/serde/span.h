#pragma once

#include "./de.h"
#include "./ser.h"

namespace nb {
    class AsyncSpanWritable {
        etl::span<uint8_t> span_;
        uint8_t initial_length_;

      public:
        explicit AsyncSpanWritable(etl::span<uint8_t> span)
            : span_{span},
              initial_length_{static_cast<uint8_t>(span.size())} {}

        inline nb::Poll<ser::SerializeResult> poll_writable(uint8_t count) {
            return span_.size() >= count ? ser::SerializeResult::Ok
                                         : ser::SerializeResult::NotEnoughLength;
        }

        inline void write_unchecked(uint8_t src) {
            span_[0] = src;
            span_ = span_.subspan(1);
        }

        inline nb::Poll<ser::SerializeResult> write(uint8_t src) {
            SERDE_SERIALIZE_OR_RETURN(poll_writable(1));
            write_unchecked(src);
            return ser::SerializeResult::Ok;
        }
    };

    template <typename S>
    inline nb::Poll<ser::SerializeResult> serialize_span(etl::span<uint8_t> src, S &serializer)
        requires nb::ser::AsyncSerializable<S, AsyncSpanWritable>
    {
        AsyncSpanWritable dest_writable{src};
        return serializer.serialize(dest_writable);
    }

    template <typename S>
    inline nb::Poll<ser::SerializeResult> serialize_span(etl::span<uint8_t> src, S &&serializer)
        requires nb::ser::AsyncSerializable<S, AsyncSpanWritable>
    {
        AsyncSpanWritable dest_writable{src};
        return serializer.serialize(dest_writable);
    }

    class AsyncSpanReadable {
        etl::span<const uint8_t> span_;

      public:
        explicit AsyncSpanReadable(etl::span<const uint8_t> span) : span_{span} {}

        inline nb::Poll<de::DeserializeResult> poll_readable(uint8_t count) {
            return span_.size() >= count ? de::DeserializeResult::Ok
                                         : de::DeserializeResult::NotEnoughLength;
        }

        inline uint8_t read_unchecked() {
            uint8_t result = span_[0];
            span_ = span_.subspan(1);
            return result;
        }

        inline nb::Poll<de::DeserializeResult> read(uint8_t &dest) {
            SERDE_DESERIALIZE_OR_RETURN(poll_readable(1));
            dest = read_unchecked();
            return de::DeserializeResult::Ok;
        }

        inline etl::span<const uint8_t> read_span_unchecked(uint8_t count) {
            auto result = span_.subspan(0, count);
            span_ = span_.subspan(count);
            return result;
        }
    };

    template <typename D>
    inline nb::Poll<de::DeserializeResult>
    deserialize_span(etl::span<const uint8_t> src, D &&deserializer)
        requires nb::de::AsyncDeserializable<D, AsyncSpanReadable>
    {
        AsyncSpanReadable src_readable{src};
        return deserializer.deserialize(src_readable);
    }

    template <typename D>
    inline nb::Poll<de::DeserializeResult>
    deserialize_span(etl::span<const uint8_t> src, D &deserializer)
        requires nb::de::AsyncDeserializable<D, AsyncSpanReadable>
    {
        AsyncSpanReadable src_readable{src};
        return deserializer.deserialize(src_readable);
    }
} // namespace nb
