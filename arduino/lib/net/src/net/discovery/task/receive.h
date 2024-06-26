#pragma once

#include "../frame.h"
#include <net/neighbor.h>

namespace net::discovery::task {
    class ReceiveFrameTask {
        neighbor::ReceivedNeighborFrame frame_;
        AsyncDiscoveryFrameDeserializer deserializer_{};

      public:
        ReceiveFrameTask(neighbor::ReceivedNeighborFrame &&frame) : frame_{etl::move(frame)} {}

        template <uint8_t N>
        inline nb::Poll<etl::optional<ReceivedDiscoveryFrame>> execute(
            neighbor::NeighborService &ns,
            frame::FrameIdCache<N> &frame_id_cache,
            const local::LocalNodeInfo &local,
            util::Time &time
        ) {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.reader.deserialize(deserializer_));
            if (result != nb::DeserializeResult::Ok) {
                return etl::optional<ReceivedDiscoveryFrame>{};
            }

            auto &&frame = deserializer_.received_result(frame_);
            if (frame_id_cache.insert_and_check_contains(frame.frame_id)) {
                return etl::optional<ReceivedDiscoveryFrame>{};
            }

            return etl::optional(frame);
        }
    };
} // namespace net::discovery::task
