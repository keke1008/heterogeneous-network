#pragma once

#include "./frame.h"
#include "./table.h"
#include <etl/variant.h>
#include <nb/poll.h>
#include <net/frame/service.h>
#include <net/link.h>

namespace net::routing::neighbor {
    struct NodeConnectedEvent {
        node::NodeId id;
        node::Cost link_cost;
    };

    struct NodeDisconnectedEvent {
        node::NodeId id;
    };

    using Event = etl::variant<etl::monostate, NodeConnectedEvent, NodeDisconnectedEvent>;

    class ReceiveFrameTask {
        link::LinkFrame link_frame_;
        AsyncNeighborFrameDeserializer deserializer_;

      public:
        explicit ReceiveFrameTask(link::LinkFrame &&link_frame)
            : link_frame_{etl::move(link_frame)} {}

        const link::Address &remote() const {
            // 受信したフレームの送信元は必ずユニキャストである
            return link_frame_.remote.unwrap_unicast().address;
        }

        nb::Poll<etl::optional<NeighborFrame>> execute() {
            auto result = POLL_UNWRAP_OR_RETURN(link_frame_.reader.deserialize(deserializer_));
            return result == nb::de::DeserializeResult::Ok ? etl::optional{deserializer_.result()}
                                                           : etl::nullopt;
        }
    };

    class SerializeFrameTask {
        link::Address destination_;
        AsyncNeighborFrameSerializer serializer_;

      public:
        template <typename T>
        explicit SerializeFrameTask(const link::Address &destination, T &&frame)
            : destination_{destination},
              serializer_{etl::forward<T>(frame)} {}

        const link::Address &destination() const {
            return destination_;
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

      public:
        explicit SendFrameTask(
            const link::LinkAddress &destination,
            frame::FrameBufferReader &&reader
        )
            : destination_{destination},
              reader_{etl::move(reader)} {}

        template <nb::AsyncReadableWritable RW>
        inline etl::expected<nb::Poll<void>, link::SendFrameError>
        execute(link::LinkSocket<RW> &link_socket) {
            return link_socket.poll_send_frame(destination_, etl::move(reader_));
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

        inline etl::optional<etl::span<const link::Address>>
        get_address_of(const node::NodeId &node_id) const {
            return neighbor_list_.get_addresses_of(node_id);
        }

        template <uint8_t N>
        inline void get_neighbors(tl::Vec<NeighborNode, N> &dest) const {
            neighbor_list_.get_neighbors(dest);
        }

        inline nb::Poll<void> request_send_hello(
            const link::Address &destination,
            const node::NodeId &self_node_id,
            node::Cost self_node_cost,
            node::Cost link_cost
        ) {
            POLL_UNWRAP_OR_RETURN(poll_wait_for_task_addable());

            task_.emplace<SerializeFrameTask>(
                destination,
                HelloFrame{
                    .sender_id = self_node_id,
                    .node_cost = self_node_cost,
                    .link_cost = link_cost,
                }
            );
            return nb::ready();
        }

        inline nb::Poll<void>
        request_goodbye(const node::NodeId &destination, const node::NodeId &self_node_id) {
            POLL_UNWRAP_OR_RETURN(poll_wait_for_task_addable());

            auto addresses = neighbor_list_.get_addresses_of(self_node_id);
            if (addresses.has_value()) {
                neighbor_list_.remove_neighbor_node(destination);
                auto &address = addresses.value().front();
                task_.emplace<SerializeFrameTask>(address, GoodbyeFrame{.sender_id = self_node_id});
            }
            return nb::ready();
        }

      private:
        Event handle_received_hello_frame(
            HelloFrame &&frame,
            const link::Address &remote,
            const node::NodeId &self_node_id,
            node::Cost self_node_cost
        ) {
            if (frame.is_ack) {
                task_.emplace<etl::monostate>();
            } else {
                task_.emplace<SerializeFrameTask>(
                    remote,
                    HelloFrame{
                        .is_ack = true,
                        .sender_id = self_node_id,
                        .node_cost = self_node_cost,
                        .link_cost = frame.link_cost,
                    }
                );
            }

            auto result =
                neighbor_list_.add_neighbor_link(frame.sender_id, remote, frame.link_cost);
            if (result == AddLinkResult::NewNodeConnected) {
                LOG_INFO("new neighbor connected: ", frame.sender_id, " via ", remote);
                return NodeConnectedEvent{.id = frame.sender_id, .link_cost = frame.link_cost};
            } else {
                return etl::monostate{};
            }
        }

        Event handle_received_goodbye_frame(GoodbyeFrame &&frame) {
            task_.emplace<etl::monostate>();

            auto result = neighbor_list_.remove_neighbor_node(frame.sender_id);
            if (result == RemoveNodeResult::Disconnected) {
                return NodeDisconnectedEvent{.id = frame.sender_id};
            } else {
                return etl::monostate{};
            }
        }

        Event on_receive_frame_task(const node::NodeId &self_node_id, node::Cost self_node_cost) {
            auto &task = etl::get<ReceiveFrameTask>(task_);
            auto poll_opt_frame = task.execute();
            if (poll_opt_frame.is_pending()) {
                return etl::monostate{};
            }

            auto opt_frame = poll_opt_frame.unwrap();
            if (!opt_frame.has_value()) {
                task_.emplace<etl::monostate>();
                return etl::monostate{};
            }

            return etl::visit(
                util::Visitor{
                    [&](HelloFrame &frame) {
                        // handle_received_hello_frameでtaskが変わるので，一旦コピーする
                        auto remote = task.remote();
                        return handle_received_hello_frame(
                            etl::move(frame), remote, self_node_id, self_node_cost
                        );
                    },
                    [&](GoodbyeFrame &frame) {
                        return handle_received_goodbye_frame(etl::move(frame));
                    },
                },
                opt_frame.value()
            );
        }

      public:
        Event execute(
            frame::FrameService &frame_service,
            link::LinkService<RW> &link_service,
            const node::NodeId &self_node_id,
            node::Cost self_node_cost
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                auto &&poll_frame = link_socket_.poll_receive_frame();
                if (poll_frame.is_pending()) {
                    return etl::monostate{};
                }

                auto &&frame = poll_frame.unwrap();
                task_.emplace<ReceiveFrameTask>(etl::move(frame));
            }

            Event result;

            if (etl::holds_alternative<ReceiveFrameTask>(task_)) {
                result = on_receive_frame_task(self_node_id, self_node_cost);
                if (!etl::holds_alternative<etl::monostate>(task_)) {
                    return result;
                }
            }

            if (etl::holds_alternative<SerializeFrameTask>(task_)) {
                auto &task = etl::get<SerializeFrameTask>(task_);
                auto poll_frame = task.execute(frame_service);
                if (poll_frame.is_pending()) {
                    return result;
                }
                task_.emplace<SendFrameTask>(
                    link::LinkAddress(task.destination()), etl::move(poll_frame.unwrap())
                );
            }

            if (etl::holds_alternative<SendFrameTask>(task_)) {
                auto &task = etl::get<SendFrameTask>(task_);
                auto expect_poll_send = task.execute(link_socket_);
                if (!expect_poll_send.has_value()) {
                    uint8_t error = static_cast<uint8_t>(expect_poll_send.error());
                    LOG_WARNING("send frame failed. SendFrameError:", error);

                    task_.emplace<etl::monostate>();
                    return result;
                }
                if (expect_poll_send.value().is_pending()) {
                    return result;
                }
                task_.emplace<etl::monostate>();
            }

            return result;
        }
    };
} // namespace net::routing::neighbor
