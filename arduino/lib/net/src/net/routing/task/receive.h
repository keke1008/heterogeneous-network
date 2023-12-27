#pragma once

#include "../frame.h"
#include <net/neighbor.h>

namespace net::routing::task {
    class ReceiveFrameTask {
        neighbor::NeighborFrame frame_;
        AsyncRoutingFrameHeaderDeserializer deserializer_{};

      public:
        explicit ReceiveFrameTask(neighbor::NeighborFrame &&frame) : frame_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW, uint8_t N, uint8_t M>
        inline nb::Poll<void> execute(
            neighbor::NeighborService<RW> &ns,
            nb::DelayPool<RoutingFrame, N> &delay_pool,
            frame::FrameIdCache<M> &frame_id_cache,
            const local::LocalNodeInfo &local,
            util::Time &time
        ) {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.reader.deserialize(deserializer_));
            if (result != nb::DeserializeResult::Ok) {
                return nb::ready();
            }

            auto &&frame = deserializer_.as_frame(frame_.reader.subreader());

            if (frame_id_cache.insert_and_check_contains(frame.frame_id)) {
                return nb::ready();
            }

            auto link_cost = ns.get_link_cost(frame.previous_hop);
            if (!link_cost.has_value()) {
                return nb::ready();
            }
            auto delay_cost = *link_cost + local.cost;

            // プールが満杯の場合はフレームを無視する
            if (delay_pool.poll_pushable().is_ready()) {
                delay_pool.push(etl::move(frame), util::Duration(delay_cost), time);
            }

            return nb::ready();
        }
    };
} // namespace net::routing::task
