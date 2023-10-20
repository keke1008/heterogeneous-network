#pragma once

#include "./frame.h"
#include <etl/circular_buffer.h>
#include <util/rand.h>

namespace net::routing::link_state {
    class FrameIdCache {
        static constexpr size_t CACHE_SIZE = 16;
        etl::circular_buffer<FrameId, CACHE_SIZE> cache_;

      public:
        FrameIdCache() = default;
        FrameIdCache(const FrameIdCache &) = delete;
        FrameIdCache(FrameIdCache &&) = delete;
        FrameIdCache &operator=(const FrameIdCache &) = delete;
        FrameIdCache &operator=(FrameIdCache &&) = delete;

        inline bool contains(const FrameId &frame_id) const {
            return etl::find(cache_.begin(), cache_.end(), frame_id) != cache_.end();
        }

        inline void push(const FrameId &frame_id) {
            cache_.push(frame_id);
        }

        FrameId generate(util::Rand &rand) {
            FrameId frame_id{rand.gen_uint8_t(0, UINT8_MAX)};
            while (contains(frame_id)) {
                frame_id = FrameId{rand.gen_uint8_t(0, UINT8_MAX)};
            }
            cache_.push(frame_id);
            return frame_id;
        }
    };
} // namespace net::routing::link_state
