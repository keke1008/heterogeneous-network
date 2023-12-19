#pragma once

#include <etl/circular_buffer.h>
#include <nb/serde.h>
#include <stdint.h>
#include <util/rand.h>

namespace net::frame {
    class FrameId {
        template <uint8_t MAX_CACHE_SIZE>
        friend class FrameIdCache;
        friend class AsyncFrameIdDeserializer;
        friend class AsyncFrameIdSerializer;

        uint16_t id_;

        explicit FrameId(uint16_t id) : id_(id) {}

        inline uint16_t get() const {
            return id_;
        }

      public:
        static FrameId random(util::Rand &rand) {
            return FrameId{rand.gen_uint16_t()};
        }

        inline bool operator==(const FrameId &other) const {
            return id_ == other.id_;
        }

        inline bool operator!=(const FrameId &other) const {
            return id_ != other.id_;
        }
    };

    class AsyncFrameIdDeserializer {
        nb::de::Bin<uint16_t> id_;

      public:
        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return id_.deserialize(r);
        }

        inline FrameId result() const {
            return FrameId{id_.result()};
        }
    };

    class AsyncFrameIdSerializer {
        nb::ser::Bin<uint16_t> id_;

      public:
        explicit AsyncFrameIdSerializer(FrameId id) : id_{id.get()} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            return id_.serialize(w);
        }

        static constexpr uint8_t SERIALIZED_LENGTH = 2;

        constexpr inline uint8_t serialized_length() const {
            return id_.serialized_length();
        }
    };

    template <uint8_t MAX_CACHE_SIZE>
    class FrameIdCache {
        etl::circular_buffer<FrameId, MAX_CACHE_SIZE> cache_{};

        inline bool contains(FrameId id) const {
            return etl::any_of(cache_.begin(), cache_.end(), [id](FrameId other) {
                return id == other;
            });
        }

      public:
        FrameIdCache() = default;

        inline void insert(FrameId id) {
            cache_.push(id);
        }

        inline bool insert_and_check_contains(FrameId id) {
            bool result = contains(id);
            insert(id);
            return result;
        }

        inline FrameId generate(util::Rand &rand) {
            FrameId id = FrameId::random(rand);
            while (contains(id)) {
                id = FrameId::random(rand);
            }
            insert(id);
            return id;
        }
    };
} // namespace net::frame
