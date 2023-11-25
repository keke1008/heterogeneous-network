#pragma once

#include "../neighbor.h"
#include "./cache.h"
#include "./frame.h"
#include <etl/variant.h>

namespace net::routing::reactive {
    class ReceiveFrameTask {
        link::LinkFrame frame_;

      public:
        ReceiveFrameTask(link::LinkFrame &&frame) : frame_{etl::move(frame)} {}

        nb::Poll<RouteDiscoveryFrame> execute() {
            if (!frame_.reader.is_buffer_filled()) {
                return nb::pending;
            }
            return RouteDiscoveryFrame::deserialize(frame_.reader.written_buffer());
        }
    };

    class CreateFrameTask {
        NodeId remote_;
        RouteDiscoveryFrame frame_;

      public:
        CreateFrameTask(const NodeId &remote, RouteDiscoveryFrame &&frame)
            : remote_{remote},
              frame_{etl::move(frame)} {}

        inline NodeId remote() const {
            return remote_;
        }

        nb::Poll<frame::FrameBufferReader>
        execute(frame::FrameService &frame_service, neighbor::NeighborSocket &neighbor_socket) {
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(
                neighbor_socket.poll_frame_writer(frame_service, frame_.serialized_length())
            );

            writer.write(frame_);
            return writer.make_initial_reader();
        }
    };

    class CreateBroadcastFrameTask {
        etl::optional<NodeId> ignore_id_;
        RouteDiscoveryFrame frame_;

      public:
        CreateBroadcastFrameTask(RouteDiscoveryFrame &&frame) : frame_{etl::move(frame)} {}

        CreateBroadcastFrameTask(const NodeId &ignore_id_, RouteDiscoveryFrame &&frame)
            : ignore_id_{ignore_id_},
              frame_{etl::move(frame)} {}

        const etl::optional<NodeId> &ignore_id() const {
            return ignore_id_;
        }

        nb::Poll<frame::FrameBufferReader>
        execute(frame::FrameService &frame_service, neighbor::NeighborSocket &neighbor_socket) {
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(
                neighbor_socket.poll_max_length_frame_writer(frame_service)
            );

            writer.write(frame_);
            return writer.make_initial_reader();
        }
    };

    class SendFrameTask {
        NodeId remote_;
        frame::FrameBufferReader reader_;

      public:
        SendFrameTask(const NodeId &remote, frame::FrameBufferReader &&reader)
            : remote_{remote},
              reader_{etl::move(reader)} {}

        etl::expected<nb::Poll<void>, neighbor::SendError> execute(
            neighbor::NeighborService &neighbor_service,
            neighbor::NeighborSocket &neighbor_socket
        ) {
            return neighbor_socket.poll_send_frame(neighbor_service, remote_, etl::move(reader_));
        }
    };

    class SendBroadcastFrameTask {
        frame::FrameBufferReader reader_;
        etl::optional<NodeId> ignore_id_;

      public:
        SendBroadcastFrameTask(
            frame::FrameBufferReader &&reader,
            const etl::optional<NodeId> &ignore_id
        )
            : reader_{etl::move(reader)},
              ignore_id_{ignore_id} {}

        nb::Poll<void> execute(
            neighbor::NeighborService &neighbor_service,
            neighbor::NeighborSocket &neighbor_socket
        ) {
            return neighbor_socket.poll_send_broadcast_frame(
                neighbor_service, etl::move(reader_), ignore_id_
            );
        }
    };

    struct DiscoverEvent {
        NodeId remote_id;
        NodeId gateway_id;
        Cost cost;
    };

    class TaskExecutor {
        neighbor::NeighborSocket neighbor_socket_;
        frame::FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_{};
        etl::variant<
            etl::monostate,
            ReceiveFrameTask,
            CreateFrameTask,
            CreateBroadcastFrameTask,
            SendFrameTask,
            SendBroadcastFrameTask>
            task_{};

      public:
        explicit TaskExecutor(neighbor::NeighborSocket &&neighbor_socket)
            : neighbor_socket_{etl::move(neighbor_socket)} {}

        nb::Poll<void> request_send_discovery_frame(
            const NodeId &target_id,
            const NodeId &self_id,
            Cost self_cost,
            util::Rand &rand
        ) {
            if (!etl::holds_alternative<etl::monostate>(task_)) {
                return nb::pending;
            }

            task_.emplace<CreateBroadcastFrameTask>(RouteDiscoveryFrame{
                .type = RouteDiscoveryFrameType::REQUEST,
                .frame_id = frame_id_cache_.generate(rand),
                .total_cost = self_cost,
                .source_id = self_id,
                .target_id = target_id,
                .sender_id = self_id,
            });
            return nb::ready();
        }

      private:
        void repeat_received_frame(
            const RouteDiscoveryFrame &frame,
            RouteCache &route_cache,
            const NodeId &self_id,
            Cost link_cost,
            Cost self_cost
        ) {
            RouteDiscoveryFrame repeat_frame{
                .type = frame.type,
                .frame_id = frame.frame_id,
                .total_cost = frame.total_cost + link_cost + self_cost,
                .source_id = frame.source_id,
                .target_id = frame.target_id,
                .sender_id = self_id,
            };
            auto opt_gateway = route_cache.get(frame.target_id);
            if (opt_gateway) {
                // 探索対象がキャッシュにある場合，キャッシュからゲートウェイを取得して中継する
                task_.emplace<CreateFrameTask>(*opt_gateway, etl::move(repeat_frame));
            } else {
                // 探索対象がキャッシュにない場合，ブロードキャストする
                task_.emplace<CreateBroadcastFrameTask>(etl::move(repeat_frame));
            }
        }

        void reply_received_frame(
            const RouteDiscoveryFrame &frame,
            RouteCache &route_cache,
            const NodeId &self_id,
            util::Rand &rand
        ) {
            task_.emplace<CreateFrameTask>(
                frame.source_id,
                RouteDiscoveryFrame{
                    .type = RouteDiscoveryFrameType::REPLY,
                    .frame_id = frame_id_cache_.generate(rand),
                    .total_cost = Cost(0),
                    .source_id = self_id,
                    .target_id = frame.source_id,
                    .sender_id = self_id,
                }
            );
        }

        etl::optional<DiscoverEvent> on_hold_receive_frame_task(
            neighbor::NeighborService &neighbor_service,
            RouteCache &route_cache,
            util::Rand &rand,
            const NodeId &self_id,
            Cost self_cost
        ) {
            auto &&task = etl::get<ReceiveFrameTask>(task_);
            auto &&poll_frame = task.execute();
            if (poll_frame.is_pending()) {
                return etl::nullopt;
            }
            auto &&frame = poll_frame.unwrap();

            // 既にキャッシュにある（受信済み）場合は無視する
            if (frame_id_cache_.contains(frame.frame_id)) {
                task_.emplace<etl::monostate>();
                return etl::nullopt;
            }
            frame_id_cache_.add(frame.frame_id);

            // 送信元がNeighborでない場合は無視する
            auto opt_cost = neighbor_service.get_link_cost(frame.sender_id);
            if (!opt_cost) {
                task_.emplace<etl::monostate>();
                return etl::nullopt;
            }

            // 返信に備えてキャッシュに追加
            route_cache.add(frame.source_id, frame.sender_id);

            if (frame.target_id == self_id) {
                // 探索対象が自分自身である場合，Requestであれば探索元に返信する
                if (frame.type == RouteDiscoveryFrameType::REQUEST) {
                    reply_received_frame(frame, route_cache, self_id, rand);
                    return etl::nullopt;
                }

                // Replyであれば探索結果を返す
                return DiscoverEvent{
                    .remote_id = frame.source_id,
                    .gateway_id = frame.sender_id,
                    .cost = frame.total_cost,
                };
            }

            // 探索対象が自分自身でない場合，探索対象に中継する
            repeat_received_frame(frame, route_cache, self_id, *opt_cost, self_cost);
            return etl::nullopt;
        }

      public:
        etl::optional<DiscoverEvent> execute(
            frame::FrameService &frame_service,
            neighbor::NeighborService &neighbor_service,
            RouteCache &route_cache,
            util::Rand &rand,
            const NodeId &self_id,
            Cost self_cost
        ) {
            neighbor_socket_.execute();

            if (etl::holds_alternative<etl::monostate>(task_)) {
                auto &&poll_frame = neighbor_socket_.poll_receive_frame();
                if (poll_frame.is_ready()) {
                    task_.emplace<ReceiveFrameTask>(etl::move(poll_frame.unwrap()));
                }
            }

            etl::optional<DiscoverEvent> event;
            if (etl::holds_alternative<ReceiveFrameTask>(task_)) {
                event = on_hold_receive_frame_task(
                    neighbor_service, route_cache, rand, self_id, self_cost
                );
            }

            if (etl::holds_alternative<CreateFrameTask>(task_)) {
                auto &&task = etl::get<CreateFrameTask>(task_);
                auto &&poll_reader = task.execute(frame_service, neighbor_socket_);
                if (poll_reader.is_pending()) {
                    return event;
                }

                auto &&reader = poll_reader.unwrap();
                task_.emplace<SendFrameTask>(task.remote(), etl::move(reader));
            }

            if (etl::holds_alternative<CreateBroadcastFrameTask>(task_)) {
                auto &&task = etl::get<CreateBroadcastFrameTask>(task_);
                auto &&poll_reader = task.execute(frame_service, neighbor_socket_);
                if (poll_reader.is_pending()) {
                    return event;
                }

                auto &&reader = poll_reader.unwrap();
                task_.emplace<SendBroadcastFrameTask>(etl::move(reader), task.ignore_id());
            }

            if (etl::holds_alternative<SendFrameTask>(task_)) {
                auto &&task = etl::get<SendFrameTask>(task_);
                auto &&poll_result = task.execute(neighbor_service, neighbor_socket_);
                if (!poll_result || poll_result.value().is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }

            if (etl::holds_alternative<SendBroadcastFrameTask>(task_)) {
                auto &&task = etl::get<SendBroadcastFrameTask>(task_);
                auto &&poll_result = task.execute(neighbor_service, neighbor_socket_);
                if (poll_result.is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }

            return event;
        }
    };
} // namespace net::routing::reactive
