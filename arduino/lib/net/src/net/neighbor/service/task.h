#pragma once

#include "../delay_socket.h"
#include "../table.h"
#include "./frame.h"
#include <net/link.h>
#include <net/local.h>

namespace net::neighbor::service {
    struct ReceivedFrame {
        link::Address source;
        link::MediaPortNumber port;
        NeighborFrame frame;
    };

    class SendFrameTask {
        struct Serialize {
            AsyncNeighborFrameSerializer serializer;
        };

        struct Send {
            frame::FrameBufferReader reader;
        };

        etl::variant<Serialize, Send> state_;

        link::Address destination_;
        etl::optional<link::MediaPortNumber> port_;

      public:
        template <typename Frame>
        explicit SendFrameTask(
            Frame &&frame,
            const link::Address &destination,
            etl::optional<link::MediaPortNumber> port
        )
            : state_{Serialize{AsyncNeighborFrameSerializer{frame}}},
              destination_{destination},
              port_{port} {}

        template <nb::ser::AsyncWritable W, typename T, uint8_t N>
        nb::Poll<void> execute(frame::FrameService &fs, DelaySocket<W, T, N> &socket) {
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
                    link::LinkAddress(destination_), etl::move(reader), port_
                );
                if (!result.has_value() || result.value().is_ready()) {
                    return nb::ready();
                }
            }

            return nb::pending;
        }
    };

    class ReceiveLinkFrameTask {
        struct Deserialize {
            frame::FrameBufferReader reader;
            AsyncNeighborFrameDeserializer deserializer{};

            explicit Deserialize(frame::FrameBufferReader &&reader) : reader{etl::move(reader)} {}
        };

        struct Delay {
            NeighborFrame frame;
            node::Cost delay_cost;
        };

        etl::variant<Deserialize, Delay> state_;
        link::Address source_;
        link::MediaPortNumber port_;

      public:
        explicit ReceiveLinkFrameTask(link::LinkReceivedFrame &&frame)
            : state_{Deserialize{etl::move(frame.frame.reader)}},
              source_{frame.frame.remote.unwrap_unicast().address},
              port_{frame.port} {}

        template <nb::AsyncReadable R, uint8_t N>
        nb::Poll<void> execute(
            DelaySocket<R, ReceivedFrame, N> &socket,
            const NeighborList &list,
            const local::LocalNodeInfo &info,
            util::Time &time
        ) {
            if (etl::holds_alternative<Deserialize>(state_)) {
                auto &state = etl::get<Deserialize>(state_);
                POLL_UNWRAP_OR_RETURN(state.reader.deserialize(state.deserializer));
                auto &&frame = state.deserializer.result();

                auto link_cost = etl::visit(
                    util::Visitor{
                        [&](const HelloFrame &frame) { return frame.link_cost; },
                        [&](const GoodbyeFrame &frame) {
                            return list.get_link_cost(frame.sender_id).value_or(node::Cost(0));
                        },
                    },
                    frame
                );
                state_.emplace<Delay>(etl::move(frame), info.cost + link_cost);
            }

            if (etl::holds_alternative<Delay>(state_)) {
                POLL_UNWRAP_OR_RETURN(socket.poll_delaying_frame_pushable());
                auto &state = etl::get<Delay>(state_);
                socket.poll_push_delaying_frame(
                    ReceivedFrame{
                        .source = source_,
                        .port = port_,
                        .frame = etl::move(state.frame),
                    },
                    util::Duration(state.delay_cost), time
                );
            }

            return nb::pending;
        }
    };

    template <nb::AsyncReadableWritable RW, uint8_t DELAY_POOL_SIZE>
    class TaskExecutor {
        DelaySocket<RW, ReceivedFrame, DELAY_POOL_SIZE> socket_;
        etl::variant<etl::monostate, SendFrameTask, ReceiveLinkFrameTask> task_;

        void on_receive_delayed_frame(
            ReceivedFrame &&received,
            notification::NotificationService &nts,
            NeighborList &list,
            const local::LocalNodeInfo &info,
            util::Time &time
        ) {
            FASSERT(etl::holds_alternative<etl::monostate>(task_));

            etl::visit(
                util::Visitor{
                    [&](const HelloFrame &frame) {
                        auto result = list.add_neighbor_link(
                            frame.source.node_id, received.source, frame.link_cost
                        );
                        if (result == AddLinkResult::NoChange) {
                            return;
                        }

                        LOG_INFO(
                            FLASH_STRING("new neigh: "), frame.source.node_id,
                            FLASH_STRING(" via "), received.source
                        );
                        nts.notify(notification::NeighborUpdated{
                            .neighbor = frame.source,
                            .neighbor_cost = frame.node_cost,
                            .link_cost = frame.link_cost,
                        });
                        if (!frame.is_ack) {
                            task_.emplace<SendFrameTask>(
                                HelloFrame{
                                    .is_ack = true,
                                    .source = info.source,
                                    .node_cost = info.cost,
                                    .link_cost = frame.link_cost
                                },
                                received.source, received.port
                            );
                        }
                    },
                    [&](const GoodbyeFrame &frame) {
                        auto result = list.remove_neighbor_node(frame.sender_id);
                        if (result == RemoveNodeResult::Disconnected) {
                            LOG_INFO(FLASH_STRING("neigh disconnect: "), frame.sender_id);
                            nts.notify(notification::NeighborRemoved{.neighbor_id = frame.sender_id}
                            );
                        }
                    },
                },
                received.frame
            );
        }

        inline nb::Poll<void> poll_task_addable() {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

      public:
        explicit TaskExecutor(link::LinkSocket<RW> &&socket) : socket_{etl::move(socket)} {}

        void execute(
            frame::FrameService &fs,
            notification::NotificationService &nts,
            const local::LocalNodeInfo &info,
            NeighborList &list,
            util::Time &time
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                nb::Poll<ReceivedFrame> &&poll_frame = socket_.poll_receive_frame(time);
                if (poll_frame.is_ready()) {
                    on_receive_delayed_frame(etl::move(poll_frame.unwrap()), nts, list, info, time);
                }

                if (etl::holds_alternative<etl::monostate>(task_)) {
                    auto &&poll_link_frame = socket_.poll_receive_link_frame();
                    if (poll_link_frame.is_ready()) {
                        task_.emplace<ReceiveLinkFrameTask>(etl::move(poll_link_frame.unwrap()));
                    }
                }
            }

            if (etl::holds_alternative<SendFrameTask>(task_)) {
                auto &task = etl::get<SendFrameTask>(task_);
                if (task.execute(fs, socket_).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }

            if (etl::holds_alternative<ReceiveLinkFrameTask>(task_)) {
                auto &task = etl::get<ReceiveLinkFrameTask>(task_);
                if (task.execute(socket_, list, info, time).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }
        }

        inline nb::Poll<void> poll_send_hello(
            const local::LocalNodeService &lns,
            const link::Address &destination,
            node::Cost link_cost,
            etl::optional<link::MediaPortNumber> port = etl::nullopt
        ) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());

            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            task_.emplace<SendFrameTask>(
                HelloFrame{
                    .is_ack = false,
                    .source = info.source,
                    .node_cost = info.cost,
                    .link_cost = link_cost,
                },
                destination, port
            );

            return nb::ready();
        }

        inline nb::Poll<void> poll_send_goodbye(
            const local::LocalNodeService &lns,
            const node::NodeId &destination,
            NeighborList &list
        ) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());

            auto addresses = list.get_addresses_of(destination);
            if (addresses.has_value()) {
                list.remove_neighbor_node(destination);
                auto &address = addresses.value().front();
                task_.emplace<SendFrameTask>(
                    GoodbyeFrame{.sender_id = info.source.node_id}, address, etl::nullopt
                );
            }

            return nb::ready();
        }
    };
} // namespace net::neighbor::service
