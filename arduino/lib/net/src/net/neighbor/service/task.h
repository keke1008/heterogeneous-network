#pragma once

#include "../delay_socket.h"
#include "./constants.h"
#include "./frame.h"
#include "./table.h"
#include <net/link.h>
#include <net/local.h>

namespace net::neighbor::service {
    struct ReceivedFrame {
        link::MediaPortMask received_port_mask;
        link::Address source;
        NeighborControlFrame frame;
    };

    class SendFrameTask {
        struct Serialize {
            AsyncNeighborControlFrameSerializer serializer;
        };

        struct Send {
            frame::FrameBufferReader reader;
        };

        etl::variant<Serialize, Send> state_;

        link::MediaPortMask send_port_mask_;
        link::Address destination_;
        etl::variant<etl::monostate, link::AddressType, node::NodeId> destination_node_;

      public:
        explicit SendFrameTask(
            NeighborControlFrame &&frame,
            link::MediaPortMask send_port_mask,
            const link::Address &destination,
            etl::variant<etl::monostate, link::AddressType, node::NodeId> destination_node
        )
            : state_{Serialize{AsyncNeighborControlFrameSerializer{frame}}},
              send_port_mask_{send_port_mask},
              destination_{destination},
              destination_node_{destination_node} {}

        template <typename T, uint8_t N>
        nb::Poll<void>
        execute(frame::FrameService &fs, DelaySocket<T, N> &socket, util::Time &time) {
            if (etl::holds_alternative<Serialize>(state_)) {
                auto &serializer = etl::get<Serialize>(state_).serializer;
                uint8_t length = serializer.serialized_length();
                auto &&writer = POLL_MOVE_UNWRAP_OR_RETURN(fs.request_frame_writer(length));

                writer.serialize_all_at_once(serializer);
                state_ = Send{writer.create_reader()};
            }

            if (etl::holds_alternative<Send>(state_)) {
                auto &reader = etl::get<Send>(state_).reader;
                auto result = socket.poll_send_frame(
                    send_port_mask_, link::Address(destination_), etl::move(reader), time
                );
                if (!result.has_value() || result.value().is_ready()) {
                    return nb::ready();
                }
            }

            return nb::pending;
        }

        const etl::variant<etl::monostate, link::AddressType, node::NodeId> &
        destination_node() const {
            return destination_node_;
        }
    };

    class ReceiveLinkFrameTask {
        frame::FrameBufferReader reader_;
        AsyncNeighborControlFrameDeserializer deserializer_{};

        link::MediaPortMask received_port_mask_;
        link::Address source_;

      public:
        explicit ReceiveLinkFrameTask(link::LinkFrame &&frame)
            : reader_{etl::move(frame.reader)},
              received_port_mask_{frame.media_port_mask},
              source_{frame.remote} {}

        template <uint8_t N>
        nb::Poll<void> execute(
            DelaySocket<ReceivedFrame, N> &socket,
            const local::LocalNodeService &lns,
            const NeighborList &list,
            util::Time &time
        ) {
            auto result = POLL_UNWRAP_OR_RETURN(reader_.deserialize(deserializer_));
            if (result != nb::de::DeserializeResult::Ok) {
                return nb::ready();
            }

            auto &&frame = deserializer_.result();
            if (socket.poll_delaying_frame_pushable().is_pending()) {
                // フレームが溢れているので、受信したフレームを破棄する
                LOG_INFO(FLASH_STRING("neighbor service: frame buffer is full"));
                return nb::ready();
            }

            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            auto total_cost = info.cost + frame.link_cost;
            socket.push_delaying_frame(
                lns,
                ReceivedFrame{
                    .received_port_mask = received_port_mask_,
                    .source = source_,
                    .frame = etl::move(frame),
                },
                util::Duration(total_cost), time
            );

            return nb::ready();
        }
    };

    template <uint8_t DELAY_POOL_SIZE>
    class TaskExecutor {
        DelaySocket<ReceivedFrame, DELAY_POOL_SIZE> socket_;
        etl::variant<etl::monostate, SendFrameTask, ReceiveLinkFrameTask> task_;

        void on_receive_delayed_frame(
            ReceivedFrame &&received,
            notification::NotificationService &nts,
            NeighborList &list,
            const local::LocalNodeInfo &info,
            util::Time &time
        ) {
            FASSERT(etl::holds_alternative<etl::monostate>(task_));
            auto &frame = received.frame;
            auto result = list.add_neighbor(
                frame.source_node_id, received.frame.link_cost, received.source,
                received.received_port_mask, time
            );

            if (result == AddNeighborResult::Full) {
                LOG_INFO(FLASH_STRING("Neighbor list is full"));
                return;
            } else if (result == AddNeighborResult::Updated) {
                nts.notify(notification::NeighborUpdated{
                    .neighbor_id = frame.source_node_id,
                    .link_cost = frame.link_cost,
                });
            }

            list.delay_expiration(frame.source_node_id, time);

            if (!frame.flags.should_reply_immediately()) {
                task_.emplace<etl::monostate>();
                return;
            }

            NeighborControlFrame reply_frame{
                .flags = NeighborControlFlags::KEEP_ALIVE(),
                .source_node_id = info.source.node_id,
                .link_cost = frame.link_cost,
            };

            task_.emplace<SendFrameTask>(
                etl::move(reply_frame), received.received_port_mask, received.source,
                frame.source_node_id
            );
        } // namespace net::neighbor::service

        inline nb::Poll<void> poll_task_addable() {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

      public:
        explicit TaskExecutor(link::LinkSocket &&socket)
            : socket_{etl::move(socket), SOCKET_CONFIG} {}

        void execute(
            frame::FrameService &fs,
            notification::NotificationService &nts,
            const local::LocalNodeService &lns,
            NeighborList &list,
            util::Time &time
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                const auto &poll_info = lns.poll_info();
                if (poll_info.is_pending()) {
                    return;
                }
                const auto &info = poll_info.unwrap();

                nb::Poll<ReceivedFrame> &&poll_frame = socket_.poll_receive_frame(time);
                if (poll_frame.is_ready()) {
                    on_receive_delayed_frame(etl::move(poll_frame.unwrap()), nts, list, info, time);
                }

                if (etl::holds_alternative<etl::monostate>(task_)) {
                    auto &&poll_link_frame = socket_.poll_receive_link_frame(time);
                    if (poll_link_frame.is_ready()) {
                        task_.emplace<ReceiveLinkFrameTask>(etl::move(poll_link_frame.unwrap()));
                    }
                }
            }

            if (etl::holds_alternative<SendFrameTask>(task_)) {
                auto &task = etl::get<SendFrameTask>(task_);
                if (task.execute(fs, socket_, time).is_ready()) {
                    const auto &destination_node = task.destination_node();
                    etl::visit(
                        tl::Visitor{
                            [&](etl::monostate) {},
                            [&](const auto &node) { list.delay_hello_interval(node, time); },
                        },
                        destination_node
                    );
                    task_.emplace<etl::monostate>();
                }
            }

            if (etl::holds_alternative<ReceiveLinkFrameTask>(task_)) {
                auto &task = etl::get<ReceiveLinkFrameTask>(task_);
                if (task.execute(socket_, lns, list, time).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }
        }

        inline nb::Poll<void> poll_send_initial_hello(
            const local::LocalNodeInfo &info,
            link::MediaPortMask send_port_mask,
            const link::Address &destination,
            node::Cost link_cost
        ) {
            NeighborControlFrame frame{
                .flags = NeighborControlFlags::EMPTY(),
                .source_node_id = info.source.node_id,
                .link_cost = link_cost,
            };

            task_.emplace<SendFrameTask>(
                etl::move(frame), send_port_mask, destination, etl::monostate{}
            );
            return nb::ready();
        }

        inline nb::Poll<void> poll_send_keep_alive(
            const local::LocalNodeInfo &info,
            link::MediaPortMask send_port_mask,
            const link::Address &destination,
            node::Cost link_cost,
            etl::variant<etl::monostate, link::AddressType, node::NodeId> destination_node
        ) {
            NeighborControlFrame frame{
                .flags = NeighborControlFlags::KEEP_ALIVE(),
                .source_node_id = info.source.node_id,
                .link_cost = link_cost,
            };

            task_.emplace<SendFrameTask>(
                etl::move(frame), send_port_mask, destination, destination_node
            );
            return nb::ready();
        }
    };
} // namespace net::neighbor::service
