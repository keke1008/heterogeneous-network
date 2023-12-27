#pragma once

#include "../cache.h"
#include "../frame.h"
#include <net/neighbor.h>

namespace net::discovery::task {
    class SendFrameTask {
        struct UnicastDestination {
            node::NodeId node_id;
        };

        struct BroadcastDestination {
            etl::optional<node::NodeId> ignore_node_id;
        };

        using DiscoveryDestination = etl::variant<UnicastDestination, BroadcastDestination>;

        struct CreateFrame {
            AsyncDiscoveryFrameSerializer serializer;
        };

        struct SendFrame {
            frame::FrameBufferReader reader;
        };

        DiscoveryDestination gateway_;
        etl::variant<CreateFrame, SendFrame> state_;

        SendFrameTask(const DiscoveryDestination &gateway, const DiscoveryFrame &frame)
            : gateway_{gateway},
              state_{CreateFrame{AsyncDiscoveryFrameSerializer{frame}}} {}

      public:
        template <uint8_t FRAME_ID_CACHE_SIZE>
        static SendFrameTask request(
            const node::Destination &destination,
            const local::LocalNodeInfo &local,
            frame::FrameIdCache<FRAME_ID_CACHE_SIZE> &frame_id_cache,
            util::Rand &rand
        ) {
            return SendFrameTask{
                BroadcastDestination{etl::nullopt},
                DiscoveryFrame::request(frame_id_cache.generate(rand), local.source, destination),
            };
        }

        template <uint8_t FRAME_ID_CACHE_SIZE>
        static SendFrameTask reply(
            const DiscoveryFrame &received_frame,
            const local::LocalNodeInfo &local,
            frame::FrameIdCache<FRAME_ID_CACHE_SIZE> &frame_id_cache,
            util::Rand &rand
        ) {
            return SendFrameTask{
                UnicastDestination{received_frame.sender.node_id},
                received_frame.reply(frame_id_cache.generate(rand), local.source)
            };
        }

        static SendFrameTask reply_by_cache(
            const DiscoveryFrame &received_frame,
            const local::LocalNodeInfo &local,
            frame::FrameIdCache<FRAME_ID_CACHE_SIZE> &frame_id_cache,
            const CacheValue &cache,
            util::Rand &rand
        ) {
            return SendFrameTask{
                UnicastDestination{received_frame.sender.node_id},
                received_frame.reply_by_cache(
                    frame_id_cache.generate(rand), local.source, cache.total_cost
                )
            };
        }

        static SendFrameTask repeat_unicast(
            const DiscoveryFrame &received_frame,
            const local::LocalNodeInfo &local,
            TotalCost total_cost,
            const node::NodeId &gateway
        ) {
            return SendFrameTask{
                UnicastDestination{gateway},
                received_frame.repeat(local.source, total_cost),
            };
        }

        static SendFrameTask repeat_broadcast(
            const DiscoveryFrame &received_frame,
            const local::LocalNodeInfo &local,
            TotalCost total_cost
        ) {
            return SendFrameTask{
                BroadcastDestination{received_frame.sender.node_id},
                received_frame.repeat(local.source, total_cost),
            };
        }

        template <nb::AsyncReadableWritable RW, uint8_t N>
        nb::Poll<void> execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            neighbor::NeighborSocket<RW, N> &socket
        ) {
            if (etl::holds_alternative<CreateFrame>(state_)) {
                auto &serializer = etl::get<CreateFrame>(state_).serializer;
                uint8_t length = serializer.serialized_length();

                auto &&writer =
                    POLL_MOVE_UNWRAP_OR_RETURN(socket.poll_frame_writer(fs, lns, length));
                writer.serialize_all_at_once(serializer);
                state_.emplace<SendFrame>(writer.create_reader());
            }

            if (etl::holds_alternative<SendFrame>(state_)) {
                auto &reader = etl::get<SendFrame>(state_).reader;
                return etl::visit<nb::Poll<void>>(
                    util::Visitor{
                        [&](const UnicastDestination &destination) -> nb::Poll<void> {
                            etl::expected<nb::Poll<void>, neighbor::SendError> result =
                                socket.poll_send_frame(ns, destination.node_id, etl::move(reader));
                            return (!result.has_value() || result.value().is_ready()) ? nb::ready()
                                                                                      : nb::pending;
                        },
                        [&](const BroadcastDestination &destination) -> nb::Poll<void> {
                            return socket.poll_send_broadcast_frame(
                                ns, etl::move(reader), destination.ignore_node_id
                            );
                        }
                    },
                    gateway_
                );
            }

            return nb::pending;
        }
    };
} // namespace net::discovery::task
