#pragma once

#include "../delay_socket.h"
#include "../service.h"
#include "./frame.h"

namespace net::neighbor {
    enum struct SendError : uint8_t {
        SupportedMediaNotFound,
        UnreachableNode,
    };

    class SendFrameUnicastTask {
        link::LinkAddress destination_;
        frame::FrameBufferReader reader_;
        node::NodeId destination_node_id_;

      public:
        explicit SendFrameUnicastTask(
            link::Address destination,
            frame::FrameBufferReader &&reader,
            node::NodeId destination_node_id
        )
            : destination_{destination},
              reader_{etl::move(reader)},
              destination_node_id_{destination_node_id} {}

        template <nb::AsyncReadableWritable RW, uint8_t N>
        nb::Poll<void> execute(
            NeighborService<RW> &ns,
            DelaySocket<RW, NeighborFrame, N> &socket,
            util::Time &time
        ) {
            auto result =
                socket.poll_send_frame(destination_, etl::move(reader_), etl::nullopt, time);
            if (!result.has_value()) {
                return nb::ready();
            }

            POLL_UNWRAP_OR_RETURN(result.value());
            ns.on_frame_sent(destination_node_id_, time);
            return nb::ready();
        }
    };

    class SendFrameBroadcastTask {
        frame::FrameBufferReader reader_;
        etl::optional<node::NodeId> ignore_id_;

        NeighborListCursor cursor_;

        link::AddressTypeSet broadcast_types_;
        link::AddressTypeSet broadcast_unreached_types_;

      public:
        template <nb::AsyncReadableWritable RW>
        explicit SendFrameBroadcastTask(
            const link::LinkSocket<RW> &link_socket,
            NeighborListCursor &&cursor,
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id
        )
            : reader_{etl::move(reader)},
              ignore_id_{ignore_id},
              cursor_{etl::move(cursor)} {
            broadcast_types_ = link_socket.broadcast_supported_address_types();
            broadcast_unreached_types_ = broadcast_types_;
        }

        template <nb::AsyncReadableWritable RW, typename T, uint8_t N>
        nb::Poll<void>
        execute(NeighborService<RW> &ns, DelaySocket<RW, T, N> &socket, util::Time &time) {
            // 1. ブロードキャスト可能なアドレスに対してブロードキャスト
            while (broadcast_unreached_types_.any()) {
                auto type = *broadcast_unreached_types_.pick();
                const auto &expected_poll = socket.poll_send_frame(
                    link::LinkAddress{link::LinkBroadcastAddress{type}},
                    reader_.make_initial_clone(), etl::nullopt, time
                );
                if (expected_poll.has_value() && expected_poll.value().is_pending()) {
                    return nb::pending;
                }
                broadcast_unreached_types_.reset(type);
            }
            ns.on_frame_sent(broadcast_types_, time);

            // 2. ブロードキャスト可能なアドレスを持たないNeighborに対してユニキャスト
            while (true) {
                etl::optional<etl::reference_wrapper<const NeighborNode>> opt_neighbor =
                    ns.get_neighbor_node(cursor_);
                if (!opt_neighbor.has_value()) {
                    return nb::ready();
                }

                // ignore_id_が指定されている場合，そのノードには送信しない
                const auto &neighbor = opt_neighbor->get();
                if (ignore_id_ && neighbor.id() == *ignore_id_) {
                    cursor_.advance();
                    continue;
                }

                // ブロードキャスト可能なアドレスを持つ場合，既に1.でブロードキャスト済みのためスキップ
                if (neighbor.overlap_addresses_type(broadcast_types_)) {
                    cursor_.advance();
                    continue;
                }

                auto addresses = neighbor.addresses();
                if (addresses.empty()) {
                    cursor_.advance();
                    continue;
                }

                // ブロードキャスト可能なアドレスを持たないNeighborに対してユニキャスト
                const auto &expected_poll = socket.poll_send_frame(
                    link::LinkAddress{link::LinkUnicastAddress{addresses.front()}},
                    reader_.make_initial_clone(), etl::nullopt, time
                );
                if (expected_poll.has_value() && expected_poll.value().is_pending()) {
                    return nb::pending;
                }
                if (expected_poll.has_value()) {
                    ns.on_frame_sent(neighbor.id(), time);
                }

                cursor_.advance();
            }

            return nb::ready();
        }
    };

    class ReceiveLinkFrameTask {
        struct Deserialize {
            link::LinkFrame frame;
            AsyncNeighborFrameHeaderDeserializer deserializer{};

            explicit Deserialize(link::LinkFrame &&frame) : frame{etl::move(frame)} {}
        };

        struct Delay {
            NeighborFrame frame;
            node::Cost delay;
        };

        etl::variant<Deserialize, Delay> state_;

      public:
        explicit ReceiveLinkFrameTask(link::LinkReceivedFrame &&frame)
            : state_{Deserialize{etl::move(frame.frame)}} {}

        template <nb::AsyncReadableWritable RW, uint8_t N>
        nb::Poll<void> execute(
            neighbor::NeighborService<RW> &ns,
            const local::LocalNodeInfo &info,
            DelaySocket<RW, NeighborFrame, N> &socket,
            util::Time &time
        ) {
            if (etl::holds_alternative<Deserialize>(state_)) {
                auto &&[link_frame, deserializer] = etl::get<Deserialize>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(link_frame.reader.deserialize(deserializer));
                if (result != nb::DeserializeResult::Ok) {
                    return nb::ready();
                }

                auto &&frame = deserializer.result();
                ns.on_frame_received(frame.sender.node_id, time);

                auto opt_cost = ns.get_link_cost(frame.sender.node_id);
                if (!opt_cost.has_value()) {
                    return nb::ready();
                }
                state_.emplace<Delay>(
                    deserializer.result().to_frame(etl::move(link_frame.reader)), *opt_cost
                );
            }

            if (etl::holds_alternative<Delay>(state_)) {
                auto &&[frame, cost] = etl::get<Delay>(state_);
                POLL_UNWRAP_OR_RETURN(socket.poll_delaying_frame_pushable());
                socket.push_delaying_frame(etl::move(frame), util::Duration(cost), time);
            }

            return nb::ready();
        }
    };

    class TaskExecutor {
        etl::variant<
            etl::monostate,
            SendFrameUnicastTask,
            SendFrameBroadcastTask,
            ReceiveLinkFrameTask>
            task_{};

        inline nb::Poll<void> poll_task_addable() {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

      public:
        template <nb::AsyncReadableWritable RW, uint8_t DELAY_POOL_SIZE>
        inline nb::Poll<neighbor::NeighborFrame> poll_receive_frame(
            DelaySocket<RW, NeighborFrame, DELAY_POOL_SIZE> &socket,
            util::Time &time
        ) {
            return socket.poll_receive_frame(time);
        }

        template <nb::AsyncReadableWritable RW>
        inline etl::expected<nb::Poll<void>, SendError> poll_send_frame(
            NeighborService<RW> &ns,
            const node::NodeId &destination,
            frame::FrameBufferReader &&reader
        ) {
            if (poll_task_addable().is_pending()) {
                return etl::expected<nb::Poll<void>, SendError>{nb::pending};
            }

            etl::optional<etl::span<const link::Address>> opt_addresses =
                ns.get_address_of(destination);
            if (!opt_addresses.has_value()) {
                return etl::expected<nb::Poll<void>, SendError>{
                    etl::unexpected<SendError>{SendError::UnreachableNode}
                };
            }

            task_.emplace<SendFrameUnicastTask>(
                opt_addresses->front(), etl::move(reader), destination
            );
            return etl::expected<nb::Poll<void>, SendError>{nb::ready()};
        }

        template <nb::AsyncReadableWritable RW, uint8_t DELAY_POOL_SIZE>
        inline nb::Poll<void> poll_send_broadcast_frame(
            NeighborService<RW> &ns,
            DelaySocket<RW, NeighborFrame, DELAY_POOL_SIZE> &socket,
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id
        ) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            auto &&cursor = POLL_MOVE_UNWRAP_OR_RETURN(ns.poll_cursor());
            task_.emplace<SendFrameBroadcastTask>(
                socket.link_socket(), etl::move(cursor), etl::move(reader), ignore_id
            );
            return nb::ready();
        }

        template <nb::AsyncReadableWritable RW, uint8_t DELAY_POOL_SIZE>
        inline void execute(
            NeighborService<RW> &ns,
            DelaySocket<RW, NeighborFrame, DELAY_POOL_SIZE> &socket,
            const local::LocalNodeInfo &info,
            util::Time &time
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                nb::Poll<link::LinkReceivedFrame> &&poll_frame = socket.poll_receive_link_frame();
                if (poll_frame.is_pending()) {
                    return;
                }

                task_.emplace<ReceiveLinkFrameTask>(etl::move(poll_frame.unwrap()));
            }

            if (etl::holds_alternative<SendFrameUnicastTask>(task_)) {
                auto &&poll_result =
                    etl::get<SendFrameUnicastTask>(task_).execute(ns, socket, time);
                if (poll_result.is_pending()) {
                    return;
                }
                task_.emplace<etl::monostate>();
            }

            if (etl::holds_alternative<SendFrameBroadcastTask>(task_)) {
                auto &&poll_result =
                    etl::get<SendFrameBroadcastTask>(task_).execute(ns, socket, time);
                if (poll_result.is_pending()) {
                    return;
                }
                task_.emplace<etl::monostate>();
            }

            if (etl::holds_alternative<ReceiveLinkFrameTask>(task_)) {
                auto &task = etl::get<ReceiveLinkFrameTask>(task_);
                auto &&poll_result = task.execute(ns, info, socket, time);
                if (poll_result.is_pending()) {
                    return;
                }
                task_.emplace<etl::monostate>();
            }
        }
    };
} // namespace net::neighbor
