#pragma once

#include "./service.h"
#include <net/link.h>

namespace net::neighbor {
    enum struct SendError : uint8_t {
        SupportedMediaNotFound,
        UnreachableNode,
    };

    class SendBroadcastTask {
        frame::FrameBufferReader reader_;
        etl::optional<node::NodeId> ignore_id_;

        NeighborListCursor cursor_;

        link::AddressTypeSet broadcast_types_;
        link::AddressTypeSet broadcast_unreached_types_;

      public:
        template <nb::AsyncReadableWritable RW>
        explicit SendBroadcastTask(
            link::LinkSocket<RW> &link_socket,
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

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(NeighborService<RW> &ns, link::LinkSocket<RW> &link_socket) {
            // 1. ブロードキャスト可能なアドレスに対してブロードキャスト
            while (broadcast_unreached_types_.any()) {
                auto type = *broadcast_unreached_types_.pick();
                const auto &expected_poll = link_socket.poll_send_frame(
                    link::LinkAddress{link::LinkBroadcastAddress{type}},
                    reader_.make_initial_clone()
                );
                if (expected_poll.has_value() && expected_poll.value().is_pending()) {
                    return nb::pending;
                }
                broadcast_unreached_types_.reset(type);
            }

            // 2. ブロードキャスト不可能なアドレスしか持たないNeighborに対してユニキャスト
            while (true) {
                etl::optional<etl::reference_wrapper<const NeighborNode>> opt_neighbor =
                    ns.get_neighbor_node(cursor_);
                if (!opt_neighbor.has_value()) {
                    return nb::ready();
                }

                const auto &neighbor = opt_neighbor->get();
                if (ignore_id_ && neighbor.id() == *ignore_id_) {
                    cursor_.advance();
                    continue;
                }

                const link::Address *unreached = nullptr;
                for (const auto &address : neighbor.addresses()) {
                    // ブロードキャスト可能なアドレスを持つ場合，既に1.でブロードキャスト済みのためスキップ
                    if (broadcast_types_.test(address.type())) {
                        unreached = nullptr;
                        break;
                    }

                    if (!unreached) {
                        unreached = &address;
                    }
                }

                if (unreached != nullptr) {
                    const auto &expected_poll = link_socket.poll_send_frame(
                        link::LinkAddress{link::LinkUnicastAddress{*unreached}},
                        reader_.make_initial_clone()
                    );
                    if (expected_poll.has_value() && expected_poll.value().is_pending()) {
                        return nb::pending;
                    }
                }

                cursor_.advance();
            }

            return nb::ready();
        }
    };

    template <nb::AsyncReadableWritable RW>
    class NeighborSocket {
        link::LinkSocket<RW> link_socket_;
        etl::optional<SendBroadcastTask> send_broadcast_task_;

      public:
        explicit NeighborSocket(link::LinkSocket<RW> &&link_socket)
            : link_socket_{etl::move(link_socket)} {}

        nb::Poll<link::LinkFrame> poll_receive_frame() {
            auto &&frame = POLL_MOVE_UNWRAP_OR_RETURN(link_socket_.poll_receive_frame());
            return etl::move(frame.frame);
        }

        etl::expected<nb::Poll<void>, SendError> poll_send_frame(
            NeighborService<RW> &neighbor_service,
            const node::NodeId &remote_id,
            frame::FrameBufferReader &&reader
        ) {
            auto opt_addresses = neighbor_service.get_address_of(remote_id);
            if (!opt_addresses || opt_addresses->size() == 0) {
                return etl::expected<nb::Poll<void>, SendError>(
                    etl::unexpect, SendError::UnreachableNode
                );
            }

            const auto &address = opt_addresses->front();
            const auto &&result =
                link_socket_.poll_send_frame(link::LinkAddress(address), etl::move(reader));
            if (result) {
                return etl::expected<nb::Poll<void>, SendError>(result.value());
            } else {
                return etl::expected<nb::Poll<void>, SendError>(
                    etl::unexpect, SendError::SupportedMediaNotFound
                );
            }
        }

        nb::Poll<void> poll_send_broadcast_frame(
            NeighborService<RW> &neighbor_service,
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id = etl::nullopt
        ) {
            if (send_broadcast_task_) {
                return nb::pending;
            }

            auto &&cursor = POLL_MOVE_UNWRAP_OR_RETURN(neighbor_service.poll_cursor());
            send_broadcast_task_.emplace(
                link_socket_, etl::move(cursor), etl::move(reader), ignore_id
            );
            return nb::ready();
        }

        inline constexpr uint8_t max_payload_length() const {
            return link_socket_.max_payload_length();
        }

        inline nb::Poll<frame::FrameBufferWriter>
        poll_frame_writer(frame::FrameService &frame_service, uint8_t payload_length) {
            return link_socket_.poll_frame_writer(frame_service, payload_length);
        }

        inline nb::Poll<frame::FrameBufferWriter>
        poll_max_length_frame_writer(frame::FrameService &frame_service) {
            return link_socket_.poll_max_length_frame_writer(frame_service);
        }

        inline void execute(NeighborService<RW> &ns) {
            if (send_broadcast_task_) {
                if (send_broadcast_task_->execute(ns, link_socket_).is_ready()) {
                    send_broadcast_task_.reset();
                }
            }
        }
    };
} // namespace net::neighbor
