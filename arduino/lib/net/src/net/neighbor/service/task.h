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

        link::Address destination_;
        etl::optional<link::MediaPortNumber> port_;
        etl::variant<etl::monostate, link::AddressType, node::NodeId> destination_node_;

      public:
        template <typename Frame>
        explicit SendFrameTask(
            Frame &&frame,
            const link::Address &destination,
            etl::optional<link::MediaPortNumber> port,
            etl::variant<etl::monostate, link::AddressType, node::NodeId> destination_node
        )
            : state_{Serialize{AsyncNeighborControlFrameSerializer{frame}}},
              destination_{destination},
              port_{port},
              destination_node_{destination_node} {}

        template <nb::ser::AsyncWritable W, typename T, uint8_t N>
        nb::Poll<void>
        execute(frame::FrameService &fs, DelaySocket<W, T, N> &socket, util::Time &time) {
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
                    link::LinkAddress(destination_), etl::move(reader), port_, time
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
        struct Deserialize {
            frame::FrameBufferReader reader;
            AsyncNeighborControlFrameDeserializer deserializer{};

            explicit Deserialize(frame::FrameBufferReader &&reader) : reader{etl::move(reader)} {}
        };

        struct Delay {
            NeighborControlFrame frame;
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
                auto result = POLL_UNWRAP_OR_RETURN(state.reader.deserialize(state.deserializer));
                if (result != nb::de::DeserializeResult::Ok) {
                    return nb::ready();
                }
                auto &&frame = state.deserializer.result();
                state_.emplace<Delay>(etl::move(frame), info.cost + frame.link_cost);
            }

            if (etl::holds_alternative<Delay>(state_)) {
                POLL_UNWRAP_OR_RETURN(socket.poll_delaying_frame_pushable());
                auto &state = etl::get<Delay>(state_);
                socket.push_delaying_frame(
                    ReceivedFrame{
                        .source = source_,
                        .port = port_,
                        .frame = etl::move(state.frame),
                    },
                    util::Duration(state.delay_cost), time
                );
            }

            return nb::ready();
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
            auto &frame = received.frame;
            auto result = list.add_neighbor(
                frame.source.node_id, received.frame.link_cost, received.source, time
            );

            if (result == AddNeighborResult::Full) {
                LOG_INFO(FLASH_STRING("Neighbor list is full"));
                return;
            } else if (result == AddNeighborResult::Updated) {
                nts.notify(notification::NeighborUpdated{
                    .neighbor_id = frame.source.node_id,
                    .link_cost = frame.link_cost,
                });
            }

            list.delay_expiration(frame.source.node_id, time);

            if (!frame.flags.should_reply_immediately()) {
                task_.emplace<etl::monostate>();
                return;
            }

            NeighborControlFrame reply_frame{
                .flags = NeighborControlFlags::KEEP_ALIVE(),
                .source = info.source,
                .source_cost = info.cost,
                .link_cost = frame.link_cost,
            };

            task_.emplace<SendFrameTask>(
                reply_frame, received.source, received.port, frame.source.node_id
            );
        } // namespace net::neighbor::service

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
                if (task.execute(fs, socket_, time).is_ready()) {
                    const auto &destination_node = task.destination_node();
                    etl::visit(
                        util::Visitor{
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
                if (task.execute(socket_, list, info, time).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }
        }

        inline nb::Poll<void> poll_send_initial_hello(
            const local::LocalNodeInfo &info,
            node::Cost link_cost,
            const link::Address &destination,
            etl::optional<link::MediaPortNumber> port
        ) {
            NeighborControlFrame frame{
                .flags = NeighborControlFlags::EMPTY(),
                .source = info.source,
                .source_cost = info.cost,
                .link_cost = link_cost,
            };

            task_.emplace<SendFrameTask>(etl::move(frame), destination, port, etl::monostate{});
            return nb::ready();
        }

        inline nb::Poll<void> poll_send_keep_alive(
            const local::LocalNodeInfo &info,
            const link::Address &destination,
            node::Cost link_cost,
            etl::variant<etl::monostate, link::AddressType, node::NodeId> destination_node
        ) {
            NeighborControlFrame frame{
                .flags = NeighborControlFlags::KEEP_ALIVE(),
                .source = info.source,
                .source_cost = info.cost,
                .link_cost = link_cost,
            };
            task_.emplace<SendFrameTask>(
                etl::move(frame), destination, etl::nullopt, destination_node
            );
            return nb::ready();
        }
    };
} // namespace net::neighbor::service
