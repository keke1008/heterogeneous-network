#pragma once

#include "../frame.h"
#include "./delay.h"
#include <net/neighbor.h>

namespace net::discovery::task {
    class ReceiveFrameTask {
        link::LinkFrame frame_;
        AsyncDiscoveryFrameDeserializer deserializer_{};
        etl::optional<node::Cost> delay_cost_{};

      public:
        ReceiveFrameTask(link::LinkFrame &&frame) : frame_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW, uint8_t N>
        inline nb::Poll<void> execute(
            neighbor::NeighborService<RW> &ns,
            FrameDelayPool &delay_pool,
            frame::FrameIdCache<N> &frame_id_cache,
            const node::LocalNodeInfo &local,
            util::Time &time
        ) {
            if (!delay_cost_.has_value()) {
                auto result = POLL_UNWRAP_OR_RETURN(frame_.reader.deserialize(deserializer_));
                if (result != nb::DeserializeResult::Ok) {
                    return nb::ready();
                }

                if (frame_id_cache.insert_and_check_contains(deserializer_.frame_id())) {
                    return nb::ready();
                }

                auto link_cost = ns.get_link_cost(deserializer_.sender_id());
                if (!link_cost.has_value()) {
                    return nb::ready();
                }

                delay_cost_ = *link_cost + local.cost;
            }

            POLL_UNWRAP_OR_RETURN(delay_pool.poll_pushable(time));
            delay_pool.push(deserializer_.result(), util::Duration(*delay_cost_), time);

            return nb::ready();
        }
    };
} // namespace net::discovery::task
