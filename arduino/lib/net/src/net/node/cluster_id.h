#pragma once

#include <logger.h>
#include <nb/serde.h>
#include <stdint.h>

namespace net::node {
    constexpr uint8_t NO_CLUSTER = 0;

    class ClusterId {
        friend class AsyncClusterIdDeserializer;
        friend class AsyncClusterIdSerializer;
        friend class OptionalClusterId;

        uint8_t id_;

        explicit constexpr ClusterId(uint8_t id) : id_{id} {}

      public:
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
            SERDE_DESERIALIZE_OR_RETURN(id_.deserialize(r));
            return id_.result() == NO_CLUSTER ? nb::DeserializeResult::Invalid
                                              : nb::DeserializeResult::Ok;
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

        constexpr inline uint8_t serialized_length() const {
            return id_.serialized_length();
        }
    };

    class OptionalClusterId {
        friend class AsyncOptionalClusterIdSerializer;

        uint8_t id_;

      public:
        explicit constexpr OptionalClusterId(uint8_t id) : id_{id} {}

        explicit constexpr OptionalClusterId(const ClusterId &id) : id_{id.id_} {}

        static constexpr inline OptionalClusterId no_cluster() {
            return OptionalClusterId{NO_CLUSTER};
        }

        constexpr inline bool has_value() const {
            return id_ != NO_CLUSTER;
        }

        inline constexpr ClusterId value() const {
            return ClusterId{id_};
        }

        constexpr inline bool operator==(const OptionalClusterId &other) const {
            return id_ == other.id_;
        }

        constexpr inline bool operator!=(const OptionalClusterId &other) const {
            return id_ != other.id_;
        }

        constexpr inline bool operator==(const ClusterId &other) const {
            return id_ == other.id_;
        }

        constexpr inline bool operator!=(const ClusterId &other) const {
            return id_ != other.id_;
        }

        inline friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const OptionalClusterId &id) {
            return printer << id.id_;
        }
    };

    class AsyncOptionalClusterIdDeserializer {
        nb::de::Bin<uint8_t> id_;

      public:
        inline OptionalClusterId result() const {
            return OptionalClusterId{id_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            return id_.deserialize(r);
        }
    };

    class AsyncOptionalClusterIdSerializer {
        nb::ser::Bin<uint8_t> id_;

      public:
        inline AsyncOptionalClusterIdSerializer(const OptionalClusterId &id) : id_{id.id_} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            return id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return id_.serialized_length();
        }
    };
} // namespace net::node
