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
        link::Address destination_;
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

        template <uint8_t N>
        nb::Poll<void> execute(
            NeighborService &ns,
            DelaySocket<ReceivedNeighborFrame, N> &socket,
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

        struct Broadcast {
            link::AddressTypeIterator send_type{};
            link::AddressTypeSet sent_types{};
        };

        struct PollCursor {
            link::AddressTypeSet broadcast_sent_types;
        };

        struct Unicast {
            NeighborListCursor cursor;
            link::AddressTypeSet broadcast_sent_types;
        };

        etl::variant<Broadcast, PollCursor, Unicast> state_;

      public:
        explicit SendFrameBroadcastTask(
            const link::LinkSocket &link_socket,
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id
        )
            : reader_{etl::move(reader)},
              ignore_id_{ignore_id} {}

        template <typename T, uint8_t N>
        nb::Poll<void> execute(
            link::MediaService auto &ms,
            NeighborService &ns,
            DelaySocket<T, N> &socket,
            util::Time &time
        ) {
            // 1. ブロードキャスト可能なアドレスに対してブロードキャスト
            if (etl::holds_alternative<Broadcast>(state_)) {
                auto &[send_type, sent_types] = etl::get<Broadcast>(state_);
                while (send_type != send_type.end()) {
                    auto type = *send_type;
                    const auto &opt_address = ms.get_broadcast_address(type);
                    if (!opt_address.has_value()) {
                        ++send_type;
                        continue;
                    }

                    const auto &expected_poll = socket.poll_send_frame(
                        *opt_address, reader_.make_initial_clone(), etl::nullopt, time
                    );
                    if (expected_poll.has_value() && expected_poll.value().is_pending()) {
                        return nb::pending;
                    }

                    sent_types.set(type);
                    ++send_type;
                }

                ns.on_frame_sent(sent_types, time);
                auto types = sent_types; // emplace前に参照できなくなるためコピー
                state_.emplace<PollCursor>(types);
            }

            if (etl::holds_alternative<PollCursor>(state_)) {
                auto poll_cursor = etl::get<PollCursor>(state_);
                auto &&cursor = POLL_MOVE_UNWRAP_OR_RETURN(ns.poll_cursor());
                state_.emplace<Unicast>(etl::move(cursor), poll_cursor.broadcast_sent_types);
            }

            // 2. ブロードキャスト可能なアドレスを持たないNeighborに対してユニキャスト
            if (etl::holds_alternative<Unicast>(state_)) {
                auto &[cursor, broadcast_sent_types] = etl::get<Unicast>(state_);
                while (true) {
                    etl::optional<etl::reference_wrapper<const NeighborNode>> opt_neighbor =
                        ns.get_neighbor_node(cursor);
                    if (!opt_neighbor.has_value()) {
                        return nb::ready();
                    }

                    // ignore_id_が指定されている場合，そのノードには送信しない
                    const auto &neighbor = opt_neighbor->get();
                    if (ignore_id_ && neighbor.id() == *ignore_id_) {
                        cursor.advance();
                        continue;
                    }

                    // ブロードキャスト可能なアドレスを持つ場合，既に1.でブロードキャスト済みのためスキップ
                    if (neighbor.overlap_addresses_type(broadcast_sent_types)) {
                        cursor.advance();
                        continue;
                    }

                    auto addresses = neighbor.addresses();
                    if (addresses.empty()) {
                        cursor.advance();
                        continue;
                    }

                    // ブロードキャスト可能なアドレスを持たないNeighborに対してユニキャスト
                    const auto &expected_poll = socket.poll_send_frame(
                        addresses.front(), reader_.make_initial_clone(), etl::nullopt, time
                    );
                    if (expected_poll.has_value() && expected_poll.value().is_pending()) {
                        return nb::pending;
                    }
                    if (expected_poll.has_value()) {
                        ns.on_frame_sent(neighbor.id(), time);
                    }

                    cursor.advance();
                }
            }

            return nb::ready();
        }
    };

    class TaskExecutor {
        etl::variant<etl::monostate, SendFrameUnicastTask, SendFrameBroadcastTask> task_{};

        inline nb::Poll<void> poll_task_addable() {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

      public:
        template <uint8_t DELAY_POOL_SIZE>
        inline nb::Poll<neighbor::ReceivedNeighborFrame> poll_receive_frame(
            DelaySocket<ReceivedNeighborFrame, DELAY_POOL_SIZE> &socket,
            util::Time &time
        ) {
            return socket.poll_receive_frame(time);
        }

        inline etl::expected<nb::Poll<void>, SendError> poll_send_frame(
            NeighborService &ns,
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

        template <uint8_t DELAY_POOL_SIZE>
        inline nb::Poll<void> poll_send_broadcast_frame(
            NeighborService &ns,
            DelaySocket<ReceivedNeighborFrame, DELAY_POOL_SIZE> &socket,
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id
        ) {
            POLL_UNWRAP_OR_RETURN(poll_task_addable());
            auto &&cursor = POLL_MOVE_UNWRAP_OR_RETURN(ns.poll_cursor());
            task_.emplace<SendFrameBroadcastTask>(
                socket.link_socket(), etl::move(reader), ignore_id
            );
            return nb::ready();
        }

        template <uint8_t DELAY_POOL_SIZE>
        inline void execute(
            link::MediaService auto &ms,
            NeighborService &ns,
            DelaySocket<ReceivedNeighborFrame, DELAY_POOL_SIZE> &socket,
            const local::LocalNodeInfo &info,
            util::Time &time
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                nb::Poll<link::LinkReceivedFrame> &&poll_frame = socket.poll_receive_link_frame();
                if (poll_frame.is_pending()) {
                    return;
                }

                auto &frame = poll_frame.unwrap().frame;
                const auto &opt_previous_hop = ns.resolve_neighbor_node_from_address(frame.remote);
                if (!opt_previous_hop.has_value()) {
                    return; // neighborでない場合は無視する
                }

                ns.on_frame_received(opt_previous_hop->get().id(), time);

                const NeighborNode &previous_hop = opt_previous_hop->get();
                auto total_cost = previous_hop.link_cost() + info.cost;
                if (socket.poll_delaying_frame_pushable().is_pending()) {
                    return; // ディレイキューが満杯の場合は，フレームがあふれているので無視する
                }

                socket.push_delaying_frame(
                    ReceivedNeighborFrame{
                        .sender = previous_hop.id(),
                        .reader = etl::move(frame.reader),
                    },
                    util::Duration(total_cost), time
                );
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
                    etl::get<SendFrameBroadcastTask>(task_).execute(ms, ns, socket, time);
                if (poll_result.is_pending()) {
                    return;
                }
                task_.emplace<etl::monostate>();
            }
        }
    };
} // namespace net::neighbor
