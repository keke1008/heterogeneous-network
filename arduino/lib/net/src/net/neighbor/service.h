#pragma once

#include "./frame.h"
#include "./table.h"
#include <nb/poll.h>
#include <net/frame.h>
#include <net/link.h>
#include <net/local.h>
#include <net/notification.h>

namespace net::neighbor {
    class ReceiveFrameTask {
        link::LinkReceivedFrame link_frame_;
        AsyncNeighborFrameDeserializer deserializer_;

      public:
        explicit ReceiveFrameTask(link::LinkReceivedFrame &&link_frame)
            : link_frame_{etl::move(link_frame)} {}

        const link::Address &remote() const {
            // 受信したフレームの送信元は必ずユニキャストである
            return link_frame_.frame.remote.unwrap_unicast().address;
        }

        const link::MediaPortNumber port() const {
            return link_frame_.port;
        }

        nb::Poll<etl::optional<NeighborFrame>> execute() {
            auto result =
                POLL_UNWRAP_OR_RETURN(link_frame_.frame.reader.deserialize(deserializer_));
            return result == nb::de::DeserializeResult::Ok ? etl::optional{deserializer_.result()}
                                                           : etl::nullopt;
        }
    };

    class SerializeFrameTask {
        link::Address destination_;
        etl::optional<link::MediaPortNumber> port_;
        AsyncNeighborFrameSerializer serializer_;

      public:
        template <typename T>
        explicit SerializeFrameTask(
            const link::Address &destination,
            etl::optional<link::MediaPortNumber> port,
            T &&frame
        )
            : destination_{destination},
              port_{port},
              serializer_{etl::forward<T>(frame)} {}

        const link::Address &destination() const {
            return destination_;
        }

        link::MediaPortNumber port() const {
            return port_.value();
        }

        nb::Poll<frame::FrameBufferReader> execute(frame::FrameService &frame_service) {
            uint8_t length = serializer_.serialized_length();
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(length));
            writer.serialize_all_at_once(serializer_);
            return writer.create_reader();
        }
    };

    class SendFrameTask {
        link::LinkAddress destination_;
        frame::FrameBufferReader reader_;
        etl::optional<link::MediaPortNumber> port_;

      public:
        explicit SendFrameTask(
            const link::LinkAddress &destination,
            etl::optional<link::MediaPortNumber> port,
            frame::FrameBufferReader &&reader
        )
            : destination_{destination},
              reader_{etl::move(reader)},
              port_{port} {}

        template <nb::AsyncReadableWritable RW>
        inline etl::expected<nb::Poll<void>, link::SendFrameError>
        execute(link::LinkSocket<RW> &link_socket) {
            return link_socket.poll_send_frame(destination_, etl::move(reader_), port_);
        }
    };

    template <nb::AsyncReadableWritable RW>
    class NeighborService {
        NeighborList neighbor_list_;
        link::LinkSocket<RW> link_socket_;
        etl::variant<etl::monostate, ReceiveFrameTask, SerializeFrameTask, SendFrameTask> task_{};

        inline nb::Poll<void> poll_wait_for_task_addable() const {
            return etl::holds_alternative<etl::monostate>(task_) ? nb::ready() : nb::pending;
        }

      public:
        explicit NeighborService(link::LinkService<RW> &link_service)
            : link_socket_{link_service.open(frame::ProtocolNumber::RoutingNeighbor)} {}

        inline etl::optional<node::Cost> get_link_cost(const node::NodeId &neighbor_id) const {
            return neighbor_list_.get_link_cost(neighbor_id);
        }

        inline bool has_neighbor(const node::NodeId &neighbor_id) const {
            return neighbor_list_.has_neighbor_node(neighbor_id);
        }

        inline etl::optional<etl::span<const link::Address>>
        get_address_of(const node::NodeId &node_id) const {
            return neighbor_list_.get_addresses_of(node_id);
        }

        inline nb::Poll<NeighborListCursor> poll_cursor() {
            return neighbor_list_.poll_cursor();
        }

        inline etl::optional<etl::reference_wrapper<const NeighborNode>>
        get_neighbor_node(const NeighborListCursor &cursor) const {
            return neighbor_list_.get_neighbor_node(cursor);
        }

        inline nb::Poll<void> poll_send_hello(
            const local::LocalNodeService &local_node_service,
            const link::Address &destination,
            node::Cost link_cost,
            etl::optional<link::MediaPortNumber> port = etl::nullopt
        ) {
            POLL_UNWRAP_OR_RETURN(poll_wait_for_task_addable());
            const auto &info = POLL_UNWRAP_OR_RETURN(local_node_service.poll_info());

            task_.emplace<SerializeFrameTask>(
                destination, port,
                HelloFrame{
                    .is_ack = false,
                    .source = info.source,
                    .node_cost = info.cost,
                    .link_cost = link_cost,
                }
            );
            return nb::ready();
        }

        inline nb::Poll<void> poll_send_goodbye(
            const local::LocalNodeService &local_node_service,
            const node::NodeId &destination
        ) {
            POLL_UNWRAP_OR_RETURN(poll_wait_for_task_addable());
            const auto &info = POLL_UNWRAP_OR_RETURN(local_node_service.poll_info());

            auto addresses = neighbor_list_.get_addresses_of(destination);
            if (addresses.has_value()) {
                neighbor_list_.remove_neighbor_node(destination);
                auto &address = addresses.value().front();
                task_.emplace<SerializeFrameTask>(
                    address, etl::nullopt, GoodbyeFrame{.sender_id = info.source.node_id}
                );
            }
            return nb::ready();
        }

      private:
        void handle_received_hello_frame(
            notification::NotificationService &ns,
            HelloFrame &&frame,
            const link::Address &remote,
            link::MediaPortNumber port,
            const local::LocalNodeInfo &self_node_info
        ) {
            if (frame.is_ack) {
                task_.emplace<etl::monostate>();
            } else {
                task_.emplace<SerializeFrameTask>(
                    remote, port,
                    HelloFrame{
                        .is_ack = true,
                        .source = self_node_info.source,
                        .node_cost = self_node_info.cost,
                        .link_cost = frame.link_cost,
                    }
                );
            }

            auto result =
                neighbor_list_.add_neighbor_link(frame.source.node_id, remote, frame.link_cost);
            if (result == AddLinkResult::NewNodeConnected) {
                LOG_INFO(
                    FLASH_STRING("new neigh: "), frame.source.node_id, FLASH_STRING(" via "), remote
                );
                ns.notify(notification::NeighborUpdated{
                    .neighbor = frame.source,
                    .neighbor_cost = frame.node_cost,
                    .link_cost = frame.link_cost,
                });
            }
        }

        void
        handle_received_goodbye_frame(notification::NotificationService &ns, GoodbyeFrame &&frame) {
            task_.emplace<etl::monostate>();

            auto result = neighbor_list_.remove_neighbor_node(frame.sender_id);
            if (result == RemoveNodeResult::Disconnected) {
                LOG_INFO(FLASH_STRING("neighbor disconnected: "), frame.sender_id);
                ns.notify(notification::NeighborRemoved{.neighbor_id = frame.sender_id});
            }
        }

        void on_receive_frame_task(
            notification::NotificationService &ns,
            const local::LocalNodeInfo &info
        ) {
            auto &task = etl::get<ReceiveFrameTask>(task_);
            auto poll_opt_frame = task.execute();
            if (poll_opt_frame.is_pending()) {
                return;
            }

            auto opt_frame = poll_opt_frame.unwrap();
            if (!opt_frame.has_value()) {
                task_.emplace<etl::monostate>();
                return;
            }

            return etl::visit(
                util::Visitor{
                    [&](HelloFrame &frame) {
                        // handle_received_hello_frameでtaskが変わるので，一旦コピーする
                        auto remote = task.remote();
                        auto port = task.port();
                        handle_received_hello_frame(ns, etl::move(frame), remote, port, info);
                    },
                    [&](GoodbyeFrame &frame) {
                        handle_received_goodbye_frame(ns, etl::move(frame));
                    },
                },
                opt_frame.value()
            );
        }

      public:
        void execute(
            frame::FrameService &frame_service,
            link::LinkService<RW> &link_service,
            const local::LocalNodeService &local_node_service,
            notification::NotificationService &ns
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                auto &&poll_frame = link_socket_.poll_receive_frame();
                if (poll_frame.is_pending()) {
                    return;
                }

                auto &&frame = poll_frame.unwrap();
                task_.emplace<ReceiveFrameTask>(etl::move(frame));
            }

            if (etl::holds_alternative<ReceiveFrameTask>(task_)) {
                const auto &poll_info = local_node_service.poll_info();
                if (poll_info.is_pending()) {
                    return;
                }
                const auto &info = poll_info.unwrap();
                on_receive_frame_task(ns, info);
            }

            if (etl::holds_alternative<SerializeFrameTask>(task_)) {
                auto &task = etl::get<SerializeFrameTask>(task_);
                auto poll_frame = task.execute(frame_service);
                if (poll_frame.is_pending()) {
                    return;
                }

                auto port = task.port();
                task_.emplace<SendFrameTask>(
                    link::LinkAddress(task.destination()), port, etl::move(poll_frame.unwrap())
                );
            }

            if (etl::holds_alternative<SendFrameTask>(task_)) {
                auto &task = etl::get<SendFrameTask>(task_);
                auto expect_poll_send = task.execute(link_socket_);
                if (!expect_poll_send.has_value()) {
                    uint8_t error = static_cast<uint8_t>(expect_poll_send.error());
                    LOG_WARNING(FLASH_STRING("send frame failed. SendFrameError:"), error);
                    task_.emplace<etl::monostate>();
                    return;
                }
                if (expect_poll_send.value().is_pending()) {
                    return;
                }
                task_.emplace<etl::monostate>();
            }
        }
    };
} // namespace net::neighbor
