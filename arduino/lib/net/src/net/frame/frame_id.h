#pragma once

#include <etl/circular_buffer.h>
#include <nb/buf.h>
#include <stdint.h>
#include <util/rand.h>

namespace net::frame {
    class FrameId {
        uint16_t id_;

      public:
        explicit FrameId(uint16_t id) : id_(id) {}

        static FrameId random(util::Rand &rand) {
            return FrameId{rand.gen_uint16_t()};
        }

        inline uint16_t get() const {
            return id_;
        }

        inline bool operator==(const FrameId &other) const {
            return id_ == other.id_;
        }

        inline bool operator!=(const FrameId &other) const {
            return id_ != other.id_;
        }

        inline uint8_t serialized_length() const {
            return sizeof(id_);
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(nb::buf::FormatBinary<uint16_t>(id_));
        }
    };

    struct FrameIdDeserializer {
        inline FrameId parse(nb::buf::BufferSplitter &splitter) {
            return FrameId{splitter.parse<nb::buf::BinParser<uint16_t>>()};
        }
    };

    template <uint8_t MAX_CACHE_SIZE>
    class FrameIdCache {
        etl::circular_buffer<FrameId, MAX_CACHE_SIZE> cache_{};

      public:
        FrameIdCache() = default;

        inline void add(FrameId id) {
            cache_.push_back(id);
        }

        inline bool contains(FrameId id) const {
            return etl::any_of(cache_.begin(), cache_.end(), [id](FrameId other) {
                return id == other;
            });
        }

        inline FrameId generate(util::Rand &rand) {
            FrameId id = FrameId::random(rand);
            while (contains(id)) {
                id = FrameId::random(rand);
            }
            add(id);
            return id;
        }
    };
} // namespace net::frame
