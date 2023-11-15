#pragma once

#include "./frame.h"
#include "./neighbor.h"
#include "./reactive.h"
#include <net/frame/protocol_number.h>

namespace net::routing {
    using SendError = neighbor::SendError;

    class RoutingService {
        friend class RouteDiscoverTask;
        friend class RequestSendFrameTask;
        friend class RoutingSocket;

        neighbor::NeighborService neighbor_service_;
        reactive::ReactiveService reactive_service_;

        etl::optional<NodeId> self_id_;
        Cost self_cost_{0};

      public:
        explicit RoutingService(link::LinkService &link_service, util::Time &time)
            : neighbor_service_{link_service},
              reactive_service_{link_service, time} {}

        inline const etl::optional<NodeId> &self_id() const {
            return self_id_;
        }

        inline nb::Poll<void> poll_send_hello(const link::Address &destination, Cost link_cost) {
            if (!self_id_) {
                return nb::pending;
            }
            return neighbor_service_.request_send_hello(
                destination, *self_id_, link_cost, self_cost_
            );
        }

        inline nb::Poll<void> poll_send_goodbye(const NodeId &destination) {
            if (!self_id_) {
                return nb::pending;
            }
            return neighbor_service_.request_goodbye(destination, *self_id_);
        }

        template <uint8_t N>
        inline void get_neighbors(tl::Vec<neighbor::NeighborNode, N> &neighbors) {
            neighbor_service_.get_neighbors(neighbors);
        }

        inline void execute(
            frame::FrameService &frame_service,
            link::LinkService &link_service,
            util::Rand &rand
        ) {
            if (!self_id_) {
                const auto &opt_self_id = link_service.get_media_address();
                if (opt_self_id) {
                    self_id_ = NodeId(opt_self_id.value());
                    LOG_INFO("Local Id set.");
                } else {
                    return;
                }
            }

            const auto &event =
                neighbor_service_.execute(frame_service, link_service, *self_id_, self_cost_);
            reactive_service_.on_neighbor_event(event);

            reactive_service_.execute(
                frame_service, neighbor_service_, *self_id_, self_cost_, rand
            );
        }
    };

    class RouteDiscoverTask {
        reactive::RouteDiscoverTask inner_task_;

      public:
        explicit RouteDiscoverTask(const NodeId &target_id) : inner_task_{target_id} {}

        inline nb::Poll<etl::optional<NodeId>>
        execute(RoutingService &routing_service, util::Time &time, util::Rand &rand) {
            if (!routing_service.self_id_) {
                return nb::pending;
            }

            return inner_task_.execute(
                routing_service.reactive_service_, *routing_service.self_id_,
                routing_service.self_cost_, time, rand
            );
        }
    };

    class RequestSendFrameTask {
        NodeId remote_id_;
        frame::FrameBufferReader reader_;

      public:
        explicit RequestSendFrameTask(const NodeId &remote_id, frame::FrameBufferReader &&reader)
            : remote_id_{remote_id},
              reader_{etl::move(reader)} {}

        inline etl::expected<nb::Poll<void>, neighbor::SendError>
        execute(RoutingService &routing_service, neighbor::NeighborSocket &neighbor_socket) {
            return neighbor_socket.poll_send_frame(
                routing_service.neighbor_service_, remote_id_, etl::move(reader_)
            );
        }
    };

    class SendFrameTask {
        etl::variant<RouteDiscoverTask, RequestSendFrameTask> inner_task_;
        frame::FrameBufferReader reader_;
        nb::Promise<etl::expected<void, neighbor::SendError>> promise_;

      public:
        explicit SendFrameTask(
            const NodeId &target_id,
            frame::FrameBufferReader &&reader,
            nb::Promise<etl::expected<void, neighbor::SendError>> &&promise
        )
            : inner_task_{RouteDiscoverTask{etl::move(target_id)}},
              reader_{etl::move(reader)},
              promise_{etl::move(promise)} {}

        inline nb::Poll<void> execute(
            RoutingService &routing_service,
            neighbor::NeighborSocket &neighbor_socket,
            util::Time &time,
            util::Rand &rand
        ) {
            if (etl::holds_alternative<RouteDiscoverTask>(inner_task_)) {
                auto &task = etl::get<RouteDiscoverTask>(inner_task_);
                const auto &opt_gateway_id =
                    POLL_UNWRAP_OR_RETURN(task.execute(routing_service, time, rand));
                if (!opt_gateway_id) {
                    promise_.set_value({etl::unexpected<SendError>{SendError::UnreachableNode}});
                    return nb::ready();
                }
                inner_task_.emplace<RequestSendFrameTask>(*opt_gateway_id, etl::move(reader_));
            }

            auto &task = etl::get<RequestSendFrameTask>(inner_task_);
            auto result = task.execute(routing_service, neighbor_socket);
            if (result) {
                POLL_UNWRAP_OR_RETURN(result.value());
                promise_.set_value(etl::expected<void, neighbor::SendError>{});
                return nb::ready();
            } else {
                promise_.set_value({etl::unexpected<SendError>{result.error()}});
                return nb::ready();
            }
        }
    };

    class ReceiveFrameTask {
        link::LinkFrame link_frame_;
        AsyncRoutingFrameHeaderDeserializer header_parser_;

      public:
        explicit ReceiveFrameTask(link::LinkFrame &&link_frame)
            : link_frame_{etl::move(link_frame)} {}

        const link::Address &remote() const {
            // 受信したフレームの送信元は必ずユニキャストである
            return link_frame_.remote.unwrap_unicast().address;
        }

        nb::Poll<RoutingFrame> execute() {
            POLL_UNWRAP_OR_RETURN(link_frame_.reader.read(header_parser_));
            return RoutingFrame{
                .header = header_parser_.result(),
                .payload = etl::move(link_frame_.reader),
            };
        }
    };

    class RoutingSocket {
        neighbor::NeighborSocket neighbor_socket_;
        etl::optional<ReceiveFrameTask> receive_frame_task_;
        etl::optional<SendFrameTask> send_frame_task_;

      public:
        explicit RoutingSocket(link::LinkSocket &&link_socket)
            : neighbor_socket_{etl::move(link_socket)} {}

        nb::Poll<RoutingFrame> poll_receive_frame() {
            if (!receive_frame_task_) {
                auto &&link_frame =
                    POLL_MOVE_UNWRAP_OR_RETURN(neighbor_socket_.poll_receive_frame());
                receive_frame_task_ = ReceiveFrameTask(etl::move(link_frame));
            }

            auto &&frame = POLL_MOVE_UNWRAP_OR_RETURN(receive_frame_task_->execute());
            receive_frame_task_.reset();
            return etl::move(frame);
        }

        nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>> poll_send_frame(
            RoutingService &routing_service,
            const NodeId &remote_id,
            frame::FrameBufferReader &&reader,
            util::Time &time,
            util::Rand &rand
        ) {
            if (send_frame_task_) {
                return nb::pending;
            }
            auto [f, p] = nb::make_future_promise_pair<etl::expected<void, neighbor::SendError>>();
            send_frame_task_.emplace(remote_id, etl::move(reader), etl::move(p));
            return etl::move(f);
        }

        nb::Poll<void> poll_send_broadcast_frame(
            RoutingService &routing_service,
            frame::FrameBufferReader &&reader,
            const etl::optional<NodeId> &ignore_id = etl::nullopt
        ) {
            return neighbor_socket_.poll_send_broadcast_frame(
                routing_service.neighbor_service_, etl::move(reader), ignore_id
            );
        }

        inline nb::Poll<uint8_t>
        max_payload_length(RoutingService &routing_service, const NodeId &remote_id) const {
            const auto &opt_self_id = routing_service.self_id();
            if (!opt_self_id) {
                return nb::pending;
            }

            return neighbor_socket_.max_payload_length() -
                calculate_frame_header_length(*opt_self_id, remote_id);
        }

        inline nb::Poll<frame::FrameBufferWriter> poll_frame_writer(
            frame::FrameService &frame_service,
            RoutingService &routing_service,
            const NodeId &remote_id,
            uint8_t payload_length
        ) {
            const auto &opt_self_id = routing_service.self_id();
            if (!opt_self_id) {
                return nb::pending;
            }

            ASSERT(payload_length <= max_payload_length(routing_service, remote_id).unwrap());

            uint8_t total_length =
                calculate_frame_header_length(*opt_self_id, remote_id) + payload_length;
            auto &&writer =
                POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(total_length));
            write_frame_header(writer, *opt_self_id, remote_id);
            return etl::move(writer);
        }

        inline nb::Poll<frame::FrameBufferWriter> poll_max_length_frame_writer(
            frame::FrameService &frame_service,
            RoutingService &routing_service,
            const NodeId &remote_id
        ) {
            const auto &opt_self_id = routing_service.self_id();
            if (!opt_self_id) {
                return nb::pending;
            }

            auto &&writer =
                POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_max_length_frame_writer());
            write_frame_header(writer, *opt_self_id, remote_id);
            return etl::move(writer);
        };

        void execute(RoutingService &routing_service, util::Time &time, util::Rand &rand) {
            neighbor_socket_.execute();
            if (send_frame_task_) {
                auto poll =
                    send_frame_task_->execute(routing_service, neighbor_socket_, time, rand);
                if (poll.is_ready()) {
                    send_frame_task_.reset();
                }
            }
        }
    };
} // namespace net::routing
