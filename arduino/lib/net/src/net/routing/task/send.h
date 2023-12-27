#pragma once

#include "../frame.h"
#include <net/discovery.h>

namespace net::routing::task {
    class SendFrameTask {
        struct Discovery {
            discovery::DiscoveryTask discovery_task;

            explicit Discovery(const node::Destination &destination)
                : discovery_task{destination} {}
        };

        struct SendUnicast {
            node::NodeId gateway_id;
        };

        struct SendBroadcast {
            etl::optional<node::NodeId> ignore_id;
        };

        using State = etl::variant<Discovery, SendUnicast, SendBroadcast>;

        State state_;
        frame::FrameBufferReader reader_;
        etl::optional<nb::Promise<SendResult>> promise_;

        explicit SendFrameTask(
            State &&state,
            frame::FrameBufferReader &&reader,
            etl::optional<nb::Promise<SendResult>> &&promise
        )
            : state_{etl::move(state)},
              reader_{etl::move(reader)},
              promise_{etl::move(promise)} {}

      public:
        static SendFrameTask unicast(
            const node::Destination &destination,
            frame::FrameBufferReader &&reader,
            etl::optional<nb::Promise<SendResult>> &&promise
        ) {
            return SendFrameTask{Discovery{destination}, etl::move(reader), etl::move(promise)};
        }

        static SendFrameTask broadcast(
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id,
            etl::optional<nb::Promise<SendResult>> &&promise
        ) {
            return SendFrameTask{SendBroadcast{ignore_id}, etl::move(reader), etl::move(promise)};
        }

        template <nb::AsyncReadableWritable RW, uint8_t N>
        nb::Poll<void> execute(
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            neighbor::NeighborSocket<RW, N> &socket,
            util::Time &time,
            util::Rand &rand
        ) {
            if (etl::holds_alternative<Discovery>(state_)) {
                auto &state = etl::get<Discovery>(state_);
                const auto &opt_gateway =
                    POLL_UNWRAP_OR_RETURN(state.discovery_task.execute(lns, ns, ds, time, rand));
                if (!opt_gateway.has_value()) {
                    if (promise_.has_value()) {
                        promise_->set_value(etl::unexpected<neighbor::SendError>{
                            neighbor::SendError::UnreachableNode
                        });
                    }

                    return nb::ready();
                }

                state_ = SendUnicast{.gateway_id = *opt_gateway};
            }

            if (etl::holds_alternative<SendUnicast>(state_)) {
                auto &state = etl::get<SendUnicast>(state_);
                etl::expected<nb::Poll<void>, neighbor::SendError> result =
                    socket.poll_send_frame(ns, state.gateway_id, etl::move(reader_));
                if (!result.has_value()) {
                    if (promise_.has_value()) {
                        promise_->set_value(etl::unexpected<neighbor::SendError>{result.error()});
                    }
                    return nb::ready();
                }

                POLL_UNWRAP_OR_RETURN(result.value());
                if (promise_.has_value()) {
                    promise_->set_value(etl::expected<void, neighbor::SendError>{});
                }
                return nb::ready();
            }

            if (etl::holds_alternative<SendBroadcast>(state_)) {
                auto &state = etl::get<SendBroadcast>(state_);
                POLL_UNWRAP_OR_RETURN(
                    socket.poll_send_broadcast_frame(ns, etl::move(reader_), state.ignore_id)
                );
                return nb::ready();
            }

            return nb::pending;
        }
    };
} // namespace net::routing::task
