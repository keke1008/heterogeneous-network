#pragma once

#include "../constants.h"
#include "../frame.h"
#include <net/neighbor.h>

namespace net::discovery::task {
    class ReceiveFrameTask {
        neighbor::NeighborFrame frame_;
        AsyncDiscoveryFrameDeserializer deserializer_{};

      public:
        ReceiveFrameTask(neighbor::NeighborFrame &&frame) : frame_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW, uint8_t N>
        inline nb::Poll<void> execute(
            neighbor::NeighborService<RW> &ns,
            nb::DelayPool<DiscoveryFrame, FRAME_DELAY_POOL_SIZE> &delay_pool,
            frame::FrameIdCache<N> &frame_id_cache,
            const local::LocalNodeInfo &local,
            util::Time &time
        ) {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.reader.deserialize(deserializer_));
            if (result != nb::DeserializeResult::Ok) {
                return nb::ready();
            }

            auto &&frame = deserializer_.result();

            if (frame_id_cache.insert_and_check_contains(frame.frame_id)) {
                return nb::ready();
            }

            auto link_cost = ns.get_link_cost(frame.sender.node_id);
            if (!link_cost.has_value()) {
                return nb::ready();
            }

            // プールが満杯の場合はフレームを無視する
            if (delay_pool.poll_pushable().is_ready()) {
                auto delay_cost = *link_cost + local.cost;
                delay_pool.push(etl::move(frame), util::Duration(delay_cost), time);
            }

            return nb::ready();
        }
    };
} // namespace net::discovery::task
