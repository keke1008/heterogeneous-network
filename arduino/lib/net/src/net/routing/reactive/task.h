#pragma once

#include "../neighbor.h"
#include "./cache.h"
#include "./frame.h"
#include <etl/variant.h>

namespace net::routing::reactive {
    class ReceiveFrameTask {
        link::LinkFrame frame_;
        AsyncRouteDiscoveryFrameDeserializer deserializer_;

      public:
        ReceiveFrameTask(link::LinkFrame &&frame) : frame_{etl::move(frame)} {}

        inline nb::Poll<etl::optional<RouteDiscoveryFrame>> execute() {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.reader.deserialize(deserializer_));
            return result == nb::de::DeserializeResult::Ok ? etl::optional{deserializer_.result()}
                                                           : etl::nullopt;
        }
    };

    class CreateFrameTask {
        AsyncRouteDiscoveryFrameSerializer serializer_;

      public:
        CreateFrameTask(RouteDiscoveryFrame &&frame) : serializer_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<frame::FrameBufferReader>
        execute(frame::FrameService &frame_service, neighbor::NeighborSocket<RW> &socket) {
            uint8_t length = serializer_.serialized_length();
            frame::FrameBufferWriter writer =
                POLL_MOVE_UNWRAP_OR_RETURN(socket.poll_frame_writer(frame_service, length));

            writer.serialize_all_at_once(serializer_);
            return writer.create_reader();
        }
    };

    class CreateUnicastFrameTask {
        CreateFrameTask task_;
        node::NodeId remote_;

      public:
        CreateUnicastFrameTask(const node::NodeId &remote, RouteDiscoveryFrame &&frame)
            : task_{etl::move(frame)},
              remote_{remote} {}

        inline node::NodeId remote() const {
            return remote_;
        }

        template <nb::AsyncReadableWritable RW>
        nb::Poll<frame::FrameBufferReader>
        execute(frame::FrameService &frame_service, neighbor::NeighborSocket<RW> &socket) {
            return task_.execute(frame_service, socket);
        }
    };

    class CreateBroadcastFrameTask {
        CreateFrameTask task_;
        etl::optional<node::NodeId> ignore_id_;

      public:
        CreateBroadcastFrameTask(RouteDiscoveryFrame &&frame) : task_{etl::move(frame)} {}

        CreateBroadcastFrameTask(const node::NodeId &ignore_id_, RouteDiscoveryFrame &&frame)
            : task_{etl::move(frame)},
              ignore_id_{ignore_id_} {}

        const etl::optional<node::NodeId> &ignore_id() const {
            return ignore_id_;
        }

        template <nb::AsyncReadableWritable RW>
        nb::Poll<frame::FrameBufferReader>
        execute(frame::FrameService &frame_service, neighbor::NeighborSocket<RW> &socket) {
            return task_.execute(frame_service, socket);
        }
    };

    class SendFrameTask {
        node::NodeId remote_;
        frame::FrameBufferReader reader_;

      public:
        SendFrameTask(const node::NodeId &remote, frame::FrameBufferReader &&reader)
            : remote_{remote},
              reader_{etl::move(reader)} {}

        template <nb::AsyncReadableWritable RW>
        etl::expected<nb::Poll<void>, neighbor::SendError> execute(
            neighbor::NeighborService<RW> &neighbor_service,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            return neighbor_socket.poll_send_frame(neighbor_service, remote_, etl::move(reader_));
        }
    };

    class SendBroadcastFrameTask {
        frame::FrameBufferReader reader_;
        etl::optional<node::NodeId> ignore_id_;

      public:
        SendBroadcastFrameTask(
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id
        )
            : reader_{etl::move(reader)},
              ignore_id_{ignore_id} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(
            neighbor::NeighborService<RW> &neighbor_service,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            return neighbor_socket.poll_send_broadcast_frame(
                neighbor_service, etl::move(reader_), ignore_id_
            );
        }
    };

    struct DiscoverEvent {
        node::NodeId remote_id;
        node::NodeId gateway_id;
        node::Cost cost;
    };

    template <nb::AsyncReadableWritable RW>
    class TaskExecutor {
        neighbor::NeighborSocket<RW> neighbor_socket_;
        frame::FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_{};
        etl::variant<
            etl::monostate,
            ReceiveFrameTask,
            CreateUnicastFrameTask,
            CreateBroadcastFrameTask,
            SendFrameTask,
            SendBroadcastFrameTask>
            task_{};

      public:
        explicit TaskExecutor(neighbor::NeighborSocket<RW> &&neighbor_socket)
            : neighbor_socket_{etl::move(neighbor_socket)} {}

        nb::Poll<void> request_send_discovery_frame(
            const node::NodeId &target_id,
            const node::NodeId &self_id,
            node::Cost self_cost,
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
            const node::NodeId &self_id,
            node::Cost link_cost,
            node::Cost self_cost
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
                task_.emplace<CreateUnicastFrameTask>(*opt_gateway, etl::move(repeat_frame));
            } else {
                // 探索対象がキャッシュにない場合，ブロードキャストする
                task_.emplace<CreateBroadcastFrameTask>(etl::move(repeat_frame));
            }
        }

        void reply_received_frame(
            const RouteDiscoveryFrame &frame,
            RouteCache &route_cache,
            const node::NodeId &self_id,
            util::Rand &rand
        ) {
            task_.emplace<CreateUnicastFrameTask>(
                frame.source_id,
                RouteDiscoveryFrame{
                    .type = RouteDiscoveryFrameType::REPLY,
                    .frame_id = frame_id_cache_.generate(rand),
                    .total_cost = node::Cost(0),
                    .source_id = self_id,
                    .target_id = frame.source_id,
                    .sender_id = self_id,
                }
            );
        }

        etl::optional<DiscoverEvent> on_hold_receive_frame_task(
            neighbor::NeighborService<RW> &neighbor_service,
            RouteCache &route_cache,
            util::Rand &rand,
            const node::NodeId &self_id,
            node::Cost self_cost
        ) {
            auto &&task = etl::get<ReceiveFrameTask>(task_);
            auto &&poll_opt_frame = task.execute();
            if (poll_opt_frame.is_pending()) {
                return etl::nullopt;
            }

            auto &&opt_frame = poll_opt_frame.unwrap();
            if (!opt_frame) { // フレームの受信に失敗した場合は無視する
                task_.emplace<etl::monostate>();
                return etl::nullopt;
            }
            auto &&frame = opt_frame.value();

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
            neighbor::NeighborService<RW> &neighbor_service,
            RouteCache &route_cache,
            util::Rand &rand,
            const node::NodeId &self_id,
            node::Cost self_cost
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

            if (etl::holds_alternative<CreateUnicastFrameTask>(task_)) {
                auto &&task = etl::get<CreateUnicastFrameTask>(task_);
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
