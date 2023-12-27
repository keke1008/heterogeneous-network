#pragma once

#include "../frame.h"
#include <net/neighbor.h>

namespace net::discovery::task {
    class ReceiveFrameTask {
        neighbor::NeighborFrame frame_;
        AsyncDiscoveryFrameDeserializer deserializer_{};

      public:
        ReceiveFrameTask(neighbor::NeighborFrame &&frame) : frame_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW, uint8_t N>
        inline nb::Poll<etl::optional<DiscoveryFrame>> execute(
            neighbor::NeighborService<RW> &ns,
            frame::FrameIdCache<N> &frame_id_cache,
            const local::LocalNodeInfo &local,
            util::Time &time
        ) {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.reader.deserialize(deserializer_));
            if (result != nb::DeserializeResult::Ok) {
                return etl::optional<DiscoveryFrame>{};
            }

            auto &&frame = deserializer_.result();

            if (frame_id_cache.insert_and_check_contains(frame.frame_id)) {
                return etl::optional<DiscoveryFrame>{};
            }

            return etl::optional(frame);
        }
    };
} // namespace net::discovery::task
